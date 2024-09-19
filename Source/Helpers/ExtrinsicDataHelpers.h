#pragma once

#include <vlSDK.h>

#include <array>
#include <iostream>
#include <string>

namespace ExtrinsicDataHelpers
{
struct Extrinsic
{
    std::array<float, 3> t;
    std::array<float, 4> q;
    bool valid;
};

Extrinsic toExtrinsic(vlExtrinsicDataWrapper_t* extrinsicDataWrapper);
Extrinsic toExtrinsic(vlSimilarityTransformWrapper_t* similarityTransformWrapper);

std::string to_string(const ExtrinsicDataHelpers::Extrinsic& ext);

template<size_t size>
std::string describe(const std::array<float, size>& arr)
{
    std::string descr = "[";
    for (size_t i = 0; i < size - 1; i++)
    {
        descr += std::to_string(arr[i]) + ", ";
    }
    return descr + std::to_string(arr[size - 1]) + "]";
}
} // namespace ExtrinsicDataHelpers

std::ostream& operator<<(std::ostream& os, const ExtrinsicDataHelpers::Extrinsic& ext);
