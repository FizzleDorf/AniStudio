#include "BasePlugin.hpp"
#include "EntityManager.hpp"
#include "ViewManager.hpp"
#include "Events.hpp"
#include "DiffusionCallbackUtils.hpp"
#include "FilePathSystem.hpp"
#include "FilePathComponent.hpp"
#include "EngineContext.hpp"
#include "StudioContext.hpp"
#include "ProjectManager.hpp"
#include "WindowState.hpp"
#include <thread>
#include <chrono>
#include "SDCPPComponents.h"
#include "SDcppSystem.hpp"
#include "ModelCacheSystem.hpp"
#include "ADetailerComponent.hpp"

#include "DiffusionView.hpp"
#include "ConvertView.hpp"
#include "UpscaleView.hpp"
#include "VideoDiffusionView.hpp"
#include "ModelCacheView.hpp"

#include <iostream>
#include <filesystem>
#include "MissingPathsPopup.hpp"

namespace Utils {
    ECS::FilePathSystem* g_FilePathSystem = nullptr;
}

class DiffusionAddon : public Plugins::BasePlugin {
public:
    DiffusionAddon() : BasePlugin("DiffusionAddon", "1.0.0"), m_imguiContext(nullptr) {}

    bool OnEngineInit(ECS::EntityManager& entityMgr) override {
        LogInfo("Initializing Stable Diffusion Addon...");

        auto context = GetEngineContext();

        if (!context) {
            auto studioCtx = GetStudioContext();
            if (studioCtx) {
                context = std::static_pointer_cast<ANI::EngineContext>(studioCtx);
                LogInfo("Got EngineContext from StudioContext fallback");
            }
        }

        if (!context) {
            LogError("Engine context is null! Plugin cannot initialize.");
            return false;
        }

        LogInfo("Engine context obtained successfully");

        auto fs = entityMgr.GetSystem<ECS::FilePathSystem>();
        if (!fs) {
            LogError("FilePathSystem not found! Cannot initialize.");
            return false;
        }
        Utils::g_FilePathSystem = fs.get();
        LogInfo("FilePathSystem obtained.");

        if (!fs->HasPath("ModelRoot")) {
            fs->SetPath("ModelRoot", "");
        }
        std::string dataPath = fs->GetPath("DataPath");
        std::string defaultModelRoot;
        if (!dataPath.empty()) {
            defaultModelRoot = (std::filesystem::path(dataPath) / "models").string();
        }
        else {
            defaultModelRoot = (std::filesystem::current_path() / "models").string();
        }
        Utils::SetDefaultPath("ModelRoot", defaultModelRoot);

        const std::vector<std::pair<std::string, std::string>> modelKeys = {
            {"Checkpoint", "checkpoints"},
            {"Unet", "unet"},
            {"Vae", "vae"},
            {"Taesd", "taesd"},
            {"Lora", "lora"},
            {"Embed", "embed"},
            {"Encoder", "encoder"},
            {"ControlNet", "controlnet"}
        };

        for (const auto& [key, subdir] : modelKeys) {
            if (!fs->HasPath(key)) {
                fs->SetPath(key, "");
            }
            std::string defaultPath = (std::filesystem::path(defaultModelRoot) / subdir).string();
            Utils::SetDefaultPath(key, defaultPath);
        }

        if (!dataPath.empty()) {
            std::string filePath = (std::filesystem::path(dataPath) / "filepaths.json").string();
            if (std::filesystem::exists(filePath))
                fs->LoadFromFile(filePath);
        }

        Utils::CheckMissingPaths(fs);

        // Sampling & Inference
        entityMgr.RegisterComponent<ECS::PromptComponent>("Prompt");
        entityMgr.RegisterComponent<ECS::SamplerComponent>("Sampler");
        entityMgr.RegisterComponent<ECS::GuidanceComponent>("Guidance");
        entityMgr.RegisterComponent<ECS::LatentComponent>("Latent");

        // Model components
        entityMgr.RegisterComponent<ECS::CheckpointComponent>("Checkpoint");
        entityMgr.RegisterComponent<ECS::DiffusionModelComponent>("DiffusionModel");
        entityMgr.RegisterComponent<ECS::HighNoiseDiffusionModelComponent>("HighNoiseDiffusionModel");
        entityMgr.RegisterComponent<ECS::UncondDiffusionModelComponent>("UncondDiffusionModel");
        entityMgr.RegisterComponent<ECS::AudioVAEComponent>("AudioVAE");
        entityMgr.RegisterComponent<ECS::EmbeddingComponent>("Embedding");          // connectors
        entityMgr.RegisterComponent<ECS::MotionModuleComponent>("MotionModule");

        // Encoders
        entityMgr.RegisterComponent<ECS::ClipLComponent>("ClipL");
        entityMgr.RegisterComponent<ECS::ClipGComponent>("ClipG");
        entityMgr.RegisterComponent<ECS::T5XXLComponent>("T5XXL");
        entityMgr.RegisterComponent<ECS::LlmEncoderComponent>("LlmEncoder");

        // Vision encoders
        entityMgr.RegisterComponent<ECS::ClipVisionComponent>("ClipVision");
        entityMgr.RegisterComponent<ECS::LlmVisionComponent>("LlmVision");

        // VAE / TAESD
        entityMgr.RegisterComponent<ECS::VaeComponent>("Vae");
        entityMgr.RegisterComponent<ECS::TaesdComponent>("Taesd");

        // Upscaling
        entityMgr.RegisterComponent<ECS::HiresComponent>("Hires");
        entityMgr.RegisterComponent<ECS::EsrganComponent>("Esrgan");

        // Other models
        entityMgr.RegisterComponent<ECS::PhotoMakerComponent>("PhotoMaker");
        entityMgr.RegisterComponent<ECS::PulidWeightsComponent>("PulidWeights");
        entityMgr.RegisterComponent<ECS::ControlNetComponent>("ControlNet");
        entityMgr.RegisterComponent<ECS::LoraComponent>("Lora");

        // Video
        entityMgr.RegisterComponent<ECS::VideoParamsComponent>("VideoParams");
        entityMgr.RegisterComponent<ECS::HighNoiseSamplerComponent>("HighNoiseSampler");

        // Advanced / extra
        entityMgr.RegisterComponent<ECS::ADetailerComponent>("ADetailer");
        entityMgr.RegisterComponent<ECS::ChromaComponent>("Chroma");
        entityMgr.RegisterComponent<ECS::StackedIdEmbedComponent>("StackedIdEmbed");
        entityMgr.RegisterComponent<ECS::EasyCacheComponent>("EasyCache");

        // Conversion
        entityMgr.RegisterComponent<ECS::ConversionComponent>("Conversion");

        // Embeddings (textual inversion)
        entityMgr.RegisterComponent<ECS::EmbeddingsComponent>("Embeddings");

        // Image components (input/output)
        entityMgr.RegisterComponent<ECS::ControlNetImageComponent>("ControlNetImage");
        entityMgr.RegisterComponent<ECS::PhotoMakerImageComponent>("PhotoMakerImage");
        entityMgr.RegisterComponent<ECS::RefImagesComponent>("RefImages");
        entityMgr.RegisterComponent<ECS::ControlFramesComponent>("ControlFrames");

        // Global settings
        entityMgr.RegisterComponent<ECS::SDCPPSettingsComponent>("SDCPPSettings");

        entityMgr.RegisterSystem<ECS::ModelCacheSystem>();
        entityMgr.RegisterSystem<ECS::SDCPPSystem>();

        m_entityMgr = &entityMgr;

        auto settingsSys = entityMgr.GetSystem<ECS::SettingsSystem>();
        if (settingsSys) {
            EntityID settingsEntity = settingsSys->GetSettingsEntity();
            if (entityMgr.IsEntityValid(settingsEntity) && !entityMgr.HasComponent<ECS::SDCPPSettingsComponent>(settingsEntity)) {
                entityMgr.AddComponent<ECS::SDCPPSettingsComponent>(settingsEntity);
                LogInfo("Added SDCPPSettingsComponent to global settings entity");
            }
        }
        else {
            LogError("SettingsSystem not found, SDCPP global settings will not be available");
        }

        RegisterEventHandlers();

        LogInfo("SDCPPSystem registered");
        LogInfo("Stable Diffusion Addon initialized");
        return true;
    }

    bool OnStudioInit(ECS::EntityManager& entityMgr, GUI::ViewManager& viewMgr) override {
        LogInfo("Registering Stable Diffusion views...");

        if (!m_imguiContext) {
            LogError("ImGui context not set! Make sure SetImGuiContext was called.");
            return false;
        }

        ImGui::SetCurrentContext(m_imguiContext);
        LogInfo("ImGui context set for DiffusionAddon");

        std::cout << "[DiffusionAddon] Using stored ImGui context: " << m_imguiContext << std::endl;

        m_viewMgr = &viewMgr;

        viewMgr.RegisterView<GUI::DiffusionView>("DiffusionView", "Diffusion");
        viewMgr.RegisterView<GUI::ConvertView>("ConvertView", "Diffusion");
        viewMgr.RegisterView<GUI::UpscaleView>("UpscaleView", "Diffusion");
        viewMgr.RegisterView<GUI::VideoDiffusionView>("VideoDiffusionView", "Diffusion");
        viewMgr.RegisterView<GUI::ModelCacheView>("ModelCacheView", "Diffusion");

        LogInfo("Views registered via direct ViewManager");
        return true;
    }

    void SetImGuiContext(ImGuiContext* context) override {
        m_imguiContext = context;
        LogInfo("ImGui context set for DiffusionAddon: " + std::to_string(reinterpret_cast<uintptr_t>(m_imguiContext)));
    }

    bool HasValidImGuiContext() const override {
        return m_imguiContext != nullptr;
    }

    void OnShutdown() override {
        LogInfo("SHUTTING DOWN DIFFUSION ADDON");

        auto fs = m_entityMgr ? m_entityMgr->GetSystem<ECS::FilePathSystem>() : nullptr;
        if (fs) {
            std::string configPath = fs->GetPath("DataPath");
            if (!configPath.empty()) {
                std::string filePath = (std::filesystem::path(configPath) / "filepaths.json").string();
                fs->SaveToFile(filePath);
            }
        }

        UnregisterEventHandlers();

        if (m_entityMgr) {
            auto system = m_entityMgr->GetSystem<ECS::SDCPPSystem>();
            if (system) {
                system->TerminateImmediately();
                LogInfo("SDCPPSystem terminated");

                std::this_thread::sleep_for(std::chrono::milliseconds(200));

                try {
                    m_entityMgr->UnregisterSystem<ECS::SDCPPSystem>();
                    LogInfo("Unregistered SDCPPSystem");
                }
                catch (const std::exception& e) {
                    LogError("Error unregistering SDCPPSystem: " + std::string(e.what()));
                }
            }
        }

        if (m_viewMgr) {
            // Close all views but do NOT unregister them to avoid deadlock
            const char* viewNames[] = {
                "DiffusionView", "ConvertView", "UpscaleView", "VideoDiffusionView", "ModelCacheView"
            };

            for (const char* viewName : viewNames) {
                try {
                    m_viewMgr->CloseAllViewsOfType(viewName);
                    LogInfo("Closed all views of type: " + std::string(viewName));
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                }
                catch (...) {
                }
            }
        }

        if (m_entityMgr) {
            auto allEntities = m_entityMgr->GetAllEntities();
            std::vector<EntityID> entitiesToDestroy;

            for (EntityID entity : allEntities) {
                if (CheckEntityForDiffusionComponents(entity)) {
                    entitiesToDestroy.push_back(entity);
                }
            }

            LogInfo("Found " + std::to_string(entitiesToDestroy.size()) +
                " entities to destroy out of " + std::to_string(allEntities.size()));

            for (auto it = entitiesToDestroy.rbegin(); it != entitiesToDestroy.rend(); ++it) {
                try {
                    m_entityMgr->DestroyEntity(*it);
                    LogInfo("Destroyed entity: " + std::to_string(*it));
                }
                catch (const std::exception& e) {
                    LogError("Error destroying entity " + std::to_string(*it) + ": " + std::string(e.what()));
                }
            }
        }

        Utils::g_FilePathSystem = nullptr;
        m_entityMgr = nullptr;
        m_viewMgr = nullptr;
        m_imguiContext = nullptr;

        LogInfo("DiffusionAddon shutdown complete");
    }

    bool CheckEntityForDiffusionComponents(EntityID entity) {
        if (!m_entityMgr) return false;

        try {
            if (m_entityMgr->HasComponent<ECS::PromptComponent>(entity)) return true;
            if (m_entityMgr->HasComponent<ECS::CheckpointComponent>(entity)) return true;
            if (m_entityMgr->HasComponent<ECS::DiffusionModelComponent>(entity)) return true;
            if (m_entityMgr->HasComponent<ECS::LatentComponent>(entity)) return true;
            return false;
        }
        catch (...) {
            return false;
        }
    }

    void OnUpdate(float deltaTime) override {}

private:
    std::vector<std::string> m_registeredEvents;

    void RegisterEventHandlers() {
        auto& events = ANI::Events::Ref();

        m_registeredEvents = {
            "QueueDiffusionTask",
            "RemoveFromDiffusionQueue",
            "MoveInDiffusionQueue",
            "StopCurrentDiffusionTask",
            "CancelCurrentDiffusionTask",
            "ClearDiffusionQueue",
            "ClearAllDiffusionTasks",
            "PauseDiffusionWorker",
            "ResumeDiffusionWorker"
        };

        events.RegisterEventWithData("QueueDiffusionTask",
            [this](const std::any& data) {
                this->OnQueueDiffusionTask(data);
            });

        events.RegisterEventWithData("RemoveFromDiffusionQueue",
            [this](const std::any& data) {
                this->OnRemoveFromDiffusionQueue(data);
            });

        events.RegisterEventWithData("MoveInDiffusionQueue",
            [this](const std::any& data) {
                this->OnMoveInDiffusionQueue(data);
            });

        events.RegisterEvent("StopCurrentDiffusionTask",
            [this]() {
                this->OnStopCurrentDiffusionTask();
            });

        events.RegisterEvent("CancelCurrentDiffusionTask",
            [this]() {
                this->OnCancelCurrentDiffusionTask();
            });

        events.RegisterEvent("ClearDiffusionQueue",
            [this]() {
                this->OnClearDiffusionQueue();
            });

        events.RegisterEvent("ClearAllDiffusionTasks",
            [this]() {
                this->OnClearAllDiffusionTasks();
            });

        events.RegisterEvent("PauseDiffusionWorker",
            [this]() {
                this->OnPauseDiffusionWorker();
            });

        events.RegisterEvent("ResumeDiffusionWorker",
            [this]() {
                this->OnResumeDiffusionWorker();
            });

        LogInfo("Diffusion event handlers registered");
    }

    void UnregisterEventHandlers() {
        LogInfo("Unregistering diffusion event handlers");
        auto& events = ANI::Events::Ref();

        for (const auto& eventName : m_registeredEvents) {
            try {
                events.UnregisterEvent(eventName);
                LogInfo("Unregistered event: " + eventName);
            }
            catch (const std::exception& e) {
                LogError("Failed to unregister event " + eventName + ": " + e.what());
            }
        }
        m_registeredEvents.clear();
    }

    void OnQueueDiffusionTask(const std::any& data) {
        try {
            auto taskData = std::any_cast<std::pair<ECS::EntityID, ECS::SDCPPSystem::TaskType>>(data);
            auto entityID = taskData.first;
            auto taskType = taskData.second;

            auto system = m_entityMgr->GetSystem<ECS::SDCPPSystem>();
            if (system) {
                system->QueueTask(entityID, taskType);
                LogInfo("Queued diffusion task for entity " + std::to_string(entityID));
            }
            else {
                LogError("SDCPPSystem not found when queuing task");
            }
        }
        catch (const std::bad_any_cast& e) {
            LogError("Invalid data type for QueueDiffusionTask event: " + std::string(e.what()));
        }
    }

    void OnRemoveFromDiffusionQueue(const std::any& data) {
        try {
            auto index = std::any_cast<size_t>(data);

            auto system = m_entityMgr->GetSystem<ECS::SDCPPSystem>();
            if (system) {
                system->RemoveFromQueue(index);
                LogInfo("Removed item from diffusion queue at index " + std::to_string(index));
            }
            else {
                LogError("SDCPPSystem not found when removing from queue");
            }
        }
        catch (const std::bad_any_cast& e) {
            LogError("Invalid data type for RemoveFromDiffusionQueue event: " + std::string(e.what()));
        }
    }

    void OnMoveInDiffusionQueue(const std::any& data) {
        try {
            auto indices = std::any_cast<std::pair<size_t, size_t>>(data);
            auto fromIndex = indices.first;
            auto toIndex = indices.second;

            auto system = m_entityMgr->GetSystem<ECS::SDCPPSystem>();
            if (system) {
                system->MoveInQueue(fromIndex, toIndex);
                LogInfo("Moved item in diffusion queue from " + std::to_string(fromIndex) + " to " + std::to_string(toIndex));
            }
            else {
                LogError("SDCPPSystem not found when moving in queue");
            }
        }
        catch (const std::bad_any_cast& e) {
            LogError("Invalid data type for MoveInDiffusionQueue event: " + std::string(e.what()));
        }
    }

    void OnStopCurrentDiffusionTask() {
        auto system = m_entityMgr->GetSystem<ECS::SDCPPSystem>();
        if (system) {
            system->StopCurrentTask();
            LogInfo("Stopped current diffusion task");
        }
        else {
            LogError("SDCPPSystem not found when stopping task");
        }
    }

    void OnCancelCurrentDiffusionTask() {
        auto system = m_entityMgr->GetSystem<ECS::SDCPPSystem>();
        if (system) {
            system->CancelCurrentTask();
            LogInfo("Cancelled current diffusion task");
        }
        else {
            LogError("SDCPPSystem not found when cancelling task");
        }
    }

    void OnClearDiffusionQueue() {
        auto system = m_entityMgr->GetSystem<ECS::SDCPPSystem>();
        if (system) {
            system->ClearQueuedTasks();
            LogInfo("Cleared queued diffusion tasks");
        }
        else {
            LogError("SDCPPSystem not found when clearing queue");
        }
    }

    void OnClearAllDiffusionTasks() {
        auto system = m_entityMgr->GetSystem<ECS::SDCPPSystem>();
        if (system) {
            system->ClearAllTasks();
            LogInfo("Cleared all diffusion tasks");
        }
        else {
            LogError("SDCPPSystem not found when clearing all tasks");
        }
    }

    void OnPauseDiffusionWorker() {
        auto system = m_entityMgr->GetSystem<ECS::SDCPPSystem>();
        if (system) {
            system->PauseWorker();
            LogInfo("Paused diffusion worker");
        }
        else {
            LogError("SDCPPSystem not found when pausing worker");
        }
    }

    void OnResumeDiffusionWorker() {
        auto system = m_entityMgr->GetSystem<ECS::SDCPPSystem>();
        if (system) {
            system->ResumeWorker();
            LogInfo("Resumed diffusion worker");
        }
        else {
            LogError("SDCPPSystem not found when resuming worker");
        }
    }

private:
    ECS::SystemTypeID m_systemId = 0;
    ECS::EntityManager* m_entityMgr = nullptr;
    GUI::ViewManager* m_viewMgr = nullptr;
    ImGuiContext* m_imguiContext = nullptr;
};

extern "C" {
    PLUGIN_EXPORT Plugins::BasePlugin* CreatePlugin() {
        return new DiffusionAddon();
    }

    PLUGIN_EXPORT void DestroyPlugin(Plugins::BasePlugin* plugin) {
        delete plugin;
    }
}