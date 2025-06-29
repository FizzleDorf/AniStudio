// AniEngine.cpp - FIXED to match working StudioCore pattern
#define ANI_ENGINE_EXPORTS

#include "AniEngine.hpp"
#include "ECS.h"
#include "PluginManager.hpp"
#include "utils.h"
#include "components.h"
#include "systems.h"
#include <iostream>
#include <filesystem>

using namespace ECS;

namespace ANI {
	// Static members initialization
	bool EngineCore::s_initialized = false;
	bool EngineCore::s_running = false;

	// CRITICAL FIX: Direct static instances like the working StudioCore
	static ECS::EntityManager g_entityManager;
	static Plugin::PluginManager g_pluginManager(g_entityManager);

	ECS::EntityManager& EngineCore::GetEntityManagerImpl() {
		return g_entityManager;
	}

	Plugin::PluginManager& EngineCore::GetPluginManagerImpl() {
		return g_pluginManager;
	}

	void EngineCore::RegisterCoreComponents(ECS::EntityManager& mgr) {
		// Register Component Names - EXACTLY like working StudioCore
		mgr.RegisterComponentName<ModelComponent>("Model");
		mgr.RegisterComponentName<ClipLComponent>("ClipL");
		mgr.RegisterComponentName<ClipGComponent>("ClipG");
		mgr.RegisterComponentName<T5XXLComponent>("T5XXL");
		mgr.RegisterComponentName<DiffusionModelComponent>("DiffusionModel");
		mgr.RegisterComponentName<LatentComponent>("Latent");
		mgr.RegisterComponentName<LoraComponent>("Lora");
		mgr.RegisterComponentName<PromptComponent>("Prompt");
		mgr.RegisterComponentName<SamplerComponent>("Sampler");
		mgr.RegisterComponentName<GuidanceComponent>("Guidance");
		mgr.RegisterComponentName<EsrganComponent>("Esrgan");
		mgr.RegisterComponentName<ClipSkipComponent>("ClipSkip");
		mgr.RegisterComponentName<VaeComponent>("Vae");
		mgr.RegisterComponentName<TaesdComponent>("Taesd");
		mgr.RegisterComponentName<ImageComponent>("Image");
		mgr.RegisterComponentName<InputImageComponent>("InputImage");
		mgr.RegisterComponentName<OutputImageComponent>("OutputImage");
		mgr.RegisterComponentName<EmbeddingComponent>("Embedding");
		mgr.RegisterComponentName<ControlnetComponent>("Controlnet");
		mgr.RegisterComponentName<LayerSkipComponent>("LayerSkip");
		mgr.RegisterComponentName<VideoComponent>("Video");
		mgr.RegisterComponentName<InputVideoComponent>("InputVideo");
		mgr.RegisterComponentName<OutputVideoComponent>("OutputVideo");
		mgr.RegisterComponentName<PythonComponent>("Python");

		std::cout << "[EngineCore] Core components registered" << std::endl;
	}

	void EngineCore::RegisterCoreSystems(ECS::EntityManager& mgr) {
		// Register core systems - EXACTLY like working StudioCore
		mgr.RegisterSystem<SDCPPSystem>();
		mgr.RegisterSystem<ImageSystem>();
		mgr.RegisterSystem<VideoSystem>();
		mgr.RegisterSystem<PythonSystem>();

		std::cout << "[EngineCore] Core systems registered" << std::endl;
	}

	bool EngineCore::Initialize() {
		if (s_initialized) {
			std::cerr << "[EngineCore] Already initialized!" << std::endl;
			return false;
		}

		try {
			std::cout << "[EngineCore] Initializing..." << std::endl;

			// Initialize file paths - EXACTLY like working StudioCore
			Utils::FilePaths::LoadFilePathDefaults();

			// Reset managers (no need to construct, they're already static)
			g_entityManager.Reset();

			// Invalidate ID 0 for consistency - EXACTLY like working StudioCore
			const ECS::EntityID temp = g_entityManager.AddNewEntity();
			g_entityManager.DestroyEntity(temp);

			// Register core components and systems using the static instance
			RegisterCoreComponents(g_entityManager);
			RegisterCoreSystems(g_entityManager);

			// Initialize the plugin manager
			g_pluginManager.Init();

			s_initialized = true;
			s_running = true;

			std::cout << "[EngineCore] Initialized successfully with plugin system" << std::endl;
			return true;
		}
		catch (const std::exception& e) {
			std::cerr << "[EngineCore] Initialization failed: " << e.what() << std::endl;
			return false;
		}
	}

	void EngineCore::Shutdown() {
		if (!s_initialized) return;

		std::cout << "[EngineCore] Shutting down..." << std::endl;

		s_running = false;

		// Reset managers (no need to delete, they're static)
		g_entityManager.Reset();
		// PluginManager doesn't have Reset(), but it will be destroyed with static cleanup

		s_initialized = false;
		std::cout << "[EngineCore] Shutdown complete" << std::endl;
	}

	void EngineCore::Update(float deltaTime) {
		if (!s_initialized) return;

		// Update all registered systems
		g_entityManager.Update(deltaTime);

		// Update plugins
		g_pluginManager.Update(deltaTime);
	}

	ECS::EntityManager& EngineCore::GetEntityManager() {
		if (!s_initialized) {
			throw std::runtime_error("[EngineCore] EntityManager accessed before initialization!");
		}
		return g_entityManager;
	}

	Plugin::PluginManager& EngineCore::GetPluginManager() {
		if (!s_initialized) {
			throw std::runtime_error("[EngineCore] PluginManager accessed before initialization!");
		}
		return g_pluginManager;
	}

	bool EngineCore::LoadPlugin(const std::string& path) {
		return g_pluginManager.LoadPlugin(path);
	}

	void EngineCore::LoadDefaultPlugins() {
		std::string pluginsDir = Utils::FilePaths::pluginPath;

		if (!std::filesystem::exists(pluginsDir)) {
			std::cout << "[EngineCore] Creating plugins directory: " << pluginsDir << std::endl;
			std::filesystem::create_directories(pluginsDir);
			return;
		}

		std::cout << "[EngineCore] Loading plugins from: " << pluginsDir << std::endl;

		for (const auto& entry : std::filesystem::directory_iterator(pluginsDir)) {
			if (entry.is_regular_file()) {
				std::string extension = entry.path().extension().string();

#ifdef _WIN32
				if (extension == ".dll") {
#else
				if (extension == ".so") {
#endif
					std::string pluginPath = entry.path().string();
					std::cout << "[EngineCore] Loading plugin: " << pluginPath << std::endl;
					g_pluginManager.LoadPlugin(pluginPath);
				}
				}
			}
		}

	bool EngineCore::IsRunning() {
		return s_running;
	}

	void EngineCore::SetRunning(bool running) {
		s_running = running;
	}

	} // namespace ANI