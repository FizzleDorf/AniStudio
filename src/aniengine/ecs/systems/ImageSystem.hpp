#pragma once

#include "BaseSystem.hpp"
#include "EntityManager.hpp"
#include "AssetManager.hpp"
#include "AssetHandleComponents.hpp"
#include <iostream>
#include <vector>
#include <filesystem>

namespace ECS {

	class ImageSystem : public BaseSystem {
	public:
		ImageSystem(EntityManager& entityMgr);

		void Start() override;
		void Update(const float deltaT) override;
		void Destroy() override;

		// High-level interface for loading images
		EntityID LoadImageEntity(const std::string& filePath);
		EntityID LoadImageEntitySync(const std::string& filePath);

		// Interface for setting images (used by SDCPPSystem)
		void SetImage(EntityID entity, const std::string& filePath);

		// Get all entities with loaded images
		std::vector<EntityID> GetLoadedImageEntities() const;

		// Utility methods
		bool IsImageLoaded(EntityID entity) const;
		GLuint GetImageTexture(EntityID entity) const;
		void GetImageDimensions(EntityID entity, int& width, int& height) const;

	private:
		void ProcessImageHandle(EntityID entity, ImageHandleComponent& imageHandle);
		void UpdateCachedProperties(ImageHandleComponent& imageHandle);
		void OnImageLoaded(EntityID entity, const ImageHandleComponent& imageHandle);
	};

} // namespace ECS