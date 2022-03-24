#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/core/utils/logger.hpp>

#include <vlSDK.h>

#include <nlohmann/json.hpp>

#include <HelperFunctions.h>

#include <vector>
#include <iostream>

namespace
{

	constexpr auto licenseFilename = "../Resources/license.xml";
	constexpr auto trackerConfigFilename = "../Resources/bosch_injected.vl";

	const std::string imgURI =
		"http://146.140.221.1:8089/PublicInterfaceTests/testAutoInit_data/record_bosch_winkel";
	constexpr int frameCount = 10;
	constexpr int cameraCount = 12;


	//    Simple struct that holds the data members of an Extrinsic:
	// 
	//    Translation (Vector)             : t
	//    Rotation (Quaternion)            : q
	//    Tracking quality is sufficient   : valid
	// 
	//    This is the euclidean transform from model/object coordinate system to camera coordinate system.
	//    There is typically one camera (in our example camera_0) which is at the origin of the camera coordinate system and is not rotated.
	//    All other cameras are positioned relative to this camera.

	struct Extrinsic
	{
		std::array<float, 3> t;
		std::array<float, 4> q;
		bool valid;
	};

	std::ostream& operator<<(std::ostream& os, const Extrinsic& ext)
	{
		os << "{" + toString(ext.t) + ",\n" + toString(ext.q) + "}";
		return os;
	}


	// Returns path to the specified image
	std::string getImagePath(const size_t frameIdx, const size_t camIdx)
	{
		return imgURI + "/camera_" + std::to_string(camIdx) + "/image_00000" + std::to_string(frameIdx) +
			".jpg";
	}

	// Loads a single image from hard drive
	inline cv::Mat loadImage(const std::string& filename)
	{
		unsigned long resultLength;
		auto resultPtr = ByteBuffer(vlSDKUtil_get(filename.c_str(), &resultLength));
		if (resultPtr)
		{
			return cv::imdecode(cv::Mat(resultLength, 1, CV_8UC1, resultPtr.get()), 1);
		}
		throw std::runtime_error("Unable to load image " + filename);
	}

	// Loads a frame (i.e. a set of images taken by different cameras at the same moment)
	std::vector<cv::Mat> loadFrame(const size_t frameIdx)
	{
		std::vector<cv::Mat> images(cameraCount);
		for (int camIdx = 0; camIdx < cameraCount; camIdx++)
		{
			images[camIdx] = loadImage(getImagePath(frameIdx, camIdx));
			cv::cvtColor(images[camIdx], images[camIdx], cv::ColorConversionCodes::COLOR_RGBA2GRAY);
		}
		return images;
	}

	// Injects one frame into the tracker (overriding the previous frame)
	void injectFrame(SyncWorkerFixture& worker, const std::vector<cv::Mat>& frame)
	{
		assert(frame.size() == cameraCount);
		for (int camIdx = 0; camIdx < cameraCount; camIdx++)
		{
			Image visImage(vlNew_ImageWrapper(vlImageFormat::VL_IMAGE_FORMAT_GREY));
			const auto& cvImage = frame[camIdx];
			vlImageWrapper_CopyFromBuffer(visImage.get(), cvImage.data, cvImage.cols, cvImage.rows);

			const auto key = "injectImage_" + std::to_string(camIdx);
			vlWorker_SetNodeImageSync(worker.get(), visImage.get(), "device0", key.c_str());
		}
	}

	Extrinsic getExtrinsic(SyncWorkerFixture& worker)
	{
		ExtrinsicData extrinsicVL(
			vlWorker_GetNodeExtrinsicDataSync(worker.get(), "tracker0", "extrinsic"));
		Extrinsic extrinsic;
		vlExtrinsicDataWrapper_GetT(extrinsicVL.get(), extrinsic.t.data(), 3);
		vlExtrinsicDataWrapper_GetR(extrinsicVL.get(), extrinsic.q.data(), 4);
		vlExtrinsicDataWrapper_GetValid(extrinsicVL.get());
		return extrinsic;
	}

	// Without the reset, the tracker attempts to use the detection results of the previous frame to find the object.
	void resetTracker(SyncWorkerFixture& worker, const std::string& trackerName)
	{
		nlohmann::json cmd;
		cmd["nodeName"] = trackerName;
		cmd["content"]["name"] = "resetHard";
		executeSync(worker, cmd.dump());
	}

} // namespace

int main() {
	// Supress OpenCV info messages
	cv::utils::logging::setLogLevel(cv::utils::logging::LogLevel::LOG_LEVEL_SILENT);

	SyncWorkerFixture worker(licenseFilename);
	startTrackerSync(worker, getLocalFilePath(trackerConfigFilename));
	std::cout << "Tracker started... \n";
	for (size_t frameIdx = 0; frameIdx < frameCount; frameIdx++)
	{
		std::cout << "Performing AutoInit for frame_" << frameIdx << "\n";
		const auto frame = loadFrame(frameIdx);
		injectFrame(worker, frame);
		vlWorker_RunOnceSync(worker.get());
		resetTracker(worker, "tracker0");
		std::cout << "Results for frame_" << frameIdx << ":\n     " << getExtrinsic(worker) << "\n";
	}

	return 0;
}

