#pragma once

#include <vlSDK.h>

#include <array>
#include <string>

class Extrinsic
{
    vlExtrinsicDataWrapper_t* _extrinsic = nullptr;

public:
    Extrinsic(vlExtrinsicDataWrapper_t* extrinsic);

    ~Extrinsic();

    std::array<float, 3> getT() const;

    std::array<float, 4> getR() const;

    bool getValid() const;

    std::string getDescription() const;
};
