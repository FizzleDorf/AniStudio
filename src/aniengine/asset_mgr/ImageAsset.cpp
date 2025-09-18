#include "ImageAsset.hpp"
#include <stb_image.h>
#include <iostream>

ImageAsset::ImageAsset(ResourceID id, const std::string& path)
	: Asset(id, path, AssetType::Image), width(0), height(0), channels(0), imageData(nullptr) {
}

ImageAsset::~ImageAsset() {
	UnloadImpl();
}

bool ImageAsset::LoadImpl() {
	if (imageData) {
		return true; // Already loaded
	}

	std::cout << "[ImageAsset] Loading: " << filePath << std::endl;

	// Use stb_image to load the image
	imageData = stbi_load(filePath.c_str(), &width, &height, &channels, 0);

	if (!imageData) {
		std::cerr << "[ImageAsset] Failed to load image: " << filePath << std::endl;
		std::cerr << "[ImageAsset] STB Error: " << stbi_failure_reason() << std::endl;
		return false;
	}

	std::cout << "[ImageAsset] Successfully loaded: " << filePath
		<< " (" << width << "x" << height << ", " << channels << " channels)" << std::endl;

	return true;
}

void ImageAsset::UnloadImpl() {
	if (imageData) {
		stbi_image_free(imageData);
		imageData = nullptr;
	}
	width = height = channels = 0;
}

int ImageAsset::GetWidth() const {
	std::lock_guard<std::mutex> lock(dataMutex);
	return width;
}

int ImageAsset::GetHeight() const {
	std::lock_guard<std::mutex> lock(dataMutex);
	return height;
}

int ImageAsset::GetChannels() const {
	std::lock_guard<std::mutex> lock(dataMutex);
	return channels;
}

unsigned char* ImageAsset::GetImageData() const {
	std::lock_guard<std::mutex> lock(dataMutex);
	return imageData;
}

void ImageAsset::GetDimensions(int& w, int& h, int& c) const {
	std::lock_guard<std::mutex> lock(dataMutex);
	w = width;
	h = height;
	c = channels;
}