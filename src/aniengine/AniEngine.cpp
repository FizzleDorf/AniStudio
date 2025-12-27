#define ANI_ENGINE_EXPORTS

#include "AniEngine.hpp"
#include "EngineContext.hpp"
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
	}

	EngineCore::~EngineCore() {
		if (initialized) {
			Shutdown();
		}
	}

	void EngineCore::RegisterCoreComponents() {
		if (!context || !context->entityManager) {
			std::cerr << "[EngineCore] Context or EntityManager not initialized!" << std::endl;
			return;
		}

		auto& entityManager = *context->entityManager;

		entityManager.RegisterComponent<ImageComponent>("Image");
		entityManager.RegisterComponent<InputImageComponent>("InputImage");
		entityManager.RegisterComponent<OutputImageComponent>("OutputImage");
		entityManager.RegisterComponent<MaskImageComponent>("MaskImage");

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
		if (!context || !context->entityManager) {
			std::cerr << "[EngineCore] Context or EntityManager not initialized!" << std::endl;
			return;
		}

		auto& entityManager = *context->entityManager;

		entityManager.RegisterSystem<ImageSystem>();
		entityManager.RegisterSystem<VideoSystem>();
		entityManager.RegisterSystem<PythonSystem>();

		std::cout << "[EngineCore] Core systems registered" << std::endl;
	}

	void EngineCore::InitializePlugins() {
		if (!context || !context->pluginManager) {
			std::cerr << "[EngineCore] PluginManager not created in context!" << std::endl;
			return;
		}

		std::cout << "[EngineCore] Initializing plugins from: " << context->pluginDirectory << std::endl;

		// Create plugin directory if it doesn't exist
		if (!std::filesystem::exists(context->pluginDirectory)) {
			std::filesystem::create_directories(context->pluginDirectory);
			std::cout << "[EngineCore] Created plugin directory: " << context->pluginDirectory << std::endl;
		}

		// Use FilePaths from context
		if (context->filePaths) {
			std::cout << "[EngineCore] Setting global data path: " << context->filePaths->GetDataPath() << std::endl;
			context->pluginManager->SetGlobalDataPath(context->filePaths->GetDataPath());
		}
		else {
			std::cerr << "[EngineCore] FilePaths not available in context!" << std::endl;
		}

		// Scan plugins directory (but don't auto-load everything)
		context->pluginManager->scanPluginDirectory(context->pluginDirectory);

		std::cout << "[EngineCore] Loading global plugin state..." << std::endl;
		context->pluginManager->LoadGlobalPluginState();

		std::cout << "[EngineCore] Plugin system initialized with selective loading" << std::endl;
	}

	bool EngineCore::Initialize() {
		if (initialized) {
			std::cerr << "[EngineCore] Already initialized!" << std::endl;
			return false;
		}

		try {
			std::cout << "[EngineCore] Initializing..." << std::endl;

			// Create EngineContext (which initializes FilePaths)
			context = EngineContext::Create();

			if (!context->isValid()) {
				std::cerr << "[EngineCore] Failed to create valid EngineContext!" << std::endl;
				return false;
			}

			std::cout << "[EngineCore] File paths initialized:" << std::endl;
			const char* defaultProjectPath = context->filePaths->GetPath("DefaultProject");
			std::cout << "[EngineCore]   defaultProjectPath: " << (defaultProjectPath ? defaultProjectPath : "(null)") << std::endl;

			// Invalidate ID 0 for consistency
			auto& entityManager = *context->entityManager;
			const ECS::EntityID temp = entityManager.AddNewEntity();
			entityManager.DestroyEntity(temp);

			// Register components and systems
			RegisterCoreComponents();
			RegisterCoreSystems();

			std::cout << "[EngineCore] Engine context created successfully" << std::endl;

			// Initialize plugins
			InitializePlugins();

			initialized = true;
			running = true;

			std::cout << "[EngineCore] Initialized successfully" << std::endl;
			std::cout << "[EngineCore] EntityManager address: " << context->entityManager.get() << std::endl;
			return true;
		}
		catch (const std::exception& e) {
			std::cerr << "[EngineCore] Initialization failed: " << e.what() << std::endl;
			return false;
		}
	}

	std::unique_ptr<EngineCore> EngineCore::CreateWithContext(std::shared_ptr<EngineContext> existingContext) {
		if (!existingContext || !existingContext->isValid()) {
			std::cerr << "[EngineCore] Invalid context provided to CreateWithContext!" << std::endl;
			return nullptr;
		}

		auto engineCore = std::make_unique<EngineCore>();
		engineCore->context = existingContext;

		// Register components and systems
		engineCore->RegisterCoreComponents();
		engineCore->RegisterCoreSystems();

		// Initialize plugins
		engineCore->InitializePlugins();

		engineCore->initialized = true;
		engineCore->running = true;

		std::cout << "[EngineCore] Created with existing context successfully" << std::endl;
		return engineCore;
	}

	void EngineCore::Shutdown() {
		if (!initialized || !context) return;

		std::cout << "[EngineCore] Shutting down..." << std::endl;

		running = false;

		// Save global plugin state before shutdown
		if (context->pluginManager) {
			std::cout << "[EngineCore] Saving global plugin state..." << std::endl;
			context->pluginManager->SaveGlobalPluginState();
		}

		// Reset entity manager
		if (context->entityManager) {
			context->entityManager->Reset();
		}

		// Clear the context
		context.reset();

		initialized = false;

		std::cout << "[EngineCore] Shutdown complete" << std::endl;
	}

	void EngineCore::Update(float deltaTime) {
		if (!initialized || !context) return;

		context->entityManager->Update(deltaTime);

		// Update plugins
		if (context->pluginManager) {
			context->pluginManager->checkForChanges(); // Hot reload check
			context->pluginManager->updatePlugins(deltaTime);
		}
	}

} // namespace ANI