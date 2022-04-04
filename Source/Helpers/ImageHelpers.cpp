#include <Helpers/ImageHelpers.h>

#include <opencv2/imgproc.hpp>
#include <vlSDK.h>

#include <string>

namespace Helpers
{
Image toVLImageGrey(const cv::Mat& imageGrey)
{
    if (imageGrey.channels() != 1)
    {
        throw std::runtime_error("Given images are not GREY images");
    }

    Image vlImage(vlNew_ImageWrapper(vlImageFormat::VL_IMAGE_FORMAT_GREY));
    vlImageWrapper_CopyFromBuffer(vlImage.get(), imageGrey.data, imageGrey.cols, imageGrey.rows);
    return vlImage;
}

cv::Mat toCVMat(const Image& vlImage)
{
    const auto& img = vlImage.get();
    if (!img)
    {
        throw std::runtime_error("No valid image pointer");
    }
    unsigned int w = vlImageWrapper_GetWidth(img);
    unsigned int h = vlImageWrapper_GetHeight(img);
    if (w == 0 || h == 0)
    {
        throw std::runtime_error(
            "No valid image size " + std::to_string(w) + " x " + std::to_string(h));
    }

    cv::Mat imageBGR(h, w, CV_8UC3);
    auto format = vlImageWrapper_GetFormat(img);
    if (format == vlImageFormat::VL_IMAGE_FORMAT_GREY)
    {
        cv::Mat imageVL(h, w, CV_8UC1);
        vlImageWrapper_CopyToBuffer(img, imageVL.data, w * h);
        cv::cvtColor(imageVL, imageBGR, cv::COLOR_GRAY2BGR);
    }
    else if (format == vlImageFormat::VL_IMAGE_FORMAT_RGBA)
    {
        cv::Mat imageVL(h, w, CV_8UC4);
        vlImageWrapper_CopyToBuffer(img, imageVL.data, w * h * 4);
        cv::cvtColor(imageVL, imageBGR, cv::COLOR_RGBA2BGR);
    }
    else if (format == vlImageFormat::VL_IMAGE_FORMAT_RGB)
    {
        cv::Mat imageVL(h, w, CV_8UC3);
        vlImageWrapper_CopyToBuffer(img, imageVL.data, w * h * 3);
        cv::cvtColor(imageVL, imageBGR, cv::COLOR_RGB2BGR);
    }
    else
    {
        throw std::runtime_error("Unsupported image type");
    }
    return imageBGR;
}

} // namespace Helpers
