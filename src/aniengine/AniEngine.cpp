#define ANI_ENGINE_EXPORTS

#include "AniEngine.hpp"
#include "EnginePluginManager.hpp"
#include "utils.h"
#include "components.h"
#include "systems.h"
#include <iostream>
#include <filesystem>

using namespace ECS;

namespace ANI {

	EngineCore::EngineCore()
		: initialized(false), running(false) {
		std::cout << "[EngineCore] Constructor called" << std::endl;

		// Create engine-only plugin manager (no GUI support)
		enginePluginManager = std::make_unique<Plugin::EnginePluginManager>(entityManager);
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

		entityManager.RegisterSystem<ECS::RenderSystem>();

		std::cout << "[EngineCore] Core systems registered" << std::endl;
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

			// The engine plugin manager is already initialized in constructor
			std::cout << "[EngineCore] Engine plugin manager ready" << std::endl;

			// NO auto-loading of plugins - they are loaded manually by user choice

			initialized = true;
			running = true;

			std::cout << "[EngineCore] Initialized successfully with engine plugin system" << std::endl;
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

		// Shutdown plugins first
		if (enginePluginManager) {
			enginePluginManager->UnloadAllPlugins();
		}

		entityManager.Reset();
		initialized = false;

		std::cout << "[EngineCore] Shutdown complete" << std::endl;
	}

	void EngineCore::Update(float deltaTime) {
		if (!initialized) return;

		entityManager.Update(deltaTime);

		// Update engine plugin systems
		if (enginePluginManager) {
			enginePluginManager->Update(deltaTime);
		}
	}

	bool EngineCore::LoadPlugin(const std::string& path) {
		if (!enginePluginManager) {
			std::cerr << "[EngineCore] Engine plugin manager not initialized!" << std::endl;
			return false;
		}
		return enginePluginManager->LoadPlugin(path);
	}

	bool EngineCore::UnloadPlugin(const std::string& pluginName) {
		if (!enginePluginManager) {
			std::cerr << "[EngineCore] Engine plugin manager not initialized!" << std::endl;
			return false;
		}
		return enginePluginManager->UnloadPlugin(pluginName);
	}

	std::vector<std::string> EngineCore::GetLoadedPlugins() const {
		if (!enginePluginManager) {
			return {};
		}
		return enginePluginManager->GetLoadedPluginNames();
	}
}