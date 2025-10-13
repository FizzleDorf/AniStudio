#pragma once

#include "BaseSystem.hpp"
#include "AssetManager.hpp"
#include "AssetTypes.hpp"
#include "ImageComponent.hpp"
#include "VideoComponent.hpp"
#include <iostream>

namespace ECS {

	class TextureSystem : public BaseSystem {
	public:
		explicit TextureSystem(EntityManager& entityMgr) : BaseSystem(entityMgr) {
			// Add component signatures using templates
			AddComponentSignature<ImageComponent>();
			AddComponentSignature<VideoComponent>();
		}

		void Start() override;
		void Update(const float deltaT) override;
		void Destroy() override;

		void LoadImageTexture(EntityID entity, const std::string& filePath, EntityManager& entityManager);
		bool UpdateImageTexture(EntityID entity, EntityManager& entityManager);
		bool UpdateVideoTexture(EntityID entity, EntityManager& entityManager);

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