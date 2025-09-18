#pragma once

#include "AssetTypes.hpp"
#include <iostream>
#include <filesystem>

// Video asset - handles video file metadata and frames
class VideoAsset : public Asset {
public:
	VideoAsset(ResourceID id, const std::string& path);
	~VideoAsset() override;

	double GetDuration() const;
	double GetFrameRate() const;
	int GetFrameCount() const;
	int GetWidth() const;
	int GetHeight() const;

protected:
	bool LoadImpl() override;
	void UnloadImpl() override;

private:
	double duration;
	double frameRate;
	int frameCount;
	int width, height;
};