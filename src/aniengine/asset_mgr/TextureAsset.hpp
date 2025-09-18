#pragma once

#include "AssetTypes.hpp"
#include "ImageUtils.hpp"
#include "OpenGLWrapper.hpp"

// Texture asset - handles rendering backend textures (main thread only)
class TextureAsset : public Asset {
public:
	TextureAsset(ResourceID id, const std::string& path);
	TextureAsset(ResourceID id, ResourceID sourceImageId);
	~TextureAsset() override;

	RenderHandle GetRenderHandle() const { return renderHandle; }
	int GetWidth() const { return width; }
	int GetHeight() const { return height; }
	int GetChannels() const { return channels; }

	// OpenGL-specific convenience method
	GLuint GetOpenGLTexture() const {
		return renderHandle.Get<GLuint>();
	}

protected:
	bool LoadImpl() override;
	void UnloadImpl() override;

private:
	RenderHandle renderHandle;
	int width, height, channels;
	ResourceID sourceImageAssetId = INVALID_RESOURCE_ID;

	bool LoadFromFile();

	friend class AssetManager;
};