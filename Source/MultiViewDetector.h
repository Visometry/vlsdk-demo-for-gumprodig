#pragma once

#include <Extrinsic.h>
#include <Helpers/PointerHandler.h>

#include <opencv2/core.hpp>
#include <vlSDK.h>

#include <vector>

using Frame = std::vector<cv::Mat>;

class MultiViewDetector
{
public:
    MultiViewDetector(
        const std::string& licenseFilepath,
        const std::string& trackingConfigFilepath);

    Extrinsic runDetection(const Frame& frame);

    Frame getLineModelImages() const;

private:
    void resetTracker();
    void injectFrame(const Frame& frame);

    Worker _worker;
    std::string _trackerName;
    std::string _inputName;
    unsigned int _cameraCount;
};
