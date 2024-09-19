#include <Helpers/ExtrinsicDataHelpers.h>

#include <stdexcept>

using namespace ExtrinsicDataHelpers;

Extrinsic ExtrinsicDataHelpers::toExtrinsic(vlExtrinsicDataWrapper_t* extrinsicDataWrapper)
{
    if (!extrinsicDataWrapper)
    {
        throw std::runtime_error("extrinsicDataWrapper empty");
    }
    Extrinsic result;
    result.valid = vlExtrinsicDataWrapper_GetValid(extrinsicDataWrapper);
    vlExtrinsicDataWrapper_GetT(extrinsicDataWrapper, result.t.data(), 3);
    vlExtrinsicDataWrapper_GetR(extrinsicDataWrapper, result.q.data(), 4);
    return result;
}

Extrinsic
    ExtrinsicDataHelpers::toExtrinsic(vlSimilarityTransformWrapper_t* similarityTransformWrapper)
{
    if (!similarityTransformWrapper)
    {
        throw std::runtime_error("similarityTransformWrapper empty");
    }
    ExtrinsicDataHelpers::Extrinsic result;
    result.valid = vlSimilarityTransformWrapper_GetValid(similarityTransformWrapper);
    vlSimilarityTransformWrapper_GetT(similarityTransformWrapper, result.t.data(), 3);
    vlSimilarityTransformWrapper_GetR(similarityTransformWrapper, result.q.data(), 4);
    return result;
}

std::string ExtrinsicDataHelpers::to_string(const ExtrinsicDataHelpers::Extrinsic& ext)
{
    std::string descr = "{\n";
    descr += "    \"t\": " + describe(ext.t) + ",\n";
    descr += "    \"q\": " + describe(ext.q) + ",\n";
    return descr + "    \"valid\": " + (ext.valid ? "true" : "false") + "\n}\n";
}

std::ostream& operator<<(std::ostream& os, const ExtrinsicDataHelpers::Extrinsic& ext)
{
    os << to_string(ext);
    return os;
}
