#pragma once

#include <vlSDK.h>
#include "HelperFunctions.h"

#include <nlohmann/json.hpp>

#include <chrono>
#include <string>
#include <thread>

class NullCallbackHandler
{
public:
    NullCallbackHandler(vlWorker_t* worker) {}
    NullCallbackHandler(const NullCallbackHandler& other) {}
    void operator()(const char error[], const char result[]) {}
};

// Functor for acquiring the result of a string callback. CallbackHandlerT
// will be called inside the callback function.
template <typename CallbackHandlerT = NullCallbackHandler>
class GetJsonCommandResult
{
public:
    static void VL_CALLINGCONVENTION dispatchCallback(const char error[],
        const char result[], void* clientData)
    {
        reinterpret_cast<GetJsonCommandResult*>(clientData)
            ->handleCallback(error, result);
    }

    GetJsonCommandResult() = delete;

    GetJsonCommandResult(vlWorker_t* worker, const std::string& data)
        : _worker(worker),
        _data(data),
        _callbackHandler(worker)
    {
    }

    GetJsonCommandResult(const GetJsonCommandResult&) = default;

    virtual ~GetJsonCommandResult() = default;

    void handleCallback(const char error[], const char result[])
    {
        _callbackHandler(error, result);
        _gotResult = true;
        if (error == nullptr)
        {
            _error.clear();
            if (result != nullptr)
            {
                _result = result;
            }
        }
        else
        {
            _result.clear();
            _error = error;
        }
    }

    bool operator()()
    {
        if(!_commandPushed)
        {
            bool success = vlWorker_PushJsonCommand(_worker, _data.c_str(),
                &GetJsonCommandResult::dispatchCallback, this);
            _invocationError = !success;
            _commandPushed = true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        vlWorker_ProcessCallbacks(_worker);
        vlWorker_PollEvents(_worker);
        return _gotResult;
    }

    bool getInvocationError() const
    {
        return _invocationError;
    }

    const std::string& getResult() const
    {
        return _result;
    }

    const std::string& getError() const
    {
        return _error;
    }

    bool callbackRecieved() const
    {
        return _gotResult;
    }

protected:
    vlWorker_t* _worker;
    std::string _data;
    bool _gotResult = false;
    bool _invocationError = false;
    CallbackHandlerT _callbackHandler;
    std::string _error;
    std::string _result;
    bool _commandPushed = false;
};

template <typename CallbackHandlerT = NullCallbackHandler>
GetJsonCommandResult<CallbackHandlerT> makeCreateTrackerCommand(vlWorker_t* worker, const std::string& trackerPath)
{
    nlohmann::json createTrackerCommand;
    createTrackerCommand["name"] = "createTracker";
    createTrackerCommand["param"]["uri"] = trackerPath;

    return GetJsonCommandResult<CallbackHandlerT>(worker, createTrackerCommand.dump());
}

template <typename CallbackHandlerT = NullCallbackHandler>
GetJsonCommandResult<CallbackHandlerT> makeRunTrackerCommand(vlWorker_t* worker)
{
    nlohmann::json runTrackingCommand;
    runTrackingCommand["name"] = "runTracking";

    return GetJsonCommandResult<CallbackHandlerT>(worker, runTrackingCommand.dump());
}

template <typename CallbackHandlerT = NullCallbackHandler>
GetJsonCommandResult<CallbackHandlerT> makeRunTrackerOnceCommand(vlWorker_t* worker)
{
    nlohmann::json runTrackingOnceCommand;
    runTrackingOnceCommand["name"] = "runTrackingOnce";

    return GetJsonCommandResult<CallbackHandlerT>(worker, runTrackingOnceCommand.dump());
}

template <typename CallbackHandlerT = NullCallbackHandler>
GetJsonCommandResult<CallbackHandlerT> makeGetAttributeCommand(vlWorker_t* worker, const std::string& attributeName)
{
    nlohmann::json getAttributeCommand;
    getAttributeCommand["name"] = "getAttribute";
    getAttributeCommand["param"]["att"] = attributeName;

    return GetJsonCommandResult<CallbackHandlerT>(worker, getAttributeCommand.dump());
};

template<typename CallbackHandlerT = NullCallbackHandler>
GetJsonCommandResult<CallbackHandlerT>
    makeGetModelPropertiesCommand(vlWorker_t* worker)
{
    nlohmann::json getModelPropertiesCommand;
    getModelPropertiesCommand["name"] = "getModelProperties";

    return GetJsonCommandResult<CallbackHandlerT>(
        worker, getModelPropertiesCommand.dump());
};

template<typename CallbackHandlerT = NullCallbackHandler>
GetJsonCommandResult<CallbackHandlerT>
    makeRemoveAllModelsCommand(vlWorker_t* worker)
{
    nlohmann::json removeModelsCommand;
    removeModelsCommand["name"] = "removeAllModels";

    return GetJsonCommandResult<CallbackHandlerT>(worker, removeModelsCommand.dump());
};

template<typename CallbackHandlerT = NullCallbackHandler>
GetJsonCommandResult<CallbackHandlerT>
    makeAddModelCommand(vlWorker_t* worker, const nlohmann::json& model)
{
    nlohmann::json addModelCommand;
    addModelCommand["name"] = "addModel";
    addModelCommand["param"] = model;

    return GetJsonCommandResult<CallbackHandlerT>(worker, addModelCommand.dump());
};

template<typename CallbackHandlerT = NullCallbackHandler>
GetJsonCommandResult<CallbackHandlerT> makeSetMultipleModelPropertiesCommand(
    vlWorker_t* worker,
    const nlohmann::json& modelPropertiesArray)
{
    nlohmann::json setMultipleModelPropertiesCommand;
    setMultipleModelPropertiesCommand["name"] = "setMultipleModelProperties";
    setMultipleModelPropertiesCommand["param"]["models"] = modelPropertiesArray;

    return GetJsonCommandResult<CallbackHandlerT>(worker, setMultipleModelPropertiesCommand.dump());
};

template<typename CallbackHandlerT = NullCallbackHandler>
GetJsonCommandResult<CallbackHandlerT> makeSetModelPropertiesForAllCommand(
    vlWorker_t* worker, const nlohmann::json& modelProperties)
{
    nlohmann::json setModelPropertiesForAllCommand;
    setModelPropertiesForAllCommand["name"] = "setModelPropertiesForAll";
    setModelPropertiesForAllCommand["param"] = modelProperties;

    return GetJsonCommandResult<CallbackHandlerT>(
        worker, setModelPropertiesForAllCommand.dump());
};

template<typename CallbackHandlerT = NullCallbackHandler>
GetJsonCommandResult<CallbackHandlerT>
    makeSetModelPropertiesCommand(vlWorker_t* worker, const nlohmann::json& modelProperties)
{
    nlohmann::json setModelPropertiesCommand;
    setModelPropertiesCommand["name"] = "setModelProperties";
    setModelPropertiesCommand["param"] = modelProperties;

    return GetJsonCommandResult<CallbackHandlerT>(
        worker, setModelPropertiesCommand.dump());
};

template <typename CallbackHandlerT = NullCallbackHandler>
GetJsonCommandResult<CallbackHandlerT> makeSetAttributeCommand(vlWorker_t* worker, const std::string& attributeName, const std::string& attributeValue)
{
    nlohmann::json setAttributeCommand;
    setAttributeCommand["name"] = "setAttribute";
    setAttributeCommand["param"]["att"] = attributeName;
    setAttributeCommand["param"]["val"] = attributeValue;

    return GetJsonCommandResult<CallbackHandlerT>(worker, setAttributeCommand.dump());
};

