#define ANI_ENGINE_EXPORTS

#include "AniEngine.hpp"
#include "utils.h"
#include "components.h"
#include "systems.h"
#include <iostream>
#include <filesystem>

using namespace ECS;

namespace ANI {

	EngineCore::EngineCore()
		: initialized(false), running(false), pluginManager(entityManager) {
		std::cout << "[EngineCore] Constructor called" << std::endl;
	}

	EngineCore::~EngineCore() {
		if (initialized) {
			Shutdown();
		}
	}

	void EngineCore::RegisterCoreComponents() {
		entityManager.RegisterComponentName<ModelComponent>("Model");
		entityManager.RegisterComponentName<ClipLComponent>("ClipL");
		entityManager.RegisterComponentName<ClipGComponent>("ClipG");
		entityManager.RegisterComponentName<T5XXLComponent>("T5XXL");
		entityManager.RegisterComponentName<DiffusionModelComponent>("DiffusionModel");
		entityManager.RegisterComponentName<LatentComponent>("Latent");
		entityManager.RegisterComponentName<LoraComponent>("Lora");
		entityManager.RegisterComponentName<PromptComponent>("Prompt");
		entityManager.RegisterComponentName<SamplerComponent>("Sampler");
		entityManager.RegisterComponentName<GuidanceComponent>("Guidance");
		entityManager.RegisterComponentName<EsrganComponent>("Esrgan");
		entityManager.RegisterComponentName<ClipSkipComponent>("ClipSkip");
		entityManager.RegisterComponentName<VaeComponent>("Vae");
		entityManager.RegisterComponentName<TaesdComponent>("Taesd");
		entityManager.RegisterComponentName<ImageComponent>("Image");
		entityManager.RegisterComponentName<InputImageComponent>("InputImage");
		entityManager.RegisterComponentName<OutputImageComponent>("OutputImage");
		entityManager.RegisterComponentName<EmbeddingComponent>("Embedding");
		entityManager.RegisterComponentName<ControlnetComponent>("Controlnet");
		entityManager.RegisterComponentName<LayerSkipComponent>("LayerSkip");
		entityManager.RegisterComponentName<VideoComponent>("Video");
		entityManager.RegisterComponentName<InputVideoComponent>("InputVideo");
		entityManager.RegisterComponentName<OutputVideoComponent>("OutputVideo");
		entityManager.RegisterComponentName<PythonComponent>("Python");
		entityManager.RegisterComponentName<ChromaComponent>("Chroma");
		entityManager.RegisterComponentName<StackedIdEmbedComponent>("StackedIdEmbed");

		std::cout << "[EngineCore] Core components registered" << std::endl;
	}

	void EngineCore::RegisterCoreSystems() {
		entityManager.RegisterSystem<ImageSystem>();
		entityManager.RegisterSystem<SDCPPSystem>();
		entityManager.RegisterSystem<VideoSystem>();
		entityManager.RegisterSystem<PythonSystem>();

		std::cout << "[EngineCore] Core systems registered" << std::endl;
	}

	bool EngineCore::Initialize() {
		if (initialized) {
			std::cerr << "[EngineCore] Already initialized!" << std::endl;
			return false;
		}

		try {
			std::cout << "[EngineCore] Initializing..." << std::endl;

			// Initialize file paths
			Utils::FilePaths::LoadFilePathDefaults();

			// Invalidate ID 0 for consistency
			const ECS::EntityID temp = entityManager.AddNewEntity();
			entityManager.DestroyEntity(temp);

			// Register components and systems
			RegisterCoreComponents();
			RegisterCoreSystems();

			// Initialize the plugin manager
			pluginManager.Init();

			initialized = true;
			running = true;

			std::cout << "[EngineCore] Initialized successfully with plugin system" << std::endl;
			std::cout << "[EngineCore] EntityManager address: " << &entityManager << std::endl;
			return true;
		}
		catch (const std::exception& e) {
			std::cerr << "[EngineCore] Initialization failed: " << e.what() << std::endl;
			return false;
		}
	}

	void EngineCore::Shutdown() {
		if (!initialized) return;

		std::cout << "[EngineCore] Shutting down..." << std::endl;

		running = false;
		entityManager.Reset();
		initialized = false;

		std::cout << "[EngineCore] Shutdown complete" << std::endl;
	}

	void EngineCore::Update(float deltaTime) {
		if (!initialized) return;

		entityManager.Update(deltaTime);
		pluginManager.Update(deltaTime);
	}

	bool EngineCore::LoadPlugin(const std::string& path) {
		return pluginManager.LoadPlugin(path);
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
					pluginManager.LoadPlugin(pluginPath);
				}
				}
			}
		}

	} // namespace ANI