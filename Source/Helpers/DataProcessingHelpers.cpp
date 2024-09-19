#include <Helpers/DataProcessingHelpers.h>

constexpr auto extrinsicsKeyName = "imageFileName";
constexpr int indentNumSpaces = 4;

namespace
{
nlohmann::json readJsonFile(const std::string& configFilePath)
{
    std::ifstream ifs(configFilePath);
    return nlohmann::json::parse(ifs);
}

std::unordered_map<std::string, ExtrinsicDataHelpers::Extrinsic>
    toHashMapOfExtrinsics(const nlohmann::json& json)
{
    if (!json.is_array())
    {
        throw std::runtime_error("json argument is not an array");
    }

    std::unordered_map<std::string, ExtrinsicDataHelpers::Extrinsic> extrinsics;
    for (const auto& elem : json)
    {
        const auto& key = (std::string)elem[extrinsicsKeyName];
        extrinsics[key] = ExtrinsicDataHelpers::toExtrinsic(elem);
    }
    return extrinsics;
}
} // namespace

DataProcessingHelpers::Frame DataProcessingHelpers::loadFrame(const std::string& path)
{
    DataProcessingHelpers::Frame images;
    if (!cv::imreadmulti(path, images))
    {
        throw std::runtime_error("Unable to load multipage image");
    }
    return images;
}

void DataProcessingHelpers::writeImage(const cv::Mat& cvImage, const std::string& path)
{
    std::filesystem::create_directories(std::filesystem::path(path).parent_path());
    cv::imwrite(path, cvImage);
}

std::unordered_map<std::string, ExtrinsicDataHelpers::Extrinsic>
    DataProcessingHelpers::loadTrackingResults(const std::optional<std::filesystem::path>& filePath)
{
    if (!filePath.has_value() || !std::filesystem::exists(filePath.value()) ||
        filePath.value().extension() != ".json")
    {
        throw std::runtime_error(
            "No JSON file with extrinsics found in the image sequence directory");
    }

    const auto& extrinsicsJson = readJsonFile(filePath.value().string());
    return toHashMapOfExtrinsics(extrinsicsJson);
}

void DataProcessingHelpers::writeExtrinsicsJson(
    const std::unordered_map<std::string, ExtrinsicDataHelpers::Extrinsic>& extrinsics,
    const std::string& path)
{
    nlohmann::json results;

    for (const auto& extrinsicKeyValue : extrinsics)
    {
        nlohmann::json extrinsicJson;
        extrinsicJson[extrinsicsKeyName] = extrinsicKeyValue.first;
        const auto& extrinsic = extrinsicKeyValue.second;
        nlohmann::json t = nlohmann::json::array({extrinsic.t[0], extrinsic.t[1], extrinsic.t[2]});
        extrinsicJson["t"] = t;
        nlohmann::json q =
            nlohmann::json::array({extrinsic.q[0], extrinsic.q[1], extrinsic.q[2], extrinsic.q[3]});
        extrinsicJson["r"] = q;
        results.push_back(extrinsicJson);
    }

    std::filesystem::create_directories(std::filesystem::path(path).parent_path());
    std::ofstream file(path);
    file << results.dump(indentNumSpaces);
}