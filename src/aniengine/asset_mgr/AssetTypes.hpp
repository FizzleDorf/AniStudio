#pragma once

#include <atomic>
#include <mutex>
#include <string>
#include <typeindex>
#include <GL/glew.h>

// Universal resource ID system - backend agnostic
using ResourceID = uint64_t;
constexpr ResourceID INVALID_RESOURCE_ID = 0;

enum class AssetType {
	Image,
	Video,
	Texture,
	Model,
	Audio,
	StableDiffusionModel,
	Custom
};

enum class LoadState {
	Unloaded,
	Loading,
	Loaded,
	Failed
};

// Universal handle for different rendering backends
struct RenderHandle {
	uint64_t handle = 0;           // OpenGL texture ID, Vulkan handle, etc.
	std::type_index backendType;   // Type info for the backend

	RenderHandle() : backendType(typeid(void)) {}

	template<typename T>
	RenderHandle(T h) : handle(static_cast<uint64_t>(h)), backendType(typeid(T)) {}

	template<typename T>
	T Get() const {
		if (backendType == typeid(T)) {
			return static_cast<T>(handle);
		}
		return T{ 0 };
	}

	bool IsValid() const { return handle != 0; }
	void Clear() { handle = 0; backendType = typeid(void); }
};

// Base asset class
class Asset {
public:
	Asset(ResourceID id, const std::string& path, AssetType type)
		: id(id), filePath(path), type(type), loadState(LoadState::Unloaded) {}

	virtual ~Asset() = default;

	ResourceID GetID() const { return id; }
	const std::string& GetPath() const { return filePath; }
	AssetType GetType() const { return type; }
	LoadState GetLoadState() const { return loadState.load(); }

	bool IsLoaded() const { return loadState == LoadState::Loaded; }
	bool IsLoading() const { return loadState == LoadState::Loading; }
	bool HasFailed() const { return loadState == LoadState::Failed; }

protected:
	friend class AssetManager;
	ResourceID id;
	std::string filePath;
	AssetType type;
	std::atomic<LoadState> loadState;
	mutable std::mutex dataMutex;

	// Override in derived classes for specific loading logic
	virtual bool LoadImpl() = 0;
	virtual void UnloadImpl() = 0;
	virtual void FinalizeOnMainThread() {} // Called on main thread after background loading
};