#define ANI_ENGINE_EXPORTS

#include "AniEngine.hpp"
#include "EngineContext.hpp"
#include "utils.h"
#include "Components.h"
#include "systems.h"
#include "FilePathSystem.hpp"
#include "ThreadPoolSystem.hpp"
#include <iostream>
#include <filesystem>

using namespace ECS;

namespace ANI {

	struct EngineCore::Impl {
		bool initialized = false;
		bool running = false;
		std::shared_ptr<EngineContext> context;
	};

	EngineCore::EngineCore() : pImpl(std::make_unique<Impl>()) {
		std::cout << "[EngineCore] Constructor called" << std::endl;
	}

	EngineCore::~EngineCore() {
		if (pImpl->initialized) {
			Shutdown();
		}
	}

	void EngineCore::RegisterCoreComponents() {
		if (!pImpl->context || !pImpl->context->entityManager) {
			std::cerr << "[EngineCore] Context or EntityManager not initialized!" << std::endl;
			return;
		}

		auto& entityManager = *pImpl->context->entityManager;

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
		if (!pImpl->context || !pImpl->context->entityManager) {
			std::cerr << "[EngineCore] Context or EntityManager not initialized!" << std::endl;
			return;
		}

		auto& entityManager = *pImpl->context->entityManager;
		
		entityManager.RegisterSystem<FilePathSystem>();
		entityManager.RegisterSystem<ImageSystem>();
		entityManager.RegisterSystem<VideoSystem>();
		entityManager.RegisterSystem<PythonSystem>();
		entityManager.RegisterSystem<ThreadPoolSystem>();

		std::cout << "[EngineCore] Core systems registered" << std::endl;
	}

	void EngineCore::InitializeCorePaths() {
		auto fileSys = pImpl->context->entityManager->GetSystem<FilePathSystem>();
		if (!fileSys) {
			std::cerr << "[EngineCore] FilePathSystem not available!" << std::endl;
			return;
		}

		std::filesystem::path base = std::filesystem::current_path();

		std::string dataPath = (base / "data" / "defaults").string();
		std::string assetsPath = (base / "assets").string();

		fileSys->SetPath("DataPath", dataPath);
		fileSys->SetPath("AssetsFolder", assetsPath);

		std::error_code ec;
		std::filesystem::create_directories(dataPath, ec);
		std::filesystem::create_directories(assetsPath, ec);

		std::cout << "[EngineCore] Core paths initialized (DataPath, AssetsFolder)" << std::endl;
	}

	void EngineCore::InitializePlugins() {
		if (!pImpl->context || !pImpl->context->pluginManager) {
			std::cerr << "[EngineCore] PluginManager not created in context!" << std::endl;
			return;
		}

		auto fileSys = pImpl->context->entityManager->GetSystem<FilePathSystem>();
		if (!fileSys) {
			std::cerr << "[EngineCore] FilePathSystem not available for plugin initialization!" << std::endl;
			return;
		}

		std::string pluginDirectory = fileSys->GetPath("Plugins");
		if (pluginDirectory.empty()) {
			pluginDirectory = "./plugins";
			std::cerr << "[EngineCore] WARNING: Plugins path not set, using default: " << pluginDirectory << std::endl;
		}

		std::cout << "[EngineCore] Initializing plugins from: " << pluginDirectory << std::endl;

		if (!std::filesystem::exists(pluginDirectory)) {
			std::filesystem::create_directories(pluginDirectory);
			std::cout << "[EngineCore] Created plugin directory: " << pluginDirectory << std::endl;
		}

		std::string dataPath = fileSys->GetPath("DataPath");
		if (!dataPath.empty()) {
			std::cout << "[EngineCore] Setting global data path: " << dataPath << std::endl;
			pImpl->context->pluginManager->SetGlobalDataPath(dataPath);
		}
		else {
			std::cerr << "[EngineCore] ERROR: Data path not available from FilePathSystem!" << std::endl;
		}

		pImpl->context->pluginManager->scanPluginDirectory(pluginDirectory);
		pImpl->context->pluginManager->LoadGlobalPluginState();

		std::cout << "[EngineCore] Plugin system initialized with selective loading" << std::endl;
	}

	bool EngineCore::Initialize() {
		if (pImpl->initialized) {
			std::cerr << "[EngineCore] Already initialized!" << std::endl;
			return false;
		}

		try {
			std::cout << "[EngineCore] =========================================" << std::endl;
			std::cout << "[EngineCore] Initializing..." << std::endl;

			pImpl->context = EngineContext::Create();

			if (!pImpl->context->isValid()) {
				std::cerr << "[EngineCore] Failed to create valid EngineContext!" << std::endl;
				return false;
			}

			auto& entityManager = *pImpl->context->entityManager;
			const ECS::EntityID temp = entityManager.AddNewEntity();
			entityManager.DestroyEntity(temp);

			RegisterCoreComponents();
			RegisterCoreSystems();

			InitializeCorePaths();

			std::cout << "[EngineCore] Engine context created successfully" << std::endl;

			InitializePlugins();

			pImpl->initialized = true;
			pImpl->running = true;

			std::cout << "[EngineCore] Initialized successfully" << std::endl;
			std::cout << "[EngineCore] EntityManager address: " << pImpl->context->entityManager.get() << std::endl;
			std::cout << "[EngineCore] =========================================" << std::endl;
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
		engineCore->pImpl->context = existingContext;

		engineCore->RegisterCoreComponents();
		engineCore->RegisterCoreSystems();
		engineCore->InitializeCorePaths();
		engineCore->InitializePlugins();

		engineCore->pImpl->initialized = true;
		engineCore->pImpl->running = true;

		std::cout << "[EngineCore] Created with existing context successfully" << std::endl;
		return engineCore;
	}

	void EngineCore::Shutdown() {
		if (!pImpl->initialized || !pImpl->context) return;

		std::cout << "[EngineCore] Shutting down..." << std::endl;

		pImpl->running = false;

		if (pImpl->context->pluginManager) {
			std::cout << "[EngineCore] Saving global plugin state..." << std::endl;
			pImpl->context->pluginManager->SaveGlobalPluginState();
		}

		if (pImpl->context->entityManager) {
			pImpl->context->entityManager->Reset();
		}

		pImpl->context.reset();

		pImpl->initialized = false;

		std::cout << "[EngineCore] Shutdown complete" << std::endl;
	}

	void EngineCore::Update(float deltaTime) {
		if (!pImpl->initialized || !pImpl->context) return;

		pImpl->context->entityManager->Update(deltaTime);

		if (pImpl->context->pluginManager) {
			pImpl->context->pluginManager->checkForChanges();
			pImpl->context->pluginManager->updatePlugins(deltaTime);
		}
	}

	ECS::EntityManager& EngineCore::GetEntityManager() {
		if (!pImpl->context || !pImpl->context->entityManager) {
			throw std::runtime_error("EngineContext or EntityManager not initialized");
		}
		return *pImpl->context->entityManager;
	}

	Plugins::PluginManager* EngineCore::GetPluginManager() {
		return pImpl->context ? pImpl->context->pluginManager.get() : nullptr;
	}

	std::shared_ptr<EngineContext> EngineCore::GetEngineContext() const {
		return pImpl->context;
	}

	bool EngineCore::IsRunning() const {
		return pImpl->running;
	}

	void EngineCore::SetRunning(bool isRunning) {
		pImpl->running = isRunning;
	}

	bool EngineCore::IsInitialized() const {
		return pImpl->initialized;
	}

	void EngineCore::SetPluginDirectory(const std::string& directory) {
		if (pImpl->context) {
			pImpl->context->pluginDirectory = directory;
		}
	}
}