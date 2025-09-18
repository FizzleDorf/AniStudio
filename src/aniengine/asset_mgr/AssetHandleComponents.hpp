#pragma once

#include "AssetTypes.hpp"
#include "BaseComponent.hpp"

// Asset handle components for ECS integration
// These work alongside your existing VideoComponent/ImageComponent
// They handle AssetManager integration while your main components handle functionality

struct ImageHandleComponent : public ECS::BaseComponent {
	ImageHandleComponent() {
		compName = "ImageHandle";
	}

	ResourceID imageAssetId = INVALID_RESOURCE_ID;
	ResourceID textureAssetId = INVALID_RESOURCE_ID;
	bool autoCreateTexture = true;
	bool isLoaded = false;

	// Cached properties for quick access
	int width = 0;
	int height = 0;
	int channels = 0;
	RenderHandle renderHandle;

	// Convenience methods
	GLuint GetOpenGLTexture() const {
		return renderHandle.Get<GLuint>();
	}

	bool HasImageData() const {
		return imageAssetId != INVALID_RESOURCE_ID && isLoaded;
	}

	bool HasTexture() const {
		return textureAssetId != INVALID_RESOURCE_ID && renderHandle.IsValid();
	}

	bool IsReady() const {
		return imageAssetId != INVALID_RESOURCE_ID && isLoaded;
	}

	nlohmann::json Serialize() const override {
		nlohmann::json j = ECS::BaseComponent::Serialize();
		j["imageAssetId"] = imageAssetId;
		j["textureAssetId"] = textureAssetId;
		j["autoCreateTexture"] = autoCreateTexture;
		j["isLoaded"] = isLoaded;
		j["width"] = width;
		j["height"] = height;
		j["channels"] = channels;
		return j;
	}

	void Deserialize(const nlohmann::json& j) override {
		ECS::BaseComponent::Deserialize(j);
		if (j.contains("imageAssetId")) imageAssetId = j["imageAssetId"];
		if (j.contains("textureAssetId")) textureAssetId = j["textureAssetId"];
		if (j.contains("autoCreateTexture")) autoCreateTexture = j["autoCreateTexture"];
		if (j.contains("isLoaded")) isLoaded = j["isLoaded"];
		if (j.contains("width")) width = j["width"];
		if (j.contains("height")) height = j["height"];
		if (j.contains("channels")) channels = j["channels"];
	}
};

struct VideoHandleComponent : public ECS::BaseComponent {
	VideoHandleComponent() {
		compName = "VideoHandle";
	}

	ResourceID videoAssetId = INVALID_RESOURCE_ID;
	bool isLoaded = false;

	// Cached properties (synced with AssetManager)
	double duration = 0.0;
	double frameRate = 0.0;
	int frameCount = 0;
	int width = 0;
	int height = 0;

	// Playback state (managed by VideoSystem)
	double currentTime = 0.0;
	bool isPlaying = false;
	bool loop = false;
	bool reverse = false;
	float playbackSpeed = 1.0f;

	// Convenience methods
	int GetCurrentFrame() const {
		if (frameRate <= 0.0 || frameCount <= 0) return 0;
		int frame = static_cast<int>(currentTime * frameRate);
		return std::max(0, std::min(frame, frameCount - 1));
	}

	float GetProgress() const {
		if (duration <= 0.0) return 0.0f;
		return static_cast<float>(std::max(0.0, std::min(currentTime / duration, 1.0)));
	}

	bool IsReady() const {
		return videoAssetId != INVALID_RESOURCE_ID && isLoaded;
	}

	nlohmann::json Serialize() const override {
		nlohmann::json j = ECS::BaseComponent::Serialize();
		j["videoAssetId"] = videoAssetId;
		j["isLoaded"] = isLoaded;
		j["duration"] = duration;
		j["frameRate"] = frameRate;
		j["frameCount"] = frameCount;
		j["width"] = width;
		j["height"] = height;
		j["currentTime"] = currentTime;
		j["isPlaying"] = isPlaying;
		j["loop"] = loop;
		j["reverse"] = reverse;
		j["playbackSpeed"] = playbackSpeed;
		return j;
	}

	void Deserialize(const nlohmann::json& j) override {
		ECS::BaseComponent::Deserialize(j);
		if (j.contains("videoAssetId")) videoAssetId = j["videoAssetId"];
		if (j.contains("isLoaded")) isLoaded = j["isLoaded"];
		if (j.contains("duration")) duration = j["duration"];
		if (j.contains("frameRate")) frameRate = j["frameRate"];
		if (j.contains("frameCount")) frameCount = j["frameCount"];
		if (j.contains("width")) width = j["width"];
		if (j.contains("height")) height = j["height"];
		if (j.contains("currentTime")) currentTime = j["currentTime"];
		if (j.contains("isPlaying")) isPlaying = j["isPlaying"];
		if (j.contains("loop")) loop = j["loop"];
		if (j.contains("reverse")) reverse = j["reverse"];
		if (j.contains("playbackSpeed")) playbackSpeed = j["playbackSpeed"];
	}
};

struct TextureHandleComponent : public ECS::BaseComponent {
	TextureHandleComponent() {
		compName = "TextureHandle";
	}

	ResourceID textureAssetId = INVALID_RESOURCE_ID;
	bool isLoaded = false;

	// Cached properties
	RenderHandle renderHandle;
	int width = 0;
	int height = 0;
	int channels = 0;

	// Texture parameters
	bool generateMipmaps = false;
	int minFilter = GL_LINEAR;
	int magFilter = GL_LINEAR;
	int wrapS = GL_CLAMP_TO_EDGE;
	int wrapT = GL_CLAMP_TO_EDGE;

	GLuint GetOpenGLTexture() const {
		return renderHandle.Get<GLuint>();
	}

	bool IsReady() const {
		return textureAssetId != INVALID_RESOURCE_ID && isLoaded && renderHandle.IsValid();
	}

	float GetAspectRatio() const {
		return height > 0 ? static_cast<float>(width) / static_cast<float>(height) : 1.0f;
	}

	nlohmann::json Serialize() const override {
		nlohmann::json j = ECS::BaseComponent::Serialize();
		j["textureAssetId"] = textureAssetId;
		j["isLoaded"] = isLoaded;
		j["width"] = width;
		j["height"] = height;
		j["channels"] = channels;
		j["generateMipmaps"] = generateMipmaps;
		j["minFilter"] = minFilter;
		j["magFilter"] = magFilter;
		j["wrapS"] = wrapS;
		j["wrapT"] = wrapT;
		return j;
	}

	void Deserialize(const nlohmann::json& j) override {
		ECS::BaseComponent::Deserialize(j);
		if (j.contains("textureAssetId")) textureAssetId = j["textureAssetId"];
		if (j.contains("isLoaded")) isLoaded = j["isLoaded"];
		if (j.contains("width")) width = j["width"];
		if (j.contains("height")) height = j["height"];
		if (j.contains("channels")) channels = j["channels"];
		if (j.contains("generateMipmaps")) generateMipmaps = j["generateMipmaps"];
		if (j.contains("minFilter")) minFilter = j["minFilter"];
		if (j.contains("magFilter")) magFilter = j["magFilter"];
		if (j.contains("wrapS")) wrapS = j["wrapS"];
		if (j.contains("wrapT")) wrapT = j["wrapT"];
	}
};