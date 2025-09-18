#define ANI_ENGINE_EXPORTS

#include "AniEngine.hpp"
#include "utils.h"
#include "components.h"
#include "systems.h"
#include "AssetManager.hpp" // Add this include
#include <iostream>
#include <filesystem>

using namespace ECS;

namespace ANI {

	EngineCore::EngineCore()
		: initialized(false), running(false), pluginDirectory("../plugins") {
		std::cout << "[EngineCore] Constructor called" << std::endl;
	}

	EngineCore::~EngineCore() {
		if (initialized) {
			Shutdown();
		}
	}

	void EngineCore::RegisterCoreComponents() {
		// Register new asset handle components
		entityManager.RegisterComponentName<ImageHandleComponent>("ImageHandle");
		entityManager.RegisterComponentName<VideoHandleComponent>("VideoHandle");
		entityManager.RegisterComponentName<TextureHandleComponent>("TextureHandle");

		// Keep existing components for backward compatibility during migration
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

		// Keep old image components for backward compatibility
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
		// Register new asset management systems (these replace the old ones)
		entityManager.RegisterSystem<ImageSystem>();
		entityManager.RegisterSystem<VideoSystem>();
		// Note: TextureSystem not registered since you moved it to AniStudio for headless support

		// Register other systems
		entityManager.RegisterSystem<SDCPPSystem>();
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

		std::cout << "[EngineCore] Setting global data path: " << Utils::FilePaths::dataPath << std::endl;
		pluginManager->SetGlobalDataPath(Utils::FilePaths::dataPath);

		// Scan plugins directory (but don't auto-load everything)
		pluginManager->scanPluginDirectory(pluginDirectory);

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

			// Initialize asset management system FIRST
			std::cout << "[EngineCore] Initializing AssetManager..." << std::endl;
			AssetManager::Instance().Initialize();

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

		// Shutdown asset manager last
		std::cout << "[EngineCore] Shutting down AssetManager..." << std::endl;
		AssetManager::Instance().Shutdown();

		initialized = false;

		std::cout << "[EngineCore] Shutdown complete" << std::endl;
	}

	void EngineCore::Update(float deltaTime) {
		if (!initialized) return;

		// Update asset manager first (handles main-thread texture creation)
		AssetManager::Instance().Update();

		entityManager.Update(deltaTime);

		// Update plugins
		if (pluginManager) {
			pluginManager->checkForChanges(); // Hot reload check
			pluginManager->updatePlugins(deltaTime);
		}
	}

} // namespace ANI