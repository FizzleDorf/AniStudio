#pragma once

#include "BaseSystem.hpp"
#include "EntityManager.hpp"
#include "AssetManager.hpp"
#include "AssetHandleComponents.hpp"
#include <iostream>
#include <vector>
#include <chrono>
#include <filesystem>

namespace ECS {

	class VideoSystem : public BaseSystem {
	public:
		VideoSystem(EntityManager& entityMgr);

		void Start() override;
		void Update(const float deltaT) override;
		void Destroy() override;

		// High-level interface for loading videos
		EntityID LoadVideoEntity(const std::string& filePath);

		// Interface for setting videos (used by SDCPPSystem)
		void SetVideo(EntityID entity, const std::string& filePath);

		// Get all video entities
		std::vector<EntityID> GetAllVideoEntities() const;

		// Playback control
		void PlayVideo(EntityID entity);
		void PauseVideo(EntityID entity);
		void StopVideo(EntityID entity);
		void SeekVideo(EntityID entity, double timeSeconds);
		void SetVideoLoop(EntityID entity, bool loop);
		void SetPlaybackSpeed(EntityID entity, float speed);

		// Query methods
		bool IsVideoLoaded(EntityID entity) const;
		bool IsVideoPlaying(EntityID entity) const;
		double GetVideoCurrentTime(EntityID entity) const;
		double GetVideoDuration(EntityID entity) const;
		void GetVideoDimensions(EntityID entity, int& width, int& height) const;

	private:
		void ProcessVideoHandle(EntityID entity, VideoHandleComponent& videoHandle);
		void UpdateVideoPlayback(VideoHandleComponent& videoHandle, float deltaTime);
		void OnVideoLoaded(EntityID entity, const VideoHandleComponent& videoHandle);

		// For tracking time
		std::chrono::steady_clock::time_point lastUpdateTime;
	};

} // namespace ECS