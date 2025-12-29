#define ANI_ENGINE_EXPORTS

#include "AniEngine.hpp"
#include "EngineContext.hpp"
#include "utils.h"
#include "components.h"
#include "systems.h"
#include "FilePathService.hpp"  // Add this include
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

	void EngineCore::InitializeFilePathService() {
		if (!context || !context->filePaths) {
			std::cerr << "[EngineCore] Cannot initialize FilePathService: FilePaths not available!" << std::endl;
			return;
		}

		std::cout << "[EngineCore] Initializing FilePathService..." << std::endl;

		// Initialize the service with our FilePaths instance
		Utils::FilePathService::Initialize(context->filePaths);

		if (Utils::FilePathService::IsInitialized()) {
			std::cout << "[EngineCore] FilePathService initialized successfully" << std::endl;

			// Debug: Print some key paths to verify
			std::cout << "[EngineCore] DefaultProject path: "
				<< Utils::FilePathService::GetPath("DefaultProject") << std::endl;
			std::cout << "[EngineCore] OutputFolder path: "
				<< Utils::FilePathService::GetPath("OutputFolder") << std::endl;
			std::cout << "[EngineCore] ModelRoot path: "
				<< Utils::FilePathService::GetPath("ModelRoot") << std::endl;
		}
		else {
			std::cerr << "[EngineCore] Failed to initialize FilePathService!" << std::endl;
		}
	}

	void EngineCore::InitializePlugins() {
		if (!context || !context->pluginManager) {
			std::cerr << "[EngineCore] PluginManager not created in context!" << std::endl;
			return;
		}

		std::cout << "[EngineCore] Initializing plugins from: " << context->pluginDirectory << std::endl;

		if (!std::filesystem::exists(context->pluginDirectory)) {
			std::filesystem::create_directories(context->pluginDirectory);
			std::cout << "[EngineCore] Created plugin directory: " << context->pluginDirectory << std::endl;
		}

		if (context->filePaths) {
			std::cout << "[EngineCore] Setting global data path: " << context->filePaths->GetDataPath() << std::endl;
			context->pluginManager->SetGlobalDataPath(context->filePaths->GetDataPath());
		}
		else {
			std::cerr << "[EngineCore] FilePaths not available in context!" << std::endl;
		}

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

			context = EngineContext::Create();

			if (!context->isValid()) {
				std::cerr << "[EngineCore] Failed to create valid EngineContext!" << std::endl;
				return false;
			}

			std::cout << "[EngineCore] File paths initialized:" << std::endl;
			const char* defaultProjectPath = context->filePaths->GetPath("DefaultProject");
			std::cout << "[EngineCore]   defaultProjectPath: " << (defaultProjectPath ? defaultProjectPath : "(null)") << std::endl;

			auto& entityManager = *context->entityManager;
			const ECS::EntityID temp = entityManager.AddNewEntity();
			entityManager.DestroyEntity(temp);

			// Initialize FilePathService BEFORE registering components
			InitializeFilePathService();

			// Now register components (they will use FilePathService)
			RegisterCoreComponents();
			RegisterCoreSystems();

			std::cout << "[EngineCore] Engine context created successfully" << std::endl;

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

		// Initialize FilePathService BEFORE registering components
		engineCore->InitializeFilePathService();

		engineCore->RegisterCoreComponents();
		engineCore->RegisterCoreSystems();

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

		if (context->pluginManager) {
			std::cout << "[EngineCore] Saving global plugin state..." << std::endl;
			context->pluginManager->SaveGlobalPluginState();
		}

		// Shutdown FilePathService
		std::cout << "[EngineCore] Shutting down FilePathService..." << std::endl;
		Utils::FilePathService::Shutdown();

		if (context->entityManager) {
			context->entityManager->Reset();
		}

		context.reset();

		initialized = false;

		std::cout << "[EngineCore] Shutdown complete" << std::endl;
	}

	void EngineCore::Update(float deltaTime) {
		if (!initialized || !context) return;

		context->entityManager->Update(deltaTime);

		if (context->pluginManager) {
			context->pluginManager->checkForChanges();
			context->pluginManager->updatePlugins(deltaTime);
		}
	}

} // namespace ANI