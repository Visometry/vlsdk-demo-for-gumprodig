#include <Helpers/ImageHelpers.h>
#include <MultiViewDetector.h>
#include <Visualization/ResultVisualization.h>

#include <nlohmann/json.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <vlSDK.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <vector>

namespace
{
constexpr int frameCount = 2;
constexpr auto visualizeResults = true;
constexpr auto extractTexture = true;
constexpr auto useExternalTracking = true;
constexpr auto extrinsicsKeyName = "imageFileName";
constexpr int indentNumSpaces = 4;

using Frame = std::vector<cv::Mat>;

struct TextureMappingConfig
{
    enum BRIGHTNESS_CORRECTION_METHOD
    {
        EXPLICIT_CONTRAST_MEAN = 0,
        TOTAL_LEAST_SQUARES = 1
    };

    BRIGHTNESS_CORRECTION_METHOD method = TOTAL_LEAST_SQUARES;
    // threshold values between 0 and 1
    float minScoreForOptimization = 0.4f;
    float whiteOutlierMaxScore = 0.8f;
    float whiteOutlierMinValue = 0.8f;
    // relevant only for EXPLICIT_CONTRAST_MEAN method
    int referenceFrameID = 0;

    int stride = 3;
    int width = 1024;
    int height = 1024;
    bool verbose = false;

    nlohmann::json toJson()
    {
        nlohmann::json config;
        config["method"] = static_cast<int>(method);
        config["minScoreForOptimization"] = minScoreForOptimization;
        config["whiteOutlierMaxScore"] = whiteOutlierMaxScore;
        config["whiteOutlierMinValue"] = whiteOutlierMinValue;
        config["referenceFrameID"] = referenceFrameID;
        config["stride"] = stride;
        config["width"] = width;
        config["height"] = height;
        config["verbose"] = verbose;
        return config;
    }
};

std::string composeImageName(const size_t frameIdx)
{
    return "multiViewImage_" + std::to_string(frameIdx);
}

std::string composeImagePath(const std::string& imageDir, const size_t frameIdx)
{
    return imageDir + "/" + composeImageName(frameIdx) + ".tif";
}

std::string composeTexturePath(const std::string& imageDir, const size_t frameIdx)
{
    return imageDir + "/extracted_textures/texture_from_" + composeImageName(frameIdx) + ".png";
}

Frame loadFrame(const std::string& imageDir, const size_t frameIdx)
{
    Frame images;
    if (!cv::imreadmulti(composeImagePath(imageDir, frameIdx), images))
    {
        throw std::runtime_error("Unable to load multipage image");
    }
    return images;
}

void writeImage(const cv::Mat& cvImage, const std::string& path)
{
    std::filesystem::create_directories(std::filesystem::path(path).parent_path());
    cv::imwrite(path, cvImage);
}

void writeExtrinsicsJson(
    const std::unordered_map<std::string, ExtrinsicDataHelpers::Extrinsic>& extrinsics,
    const std::string& path)
{
    nlohmann::json results;

    for (const auto& extrinsicKeyValue : extrinsics)
    {
        nlohmann::json extrinsicJson;
        extrinsicJson[extrinsicsKeyName] = extrinsicKeyValue.first;
        const auto& extrinsic = extrinsicKeyValue.second;
        nlohmann::json t = nlohmann::json::array({extrinsic.t[0], extrinsic.t[1], extrinsic.t[2]});
        extrinsicJson["t"] = t;
        nlohmann::json q =
            nlohmann::json::array({extrinsic.q[0], extrinsic.q[1], extrinsic.q[2], extrinsic.q[3]});
        extrinsicJson["r"] = q;
        results.push_back(extrinsicJson);
    }

    std::filesystem::create_directories(std::filesystem::path(path).parent_path());
    std::ofstream file(path);
    file << results.dump(indentNumSpaces);
}

nlohmann::json readJsonFile(const std::string& configFilePath)
{
    std::ifstream ifs(configFilePath);
    return nlohmann::json::parse(ifs);
}

std::unordered_map<std::string, ExtrinsicDataHelpers::Extrinsic>
    toHashMapOfExtrinsics(const nlohmann::json& json)
{
    if (!json.is_array())
    {
        throw std::runtime_error("json argument is not an array");
    }

    std::unordered_map<std::string, ExtrinsicDataHelpers::Extrinsic> extrinsics;
    for (const auto& elem : json)
    {
        const auto& key = (std::string)elem[extrinsicsKeyName];
        extrinsics[key] = ExtrinsicDataHelpers::toExtrinsic(elem);
    }
    return extrinsics;
}

std::unordered_map<std::string, ExtrinsicDataHelpers::Extrinsic>
    loadTrackingResults(const std::optional<std::filesystem::path>& filePath)
{
    if (!filePath.has_value() || !std::filesystem::exists(filePath.value()) ||
        filePath.value().extension() != ".json")
    {
        throw std::runtime_error(
            "No JSON file with extrinsics found in the image sequence directory");
    }

    const auto& extrinsicsJson = readJsonFile(filePath.value().string());
    return toHashMapOfExtrinsics(extrinsicsJson);
}

ExtrinsicDataHelpers::Extrinsic getTrackingResult(
    const std::unordered_map<std::string, ExtrinsicDataHelpers::Extrinsic>& extrinsics,
    const std::string& imgFileName)
{
    if (extrinsics.find(imgFileName) == extrinsics.end())
    {
        std::cout << "No extrinsic found for image '" << imgFileName << "'" << std::endl;
        return ExtrinsicDataHelpers::Extrinsic({{0, 0, 0}, {0, 0, 0, 1}, false});
    }

    return extrinsics.at(imgFileName);
}
} // namespace

//    The detection result is an extrinsic and consists of:
//
//    t     : Translation (Vector)
//    q     : Rotation (Quaternion)
//    valid : Tracking quality is sufficient
//
//    This is the euclidean transform from model/object coordinate system to world coordinate
//    system. The world coordinate system is identical to the camera coordinate system unless
//    tracking runs with SLAM. There is typically one camera (in our example camera_0) which is at
//    the origin of the camera coordinate system and is not rotated.

int main(int argc, char* argv[])
{
    // Load arguments
    if (argc < 4)
    {
        std::cout << "Usage: TrackingDemoMain <vl-file> <image-sequence-dir> <license-file>\n";
        return EXIT_FAILURE;
    }
    const std::string trackingConfigFilepath = argv[1];
    const std::string imageDir = argv[2];
    const std::string licenseFilepath = argv[3];

    try
    {
        std::cout << "Creating detector...\n\n";
        MultiViewDetector detector(licenseFilepath, trackingConfigFilepath);

        detector.enableTextureMapping(
            extractTexture, TextureMappingConfig().toJson()); // config is optional

        std::unordered_map<std::string, ExtrinsicDataHelpers::Extrinsic> extrinsics;
        if (useExternalTracking)
        {
            detector.disablePoseEstimation(useExternalTracking);
            extrinsics = loadTrackingResults(imageDir + "/trackingResults.json");
        }

        for (size_t frameIdx = 0; frameIdx < frameCount; frameIdx++)
        {
            std::cout << "Loading Frame " << frameIdx << "...\n";
            const auto frame = loadFrame(imageDir, frameIdx);

            ExtrinsicDataHelpers::Extrinsic extrinsic;
            if (!useExternalTracking)
            {
                std::cout << "Detecting...\n";
                extrinsic = detector.runDetection(frame);
            }
            else
            {
                std::cout << "Running with external tracking...\n";
                extrinsic = getTrackingResult(extrinsics, composeImageName(frameIdx));
                detector.runWithExternalTracking(frame, extrinsic);
            }

            std::cout << "World from model transform:\n" << extrinsic << "\n";

            if (extractTexture)
            {
                const auto& textureImage = detector.getTextureImage();
                std::string texturePath = composeTexturePath(imageDir, frameIdx);
                writeImage(textureImage, texturePath);

                std::cout << "Saved the extracted texture in " << texturePath << "\n\n";

                if (visualizeResults)
                {
                    // run showImage() before showImagesInteractive() or call cv::waitKey(0);
                    // and cv::destroyAllWindows(); after showImage()
                    Visualization::showImage(textureImage, "Extracted Texture");
                }
            }

            if (visualizeResults)
            {
                Visualization::showImagesInteractive(
                    Visualization::combineViews(frame, detector.getLineModelImages()),
                    "Detection Results");
            }
        }
    }
    catch (const std::exception& e)
    {
        std::cout << "\n\ERROR:\n" << e.what() << "\n";
        return 1;
    }
    return 0;
}
