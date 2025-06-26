#define ANI_ENGINE_EXPORTS

#include "AniEngine.hpp"
#include "PluginManager.hpp"
#include <iostream>

using namespace ECS;

namespace ANI {
	// Static members
	std::unique_ptr<ECS::EntityManager> EngineCore::s_entityManager = nullptr;
	std::unique_ptr<Plugin::PluginManager> EngineCore::s_pluginManager = nullptr;
	bool EngineCore::s_initialized = false;
	bool EngineCore::s_running = false;

	void EngineCore::RegisterCoreComponents(ECS::EntityManager& mgr) {
		// Register Component Names
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
		// Register core systems
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

			// Initialize file paths
			Utils::FilePaths::LoadFilePathDefaults();

			// Create entity manager
			s_entityManager = std::make_unique<ECS::EntityManager>();

			// Invalidate ID 0 for consistency
			const ECS::EntityID temp = s_entityManager->AddNewEntity();
			s_entityManager->DestroyEntity(temp);

			// Register core components and systems
			RegisterCoreComponents(*s_entityManager);
			RegisterCoreSystems(*s_entityManager);

			// Create plugin manager for engine-only (no GUI)
			s_pluginManager = std::make_unique<Plugin::PluginManager>(*s_entityManager);

			// Initialize the plugin manager
			s_pluginManager->Init();

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

		// Shutdown plugin manager first
		if (s_pluginManager) {
			s_pluginManager.reset();
		}

		// Reset entity manager
		if (s_entityManager) {
			s_entityManager->Reset();
			s_entityManager.reset();
		}

		s_initialized = false;
		std::cout << "[EngineCore] Shutdown complete" << std::endl;
	}

	void EngineCore::Update(float deltaTime) {
		if (!s_initialized) return;

		// Update all registered systems
		if (s_entityManager) {
			s_entityManager->Update(deltaTime);
		}

		// Update plugins
		if (s_pluginManager) {
			s_pluginManager->Update(deltaTime);
		}
	}

	ECS::EntityManager& EngineCore::GetEntityManager() {
		if (!s_initialized || !s_entityManager) {
			throw std::runtime_error("[EngineCore] EntityManager accessed before initialization!");
		}
		return *s_entityManager;
	}

	Plugin::PluginManager& EngineCore::GetPluginManager() {
		if (!s_initialized || !s_pluginManager) {
			throw std::runtime_error("[EngineCore] PluginManager accessed before initialization!");
		}
		return *s_pluginManager;
	}

	bool EngineCore::LoadPlugin(const std::string& path) {
		if (!s_pluginManager) return false;
		return s_pluginManager->LoadPlugin(path);
	}

	void EngineCore::LoadDefaultPlugins() {
		if (!s_pluginManager) return;

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
					s_pluginManager->LoadPlugin(pluginPath);
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