#include "TextureAsset.hpp"
#include "OpenGLUtils.hpp"
#include "ImageUtils.hpp"

TextureAsset::TextureAsset(ResourceID id, ResourceID imageAssetId)
	: Asset(id, "", AssetType::Texture), sourceImageAssetId(imageAssetId),
	width(0), height(0), channels(0) {
}

TextureAsset::~TextureAsset() {
	UnloadImpl();
}

bool TextureAsset::LoadImpl() {
	// Implementation handled by AssetManager::CreateTextureFromImage
	return true;
}

void TextureAsset::UnloadImpl() {
	if (renderHandle.IsValid()) {
		GLuint texId = renderHandle.Get<GLuint>();
		if (texId != 0) {
			glDeleteTextures(1, &texId);
		}
		renderHandle.Clear();
	}
	width = height = channels = 0;
}