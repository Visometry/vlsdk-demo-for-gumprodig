#pragma once

#include <Helpers/ExtrinsicDataHelpers.h>
#include <Helpers/PointerHandler.h>

#include <nlohmann/json.hpp>
#include <opencv2/core.hpp>
#include <vlSDK.h>

#include <optional>
#include <vector>

using Frame = std::vector<cv::Mat>;

class MultiViewDetector
{
public:
    MultiViewDetector(
        const std::string& licenseFilepath,
        const std::string& trackingConfigFilepath);

    void enableTextureMapping(
        const bool enabled,
        std::optional<nlohmann::json> config = std::nullopt);
    void disablePoseEstimation(const bool enabled);

    ExtrinsicDataHelpers::Extrinsic runDetection(const Frame& frame);
    void runWithExternalTracking(
        const Frame& frame,
        const ExtrinsicDataHelpers::Extrinsic& extrinsic);

    Frame getLineModelImages() const;
    cv::Mat getTextureImage() const;
    ExtrinsicDataHelpers::Extrinsic getExtrinsic() const;

private:
    void resetTracker();
    void injectFrame(const Frame& frame);
    void injectExtrinsic(const ExtrinsicDataHelpers::Extrinsic& extrinsic);

    Worker _worker;
    std::string _trackerName;
    std::string _anchorName;
    std::string _inputName;
    unsigned int _cameraCount;
    bool _textureMappingEnabled = false;
};
