#pragma once

#include "BaseSystem.hpp"
#include "AssetManager.hpp"
#include "AssetTypes.hpp"
#include "ImageComponent.hpp"
#include "VideoComponent.hpp"
#include <iostream>

namespace ECS {

	// System for handling texture assets and updating component textures
	class TextureSystem : public BaseSystem {
	public:
		// Fix: Provide proper constructor that initializes BaseSystem
		explicit TextureSystem(EntityManager& entityMgr) : BaseSystem(entityMgr) {
			// Initialize component signatures using the correct API
			signature.insert(ComponentTypeRegistry::GetIDByType<ImageComponent>());
			signature.insert(ComponentTypeRegistry::GetIDByType<VideoComponent>());
		}

		void Start() override;
		void Update(const float deltaT) override;
		void Destroy() override;

		// Image texture operations
		void LoadImageTexture(EntityID entity, const std::string& filePath, EntityManager& entityManager);
		bool UpdateImageTexture(EntityID entity, EntityManager& entityManager);

		// Video texture operations  
		bool UpdateVideoTexture(EntityID entity, EntityManager& entityManager);

		// Query methods
		bool IsTextureReady(EntityID entity, EntityManager& entityManager) const;
		GLuint GetTexture(EntityID entity, EntityManager& entityManager) const;
		void GetTextureDimensions(EntityID entity, EntityManager& entityManager, int& width, int& height) const;

	private:
		void ProcessImageComponents(EntityManager& entityManager);
		void ProcessVideoComponents(EntityManager& entityManager);
		bool UpdateImageComponentTexture(EntityID entity, ImageComponent& imageComp);
		bool UpdateVideoComponentTexture(EntityID entity, VideoComponent& videoComp);
	};

} // namespace ECS