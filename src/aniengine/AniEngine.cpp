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
		: initialized(false), running(false), pluginDirectory("../plugins") {
		std::cout << "[EngineCore] Constructor called" << std::endl;
	}

	EngineCore::~EngineCore() {
		if (initialized) {
			Shutdown();
		}
	}

	void EngineCore::RegisterCoreComponents() {

		entityManager.RegisterComponent<ImageComponent>("Image");
		entityManager.RegisterComponent<InputImageComponent>("InputImage");
		entityManager.RegisterComponent<OutputImageComponent>("OutputImage");
		entityManager.RegisterComponent<MaskImageComponent >("MaskImage");

		entityManager.RegisterComponent<VideoComponent>("Video");
		entityManager.RegisterComponent<InputVideoComponent>("InputVideo");
		entityManager.RegisterComponent<OutputVideoComponent>("OutputVideo");
		entityManager.RegisterComponent<PythonComponent>("Python");

		entityManager.RegisterComponent<ECS::TransformComponent>("Transform");
		entityManager.RegisterComponent<ECS::MeshComponent>("Mesh");
		entityManager.RegisterComponent<ECS::CameraComponent>("Camera");

		std::cout << "[EngineCore] Core components registered" << std::endl;
	}

	void EngineCore::RegisterCoreSystems() {
		entityManager.RegisterSystem<ImageSystem>();
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

		std::cout << "[EngineCore] Setting global data path: " << Utils::FilePaths::GetInstance().GetDataPath() << std::endl;
		pluginManager->SetGlobalDataPath(Utils::FilePaths::GetInstance().GetDataPath());

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

			std::cout << "[EngineCore] Initializing file paths..." << std::endl;
			Utils::FilePaths::GetInstance().Init();

			std::cout << "[EngineCore] File paths initialized:" << std::endl;
			const char* defaultProjectPath = Utils::FilePaths::GetInstance().GetPath("DefaultProject");
			std::cout << "[EngineCore]   defaultProjectPath: " << (defaultProjectPath ? defaultProjectPath : "(null)") << std::endl;

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