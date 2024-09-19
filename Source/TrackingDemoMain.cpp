#include <Helpers/DataProcessingHelpers.h>
#include <Helpers/ImageHelpers.h>
#include <MultiViewDetector.h>
#include <Visualization/ResultVisualization.h>

#include <nlohmann/json.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <vlSDK.h>

#include <filesystem>
#include <iostream>
#include <vector>

namespace
{
constexpr int frameCount = 2;
constexpr auto visualizeResults = true;
constexpr auto extractTexture = true;
constexpr auto useExternalTracking = true;

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

Frame getNextFrame(const std::string& imageDir, const size_t frameIdx)
{
    return DataProcessingHelpers::loadFrame(composeImagePath(imageDir, frameIdx));
}

void writeTextureImage(const cv::Mat& cvImage, const std::string& imageDir, const size_t& frameIdx)
{
    std::string texturePath = composeTexturePath(imageDir, frameIdx);
    DataProcessingHelpers::writeImage(cvImage, texturePath);
}

// Provided just for the demo, use your tracking algorithm instead.
ExtrinsicDataHelpers::Extrinsic getTrackingResult(
    const std::unordered_map<std::string, ExtrinsicDataHelpers::Extrinsic>& extrinsics,
    const size_t& frameIdx)
{
    const auto imgFileName = composeImageName(frameIdx);

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
            // extrinsics are loaded from the file to use the saved results as an example;
            // loading tracking results is not needed if you use a real external tracking
            // algorithm
            extrinsics =
                DataProcessingHelpers::loadTrackingResults(imageDir + "/trackingResults.json");
        }

        for (size_t frameIdx = 0; frameIdx < frameCount; frameIdx++)
        {
            std::cout << "Loading Frame " << frameIdx << "...\n";
            const auto frame = getNextFrame(imageDir, frameIdx);

            ExtrinsicDataHelpers::Extrinsic extrinsic;
            if (!useExternalTracking)
            {
                std::cout << "Detecting...\n";
                extrinsic = detector.runDetection(frame);
            }
            else
            {
                std::cout << "Running with external tracking...\n";
                extrinsic = getTrackingResult(extrinsics, frameIdx);
                detector.runWithExternalTracking(frame, extrinsic);
            }

            std::cout << "World from model transform:\n" << extrinsic << "\n";

            if (extractTexture)
            {
                const auto& textureImage = detector.getTextureImage();
                writeTextureImage(textureImage, imageDir, frameIdx);

                std::cout << "Saved the extracted texture in "
                          << composeTexturePath(imageDir, frameIdx) << "\n\n";

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
