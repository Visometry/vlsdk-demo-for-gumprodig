#include <Extrinsic.h>
#include <Helpers/ImageHelpers.h>
#include <MultiViewDetector.h>
#include <Visualization/ResultVisualization.h>

#include <opencv2/imgcodecs.hpp>
#include <vlSDK.h>

#include <iostream>
#include <vector>

constexpr int frameCount = 2;

constexpr auto licenseFilepath = "Resources/license.xml";
constexpr auto trackingConfigFilepath = "Resources/bosch_injected.vl";

constexpr auto visualizeResults = true;

using Frame = std::vector<cv::Mat>;

Frame loadFrame(const size_t frameIdx)
{
    Frame images;
    if (!cv::imreadmulti("Resources/multiViewImage_" + std::to_string(frameIdx) + ".tif", images))
    {
        throw std::runtime_error("Unable to load multipage image");
    }
    return images;
}

//    The detection result is an extrinsic and consists of:
//
//    t     : Translation (Vector)
//    q     : Rotation (Quaternion)
//    valid : Tracking quality is sufficient
//
//    This is the euclidean transform from model/object coordinate system to camera coordinate
//    system. There is typically one camera (in our example camera_0) which is at the origin of the
//    camera coordinate system and is not rotated

int main()
{
    try
    {
        std::cout << "Creating detector...\n\n";
        MultiViewDetector detector(licenseFilepath, trackingConfigFilepath);

        for (size_t frameIdx = 0; frameIdx < frameCount; frameIdx++)
        {
            std::cout << "Loading Frame " << frameIdx << "...\n";
            const auto frame = loadFrame(frameIdx);

            std::cout << "Detecting...\n";
            const auto extrinsic = detector.runDetection(frame);

            std::cout << extrinsic.getDescription() << "\n\n";

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
