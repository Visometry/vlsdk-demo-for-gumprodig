#include <MultiViewDetector.h>

#include <Helpers/ImageHelpers.h>

#include <vlSDK.h>

using namespace nlohmann;

namespace
{
std::string getCreateTrackerCommand(const std::string& trackerPath)
{
    json cmd;
    cmd["name"] = "createTracker";
    cmd["param"]["uri"] = trackerPath;
    return cmd.dump();
}

std::string getRunTrackingCommand()
{
    json cmd;
    cmd["name"] = "runTracking";
    return cmd.dump();
}

std::string getResetHardCommand(const std::string& trackerName)
{
    json cmd;
    cmd["nodeName"] = trackerName;
    cmd["content"]["name"] = "resetHard";
    return cmd.dump();
}

std::string setAttributeCommand(const std::string& attributeName, const std::string& value)
{
    json cmd;
    cmd["name"] = "setAttribute";
    cmd["param"]["att"] = attributeName;
    cmd["param"]["val"] = value;
    return cmd.dump();
}

std::string setInitPoseCommand(const ExtrinsicDataHelpers::Extrinsic& extrinsic)
{
    json setInitPoseCommand;
    setInitPoseCommand["name"] = "setInitPose";
    setInitPoseCommand["param"]["t"][0] = extrinsic.t.at(0);
    setInitPoseCommand["param"]["t"][1] = extrinsic.t.at(1);
    setInitPoseCommand["param"]["t"][2] = extrinsic.t.at(2);
    setInitPoseCommand["param"]["r"][0] = extrinsic.q.at(0);
    setInitPoseCommand["param"]["r"][1] = extrinsic.q.at(1);
    setInitPoseCommand["param"]["r"][2] = extrinsic.q.at(2);
    setInitPoseCommand["param"]["r"][3] = extrinsic.q.at(3);
    return setInitPoseCommand.dump();
}

std::string execute(const Worker& worker, const std::string& cmd)
{
    using StringPair = std::pair<std::string, std::string>;
    StringPair result;
    if (!vlWorker_ProcessJsonCommandSync(
            worker.get(),
            cmd.c_str(),
            [](const char* error, const char* data, void* clientData)
            {
                auto& result = *reinterpret_cast<StringPair*>(clientData);
                result.first = error ? error : "";
                result.second = data ? data : "";
            },
            &result))
    {
        throw std::runtime_error(
            "Command " + cmd + " could not be processed. Invalid JSON or unsupported command.");
    }
    if (!result.first.empty())
    {
        const auto resultJson = json::parse(result.first);
        throw std::runtime_error(resultJson["message"].get<std::string>());
    }
    return result.second;
}

Worker
    createSyncWorker(const std::string& licenseFilepath, const std::string& trackingConfigFilepath)
{
    Worker worker(vlNew_SyncWorker());
    if (!vlWorker_Start(worker.get()))
    {
        throw std::runtime_error("vlWorker_Start returned false.");
    }
    if (!vlWorker_SetLicenseFilePath(worker.get(), licenseFilepath.c_str()))
    {
        throw std::runtime_error("Could not set License");
    }

    try
    {
        execute(worker, getCreateTrackerCommand(trackingConfigFilepath));
    }
    catch (std::runtime_error& e)
    {
        throw std::runtime_error("Could not create tracker: " + std::string(e.what()));
    }
    try
    {
        execute(worker, getRunTrackingCommand());
    }
    catch (std::runtime_error& e)
    {
        throw std::runtime_error("Could not enable tracking: " + std::string(e.what()));
    }
    if (!vlWorker_IsRunning(worker.get()))
    {
        throw std::runtime_error("Tracker is not Running");
    }
    return worker;
}
} // namespace

MultiViewDetector::MultiViewDetector(
    const std::string& licenseFilepath,
    const std::string& trackingConfigFilepath) :
    _worker(createSyncWorker(licenseFilepath, trackingConfigFilepath))
{
    unsigned long resultLength;
    auto resultPtr = ByteBuffer(vlSDKUtil_get(trackingConfigFilepath.c_str(), &resultLength));
    if (!resultPtr)
    {
        throw std::runtime_error(
            "Could not retrieve tracking configuration file from " + trackingConfigFilepath);
    }
    auto configJson = json::parse(resultPtr.get());

    _trackerName = configJson["tracker"]["name"].get<std::string>();
    _anchorName = configJson["tracker"]["parameters"]["anchors"][0]["name"].get<std::string>();
    _inputName = configJson["input"]["useImageSource"].get<std::string>();
    _cameraCount =
        configJson["tracker"]["parameters"]["anchors"][0]["parameters"]["trackingCameras"].size();
}

void MultiViewDetector::enableTextureMapping(
    const bool enabled,
    std::optional<nlohmann::json> config)
{
    std::string enabledString = enabled ? "true" : "false";
    execute(_worker, setAttributeCommand("textureMappingEnabled", enabledString));
    _textureMappingEnabled = enabled;

    if (config.has_value())
    {
        execute(_worker, setAttributeCommand("textureMappingConfig", config.value().dump()));
    }
}

void MultiViewDetector::disablePoseEstimation(const bool disableEstimation)
{
    execute(
        _worker,
        setAttributeCommand("disablePoseEstimation", disableEstimation ? "true" : "false"));
}

ExtrinsicDataHelpers::Extrinsic MultiViewDetector::runDetection(const Frame& frame)
{
    // Without reset, the tracker tries to find the object based on the pose in the previous
    // frame
    resetTracker();
    injectFrame(frame);
    vlWorker_RunOnceSync(_worker.get());
    return getExtrinsic();
}

void MultiViewDetector::runWithExternalTracking(
    const Frame& frame,
    const ExtrinsicDataHelpers::Extrinsic& extrinsic)
{
    resetTracker();
    injectFrame(frame);
    injectExtrinsic(extrinsic);
    vlWorker_RunOnceSync(_worker.get());
}

// Images of the detected model edges on a black background, one for each camera perspective
Frame MultiViewDetector::getLineModelImages() const
{
    std::vector<cv::Mat> images;
    for (size_t camIdx = 0; camIdx < _cameraCount; camIdx++)
    {
        auto key = "imageLineModel_" + std::to_string(camIdx);
        Image visImage(vlWorker_GetNodeImageSync(_worker.get(), _trackerName.c_str(), key.c_str()));
        images.push_back(ImageHelpers::toCVMat(visImage));
    }
    return images;
}

cv::Mat MultiViewDetector::getTextureImage() const
{
    if (!_textureMappingEnabled)
    {
        throw std::runtime_error("Cannot run getTextureImage() with texture mapping disabled.");
    }
    Image visImage(vlWorker_GetNodeImageSync(
        _worker.get(), _trackerName.c_str(), ("mappedTexture" + _anchorName).c_str()));
    return ImageHelpers::toCVMat(visImage);
}

ExtrinsicDataHelpers::Extrinsic MultiViewDetector::getExtrinsic() const
{
    SimilarityTransform worldFromAnchorTransform(
        vlWorker_GetWorldFromAnchorTransform(_worker.get(), _anchorName.c_str()));
    return ExtrinsicDataHelpers::toExtrinsic(worldFromAnchorTransform.get());
}

void MultiViewDetector::resetTracker()
{
    execute(_worker, getResetHardCommand(_trackerName));
}

void MultiViewDetector::injectFrame(const Frame& frame)
{
    if (frame.size() != _cameraCount)
    {
        throw std::runtime_error(
            "Cannot inject frame: Number of images in frame does not match number of cameras!");
    }
    for (int camIdx = 0; camIdx < _cameraCount; camIdx++)
    {
        const auto key = "injectImage_" + std::to_string(camIdx);
        vlWorker_SetNodeImageSync(
            _worker.get(),
            ImageHelpers::toVLImageGrey(frame[camIdx]).get(),
            _inputName.c_str(),
            key.c_str());
    }
}

void MultiViewDetector::injectExtrinsic(const ExtrinsicDataHelpers::Extrinsic& extrinsic)
{
    execute(_worker, setInitPoseCommand(extrinsic));
}