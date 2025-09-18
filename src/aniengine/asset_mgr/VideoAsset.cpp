#include "VideoAsset.hpp"

VideoAsset::VideoAsset(ResourceID id, const std::string& path)
	: Asset(id, path, AssetType::Video), duration(0.0), frameRate(0.0), frameCount(0), width(0), height(0) {
}

VideoAsset::~VideoAsset() {
	UnloadImpl();
}

double VideoAsset::GetDuration() const {
	std::lock_guard<std::mutex> lock(dataMutex);
	return duration;
}

double VideoAsset::GetFrameRate() const {
	std::lock_guard<std::mutex> lock(dataMutex);
	return frameRate;
}

int VideoAsset::GetFrameCount() const {
	std::lock_guard<std::mutex> lock(dataMutex);
	return frameCount;
}

int VideoAsset::GetWidth() const {
	std::lock_guard<std::mutex> lock(dataMutex);
	return width;
}

int VideoAsset::GetHeight() const {
	std::lock_guard<std::mutex> lock(dataMutex);
	return height;
}

bool VideoAsset::LoadImpl() {
	// Load video metadata - integrate with your existing video loading system
	if (!std::filesystem::exists(filePath)) {
		std::cerr << "[VideoAsset] File not found: " << filePath << std::endl;
		return false;
	}

	// TODO: Integrate with your VideoSystem here
	// You would use OpenCV or your existing video loading logic here
	// For now, placeholder implementation that you can replace:

	/*
	// Example integration with OpenCV:
	cv::VideoCapture cap(filePath);
	if (!cap.isOpened()) {
		std::cerr << "[VideoAsset] Failed to open video: " << filePath << std::endl;
		return false;
	}

	double fps = cap.get(cv::CAP_PROP_FPS);
	int frameCount = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_COUNT));
	int width = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
	int height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
	double duration = frameCount / fps;

	cap.release();
	*/

	{
		std::lock_guard<std::mutex> lock(dataMutex);
		// Placeholder values - replace with actual video loading
		duration = 10.0; // seconds
		frameRate = 30.0; // fps
		frameCount = static_cast<int>(duration * frameRate);
		width = 1920;    // pixels
		height = 1080;   // pixels
	}

	std::cout << "[VideoAsset] Loaded video metadata: " << filePath
		<< " (" << width << "x" << height << ", " << duration << "s @ " << frameRate << " fps)" << std::endl;
	return true;
}

void VideoAsset::UnloadImpl() {
	std::lock_guard<std::mutex> lock(dataMutex);
	duration = frameRate = 0.0;
	frameCount = width = height = 0;

	// TODO: Clean up any video-specific resources here
	// For example, if you're caching decoded frames or have video decoder instances
}