#pragma once

#include <vlSDK.h>

#include "PointerHandler.h"

#include <nlohmann/json.hpp>

#include <chrono>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#define DEBRACE(...) __VA_ARGS__
#define INFO_MSG(...) printf(DEBRACE __VA_ARGS__)
#define WARNING_MSG(...) printf(DEBRACE __VA_ARGS__)
#define FATAL_MSG(...) printf(DEBRACE __VA_ARGS__)


template<size_t size>
std::string toString(const std::array<float, size>& arr)
{
    std::string descr = "{";
    for (size_t i = 0; i < size - 1; i++)
    {
        descr += std::to_string(arr[i]) + "f, ";
    }
    return descr + std::to_string(arr[size - 1]) + "f}";
}


struct WorkerFixture
{
    Worker worker = nullptr;

    WorkerFixture() = default;

    vlWorker_t* get()
    {
        return worker.get();
    }
};

struct AsyncWorkerFixture : public WorkerFixture
{
    AsyncWorkerFixture(const char* licenseFile);
};

struct SyncWorkerFixture : public WorkerFixture
{
    SyncWorkerFixture(const char* licenseFile);
};

std::string getLocalFilePath(const std::string& fileName);

std::string resolveFilePath(const std::string& filePath);

class DeleteFileAfterScope
{
    std::string _resolvedFilePath;

public:
    explicit DeleteFileAfterScope(const std::string& filePath);
    ~DeleteFileAfterScope();
};

std::string executeSync(SyncWorkerFixture& worker, const std::string& command);

void setAttributeSync(SyncWorkerFixture& worker, const std::string& name, const std::string& value);
std::string getAttributeSync(SyncWorkerFixture& worker, const std::string& name);

struct BinaryCommandResult
{
    nlohmann::json result;
    ByteBuffer data;
    unsigned int size;
    BinaryCommandResult() = default;
    BinaryCommandResult(BinaryCommandResult&&) = default;
    BinaryCommandResult& operator=(BinaryCommandResult&&) = default;
};

BinaryCommandResult executeSync(
    SyncWorkerFixture& worker,
    const std::string& command,
    const char data[],
    unsigned int size);

std::string getCreateTrackerCommand(const std::string& trackerPath);
std::string getCreateTrackerFromStringCommand(
    const std::string& trackerContent,
    const std::string& trackerPath);

std::string getRunTrackingCommand();

bool createTracker(AsyncWorkerFixture& worker, const std::string& trackerPath);
bool runTracker(AsyncWorkerFixture& worker);

bool startTracker(AsyncWorkerFixture& worker, const std::string& trackerPath);
bool startTrackerSync(
    SyncWorkerFixture& worker,
    const std::string& trackerPath,
    const std::string& trackerContent = "");

// Wait until the given function returns true or a timeout occurs.
// Returns true, if the function returned true before the timeout occurred.
// Returns false, otherwise.
template<typename FunctionT>
bool waitFor(FunctionT& f, const std::chrono::milliseconds& timeout)
{
    using Clock = std::chrono::steady_clock;
    Clock::time_point startTime = Clock::now();
    do
    {
        if (f())
        {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    } while ((Clock::now() - startTime) < timeout);

    return false;
}

// Functor for acquiring a valid image, extrinsic data, intrinsic data and
// tracking state
class GetValidFrameData
{
public:
    enum class Required
    {
        Image_IntrinsicData_ExtrinsicData,
        Image_IntrinsicData
    };

    static void VL_CALLINGCONVENTION dispatchImageEvent(vlImageWrapper_t* image, void* clientData)
    {
        reinterpret_cast<GetValidFrameData*>(clientData)->handleImageEvent(image);
    }

    static void VL_CALLINGCONVENTION
        dispatchExtrinsicDataEvent(vlExtrinsicDataWrapper_t* extrinsicData, void* clientData)
    {
        reinterpret_cast<GetValidFrameData*>(clientData)->handleExtrinsicDataEvent(extrinsicData);
    }

    static void VL_CALLINGCONVENTION
        dispatchIntrinsicDataEvent(vlIntrinsicDataWrapper_t* intrinsicData, void* clientData)
    {
        reinterpret_cast<GetValidFrameData*>(clientData)->handleIntrinsicDataEvent(intrinsicData);
    }

    static void VL_CALLINGCONVENTION dispatchTrackingStateEvent(const char data[], void* clientData)
    {
        reinterpret_cast<GetValidFrameData*>(clientData)->handleTrackingStateEvent(data);
    }

    GetValidFrameData(const GetValidFrameData& other) :
        _worker(other._worker),
        _gotValidImage(other._gotValidImage),
        _gotValidExtrinsicData(other._gotValidExtrinsicData),
        _gotValidIntrinsicData(other._gotValidIntrinsicData),
        _requiredElements(other._requiredElements)
    {
        vlWorker_AddImageListener(_worker, &GetValidFrameData::dispatchImageEvent, this);
        vlWorker_AddExtrinsicDataListener(
            _worker, &GetValidFrameData::dispatchExtrinsicDataEvent, this);
        vlWorker_AddIntrinsicDataListener(
            _worker, &GetValidFrameData::dispatchIntrinsicDataEvent, this);
        vlWorker_AddTrackingStateListener(
            _worker, &GetValidFrameData::dispatchTrackingStateEvent, this);
    }

    GetValidFrameData(
        vlWorker_t* worker,
        Required requiredElements = Required::Image_IntrinsicData_ExtrinsicData) :
        _worker(worker), _requiredElements(requiredElements)
    {
        vlWorker_AddImageListener(_worker, &GetValidFrameData::dispatchImageEvent, this);
        vlWorker_AddExtrinsicDataListener(
            _worker, &GetValidFrameData::dispatchExtrinsicDataEvent, this);
        vlWorker_AddIntrinsicDataListener(
            _worker, &GetValidFrameData::dispatchIntrinsicDataEvent, this);
        vlWorker_AddTrackingStateListener(
            _worker, &GetValidFrameData::dispatchTrackingStateEvent, this);
    }

    ~GetValidFrameData()
    {
        vlWorker_RemoveImageListener(_worker, &GetValidFrameData::dispatchImageEvent, this);
        vlWorker_RemoveExtrinsicDataListener(
            _worker, &GetValidFrameData::dispatchExtrinsicDataEvent, this);
        vlWorker_RemoveIntrinsicDataListener(
            _worker, &GetValidFrameData::dispatchIntrinsicDataEvent, this);
        vlWorker_RemoveTrackingStateListener(
            _worker, &GetValidFrameData::dispatchTrackingStateEvent, this);
    }

    void handleImageEvent(vlImageWrapper_t* image)
    {
        if (vlImageWrapper_GetWidth(image) > 0 && vlImageWrapper_GetHeight(image) > 0)
        {
            _gotValidImage = true;
        }
    }

    void handleExtrinsicDataEvent(vlExtrinsicDataWrapper_t* extrinsicData)
    {
        if (extrinsicData)
        {
            _gotExtrinsicData = true;
        }
        if (vlExtrinsicDataWrapper_GetValid(extrinsicData))
        {
            _gotValidExtrinsicData = true;
        }
    }

    void handleIntrinsicDataEvent(vlIntrinsicDataWrapper_t* intrinsicData)
    {
        if (intrinsicData != nullptr)
        {
            _gotValidIntrinsicData = true;
        }
    }

    void handleTrackingStateEvent(const std::string& data)
    {
        const auto trackedObject = nlohmann::json::parse(data)["objects"][0];
        _timeStamp = trackedObject["timeStamp"].get<double>();
        _quality = trackedObject["quality"].get<double>();
    }

    bool operator()() const
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        vlWorker_PollEvents(_worker);
        switch (_requiredElements)
        {
            default:
            case Required::Image_IntrinsicData_ExtrinsicData:
                return _gotValidImage && _gotValidIntrinsicData && _gotValidExtrinsicData;
            case Required::Image_IntrinsicData:
                return _gotValidImage && _gotValidIntrinsicData;
        }
    }

    bool gotValidImage() const
    {
        return _gotValidImage;
    }

    bool gotExtrinsicData() const
    {
        return _gotExtrinsicData;
    }

    bool gotValidExtrinsicData() const
    {
        return _gotValidExtrinsicData;
    }

    bool gotValidIntrinsicData() const
    {
        return _gotValidIntrinsicData;
    }

    double getTimeStamp() const
    {
        return _timeStamp;
    }

    double getQuality() const
    {
        return _quality;
    }

private:
    vlWorker_t* _worker = nullptr;
    bool _gotValidImage = false;
    bool _gotValidExtrinsicData = false;
    bool _gotExtrinsicData = false;
    bool _gotValidIntrinsicData = false;
    Required _requiredElements;

    double _timeStamp = 0.0;
    double _quality = 0.0;
};

bool didTrackSync(SyncWorkerFixture& worker);

bool tracksInFramesSync(SyncWorkerFixture& worker, int numberOfFrames);

int countTrackedFramesSync(SyncWorkerFixture& worker, int numberOfFrames);