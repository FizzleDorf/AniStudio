#include "TextureSystem.hpp"
#include "EntityManager.hpp"
#include "AssetManager.hpp"
#include "ImageAsset.hpp"
#include "TextureAsset.hpp"
#include <filesystem>

namespace ECS {

	void TextureSystem::Start() {
		std::cout << "[TextureSystem] Started with AssetManager integration" << std::endl;
	}

	void TextureSystem::Update(const float deltaT) {
		ProcessImageComponents(mgr);
		ProcessVideoComponents(mgr);
	}

	void TextureSystem::Destroy() {
		std::cout << "[TextureSystem] Destroyed" << std::endl;
	}

	void TextureSystem::LoadImageTexture(EntityID entity, const std::string& filePath, EntityManager& entityManager) {
		if (!entityManager.HasComponent<ImageComponent>(entity)) {
			std::cerr << "[TextureSystem] No ImageComponent found for entity " << entity << std::endl;
			return;
		}

		ImageComponent& imageComp = entityManager.GetComponent<ImageComponent>(entity);

		// Use AssetManager to load image asynchronously
		auto future = AssetManager::Instance().LoadImageAsync(filePath,
			[this, entity, &entityManager](ResourceID assetId, bool success) {
			if (success) {
				if (entityManager.HasComponent<ImageComponent>(entity)) {
					ImageComponent& comp = entityManager.GetComponent<ImageComponent>(entity);
					comp.imageAssetId = assetId;

					// Create texture from the loaded image
					comp.textureAssetId = AssetManager::Instance().CreateTextureFromImage(assetId);

					// Update cached properties
					UpdateImageComponentTexture(entity, comp);

					std::cout << "[TextureSystem] Successfully loaded image texture for entity " << entity << std::endl;
				}
			}
			else {
				std::cerr << "[TextureSystem] Failed to load image for entity " << entity << std::endl;
			}
		});

		// Update component with loading state
		imageComp.filePath = filePath;
		std::filesystem::path path(filePath);
		imageComp.fileName = path.filename().string();
	}

	bool TextureSystem::UpdateImageTexture(EntityID entity, EntityManager& entityManager) {
		if (!entityManager.HasComponent<ImageComponent>(entity)) return false;

		ImageComponent& imageComp = entityManager.GetComponent<ImageComponent>(entity);
		return UpdateImageComponentTexture(entity, imageComp);
	}

	bool TextureSystem::UpdateVideoTexture(EntityID entity, EntityManager& entityManager) {
		if (!entityManager.HasComponent<VideoComponent>(entity)) return false;

		VideoComponent& videoComp = entityManager.GetComponent<VideoComponent>(entity);
		return UpdateVideoComponentTexture(entity, videoComp);
	}

	bool TextureSystem::IsTextureReady(EntityID entity, EntityManager& entityManager) const {
		if (entityManager.HasComponent<ImageComponent>(entity)) {
			ImageComponent& imageComp = entityManager.GetComponent<ImageComponent>(entity);
			return imageComp.IsTextureReady();
		}
		if (entityManager.HasComponent<VideoComponent>(entity)) {
			VideoComponent& videoComp = entityManager.GetComponent<VideoComponent>(entity);
			return videoComp.IsTextureReady();
		}
		return false;
	}

	GLuint TextureSystem::GetTexture(EntityID entity, EntityManager& entityManager) const {
		if (entityManager.HasComponent<ImageComponent>(entity)) {
			ImageComponent& imageComp = entityManager.GetComponent<ImageComponent>(entity);
			return imageComp.textureID;
		}
		if (entityManager.HasComponent<VideoComponent>(entity)) {
			VideoComponent& videoComp = entityManager.GetComponent<VideoComponent>(entity);
			return videoComp.currentTexture;
		}
		return 0;
	}

	void TextureSystem::GetTextureDimensions(EntityID entity, EntityManager& entityManager, int& width, int& height) const {
		width = height = 0;

		if (entityManager.HasComponent<ImageComponent>(entity)) {
			ImageComponent& imageComp = entityManager.GetComponent<ImageComponent>(entity);
			width = imageComp.width;
			height = imageComp.height;
		}
		else if (entityManager.HasComponent<VideoComponent>(entity)) {
			VideoComponent& videoComp = entityManager.GetComponent<VideoComponent>(entity);
			width = videoComp.width;
			height = videoComp.height;
		}
	}

	void TextureSystem::ProcessImageComponents(EntityManager& entityManager) {
		for (EntityID entity : entities) {
			if (entityManager.HasComponent<ImageComponent>(entity)) {
				ImageComponent& imageComp = entityManager.GetComponent<ImageComponent>(entity);
				UpdateImageComponentTexture(entity, imageComp);
			}
		}
	}

	void TextureSystem::ProcessVideoComponents(EntityManager& entityManager) {
		for (EntityID entity : entities) {
			if (entityManager.HasComponent<VideoComponent>(entity)) {
				VideoComponent& videoComp = entityManager.GetComponent<VideoComponent>(entity);
				UpdateVideoComponentTexture(entity, videoComp);
			}
		}
	}

	bool TextureSystem::UpdateImageComponentTexture(EntityID entity, ImageComponent& imageComp) {
		if (imageComp.textureAssetId == INVALID_RESOURCE_ID) {
			return false;
		}

		auto textureAsset = AssetManager::Instance().GetAsset<TextureAsset>(imageComp.textureAssetId);
		if (!textureAsset || !textureAsset->IsLoaded()) {
			return false;
		}

		// Update cached texture properties
		if (textureAsset->GetRenderHandle().IsValid()) {
			GLuint newTextureID = textureAsset->GetRenderHandle().Get<GLuint>();
			if (newTextureID != imageComp.textureID) {
				imageComp.textureID = newTextureID;
				imageComp.width = textureAsset->GetWidth();
				imageComp.height = textureAsset->GetHeight();
				imageComp.channels = textureAsset->GetChannels();
				return true;
			}
		}
		return false;
	}

	bool TextureSystem::UpdateVideoComponentTexture(EntityID entity, VideoComponent& videoComp) {
		if (videoComp.textureAssetId == INVALID_RESOURCE_ID) {
			return false;
		}

		auto textureAsset = AssetManager::Instance().GetAsset<TextureAsset>(videoComp.textureAssetId);
		if (!textureAsset || !textureAsset->IsLoaded()) {
			return false;
		}

		// Update cached texture properties
		if (textureAsset->GetRenderHandle().IsValid()) {
			GLuint newTextureID = textureAsset->GetRenderHandle().Get<GLuint>();
			if (newTextureID != videoComp.currentTexture) {
				videoComp.currentTexture = newTextureID;
				return true;
			}
		}
		return false;
	}

} // namespace ECS