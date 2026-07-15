#define ANI_ENGINE_EXPORTS

#include "AniEngine.hpp"
#include "EngineContext.hpp"
#include "utils.h"
#include "Components.h"
#include "systems.h"
#include "FilePathSystem.hpp"
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
        entityManager.RegisterSystem<FilePathSystem>();

        std::cout << "[EngineCore] Core systems registered" << std::endl;
    }

    void EngineCore::InitializeCorePaths() {
        auto fileSys = context->entityManager->GetSystem<FilePathSystem>();
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
        if (!context || !context->pluginManager) {
            std::cerr << "[EngineCore] PluginManager not created in context!" << std::endl;
            return;
        }

        auto fileSys = context->entityManager->GetSystem<FilePathSystem>();
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
            context->pluginManager->SetGlobalDataPath(dataPath);
        }
        else {
            std::cerr << "[EngineCore] ERROR: Data path not available from FilePathSystem!" << std::endl;
        }

        context->pluginManager->scanPluginDirectory(pluginDirectory);
        context->pluginManager->LoadGlobalPluginState();

        std::cout << "[EngineCore] Plugin system initialized with selective loading" << std::endl;
    }

    bool EngineCore::Initialize() {
        if (initialized) {
            std::cerr << "[EngineCore] Already initialized!" << std::endl;
            return false;
        }

        try {
            std::cout << "[EngineCore] =========================================" << std::endl;
            std::cout << "[EngineCore] Initializing..." << std::endl;

            context = EngineContext::Create();

            if (!context->isValid()) {
                std::cerr << "[EngineCore] Failed to create valid EngineContext!" << std::endl;
                return false;
            }

            auto& entityManager = *context->entityManager;
            const ECS::EntityID temp = entityManager.AddNewEntity();
            entityManager.DestroyEntity(temp);

            RegisterCoreComponents();
            RegisterCoreSystems();

            InitializeCorePaths();

            std::cout << "[EngineCore] Engine context created successfully" << std::endl;

            InitializePlugins();

            initialized = true;
            running = true;

            std::cout << "[EngineCore] Initialized successfully" << std::endl;
            std::cout << "[EngineCore] EntityManager address: " << context->entityManager.get() << std::endl;
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
        engineCore->context = existingContext;

        engineCore->RegisterCoreComponents();
        engineCore->RegisterCoreSystems();
        engineCore->InitializeCorePaths();
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