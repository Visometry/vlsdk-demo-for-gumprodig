#include <Visualization/ResultVisualization.h>

#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

namespace
{
cv::Mat combineView(const cv::Mat& cameraImage, const cv::Mat& lineModelImage)
{
    cv::Mat resizedImage;
    cv::resize(cameraImage, resizedImage, lineModelImage.size());
    cv::cvtColor(resizedImage, resizedImage, cv::COLOR_GRAY2RGB);
    resizedImage += lineModelImage;
    return resizedImage;
}

cv::Size getRasterSize(const unsigned int numImages)
{
    const auto cols = std::ceil(std::sqrt(static_cast<double>(numImages)));
    const auto rows = std::round(std::sqrt(static_cast<double>(numImages)));
    return cv::Size(cols, rows);
}

cv::Mat createRasteredView(const std::vector<cv::Mat>& images)
{
    if (images.empty())
    {
        return cv::Mat();
    }

    const auto rasterSize = getRasterSize(images.size());

    const auto imageWidth = images[0].cols;
    const auto imageHeight = images[0].rows;

    auto rasteredView =
        cv::Mat(rasterSize.height * imageHeight, rasterSize.width * imageWidth, images[0].type());
    for (size_t imageID = 0; imageID < images.size(); imageID++)
    {
        const auto imageIndexX = imageID % rasterSize.width;
        const auto imageIndexY = imageID / rasterSize.width;

        images[imageID].copyTo(rasteredView(cv::Rect(
            imageIndexX * imageWidth, imageIndexY * imageHeight, imageWidth, imageHeight)));
    }

    return rasteredView;
}

cv::Size getWindowSize(const cv::Size& imageSize)
{
    constexpr double maxWidth = 1200.0, maxHeight = 800.0;
    const auto imgWidth = static_cast<double>(imageSize.width);
    const auto imgHeight = static_cast<double>(imageSize.height);
    const auto limitedByWidth = (imgWidth / maxWidth) > (imgHeight / maxHeight);
    if (limitedByWidth)
    {
        const auto height = maxWidth * imgHeight / imgWidth;
        return {static_cast<int>(maxWidth), static_cast<int>(height)};
    }
    const auto width = maxHeight * imgWidth / imgHeight;
    return {static_cast<int>(width), static_cast<int>(maxHeight)};
}



size_t getImageIndexInRasteredView(
    const unsigned int x,
    const unsigned int y,
    const unsigned int numImages,
    const cv::Size& rasteredViewSize)
{
    const auto rasterSize = getRasterSize(numImages);
    const auto windowSize = getWindowSize(rasteredViewSize);

    const auto imageSizeX = windowSize.width / rasterSize.width;
    const auto imageSizeY = windowSize.height / rasterSize.height;

    const auto imageIndexX = x / imageSizeX;
    const auto imageIndexY = y / imageSizeY;
    return (imageIndexY * static_cast<unsigned int>(rasterSize.width)) + imageIndexX;
}

void selectAndShowImage(int evt, int x, int y, int flags, void* userdata)
{
    if (evt == cv::EVENT_LBUTTONDOWN)
    {
        const auto& datapair =
            *reinterpret_cast<std::pair<const std::vector<cv::Mat>&, const cv::Size>*>(userdata);
        const auto& combinedViews = datapair.first;
        const auto& rasteredViewSize = datapair.second;
        const auto imageID =
            getImageIndexInRasteredView(x, y, combinedViews.size(), rasteredViewSize);
        const auto winName =
            "Camera " + std::to_string(imageID) + " - Detailed View - Press any key to continue";
        Visualization::showImage(combinedViews[imageID], winName);
    }
};
} // namespace

namespace Visualization
{
std::vector<cv::Mat> combineViews(
    const std::vector<cv::Mat>& cameraImages,
    const std::vector<cv::Mat>& lineModelImages)
{
    if (cameraImages.size() != lineModelImages.size())
    {
        throw std::runtime_error("cameraImages and lineModelImages do not have the same size!");
    }
    std::vector<cv::Mat> combinedViews;
    std::transform(
        cameraImages.begin(),
        cameraImages.end(),
        lineModelImages.begin(),
        std::back_inserter(combinedViews),
        combineView);
    return combinedViews;
}

void showImagesInteractive(const std::vector<cv::Mat>& combinedViews, const std::string& title)
{
    const auto rasteredView = createRasteredView(combinedViews);
    auto winName = title + " - Click on image for detailed view - Press any key to continue";
    showImage(rasteredView, winName);

    auto userdata = std::make_pair(std::cref(combinedViews), rasteredView.size());
    cv::setMouseCallback(winName, selectAndShowImage, reinterpret_cast<void*>(&userdata));

    cv::waitKey(0);
    cv::destroyAllWindows();
}

void showImage(const cv::Mat& image, const std::string& title)
{
    auto screenSize = getWindowSize(image.size());
    cv::Mat resizedImage;
    cv::resize(image, resizedImage, screenSize);

    cv::imshow(title, resizedImage);
}
} // namespace Visualization
