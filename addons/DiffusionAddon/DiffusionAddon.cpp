// DiffusionAddon.cpp - Complete with backend management and dynamic loading
#include "BasePlugin.hpp"
#include "PluginRegistry.hpp"
#include "EntityManager.hpp"

// Addon's own SDCPP components and systems
#include "components.h"
#include "SDcppSystem.hpp"

// Backend manager
#include "utils/SDCPPBackendManager.hpp"

#include <iostream>
#include <memory>

class DiffusionAddon : public Plugins::BasePlugin {
public:
	DiffusionAddon() : BasePlugin("DiffusionAddon", "1.0.0") {}

	~DiffusionAddon() override {
		backendManager.reset();
		LogInfo("Shutdown complete");
	}

	bool OnEngineInit(ECS::EntityManager& entityMgr, Plugins::IPluginRegistry& registry) override {
		LogInfo("Initializing Stable Diffusion Addon...");

		// Initialize backend manager
		std::string libsPath = GetAddonLibsPath();
		backendManager = std::make_unique<DiffusionAddon::SDCPPBackendManager>(libsPath);

		// Initialize will:
		// 1. Detect available backends
		// 2. Download default backend if none exist
		// 3. Load the best available backend
		if (!backendManager->Initialize()) {
			LogError("Failed to initialize SDCPP backend");
			LogError("Backend manager could not load or download stable-diffusion.cpp");
			return false;
		}

		auto currentBackend = backendManager->GetCurrentBackend();
		LogInfo("Loaded backend: " + GetBackendName(currentBackend));

		// Register all SDCPP components
		RegisterSDCPPComponents(registry);

		// Register SDCPP system
		RegisterSDCPPSystem(registry, entityMgr);

		LogInfo("Stable Diffusion Addon initialized successfully");
		return true;
	}

	void OnShutdown() override {
		LogInfo("Shutting down Stable Diffusion Addon");
		backendManager.reset();
	}

	void OnUpdate(float deltaTime) override {
		// SDCPP system handles its own updates through ECS
	}

	// Allow runtime backend switching
	bool SwitchBackend(DiffusionAddon::SDCPPBackend newBackend) {
		if (!backendManager) return false;

		LogInfo("Switching to backend: " + GetBackendName(newBackend));

		if (!backendManager->IsBackendAvailable(newBackend)) {
			LogInfo("Backend not available, downloading...");
			if (!backendManager->DownloadBackend(newBackend)) {
				LogError("Failed to download backend");
				return false;
			}
		}

		return backendManager->LoadBackend(newBackend);
	}

private:
	std::unique_ptr<DiffusionAddon::SDCPPBackendManager> backendManager;

	std::string GetAddonLibsPath() {
		// Path relative to running executable
		// When running from build/bin/, addon libs are at build/addons/DiffusionAddon/libs/
		return "../addons/DiffusionAddon/libs";
	}

	std::string GetBackendName(DiffusionAddon::SDCPPBackend backend) {
		switch (backend) {
		case DiffusionAddon::SDCPPBackend::CPU_NOAVX: return "CPU (No AVX)";
		case DiffusionAddon::SDCPPBackend::CPU_AVX: return "CPU (AVX)";
		case DiffusionAddon::SDCPPBackend::CPU_AVX2: return "CPU (AVX2)";
		case DiffusionAddon::SDCPPBackend::CPU_AVX512: return "CPU (AVX512)";
		case DiffusionAddon::SDCPPBackend::CUDA: return "CUDA";
		case DiffusionAddon::SDCPPBackend::VULKAN: return "Vulkan";
		case DiffusionAddon::SDCPPBackend::METAL: return "Metal";
		default: return "Unknown";
		}
	}

	void RegisterSDCPPComponents(Plugins::IPluginRegistry& registry) {
		LogInfo("Registering SDCPP components...");

		// Sampling and Inference
		RegisterComponent<ECS::PromptComponent>(registry, "Prompt");
		RegisterComponent<ECS::SamplerComponent>(registry, "Sampler");
		RegisterComponent<ECS::GuidanceComponent>(registry, "Guidance");
		RegisterComponent<ECS::ClipSkipComponent>(registry, "ClipSkip");

		// Models
		RegisterComponent<ECS::ModelComponent>(registry, "Model");
		RegisterComponent<ECS::ClipLComponent>(registry, "ClipL");
		RegisterComponent<ECS::ClipGComponent>(registry, "ClipG");
		RegisterComponent<ECS::T5XXLComponent>(registry, "T5XXL");
		RegisterComponent<ECS::DiffusionModelComponent>(registry, "DiffusionModel");
		RegisterComponent<ECS::VaeComponent>(registry, "Vae");
		RegisterComponent<ECS::TaesdComponent>(registry, "Taesd");
		RegisterComponent<ECS::ControlNetComponent>(registry, "ControlNet");
		RegisterComponent<ECS::LoraComponent>(registry, "Lora");
		RegisterComponent<ECS::EmbedComponent>(registry, "Embed");

		// Video models
		RegisterComponent<ECS::HighNoiseDiffusionModelComponent>(registry, "HighNoiseDiffusionModel");
		RegisterComponent<ECS::ClipVisionComponent>(registry, "ClipVision");
		RegisterComponent<ECS::HighNoiseSamplerComponent>(registry, "HighNoiseSampler");
		RegisterComponent<ECS::VideoParamsComponent>(registry, "VideoParams");
		RegisterComponent<ECS::StackedIdEmbedComponent>(registry, "StackedIdEmbed");

		// Latents
		RegisterComponent<ECS::LatentComponent>(registry, "Latent");
		RegisterComponent<ECS::LatentTransformComponent>(registry, "LatentTransform");

		// Other
		RegisterComponent<ECS::LayerSkipComponent>(registry, "LayerSkip");
		RegisterComponent<ECS::ChromaComponent>(registry, "Chroma");
		RegisterComponent<ECS::EsrganComponent>(registry, "Esrgan");

		LogInfo("Registered all SDCPP components");
	}

	void RegisterSDCPPSystem(Plugins::IPluginRegistry& registry, ECS::EntityManager& entityMgr) {
		LogInfo("Registering SDCPP system...");

		std::vector<ECS::ComponentTypeID> requiredComponents = {
			ECS::ComponentTypeRegistry::GetIDByName("Prompt"),
			ECS::ComponentTypeRegistry::GetIDByName("Sampler")
		};

		Plugins::SystemDescriptor sysDesc;
		sysDesc.name = "SDCPPSystem";
		sysDesc.creator = [](ECS::EntityManager* mgr) -> void* {
			return new ECS::SDCPPSystem(*mgr);
		};
		sysDesc.destructor = [](void* sys) {
			delete static_cast<ECS::SDCPPSystem*>(sys);
		};
		sysDesc.updater = [](void* sys, float dt) {
			static_cast<ECS::SDCPPSystem*>(sys)->Update(dt);
		};
		sysDesc.requiredComponents = requiredComponents;

		registry.RegisterSystem(sysDesc);
		LogInfo("SDCPP system registered");
	}

	template<typename T>
	void RegisterComponent(Plugins::IPluginRegistry& registry, const std::string& name) {
		Plugins::ComponentDescriptor desc;
		desc.name = name;
		desc.size = sizeof(T);
		desc.constructor = [](void* ptr, ECS::EntityID entityID) {
			new (ptr) T();
		};
		desc.destructor = [](void* ptr) {
			static_cast<T*>(ptr)->~T();
		};
		registry.RegisterComponent(desc);
	}
};

// Plugin export functions
extern "C" {
#ifdef _WIN32
#define PLUGIN_EXPORT __declspec(dllexport)
#else
#define PLUGIN_EXPORT __attribute__((visibility("default")))
#endif

	PLUGIN_EXPORT Plugins::BasePlugin* CreatePlugin() {
		return new DiffusionAddon();
	}

	PLUGIN_EXPORT void DestroyPlugin(Plugins::BasePlugin* plugin) {
		delete plugin;
	}
}