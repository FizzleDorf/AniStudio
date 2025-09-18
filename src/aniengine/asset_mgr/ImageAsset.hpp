#pragma once

#include "AssetTypes.hpp"
#include "ImageUtils.hpp"
#include <iostream>

// Image asset - handles raw image data
class ImageAsset : public Asset {
public:
	ImageAsset(ResourceID id, const std::string& path);
	~ImageAsset() override;

	int GetWidth() const;
	int GetHeight() const;
	int GetChannels() const;

	// Get raw image data (thread-safe)
	unsigned char* GetImageData() const;

	// Get image dimensions safely
	void GetDimensions(int& w, int& h, int& c) const;

protected:
	bool LoadImpl() override;
	void UnloadImpl() override;

private:
	int width, height, channels;
	unsigned char* imageData;
};