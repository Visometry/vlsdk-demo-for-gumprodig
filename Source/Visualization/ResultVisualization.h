#pragma once

#include <opencv2/core.hpp>

namespace Visualization
{
std::vector<cv::Mat> combineViews(
    const std::vector<cv::Mat>& cameraImages,
    const std::vector<cv::Mat>& lineModelImages);

void showImagesInteractive(const std::vector<cv::Mat>& uncombinedViews, const std::string& title);
} // namespace Visualization
