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

std::string composeImagePath(const std::string imageDir, const size_t frameIdx)
{
    return imageDir + "/multiViewImage_" + std::to_string(frameIdx) + ".tif";
}

std::string composeTexturePath(const std::string imageDir, const size_t frameIdx)
{
    return imageDir + "/extracted_textures/texture_from_multiViewImage_" +
           std::to_string(frameIdx) + ".png";
}

Frame loadFrame(const std::string imageDir, const size_t frameIdx)
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

        if (extractTexture)
        {
            detector.enableTextureMapping(
                true, TextureMappingConfig().toJson()); // config is optional
        }

        for (size_t frameIdx = 0; frameIdx < frameCount; frameIdx++)
        {
            std::cout << "Loading Frame " << frameIdx << "...\n";
            const auto frame = loadFrame(imageDir, frameIdx);

            std::cout << "Detecting...\n";
            const auto extrinsic = detector.runDetection(frame);

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
