#include <Extrinsic.h>

#include <numeric>

namespace
{
template<size_t size>
std::string toString(const std::array<float, size>& arr)
{
    return "{" +
           std::accumulate(
               std::next(arr.begin()),
               arr.end(),
               std::to_string(arr[0]),
               [](std::string& a, float b) { return a + "f, " + std::to_string(b); }) +
           "f}";
}

} // namespace

Extrinsic::Extrinsic(vlExtrinsicDataWrapper_t* extrinsic) : _extrinsic(extrinsic) {}

Extrinsic::~Extrinsic()
{
    vlDelete_ExtrinsicDataWrapper(_extrinsic);
}

std::array<float, 3> Extrinsic::getT() const
{
    std::array<float, 3> _t;
    vlExtrinsicDataWrapper_GetT(_extrinsic, _t.data(), 3);
    return _t;
}

std::array<float, 4> Extrinsic::getR() const
{
    std::array<float, 4> _r;
    vlExtrinsicDataWrapper_GetR(_extrinsic, _r.data(), 4);
    return _r;
}

bool Extrinsic::getValid() const
{
    return vlExtrinsicDataWrapper_GetValid(_extrinsic);
}

std::string Extrinsic::getDescription() const
{
    std::string descr = "Extrinsic:\n";
    descr += "  t     = " + toString(getT()) + "\n";
    descr += "  r     = " + toString(getR()) + "\n";
    return descr + "  valid = " + (getValid() ? "true" : "false");
}
