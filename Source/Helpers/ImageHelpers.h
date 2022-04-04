#pragma once

#include <Helpers/PointerHandler.h>

#include <opencv2/core.hpp>

namespace Helpers
{
Image toVLImageGrey(const cv::Mat& imageRGBA);

cv::Mat toCVMat(const Image& vlImage);

} // namespace Helpers
