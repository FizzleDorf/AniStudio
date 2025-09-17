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
		: initialized(false), running(false), pluginDirectory("../plugins") {  // FIXED: use ../plugins
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
		entityManager.RegisterComponentName<HighNoiseDiffusionModelComponent>("HighNoiseDiffusionModel");
		entityManager.RegisterComponentName<ClipVisionComponent>("ClipVision");
		entityManager.RegisterComponentName<HighNoiseSamplerComponent>("HighNoiseSampler");
		entityManager.RegisterComponentName<VideoParamsComponent>("VideoParams");
		entityManager.RegisterComponentName<StackedIdEmbedComponent>("StackedIdEmbed");

		entityManager.RegisterComponentName<ECS::TransformComponent>("Transform");
		entityManager.RegisterComponentName<ECS::MeshComponent>("Mesh");
		entityManager.RegisterComponentName<ECS::CameraComponent>("Camera");

		std::cout << "[EngineCore] Core components registered" << std::endl;
	}

	void EngineCore::RegisterCoreSystems() {
		entityManager.RegisterSystem<ImageSystem>();
		entityManager.RegisterSystem<SDCPPSystem>();
		entityManager.RegisterSystem<VideoSystem>();
		entityManager.RegisterSystem<PythonSystem>();

		std::cout << "[EngineCore] Core systems registered" << std::endl;
	}

	void EngineCore::InitializePlugins() {
		if (!pluginManager) {
			std::cerr << "[EngineCore] PluginManager not created!" << std::endl;
			return;
		}

		std::cout << "[EngineCore] Initializing plugins from: " << pluginDirectory << std::endl;

		// Create plugin directory if it doesn't exist
		if (!std::filesystem::exists(pluginDirectory)) {
			std::filesystem::create_directories(pluginDirectory);
			std::cout << "[EngineCore] Created plugin directory: " << pluginDirectory << std::endl;
		}

		// CRITICAL: Set up plugin state management IMMEDIATELY
		std::cout << "[EngineCore] Setting global data path: " << Utils::FilePaths::dataPath << std::endl;
		pluginManager->SetGlobalDataPath(Utils::FilePaths::dataPath);

		// Scan plugins directory (but don't auto-load everything)
		pluginManager->scanPluginDirectory(pluginDirectory);

		// FOR ENGINE-ONLY USAGE: Uncomment this to force hot reload always on
		// pluginManager->setHotReloadForce(true);

		// CRITICAL: Load ONLY the plugins that were enabled in the last session
		std::cout << "[EngineCore] Loading global plugin state..." << std::endl;
		pluginManager->LoadGlobalPluginState();

		std::cout << "[EngineCore] Plugin system initialized with selective loading" << std::endl;
	}

	bool EngineCore::Initialize() {
		if (initialized) {
			std::cerr << "[EngineCore] Already initialized!" << std::endl;
			return false;
		}

		try {
			std::cout << "[EngineCore] Initializing..." << std::endl;

			// Initialize file paths properly (sets up defaults AND loads saved paths)
			std::cout << "[EngineCore] Initializing file paths..." << std::endl;
			Utils::FilePaths::Init();

			// Invalidate ID 0 for consistency
			const ECS::EntityID temp = entityManager.AddNewEntity();
			entityManager.DestroyEntity(temp);

			// Register components and systems
			RegisterCoreComponents();
			RegisterCoreSystems();

			// Create plugin manager (ENGINE ONLY VERSION)
			pluginManager = std::make_unique<Plugins::PluginManager>(entityManager);
			std::cout << "[EngineCore] Plugin manager created" << std::endl;

			// Initialize plugins
			InitializePlugins();

			initialized = true;
			running = true;

			std::cout << "[EngineCore] Initialized successfully" << std::endl;
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

		// Save global plugin state before shutdown
		if (pluginManager) {
			std::cout << "[EngineCore] Saving global plugin state..." << std::endl;
			pluginManager->SaveGlobalPluginState();
			pluginManager.reset();
		}

		entityManager.Reset();
		initialized = false;

		std::cout << "[EngineCore] Shutdown complete" << std::endl;
	}

	void EngineCore::Update(float deltaTime) {
		if (!initialized) return;

		entityManager.Update(deltaTime);

		// Update plugins
		if (pluginManager) {
			pluginManager->checkForChanges(); // Hot reload check
			pluginManager->updatePlugins(deltaTime);
		}
	}

} // namespace ANI