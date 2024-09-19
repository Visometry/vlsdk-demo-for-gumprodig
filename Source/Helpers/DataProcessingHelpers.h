#pragma once

#include <Helpers/ExtrinsicDataHelpers.h>

#include <nlohmann/json.hpp>
#include <opencv2/highgui.hpp>

#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <unordered_map>

namespace DataProcessingHelpers
{
using Frame = std::vector<cv::Mat>;

Frame loadFrame(const std::string& path);
void writeImage(const cv::Mat& cvImage, const std::string& path);
std::unordered_map<std::string, ExtrinsicDataHelpers::Extrinsic>
    loadTrackingResults(const std::optional<std::filesystem::path>& filePath);
void writeExtrinsicsJson(
    const std::unordered_map<std::string, ExtrinsicDataHelpers::Extrinsic>& extrinsics,
    const std::string& path);
} // namespace DataProcessingHelpers