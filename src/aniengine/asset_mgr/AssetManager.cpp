#include "AssetManager.hpp"
#include "ImageAsset.hpp"
#include "VideoAsset.hpp" 
#include "TextureAsset.hpp"
#include "OpenGLWrapper.hpp"
#include "OpenGLUtils.hpp"
#include <iostream>
#include <algorithm>
#include <filesystem>

AssetManager::AssetManager() : nextResourceID(1) {
}

AssetManager::~AssetManager() {
	Shutdown();
}

void AssetManager::Initialize() {
	std::cout << "[AssetManager] Initializing..." << std::endl;
	mainThreadId = std::this_thread::get_id();
	std::cout << "[AssetManager] Using existing ThreadPoolManager for async operations" << std::endl;
	std::cout << "[AssetManager] Initialization complete" << std::endl;
}

void AssetManager::Shutdown() {
	std::cout << "[AssetManager] Shutting down..." << std::endl;

	std::lock_guard<std::mutex> lock(assetMutex);
	for (auto& pair : assets) {
		if (pair.second) {
			pair.second->UnloadImpl();
		}
	}
	assets.clear();
	pathToAsset.clear();

	std::cout << "[AssetManager] Shutdown complete" << std::endl;
}

std::future<ResourceID> AssetManager::LoadImageAsync(const std::string& filePath,
	std::function<void(ResourceID, bool)> callback) {

	// Check if already loaded
	ResourceID existingId = FindAssetByPath(filePath);
	if (existingId != INVALID_RESOURCE_ID) {
		auto promise = std::promise<ResourceID>();
		promise.set_value(existingId);

		if (callback) {
			callback(existingId, true);
		}

		return promise.get_future();
	}

	// Create new IMAGE asset (not video!)
	ResourceID assetId = GenerateResourceID();
	auto imageAsset = std::make_shared<ImageAsset>(assetId, filePath);

	{
		std::lock_guard<std::mutex> lock(assetMutex);
		assets[assetId] = imageAsset;
		pathToAsset[filePath] = assetId;
		imageAsset->loadState = LoadState::Loading;
	}

	std::cout << "[AssetManager] Queuing image load: " << filePath << " (ID: " << assetId << ")" << std::endl;

	return Utils::ThreadPoolManager::getInstance().getIOPool().submit([this, assetId, filePath, callback]() -> ResourceID {
		std::shared_ptr<Asset> asset;

		{
			std::lock_guard<std::mutex> lock(assetMutex);
			auto it = assets.find(assetId);
			if (it == assets.end()) {
				if (callback) callback(INVALID_RESOURCE_ID, false);
				return INVALID_RESOURCE_ID;
			}
			asset = it->second;
		}

		bool success = asset->LoadImpl();
		asset->loadState = success ? LoadState::Loaded : LoadState::Failed;

		if (callback) {
			callback(success ? assetId : INVALID_RESOURCE_ID, success);
		}

		if (success) {
			std::cout << "[AssetManager] Successfully loaded image: " << filePath << std::endl;
		}
		else {
			std::cerr << "[AssetManager] Failed to load image: " << filePath << std::endl;
		}

		return success ? assetId : INVALID_RESOURCE_ID;
	});
}

std::future<ResourceID> AssetManager::LoadVideoAsync(const std::string& filePath,
	std::function<void(ResourceID, bool)> callback) {

	// Check if already loaded
	ResourceID existingId = FindAssetByPath(filePath);
	if (existingId != INVALID_RESOURCE_ID) {
		auto promise = std::promise<ResourceID>();
		promise.set_value(existingId);

		if (callback) {
			callback(existingId, true);
		}

		return promise.get_future();
	}

	// Create new VIDEO asset 
	ResourceID assetId = GenerateResourceID();
	auto videoAsset = std::make_shared<VideoAsset>(assetId, filePath);

	{
		std::lock_guard<std::mutex> lock(assetMutex);
		assets[assetId] = videoAsset;
		pathToAsset[filePath] = assetId;
		videoAsset->loadState = LoadState::Loading;
	}

	std::cout << "[AssetManager] Queuing video load: " << filePath << " (ID: " << assetId << ")" << std::endl;

	return Utils::ThreadPoolManager::getInstance().getIOPool().submit([this, assetId, filePath, callback]() -> ResourceID {
		std::shared_ptr<Asset> asset;

		{
			std::lock_guard<std::mutex> lock(assetMutex);
			auto it = assets.find(assetId);
			if (it == assets.end()) {
				if (callback) callback(INVALID_RESOURCE_ID, false);
				return INVALID_RESOURCE_ID;
			}
			asset = it->second;
		}

		bool success = asset->LoadImpl();
		asset->loadState = success ? LoadState::Loaded : LoadState::Failed;

		if (callback) {
			callback(success ? assetId : INVALID_RESOURCE_ID, success);
		}

		if (success) {
			std::cout << "[AssetManager] Successfully loaded video: " << filePath << std::endl;
		}
		else {
			std::cerr << "[AssetManager] Failed to load video: " << filePath << std::endl;
		}

		return success ? assetId : INVALID_RESOURCE_ID;
	});
}

ResourceID AssetManager::CreateTextureFromImage(ResourceID imageAssetId) {
	if (!IsMainThread()) {
		std::cerr << "[AssetManager] CreateTextureFromImage must be called from main thread!" << std::endl;
		return INVALID_RESOURCE_ID;
	}

	auto imageAsset = GetAsset<ImageAsset>(imageAssetId);
	if (!imageAsset || !imageAsset->IsLoaded()) {
		std::cerr << "[AssetManager] Image asset not loaded for texture creation" << std::endl;
		return INVALID_RESOURCE_ID;
	}

	ResourceID textureId = GenerateResourceID();
	auto textureAsset = std::make_shared<TextureAsset>(textureId, imageAssetId);

	// Get image data
	int w, h, c;
	imageAsset->GetDimensions(w, h, c);
	unsigned char* data = imageAsset->GetImageData();

	if (!data) {
		std::cerr << "[AssetManager] No image data available for texture creation" << std::endl;
		return INVALID_RESOURCE_ID;
	}

	// Create OpenGL texture using your existing utilities
	GLuint texId = Utils::OpenGLUtils::GenerateTexture(w, h, c, data);
	if (texId == 0) {
		std::cerr << "[AssetManager] Failed to create OpenGL texture" << std::endl;
		return INVALID_RESOURCE_ID;
	}

	// Set texture properties
	textureAsset->renderHandle = RenderHandle(texId);
	textureAsset->width = w;
	textureAsset->height = h;
	textureAsset->channels = c;
	textureAsset->loadState = LoadState::Loaded;

	// Store texture asset
	{
		std::lock_guard<std::mutex> lock(assetMutex);
		assets[textureId] = textureAsset;
	}

	std::cout << "[AssetManager] Created texture from image (ID: " << textureId
		<< ", GL: " << texId << ", Size: " << w << "x" << h << ")" << std::endl;

	return textureId;
}

std::shared_ptr<Asset> AssetManager::GetAsset(ResourceID id) {
	std::lock_guard<std::mutex> lock(assetMutex);
	auto it = assets.find(id);
	return (it != assets.end()) ? it->second : nullptr;
}

void AssetManager::UnloadAsset(ResourceID id) {
	std::lock_guard<std::mutex> lock(assetMutex);
	auto it = assets.find(id);
	if (it != assets.end()) {
		// Remove from path mapping
		std::string path = it->second->GetPath();
		if (!path.empty()) {
			pathToAsset.erase(path);
		}

		// Unload and remove
		it->second->UnloadImpl();
		assets.erase(it);

		std::cout << "[AssetManager] Unloaded asset (ID: " << id << ")" << std::endl;
	}
}

bool AssetManager::HasAsset(ResourceID id) const {
	std::lock_guard<std::mutex> lock(assetMutex);
	return assets.find(id) != assets.end();
}

void AssetManager::Update() {
	if (!IsMainThread()) {
		return;
	}
	ProcessMainThreadTasks();
}

std::vector<std::shared_ptr<Asset>> AssetManager::GetAssetsByType(AssetType type) {
	std::lock_guard<std::mutex> lock(assetMutex);
	std::vector<std::shared_ptr<Asset>> result;
	for (const auto& pair : assets) {
		if (pair.second && pair.second->GetType() == type) {
			result.push_back(pair.second);
		}
	}
	return result;
}

AssetManager::LoadingStats AssetManager::GetStats() const {
	LoadingStats stats;
	{
		std::lock_guard<std::mutex> lock(assetMutex);
		stats.totalAssets = assets.size();
		for (const auto& pair : assets) {
			if (pair.second) {
				switch (pair.second->GetLoadState()) {
				case LoadState::Loaded: stats.loadedAssets++; break;
				case LoadState::Loading: stats.loadingAssets++; break;
				case LoadState::Failed: stats.failedAssets++; break;
				default: break;
				}
			}
		}
	}
	stats.threadPoolStats = Utils::ThreadPoolManager::getInstance().getStats();
	return stats;
}

void AssetManager::GarbageCollect() {
	std::lock_guard<std::mutex> lock(assetMutex);
	std::vector<ResourceID> toRemove;
	for (const auto& pair : assets) {
		if (pair.second.use_count() <= 1) {
			toRemove.push_back(pair.first);
		}
	}
	for (ResourceID id : toRemove) {
		auto it = assets.find(id);
		if (it != assets.end()) {
			std::string path = it->second->GetPath();
			if (!path.empty()) {
				pathToAsset.erase(path);
			}
			it->second->UnloadImpl();
			assets.erase(it);
		}
	}
	if (!toRemove.empty()) {
		std::cout << "[AssetManager] Garbage collected " << toRemove.size() << " unused assets" << std::endl;
	}
}

size_t AssetManager::GetMemoryUsage() const {
	std::lock_guard<std::mutex> lock(assetMutex);
	size_t totalMemory = 0;
	for (const auto& pair : assets) {
		if (auto imageAsset = std::dynamic_pointer_cast<ImageAsset>(pair.second)) {
			if (imageAsset->IsLoaded()) {
				totalMemory += imageAsset->GetWidth() * imageAsset->GetHeight() * imageAsset->GetChannels();
			}
		}
	}
	return totalMemory;
}

std::string AssetManager::GetAssetPath(ResourceID id) const {
	std::lock_guard<std::mutex> lock(assetMutex);
	auto it = assets.find(id);
	return (it != assets.end()) ? it->second->GetPath() : "";
}

ResourceID AssetManager::FindAssetByPath(const std::string& path) const {
	std::lock_guard<std::mutex> lock(assetMutex);
	auto it = pathToAsset.find(path);
	return (it != pathToAsset.end()) ? it->second : INVALID_RESOURCE_ID;
}

ResourceID AssetManager::GenerateResourceID() {
	return nextResourceID.fetch_add(1);
}

bool AssetManager::IsMainThread() const {
	return std::this_thread::get_id() == mainThreadId;
}

void AssetManager::ProcessMainThreadTasks() {
	std::queue<std::function<void()>> tasksToProcess;
	{
		std::lock_guard<std::mutex> lock(mainThreadTaskMutex);
		tasksToProcess = std::move(mainThreadTasks);
		mainThreadTasks = std::queue<std::function<void()>>();
	}
	while (!tasksToProcess.empty()) {
		try {
			tasksToProcess.front()();
		}
		catch (const std::exception& e) {
			std::cerr << "[AssetManager] Main thread task exception: " << e.what() << std::endl;
		}
		tasksToProcess.pop();
	}
}

void AssetManager::QueueMainThreadTask(std::function<void()> task) {
	std::lock_guard<std::mutex> lock(mainThreadTaskMutex);
	mainThreadTasks.push(std::move(task));
}