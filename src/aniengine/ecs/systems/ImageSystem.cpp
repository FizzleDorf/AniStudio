#include "ImageSystem.hpp"
#include "components.h"
#include <filesystem>

namespace ECS {

	ImageSystem::ImageSystem(EntityManager& entityMgr) : BaseSystem(entityMgr) {
		AddComponentSignature<ImageHandleComponent>();
		sysName = "ImageSystem";
	}

	void ImageSystem::Start() {
		std::cout << "[ImageSystem] Started" << std::endl;
	}

	void ImageSystem::Update(const float deltaT) {
		for (EntityID entity : entities) {
			ImageHandleComponent& imageHandle = mgr.GetComponent<ImageHandleComponent>(entity);
			ProcessImageHandle(entity, imageHandle);
		}
	}

	void ImageSystem::Destroy() {
		std::cout << "[ImageSystem] Destroyed" << std::endl;
	}

	EntityID ImageSystem::LoadImageEntity(const std::string& filePath) {
		EntityID entity = mgr.AddNewEntity();
		ImageHandleComponent& imageHandle = mgr.AddComponent<ImageHandleComponent>(entity);

		std::cout << "[ImageSystem] Loading image: " << filePath << " for entity " << entity << std::endl;

		// Start async loading
		auto future = AssetManager::Instance().LoadImageAsync(filePath,
			[this, entity](ResourceID assetId, bool success) {
			if (success) {
				std::cout << "[ImageSystem] Image loaded successfully (Entity: "
					<< entity << ", AssetID: " << assetId << ")" << std::endl;
			}
			else {
				std::cerr << "[ImageSystem] Failed to load image for entity " << entity << std::endl;
			}
		});

		return entity;
	}

	EntityID ImageSystem::LoadImageEntitySync(const std::string& filePath) {
		auto future = AssetManager::Instance().LoadImageAsync(filePath);
		ResourceID assetId = future.get(); // Block until loaded

		if (assetId == INVALID_RESOURCE_ID) {
			std::cerr << "[ImageSystem] Failed to load image synchronously: " << filePath << std::endl;
			return 0; // Invalid entity
		}

		EntityID entity = mgr.AddNewEntity();
		ImageHandleComponent& imageHandle = mgr.AddComponent<ImageHandleComponent>(entity);

		// Set loaded asset
		imageHandle.imageAssetId = assetId;

		// Create texture immediately if requested
		if (imageHandle.autoCreateTexture) {
			imageHandle.textureAssetId = AssetManager::Instance().CreateTextureFromImage(assetId);
			UpdateCachedProperties(imageHandle);
		}

		return entity;
	}

	void ImageSystem::SetImage(EntityID entity, const std::string& filePath) {
		// Ensure entity has the right components
		if (!mgr.HasComponent<ImageComponent>(entity)) {
			mgr.AddComponent<ImageComponent>(entity);
		}

		if (!mgr.HasComponent<ImageHandleComponent>(entity)) {
			mgr.AddComponent<ImageHandleComponent>(entity);
		}

		ImageComponent& imageComp = mgr.GetComponent<ImageComponent>(entity);
		ImageHandleComponent& imageHandle = mgr.GetComponent<ImageHandleComponent>(entity);

		// Set basic properties on your existing ImageComponent
		std::filesystem::path path(filePath);
		imageComp.fileName = path.filename().string();
		imageComp.filePath = filePath;

		// Load the image
		std::cout << "[ImageSystem] Setting image: " << filePath << " for entity " << entity << std::endl;

		// Start async loading
		auto future = AssetManager::Instance().LoadImageAsync(filePath,
			[this, entity](ResourceID assetId, bool success) {
			if (success) {
				std::cout << "[ImageSystem] Image set successfully (Entity: "
					<< entity << ", AssetID: " << assetId << ")" << std::endl;

				// Update the handle component with the asset ID
				if (mgr.HasComponent<ImageHandleComponent>(entity)) {
					ImageHandleComponent& handle = mgr.GetComponent<ImageHandleComponent>(entity);
					handle.imageAssetId = assetId;
				}
			}
			else {
				std::cerr << "[ImageSystem] Failed to set image for entity " << entity << std::endl;
			}
		});
	}

	std::vector<EntityID> ImageSystem::GetLoadedImageEntities() const {
		std::vector<EntityID> loaded;
		for (EntityID entity : entities) {
			if (IsImageLoaded(entity)) {
				loaded.push_back(entity);
			}
		}
		return loaded;
	}

	bool ImageSystem::IsImageLoaded(EntityID entity) const {
		if (mgr.HasComponent<ImageHandleComponent>(entity)) {
			const ImageHandleComponent& imageHandle = mgr.GetComponent<ImageHandleComponent>(entity);
			return imageHandle.isLoaded && imageHandle.HasImageData();
		}
		return false;
	}

	GLuint ImageSystem::GetImageTexture(EntityID entity) const {
		if (mgr.HasComponent<ImageHandleComponent>(entity)) {
			const ImageHandleComponent& imageHandle = mgr.GetComponent<ImageHandleComponent>(entity);
			return imageHandle.GetOpenGLTexture();
		}
		return 0;
	}

	void ImageSystem::GetImageDimensions(EntityID entity, int& width, int& height) const {
		if (mgr.HasComponent<ImageHandleComponent>(entity)) {
			const ImageHandleComponent& imageHandle = mgr.GetComponent<ImageHandleComponent>(entity);
			width = imageHandle.width;
			height = imageHandle.height;
		}
		else {
			width = height = 0;
		}
	}

	void ImageSystem::ProcessImageHandle(EntityID entity, ImageHandleComponent& imageHandle) {
		// Check if we have an image asset that just finished loading
		if (imageHandle.imageAssetId != INVALID_RESOURCE_ID && !imageHandle.isLoaded) {
			auto imageAsset = AssetManager::Instance().GetAsset<ImageAsset>(imageHandle.imageAssetId);
			if (imageAsset && imageAsset->IsLoaded()) {
				std::cout << "[ImageSystem] Image asset loaded for entity " << entity << std::endl;

				// Create texture if requested and not already created
				if (imageHandle.autoCreateTexture && imageHandle.textureAssetId == INVALID_RESOURCE_ID) {
					imageHandle.textureAssetId = AssetManager::Instance().CreateTextureFromImage(imageHandle.imageAssetId);

					if (imageHandle.textureAssetId != INVALID_RESOURCE_ID) {
						std::cout << "[ImageSystem] Texture created for entity " << entity << std::endl;
					}
				}

				UpdateCachedProperties(imageHandle);
				imageHandle.isLoaded = true;

				// Update your existing ImageComponent if it exists
				if (mgr.HasComponent<ImageComponent>(entity)) {
					ImageComponent& imageComp = mgr.GetComponent<ImageComponent>(entity);
					imageComp.width = imageHandle.width;
					imageComp.height = imageHandle.height;
					imageComp.channels = imageHandle.channels;
					imageComp.textureID = imageHandle.GetOpenGLTexture();
				}

				OnImageLoaded(entity, imageHandle);
			}
			else if (imageAsset && imageAsset->HasFailed()) {
				std::cerr << "[ImageSystem] Image asset failed to load for entity " << entity << std::endl;
			}
		}

		// Update texture handle if texture asset is loaded
		if (imageHandle.textureAssetId != INVALID_RESOURCE_ID && !imageHandle.renderHandle.IsValid()) {
			auto textureAsset = AssetManager::Instance().GetAsset<TextureAsset>(imageHandle.textureAssetId);
			if (textureAsset && textureAsset->IsLoaded()) {
				imageHandle.renderHandle = textureAsset->GetRenderHandle();
			}
		}
	}

	void ImageSystem::UpdateCachedProperties(ImageHandleComponent& imageHandle) {
		// Update cached properties from assets
		if (imageHandle.imageAssetId != INVALID_RESOURCE_ID) {
			auto imageAsset = AssetManager::Instance().GetAsset<ImageAsset>(imageHandle.imageAssetId);
			if (imageAsset && imageAsset->IsLoaded()) {
				imageAsset->GetDimensions(imageHandle.width, imageHandle.height, imageHandle.channels);
			}
		}

		if (imageHandle.textureAssetId != INVALID_RESOURCE_ID) {
			auto textureAsset = AssetManager::Instance().GetAsset<TextureAsset>(imageHandle.textureAssetId);
			if (textureAsset && textureAsset->IsLoaded()) {
				imageHandle.renderHandle = textureAsset->GetRenderHandle();
			}
		}
	}

	void ImageSystem::OnImageLoaded(EntityID entity, const ImageHandleComponent& imageHandle) {
		std::cout << "[ImageSystem] Image fully loaded for entity " << entity
			<< " (Size: " << imageHandle.width << "x" << imageHandle.height << ")" << std::endl;
	}

} // namespace ECS