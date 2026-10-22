#define ANI_ENGINE_EXPORTS

#include "AniEngine.hpp"
#include "EngineContext.hpp"
#include "AniEngineComponents.hpp"
#include "AniEngineSystems.hpp"
#include "SettingsSystem.hpp"
#include "FilePathComponent.hpp"
#include "ThreadPoolSystem.hpp"
#include "Log.hpp"
#include <filesystem>

using namespace ECS;

namespace ANI {

    struct EngineCore::Impl {
        bool initialized = false;
        bool running = false;
        std::shared_ptr<EngineContext> context;
    };

    EngineCore::EngineCore() : pImpl(std::make_unique<Impl>()) {
        ANI_LOG_INFO("EngineCore constructor");
    }

    EngineCore::~EngineCore() {
        if (pImpl->initialized) {
            Shutdown();
        }
    }

    void EngineCore::RegisterCoreComponents() {
        if (!pImpl->context || !pImpl->context->entityManager) {
            ANI_LOG_ERROR("Context or EntityManager not initialized!");
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
        entityManager.RegisterComponent<AudioComponent>("Audio");

        entityManager.RegisterComponent<ECS::TransformComponent>("Transform");
        entityManager.RegisterComponent<ECS::MeshComponent>("Mesh");
        entityManager.RegisterComponent<ECS::CameraComponent>("Camera");

        entityManager.RegisterComponent<ECS::FilePathComponent>("FilePathComponent");

        ANI_LOG_INFO("Core components registered");
    }

    void EngineCore::RegisterCoreSystems() {
        if (!pImpl->context || !pImpl->context->entityManager) {
            ANI_LOG_ERROR("Context or EntityManager not initialized!");
            return;
        }

        auto& entityManager = *pImpl->context->entityManager;

        entityManager.RegisterSystem<SettingsSystem>();
        entityManager.RegisterSystem<ThreadPoolSystem>();
        entityManager.RegisterSystem<ImageSystem>();
        entityManager.RegisterSystem<AudioSystem>();
        entityManager.RegisterSystem<VideoSystem>();

        auto settings = entityManager.GetSystem<SettingsSystem>();
        if (settings) {
            settings->AddSettingsComponent<FilePathComponent>();

            settings->SetApplyCallback(
                entityManager.GetComponentTypeIdByName("FilePathComponent"),
                [](BaseComponent&) {});
        }

        ANI_LOG_INFO("Core systems registered");
    }

    void EngineCore::RegisterPathDefaults() {
        auto settings = pImpl->context->entityManager->GetSystem<SettingsSystem>();
        if (!settings) {
            ANI_LOG_ERROR("SettingsSystem not available for path defaults!");
            return;
        }

        settings->RegisterPathDefault("DataPath", "./data");
        settings->RegisterPathDefault("DefaultProject", "./projects");
        settings->RegisterPathDefault("Plugins", "./plugins");
        settings->RegisterPathDefault("ImguiState", "./data/imgui.ini");
        settings->RegisterPathDefault("GlobalAssets", "./assets");
        settings->RegisterPathDefault("Docs", "./docs");
        settings->RegisterPathDefault("Scripts", "./scripts");
        settings->RegisterPathDefault("Templates", "./data/templates");
        settings->RegisterPathDefault("Shaders", "./shaders");
        settings->RegisterPathDefault("styles", "./data/styles");
    }

    void EngineCore::InitializeCorePaths() {
        auto settings = pImpl->context->entityManager->GetSystem<SettingsSystem>();
        if (!settings) {
            ANI_LOG_ERROR("SettingsSystem not available!");
            return;
        }

        settings->LoadAllSettings();

        ANI_LOG_INFO("Core paths resolved: DataPath=%s Plugins=%s",
            settings->GetPath("DataPath").c_str(),
            settings->GetPath("Plugins").c_str());
    }

    void EngineCore::InitializePlugins() {
        if (!pImpl->context || !pImpl->context->pluginManager) {
            ANI_LOG_ERROR("PluginManager not created in context!");
            return;
        }

        auto settings = pImpl->context->entityManager->GetSystem<SettingsSystem>();
        if (!settings) {
            ANI_LOG_ERROR("SettingsSystem not available for plugin initialization!");
            return;
        }

        std::string pluginDirectory = settings->GetPath("Plugins");
        if (pluginDirectory.empty()) {
            pluginDirectory = "./plugins";
            ANI_LOG_WARN("Plugins path not set, using default: %s", pluginDirectory.c_str());
        }

        ANI_LOG_INFO("Initializing plugins from: %s", pluginDirectory.c_str());

        if (!std::filesystem::exists(pluginDirectory)) {
            std::filesystem::create_directories(pluginDirectory);
            ANI_LOG_INFO("Created plugin directory: %s", pluginDirectory.c_str());
        }

        pImpl->context->pluginManager->scanPluginDirectory(pluginDirectory);

        ANI_LOG_INFO("Plugin system initialized (plugins load per-project)");
    }

    void EngineCore::OpenSessionLog() {
        std::string logDir = "./data/session_logs";

        if (pImpl->context && pImpl->context->entityManager) {
            auto settings = pImpl->context->entityManager->GetSystem<SettingsSystem>();
            if (settings) {
                std::string dataPath = settings->GetPath("DataPath");
                if (!dataPath.empty()) logDir = dataPath + "/session_logs";
            }
        }

        if (ANI::Log::SessionOpen(logDir.c_str())) {
            ANI_LOG_INFO("Session log opened: %s", ANI::Log::SessionPath());
        }
        else {
            ANI_LOG_WARN("Failed to open session log directory: %s", logDir.c_str());
        }
    }

    bool EngineCore::Initialize() {
        if (pImpl->initialized) {
            ANI_LOG_WARN("EngineCore already initialized!");
            return false;
        }

        try {
            ANI_LOG_INFO("=========================================");
            ANI_LOG_INFO("Initializing EngineCore...");

            pImpl->context = EngineContext::Create();

            if (!pImpl->context->isValid()) {
                ANI_LOG_ERROR("Failed to create valid EngineContext!");
                return false;
            }

            auto& entityManager = *pImpl->context->entityManager;
            const ECS::EntityID temp = entityManager.AddNewEntity();
            entityManager.DestroyEntity(temp);

            RegisterCoreComponents();
            RegisterCoreSystems();
            RegisterPathDefaults();
            InitializeCorePaths();

            OpenSessionLog();

            ANI_LOG_INFO("Engine context created successfully");

            InitializePlugins();

            pImpl->initialized = true;
            pImpl->running = true;

            ANI_LOG_INFO("EngineCore initialized successfully");
            ANI_LOG_INFO("EntityManager address: %p",
                static_cast<void*>(pImpl->context->entityManager.get()));
            ANI_LOG_INFO("=========================================");
            return true;
        }
        catch (const std::exception& e) {
            ANI_LOG_ERROR("Initialization failed: %s", e.what());
            return false;
        }
    }

    std::unique_ptr<EngineCore> EngineCore::CreateWithContext(std::shared_ptr<EngineContext> existingContext) {
        if (!existingContext || !existingContext->isValid()) {
            ANI_LOG_ERROR("Invalid context provided to CreateWithContext!");
            return nullptr;
        }

        auto engineCore = std::make_unique<EngineCore>();
        engineCore->pImpl->context = existingContext;

        engineCore->RegisterCoreComponents();
        engineCore->RegisterCoreSystems();
        engineCore->RegisterPathDefaults();
        engineCore->InitializeCorePaths();

        engineCore->OpenSessionLog();
        engineCore->InitializePlugins();

        engineCore->pImpl->initialized = true;
        engineCore->pImpl->running = true;

        ANI_LOG_INFO("EngineCore created with existing context successfully");
        return engineCore;
    }

    void EngineCore::Shutdown() {
        if (!pImpl->initialized || !pImpl->context) return;

        ANI_LOG_INFO("Shutting down EngineCore...");

        pImpl->running = false;

        if (auto settings = pImpl->context->entityManager->GetSystem<SettingsSystem>()) {
            settings->SaveAllSettings();
        }

        if (pImpl->context->entityManager) {
            pImpl->context->entityManager->Reset();
        }

        pImpl->context.reset();
        pImpl->initialized = false;

        ANI_LOG_INFO("EngineCore shutdown complete");

        ANI::Log::SessionClose();
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