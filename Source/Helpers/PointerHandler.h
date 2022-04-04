#pragma once

#include <vlSDK.h>

#include <memory>

template<typename T, void(VL_CALLINGCONVENTION* DeleteFn)(T*)>
struct DeleteFunctor
{
    void operator()(T* ptr)
    {
        DeleteFn(ptr);
    }
};

using ByteBuffer = std::unique_ptr<char, DeleteFunctor<const char, &vlReleaseBinaryBuffer>>;

using Image =
    std::unique_ptr<vlImageWrapper_t, DeleteFunctor<vlImageWrapper_t, &vlDelete_ImageWrapper>>;

using Worker = std::unique_ptr<vlWorker_t, DeleteFunctor<vlWorker_t, &vlDelete_Worker>>;
