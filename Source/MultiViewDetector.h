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

    ExtrinsicDataHelpers::Extrinsic runDetection(const Frame& frame);

    Frame getLineModelImages() const;
    cv::Mat getTextureImage() const;

private:
    void resetTracker();
    void injectFrame(const Frame& frame);

    Worker _worker;
    std::string _trackerName;
    std::string _anchorName;
    std::string _inputName;
    unsigned int _cameraCount;
    bool _textureMappingEnabled = false;
};
