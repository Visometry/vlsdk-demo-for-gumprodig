#include "HelperFunctions.h"

#include "GetJsonCommandResult.h"

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

#include <array>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#if defined(__ANDROID__)
std::string getLocalFilePath(const std::string& fileName)
{
    return "/android_asset/" + fileName;
}
#elif !TARGET_OS_IPHONE
std::string getLocalFilePath(const std::string& fileName)
{
    return fileName;
}
#endif

bool createTracker(AsyncWorkerFixture& worker, const std::string& trackerPath)
{
    static const std::chrono::milliseconds initializationTimeout(15000);

    auto create = makeCreateTrackerCommand(worker.get(), trackerPath);
    if (!waitFor(create, initializationTimeout))
    {
        std::cout << "Failed: Could not initialize Tracker";
        return false;
    }
    if (!create.getError().empty())
    {
        std::cout << "Failed: Problem while starting the tracker: " << create.getError().c_str()
                  << "\n";
        return false;
    }
    return true;
}

bool runTracker(AsyncWorkerFixture& worker)
{
    static const std::chrono::milliseconds initializationTimeout(15000);
    auto run = makeRunTrackerCommand(worker.get());
    if (!waitFor(run, initializationTimeout))
    {
        std::cout << "Failed: Could not start Tracker: " << run.getError().c_str() << "\n";
        return false;
    }
    if (!run.getError().empty())
    {
        std::cout << "Failed: Problem while running the tracker: " << run.getError().c_str()
                  << "\n";
        return false;
    }

    if (!vlWorker_IsRunning(worker.get()))
    {
        std::cout << "Failed: Tracker is not running\n";
        return false;
    }

    return true;
}

bool startTracker(AsyncWorkerFixture& worker, const std::string& trackerPath)
{
    return createTracker(worker, trackerPath) && runTracker(worker);
}

std::string executeSync(SyncWorkerFixture& worker, const std::string& cmd)
{
    using StringPair = std::pair<std::string, std::string>;
    StringPair result;
    if (!vlWorker_ProcessJsonCommandSync(
            worker.get(),
            cmd.c_str(),
            [](const char* error, const char* data, void* clientData)
            {
                auto& result = *reinterpret_cast<StringPair*>(clientData);
                result.first = error ? error : "";
                result.second = data ? data : "";
            },
            &result))
    {
        throw std::runtime_error(
            "Command " + cmd + " could not be processed. Invalid JSON or unsupported command.");
    }
    if (!result.first.empty())
    {
        const auto resultJson = nlohmann::json::parse(result.first);
        throw std::runtime_error(resultJson["message"].get<std::string>());
    }
    return result.second;
}

AsyncWorkerFixture::AsyncWorkerFixture(const char* licenseFile)
{
    worker.reset(vlNew_Worker());
    if (!worker)
    {
        throw std::runtime_error("vlNew_Worker returned nullptr.");
    }

    if (!vlWorker_Start(worker.get()))
    {
        throw std::runtime_error("vlWorker_Start returned false.");
    }

    if (!licenseFile)
    {
        return;
    }

    if (!vlWorker_SetLicenseFilePath(worker.get(), licenseFile))
    {
        throw std::runtime_error("Failed: Could not set License");
    }
}

SyncWorkerFixture::SyncWorkerFixture(const char* licenseFile)
{
    worker.reset(vlNew_SyncWorker());
    if (!worker)
    {
        throw std::runtime_error("vlNew_SyncWorker returned nullptr.");
    }

    if (!vlWorker_Start(worker.get()))
    {
        throw std::runtime_error("vlWorker_Start returned false.");
    }

    if (!licenseFile)
    {
        return;
    }

    if (!vlWorker_SetLicenseFilePath(worker.get(), licenseFile))
    {
        throw std::runtime_error("Failed: Could not set License");
    }
}

std::string resolveFilePath(const std::string& filePath)
{
    std::array<char, 512> buffer;
    if (!vlSDKUtil_retrievePhysicalPath(
            filePath.c_str(), buffer.data(), static_cast<unsigned int>(buffer.size())))
    {
        throw std::runtime_error("Could not resolve file name: " + filePath);
    }
    return std::string(buffer.data());
}

std::string getAttributeSync(SyncWorkerFixture& worker, const std::string& name)
{
    nlohmann::json cmd;
    cmd["name"] = "getAttribute";
    cmd["param"]["att"] = name;
    return nlohmann::json::parse(executeSync(worker, cmd.dump()))["value"].get<std::string>();
}

void setAttributeSync(SyncWorkerFixture& worker, const std::string& name, const std::string& value)
{
    nlohmann::json cmd;
    cmd["name"] = "setAttribute";
    cmd["param"]["att"] = name;
    cmd["param"]["val"] = value;
    executeSync(worker, cmd.dump());
}

BinaryCommandResult executeSync(
    SyncWorkerFixture& worker,
    const std::string& command,
    const char data[],
    unsigned int size)
{
    using ErrorAndResult = std::pair<nlohmann::json, BinaryCommandResult>;
    ErrorAndResult result;
    bool commandProcessed = vlWorker_ProcessJsonAndBinaryCommandSync(
        worker.get(),
        command.c_str(),
        data,
        size,
        [](const char* error,
           const char* result,
           const char* dataOut,
           unsigned int size,
           void* clientData)
        {
            auto& errorAndResult = *reinterpret_cast<ErrorAndResult*>(clientData);
            errorAndResult.first = error ? nlohmann::json::parse(error) : nlohmann::json();
            errorAndResult.second = BinaryCommandResult {
                result ? nlohmann::json::parse(result) : nlohmann::json(),
                ByteBuffer((char*)dataOut),
                size};
        },
        &result);
    if (!result.first.is_null())
    {
        throw std::runtime_error(result.first["message"].get<std::string>());
    }
    if (!commandProcessed)
    {
        throw std::runtime_error(
            "Command '" + command +
            "' could not be processed. Invalid JSON or unsupported command.");
    }
    return std::move(result.second);
}

std::string getCreateTrackerCommand(const std::string& trackerPath)
{
    nlohmann::json cmd;
    cmd["name"] = "createTracker";
    cmd["param"]["uri"] = trackerPath;
    return cmd.dump();
}

std::string getCreateTrackerFromStringCommand(
    const std::string& trackerContent,
    const std::string& trackerPath)
{
    nlohmann::json cmd;
    cmd["name"] = "createTrackerFromString";
    cmd["param"]["str"] = trackerContent;
    cmd["param"]["fakeFilename"] = trackerPath;
    return cmd.dump();
}

std::string getRunTrackingCommand()
{
    nlohmann::json cmd;
    cmd["name"] = "runTracking";
    return cmd.dump();
}

bool startTrackerSync(
    SyncWorkerFixture& worker,
    const std::string& trackerPath,
    const std::string& trackerContent)
{
    try
    {
        if (trackerContent.empty())
        {
            executeSync(worker, getCreateTrackerCommand(trackerPath));
        }
        else
        {
            executeSync(worker, getCreateTrackerFromStringCommand(trackerContent, trackerPath));
        }
    }
    catch (std::runtime_error& e)
    {
        std::cout << "Failed: Could not create tracker: " << e.what() << "\n";
        return false;
    }

    try
    {
        executeSync(worker, getRunTrackingCommand());
    }
    catch (std::runtime_error& e)
    {
        std::cout << "Failed: Could not enable tracking: " << e.what() << "\n";
        return false;
    }

    if (!vlWorker_IsRunning(worker.get()))
    {
        std::cout << "Failed: Tracker is not running\n";
        return false;
    }

    return true;
}
