#include "VideoSystem.hpp"
#include "components.h"
#include <algorithm>
#include <filesystem>

namespace ECS {

	VideoSystem::VideoSystem(EntityManager& entityMgr) : BaseSystem(entityMgr) {
		AddComponentSignature<VideoHandleComponent>();
		sysName = "VideoSystem";
		lastUpdateTime = std::chrono::steady_clock::now();
	}

	void VideoSystem::Start() {
		std::cout << "[VideoSystem] Started" << std::endl;
	}

	void VideoSystem::Update(const float deltaT) {
		auto currentTime = std::chrono::steady_clock::now();
		float deltaTime = std::chrono::duration<float>(currentTime - lastUpdateTime).count();
		lastUpdateTime = currentTime;

		for (EntityID entity : entities) {
			VideoHandleComponent& videoHandle = mgr.GetComponent<VideoHandleComponent>(entity);

			ProcessVideoHandle(entity, videoHandle);

			// Update playback if playing
			if (videoHandle.isPlaying && videoHandle.isLoaded) {
				UpdateVideoPlayback(videoHandle, deltaTime);
			}
		}
	}

	void VideoSystem::Destroy() {
		std::cout << "[VideoSystem] Destroyed" << std::endl;
	}

	EntityID VideoSystem::LoadVideoEntity(const std::string& filePath) {
		EntityID entity = mgr.AddNewEntity();
		VideoHandleComponent& videoHandle = mgr.AddComponent<VideoHandleComponent>(entity);

		std::cout << "[VideoSystem] Loading video: " << filePath << " for entity " << entity << std::endl;

		// Start async loading
		auto future = AssetManager::Instance().LoadVideoAsync(filePath,
			[this, entity](ResourceID assetId, bool success) {
			if (success) {
				std::cout << "[VideoSystem] Video loaded successfully (Entity: "
					<< entity << ", AssetID: " << assetId << ")" << std::endl;
			}
			else {
				std::cerr << "[VideoSystem] Failed to load video for entity " << entity << std::endl;
			}
		});

		return entity;
	}

	void VideoSystem::SetVideo(EntityID entity, const std::string& filePath) {
		// Ensure entity has the right components
		if (!mgr.HasComponent<VideoComponent>(entity)) {
			mgr.AddComponent<VideoComponent>(entity);
		}

		if (!mgr.HasComponent<VideoHandleComponent>(entity)) {
			mgr.AddComponent<VideoHandleComponent>(entity);
		}

		VideoComponent& videoComp = mgr.GetComponent<VideoComponent>(entity);
		VideoHandleComponent& videoHandle = mgr.GetComponent<VideoHandleComponent>(entity);

		// Set basic properties on your existing VideoComponent
		std::filesystem::path path(filePath);
		videoComp.fileName = path.filename().string();
		videoComp.filePath = filePath;

		// Load the video
		std::cout << "[VideoSystem] Setting video: " << filePath << " for entity " << entity << std::endl;

		// Start async loading
		auto future = AssetManager::Instance().LoadVideoAsync(filePath,
			[this, entity](ResourceID assetId, bool success) {
			if (success) {
				std::cout << "[VideoSystem] Video set successfully (Entity: "
					<< entity << ", AssetID: " << assetId << ")" << std::endl;

				// Update the handle component with the asset ID
				if (mgr.HasComponent<VideoHandleComponent>(entity)) {
					VideoHandleComponent& handle = mgr.GetComponent<VideoHandleComponent>(entity);
					handle.videoAssetId = assetId;
				}
			}
			else {
				std::cerr << "[VideoSystem] Failed to set video for entity " << entity << std::endl;
			}
		});
	}

	std::vector<EntityID> VideoSystem::GetAllVideoEntities() const {
		std::vector<EntityID> videoEntities;

		// Get all entities with VideoComponent
		const auto& entitiesSignatures = mgr.GetEntitiesSignatures();
		for (const auto& pair : entitiesSignatures) {
			EntityID entityId = pair.first;
			if (mgr.HasComponent<VideoComponent>(entityId)) {
				videoEntities.push_back(entityId);
			}
		}

		return videoEntities;
	}

	void VideoSystem::PlayVideo(EntityID entity) {
		if (mgr.HasComponent<VideoHandleComponent>(entity)) {
			VideoHandleComponent& videoHandle = mgr.GetComponent<VideoHandleComponent>(entity);
			if (videoHandle.isLoaded) {
				videoHandle.isPlaying = true;
				std::cout << "[VideoSystem] Started playback for entity " << entity << std::endl;
			}
		}
	}

	void VideoSystem::PauseVideo(EntityID entity) {
		if (mgr.HasComponent<VideoHandleComponent>(entity)) {
			VideoHandleComponent& videoHandle = mgr.GetComponent<VideoHandleComponent>(entity);
			videoHandle.isPlaying = false;
			std::cout << "[VideoSystem] Paused playback for entity " << entity << std::endl;
		}
	}

	void VideoSystem::StopVideo(EntityID entity) {
		if (mgr.HasComponent<VideoHandleComponent>(entity)) {
			VideoHandleComponent& videoHandle = mgr.GetComponent<VideoHandleComponent>(entity);
			videoHandle.isPlaying = false;
			videoHandle.currentTime = 0.0;
			std::cout << "[VideoSystem] Stopped playback for entity " << entity << std::endl;
		}
	}

	void VideoSystem::SeekVideo(EntityID entity, double timeSeconds) {
		if (mgr.HasComponent<VideoHandleComponent>(entity)) {
			VideoHandleComponent& videoHandle = mgr.GetComponent<VideoHandleComponent>(entity);
			if (videoHandle.isLoaded) {
				videoHandle.currentTime = std::max(0.0, std::min(timeSeconds, videoHandle.duration));
				std::cout << "[VideoSystem] Seeked to " << videoHandle.currentTime
					<< "s for entity " << entity << std::endl;
			}
		}
	}

	void VideoSystem::SetVideoLoop(EntityID entity, bool loop) {
		if (mgr.HasComponent<VideoHandleComponent>(entity)) {
			VideoHandleComponent& videoHandle = mgr.GetComponent<VideoHandleComponent>(entity);
			videoHandle.loop = loop;
			std::cout << "[VideoSystem] Set loop " << (loop ? "ON" : "OFF")
				<< " for entity " << entity << std::endl;
		}
	}

	void VideoSystem::SetPlaybackSpeed(EntityID entity, float speed) {
		if (mgr.HasComponent<VideoHandleComponent>(entity)) {
			VideoHandleComponent& videoHandle = mgr.GetComponent<VideoHandleComponent>(entity);
			videoHandle.playbackSpeed = std::max(0.1f, std::min(speed, 10.0f)); // Clamp speed
			std::cout << "[VideoSystem] Set playback speed to " << videoHandle.playbackSpeed
				<< " for entity " << entity << std::endl;
		}
	}

	bool VideoSystem::IsVideoLoaded(EntityID entity) const {
		if (mgr.HasComponent<VideoHandleComponent>(entity)) {
			const VideoHandleComponent& videoHandle = mgr.GetComponent<VideoHandleComponent>(entity);
			return videoHandle.isLoaded;
		}
		return false;
	}

	bool VideoSystem::IsVideoPlaying(EntityID entity) const {
		if (mgr.HasComponent<VideoHandleComponent>(entity)) {
			const VideoHandleComponent& videoHandle = mgr.GetComponent<VideoHandleComponent>(entity);
			return videoHandle.isPlaying;
		}
		return false;
	}

	double VideoSystem::GetVideoCurrentTime(EntityID entity) const {
		if (mgr.HasComponent<VideoHandleComponent>(entity)) {
			const VideoHandleComponent& videoHandle = mgr.GetComponent<VideoHandleComponent>(entity);
			return videoHandle.currentTime;
		}
		return 0.0;
	}

	double VideoSystem::GetVideoDuration(EntityID entity) const {
		if (mgr.HasComponent<VideoHandleComponent>(entity)) {
			const VideoHandleComponent& videoHandle = mgr.GetComponent<VideoHandleComponent>(entity);
			return videoHandle.duration;
		}
		return 0.0;
	}

	void VideoSystem::GetVideoDimensions(EntityID entity, int& width, int& height) const {
		if (mgr.HasComponent<VideoHandleComponent>(entity)) {
			const VideoHandleComponent& videoHandle = mgr.GetComponent<VideoHandleComponent>(entity);
			width = videoHandle.width;
			height = videoHandle.height;
		}
		else {
			width = height = 0;
		}
	}

	void VideoSystem::ProcessVideoHandle(EntityID entity, VideoHandleComponent& videoHandle) {
		// Check if video asset just finished loading
		if (videoHandle.videoAssetId != INVALID_RESOURCE_ID && !videoHandle.isLoaded) {
			auto videoAsset = AssetManager::Instance().GetAsset<VideoAsset>(videoHandle.videoAssetId);
			if (videoAsset && videoAsset->IsLoaded()) {
				std::cout << "[VideoSystem] Video asset loaded for entity " << entity << std::endl;

				// Update cached properties
				videoHandle.duration = videoAsset->GetDuration();
				videoHandle.frameRate = videoAsset->GetFrameRate();
				videoHandle.frameCount = videoAsset->GetFrameCount();
				videoHandle.width = videoAsset->GetWidth();
				videoHandle.height = videoAsset->GetHeight();
				videoHandle.isLoaded = true;

				// Update your existing VideoComponent if it exists (map property names)
				if (mgr.HasComponent<VideoComponent>(entity)) {
					VideoComponent& videoComp = mgr.GetComponent<VideoComponent>(entity);
					videoComp.width = videoHandle.width;
					videoComp.height = videoHandle.height;

					// Map to your component's property names
					videoComp.fps = videoHandle.frameRate;  // Your component uses 'fps' not 'frameRate'
					videoComp.frameCount = videoHandle.frameCount;
					// Note: your VideoComponent doesn't have 'duration' field, only frameCount/fps
				}

				OnVideoLoaded(entity, videoHandle);
			}
			else if (videoAsset && videoAsset->HasFailed()) {
				std::cerr << "[VideoSystem] Video asset failed to load for entity " << entity << std::endl;
			}
		}
	}

	void VideoSystem::UpdateVideoPlayback(VideoHandleComponent& videoHandle, float deltaTime) {
		videoHandle.currentTime += deltaTime * videoHandle.playbackSpeed;

		// Handle looping and bounds
		if (videoHandle.currentTime >= videoHandle.duration) {
			if (videoHandle.loop) {
				videoHandle.currentTime = 0.0;
			}
			else {
				videoHandle.isPlaying = false;
				videoHandle.currentTime = videoHandle.duration;
			}
		}
		else if (videoHandle.currentTime < 0.0) {
			videoHandle.currentTime = 0.0;
		}
	}

	void VideoSystem::OnVideoLoaded(EntityID entity, const VideoHandleComponent& videoHandle) {
		std::cout << "[VideoSystem] Video fully loaded for entity " << entity
			<< " (" << videoHandle.width << "x" << videoHandle.height
			<< ", " << videoHandle.duration << "s @ " << videoHandle.frameRate << " fps)" << std::endl;
	}

} // namespace ECS