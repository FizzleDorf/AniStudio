#pragma once

#include <unordered_map>
#include <memory>
#include <future>
#include <mutex>
#include <atomic>
#include <functional>
#include <vector>
#include <queue>
#include <thread>

#include "AssetTypes.hpp"
#include "ImageAsset.hpp"
#include "TextureAsset.hpp"
#include "VideoAsset.hpp"
#include "ThreadPool.hpp"

// Main asset manager class
class AssetManager {
public:
	static AssetManager& Instance() {
		static AssetManager instance;
		return instance;
	}

	// Initialize the asset manager
	void Initialize();

	// Shutdown and cleanup
	void Shutdown();

	// Load image asynchronously using your IO thread pool
	std::future<ResourceID> LoadImageAsync(const std::string& filePath,
		std::function<void(ResourceID, bool)> callback = nullptr);

	// Load video asynchronously using your IO thread pool
	std::future<ResourceID> LoadVideoAsync(const std::string& filePath,
		std::function<void(ResourceID, bool)> callback = nullptr);

	// Create texture from loaded image (main thread only)
	ResourceID CreateTextureFromImage(ResourceID imageAssetId);

	// Load texture directly from file (main thread only)
	ResourceID LoadTexture(const std::string& filePath);

	// Get asset by ID with type safety
	template<typename T>
	std::shared_ptr<T> GetAsset(ResourceID id) {
		std::lock_guard<std::mutex> lock(assetMutex);
		auto it = assets.find(id);
		if (it != assets.end()) {
			return std::dynamic_pointer_cast<T>(it->second);
		}
		return nullptr;
	}

	// Get asset without type checking
	std::shared_ptr<Asset> GetAsset(ResourceID id);

	// Unload asset
	void UnloadAsset(ResourceID id);

	// Check if asset exists
	bool HasAsset(ResourceID id) const;

	// Update - call this from main thread to handle main-thread tasks
	void Update();

	// Get all assets of a specific type
	std::vector<std::shared_ptr<Asset>> GetAssetsByType(AssetType type);

	// Get loading statistics for debugging
	struct LoadingStats {
		size_t totalAssets = 0;
		size_t loadedAssets = 0;
		size_t loadingAssets = 0;
		size_t failedAssets = 0;
		Utils::ThreadPoolManager::PoolStats threadPoolStats;
	};

	LoadingStats GetStats() const;

	// Memory management
	void GarbageCollect(); // Remove unused assets
	size_t GetMemoryUsage() const; // Estimate memory usage

	// Asset path utilities
	std::string GetAssetPath(ResourceID id) const;
	ResourceID FindAssetByPath(const std::string& path) const;

	// Preloading utilities
	void PreloadAssets(const std::vector<std::string>& filePaths);
	bool IsPreloadComplete() const;

private:
	AssetManager();
	~AssetManager();

	// Delete copy and move operations for singleton
	AssetManager(const AssetManager&) = delete;
	AssetManager& operator=(const AssetManager&) = delete;
	AssetManager(AssetManager&&) = delete;
	AssetManager& operator=(AssetManager&&) = delete;

	// Asset storage
	std::unordered_map<ResourceID, std::shared_ptr<Asset>> assets;
	std::unordered_map<std::string, ResourceID> pathToAsset;  // Path -> ResourceID mapping
	mutable std::mutex assetMutex;

	// Resource ID generation
	std::atomic<ResourceID> nextResourceID;

	// Main thread detection
	std::thread::id mainThreadId;

	// Main thread task queue for any needed main-thread operations
	std::queue<std::function<void()>> mainThreadTasks;
	std::mutex mainThreadTaskMutex;

	// Preloading state
	std::atomic<size_t> preloadingCount{ 0 };

	// Private utility methods
	ResourceID GenerateResourceID();
	bool IsMainThread() const;
	void ProcessMainThreadTasks();
	void QueueMainThreadTask(std::function<void()> task);

	// Asset loading helpers
	template<typename AssetType>
	std::future<ResourceID> LoadAssetAsync(const std::string& filePath,
		std::function<void(ResourceID, bool)> callback);
};