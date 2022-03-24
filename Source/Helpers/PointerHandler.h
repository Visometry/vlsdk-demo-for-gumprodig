#include <vlSDK.h>

#include <memory>

template <typename T, void (VL_CALLINGCONVENTION *DeleteFn)(T*)>
struct DeleteFunctor
{
    void operator()(T* ptr)
    {
        DeleteFn(ptr);
    }
};

using ByteBuffer = std::unique_ptr<char, DeleteFunctor<const char, &vlReleaseBinaryBuffer>>;

using ExtrinsicData = std::unique_ptr<
    vlExtrinsicDataWrapper_t, 
    DeleteFunctor<
        vlExtrinsicDataWrapper_t,
        &vlDelete_ExtrinsicDataWrapper>>;

using SimilarityTransform = std::unique_ptr<
    vlSimilarityTransformWrapper_t, 
    DeleteFunctor<
        vlSimilarityTransformWrapper_t,
        &vlDelete_SimilarityTransformWrapper>>;

using Image = std::unique_ptr <
    vlImageWrapper_t, 
    DeleteFunctor<
        vlImageWrapper_t,
        &vlDelete_ImageWrapper>>;

using IntrinsicData = std::unique_ptr<
    vlIntrinsicDataWrapper_t, 
    DeleteFunctor<
        vlIntrinsicDataWrapper_t, 
        &vlDelete_IntrinsicDataWrapper>>;

using Worker = std::unique_ptr<
    vlWorker_t, 
    DeleteFunctor<
        vlWorker_t, 
        &vlDelete_Worker>>;