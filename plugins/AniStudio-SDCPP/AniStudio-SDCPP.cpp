#include "BasePlugin.hpp"
#include "EntityManager.hpp"
#include "ViewManager.hpp"
#include "Events.hpp"
#include "DiffusionCallbackUtils.hpp"
#include "FilePathSystem.hpp"
#include "FilePathComponent.hpp"
#include "EngineContext.hpp"
#include "StudioContext.hpp"
#include "WindowState.hpp"
#include <thread>
#include <chrono>
#include "SDCPPComponents.h"
#include "SDcppSystem.hpp"
#include "ModelCacheSystem.hpp"
#include "SDCPPViews.h"
#include "ProjectSystem.hpp"

#include <iostream>
#include <filesystem>
#include "MissingPathsPopup.hpp"
#include "FilePathTab.hpp"

using namespace ECS;

namespace Utils {
    ECS::FilePathSystem* g_FilePathSystem = nullptr;
}

class DiffusionAddon : public Plugins::BasePlugin {
public:
    DiffusionAddon() : BasePlugin("DiffusionAddon", "1.0.0"), m_imguiContext(nullptr), m_wasProjectOpen(false) {}

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

        std::string dataPath = fs->GetPath("DataPath");
        if (!dataPath.empty()) {
            std::string filePath = (std::filesystem::path(dataPath) / "paths.json").string();
            if (std::filesystem::exists(filePath)) {
                fs->LoadFromFile(filePath);
                LogInfo("Loaded paths from: " + filePath);
            }
        }

        const std::vector<std::string> pluginKeys = {
            "ModelRoot",
            "checkpoint",
            "diffusion_model",
            "high_noise_diffusion_model",
            "uncond_diffusion_model",
            "motion_module",
            "vae",
            "taesd",
            "lora",
            "embed",
            "encoder",
            "llm",
            "upscale",
            "controlnet"
        };

        for (const auto& key : pluginKeys) {
            if (!fs->HasPath(key)) {
                fs->SetPath(key, "");
            }
        }

        std::string modelRoot = fs->GetPath("ModelRoot");
        if (modelRoot.empty()) {
            modelRoot = (std::filesystem::current_path() / "models").string();
        }

        ECS::FilePathTab::RegisterDefaultPath("ModelRoot", modelRoot);

        const std::vector<std::pair<std::string, std::string>> modelKeys = {
            {"checkpoint", "checkpoints"},
            {"diffusion_model", "diffusion_model"},
            {"high_noise_diffusion_model", "diffusion_model"},
            {"uncond_diffusion_model", "diffusion_model"},
            {"motion_module", "motion_module"},
            {"vae", "vae"},
            {"taesd", "taesd"},
            {"lora", "loras"},
            {"embed", "embeddings"},
            {"encoder", "clip"},
            {"llm", "llm"},
            {"upscale", "upscale_models"},
            {"controlnet", "controlnet"}
        };

        for (const auto& [key, subdir] : modelKeys) {
            ECS::FilePathTab::RegisterModelRootDependentPath(key, subdir);
        }

        if (!modelRoot.empty()) {
            ECS::FilePathTab::UpdateModelRootDefaults(modelRoot);
        }

        ECS::FilePathTab::RegisterCategoryMapper([](const std::string& key) -> std::string {
            if (key == "ModelRoot") {
                return "SDCPP";
            }
            if (key == "checkpoint" || key == "diffusion_model" || key == "high_noise_diffusion_model" ||
                key == "uncond_diffusion_model" || key == "motion_module" || key == "vae" ||
                key == "taesd" || key == "lora" || key == "embed" || key == "encoder" ||
                key == "llm" || key == "upscale" || key == "controlnet") {
                return "SDCPP";
            }
            return "";
            });

        Utils::CheckMissingPaths(fs.get());

        m_componentIds.clear();
        m_componentIds.push_back(entityMgr.RegisterComponent<ECS::PromptComponent>("Prompt"));
        m_componentIds.push_back(entityMgr.RegisterComponent<ECS::SamplerComponent>("Sampler"));
        m_componentIds.push_back(entityMgr.RegisterComponent<ECS::GuidanceComponent>("Guidance"));
        m_componentIds.push_back(entityMgr.RegisterComponent<ECS::LatentComponent>("Latent"));

        m_componentIds.push_back(entityMgr.RegisterComponent<ECS::CheckpointComponent>("Checkpoint"));
        m_componentIds.push_back(entityMgr.RegisterComponent<ECS::DiffusionModelComponent>("DiffusionModel"));
        m_componentIds.push_back(entityMgr.RegisterComponent<ECS::HighNoiseDiffusionModelComponent>("HighNoiseDiffusionModel"));
        m_componentIds.push_back(entityMgr.RegisterComponent<ECS::UncondDiffusionModelComponent>("UncondDiffusionModel"));
        m_componentIds.push_back(entityMgr.RegisterComponent<ECS::AudioVaeComponent>("AudioVae"));
        m_componentIds.push_back(entityMgr.RegisterComponent<ECS::EmbeddingsComponent>("Embeddings"));
        m_componentIds.push_back(entityMgr.RegisterComponent<ECS::MotionModuleComponent>("MotionModule"));

        m_componentIds.push_back(entityMgr.RegisterComponent<ECS::RefVideoComponent>("RefVideo"));
        m_componentIds.push_back(entityMgr.RegisterComponent<ECS::RefAudioComponent>("RefAudio"));
        m_componentIds.push_back(entityMgr.RegisterComponent<ECS::RefImagesComponent>("RefImages"));

        m_componentIds.push_back(entityMgr.RegisterComponent<ECS::ClipLComponent>("ClipL"));
        m_componentIds.push_back(entityMgr.RegisterComponent<ECS::ClipGComponent>("ClipG"));
        m_componentIds.push_back(entityMgr.RegisterComponent<ECS::T5XXLComponent>("T5XXL"));
        m_componentIds.push_back(entityMgr.RegisterComponent<ECS::LlmEncoderComponent>("LlmEncoder"));

        m_componentIds.push_back(entityMgr.RegisterComponent<ECS::ClipVisionComponent>("ClipVision"));
        m_componentIds.push_back(entityMgr.RegisterComponent<ECS::LlmVisionComponent>("LlmVision"));

        m_componentIds.push_back(entityMgr.RegisterComponent<ECS::VaeComponent>("Vae"));
        m_componentIds.push_back(entityMgr.RegisterComponent<ECS::VaeTilingComponent>("VaeTiled"));
        m_componentIds.push_back(entityMgr.RegisterComponent<ECS::TaesdComponent>("Taesd"));

        m_componentIds.push_back(entityMgr.RegisterComponent<ECS::HiresComponent>("Hires"));
        m_componentIds.push_back(entityMgr.RegisterComponent<ECS::EsrganComponent>("Esrgan"));

        m_componentIds.push_back(entityMgr.RegisterComponent<ECS::PhotoMakerComponent>("PhotoMaker"));
        m_componentIds.push_back(entityMgr.RegisterComponent<ECS::PulidWeightsComponent>("PulidWeights"));
        m_componentIds.push_back(entityMgr.RegisterComponent<ECS::ControlNetComponent>("ControlNet"));
        m_componentIds.push_back(entityMgr.RegisterComponent<ECS::LoraComponent>("Lora"));

        m_componentIds.push_back(entityMgr.RegisterComponent<ECS::VideoParamsComponent>("VideoParams"));
        m_componentIds.push_back(entityMgr.RegisterComponent<ECS::HighNoiseSamplerComponent>("HighNoiseSampler"));

        m_componentIds.push_back(entityMgr.RegisterComponent<ECS::ADetailerComponent>("ADetailer"));
        m_componentIds.push_back(entityMgr.RegisterComponent<ECS::ChromaComponent>("Chroma"));
        m_componentIds.push_back(entityMgr.RegisterComponent<ECS::StackedIdEmbedComponent>("StackedIdEmbed"));
        m_componentIds.push_back(entityMgr.RegisterComponent<ECS::EasyCacheComponent>("EasyCache"));

        m_componentIds.push_back(entityMgr.RegisterComponent<ECS::ConversionComponent>("Conversion"));

        m_componentIds.push_back(entityMgr.RegisterComponent<ECS::EmbeddingsComponent>("Embeddings"));

        m_componentIds.push_back(entityMgr.RegisterComponent<ECS::ControlNetImageComponent>("ControlNetImage"));
        m_componentIds.push_back(entityMgr.RegisterComponent<ECS::PhotoMakerImageComponent>("PhotoMakerImage"));

        m_componentIds.push_back(entityMgr.RegisterComponent<ECS::ControlFramesComponent>("ControlFrames"));

        m_componentIds.push_back(entityMgr.RegisterComponent<ECS::SDCPPSettingsComponent>("SDCPP"));

        entityMgr.RegisterSystem<ECS::ModelCacheSystem>();
        LogInfo("Registered ModelCacheSystem");

        entityMgr.RegisterSystem<ECS::SDCPPSystem>();
        LogInfo("Registered SDCPPSystem");

        m_entityMgr = &entityMgr;

        auto settingsSys = entityMgr.GetSystem<ECS::SettingsSystem>();
        if (settingsSys) {
            EntityID settingsEntity = settingsSys->GetSettingsEntity();
            if (entityMgr.IsEntityValid(settingsEntity) && !entityMgr.HasComponent<ECS::SDCPPSettingsComponent>(settingsEntity)) {
                entityMgr.AddComponent<ECS::SDCPPSettingsComponent>(settingsEntity);
                LogInfo("Added SDCPPSettingsComponent to global settings entity");
            }
            if (entityMgr.IsEntityValid(settingsEntity) && entityMgr.HasComponent<ECS::SDCPPSettingsComponent>(settingsEntity)) {
                auto& sdcppComp = entityMgr.GetComponent<ECS::SDCPPSettingsComponent>(settingsEntity);
                GUI::DiffusionCallbackUtils::SetLogLevel(sdcppComp.log_level);
            }
        }
        else {
            LogError("SettingsSystem not found, SDCPP global settings will not be available");
        }

        GUI::DiffusionCallbackUtils::InitializeCallbacks();

        RegisterEventHandlers();

        ANI::Events::Ref().RegisterEventWithData("ProjectOpened",
            [this](const std::any& data) {
                if (m_entityMgr) {
                    auto fs = m_entityMgr->GetSystem<ECS::FilePathSystem>();
                    if (fs) {
                        Utils::CheckMissingPaths(fs.get());
                    }
                }
            });
        m_registeredEvents.push_back("ProjectOpened");

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

        auto settingsSys = entityMgr.GetSystem<ECS::SettingsSystem>();
        if (settingsSys) {
            EntityID settingsEntity = settingsSys->GetSettingsEntity();
            if (entityMgr.IsEntityValid(settingsEntity) && entityMgr.HasComponent<ECS::SDCPPSettingsComponent>(settingsEntity)) {
                auto& sdcppComp = entityMgr.GetComponent<ECS::SDCPPSettingsComponent>(settingsEntity);
                auto sdcppTab = std::make_unique<ECS::SDCPPSettingsTab>(sdcppComp);
                settingsSys->RegisterTab(std::move(sdcppTab));
                LogInfo("Registered SDCPP settings tab");
            }
            else {
                LogError("SDCPPSettingsComponent not found on settings entity");
            }
        }

        m_viewTypeNames.clear();
        m_viewTypeNames.push_back("Txt2ImgView");
        m_viewTypeNames.push_back("Img2ImgView");
        m_viewTypeNames.push_back("EditView");
        m_viewTypeNames.push_back("ConvertView");
        m_viewTypeNames.push_back("UpscaleView");
        m_viewTypeNames.push_back("Img2VidView");
        m_viewTypeNames.push_back("ModelCacheView");
        m_viewTypeNames.push_back("QueueView");
        m_viewTypeNames.push_back("Txt2VidView");

        viewMgr.RegisterView<GUI::Txt2ImgView>("Txt2ImgView", "DiffusionAddon");
        viewMgr.RegisterView<GUI::Img2ImgView>("Img2ImgView", "DiffusionAddon");
        viewMgr.RegisterView<GUI::EditView>("EditView", "DiffusionAddon");
        viewMgr.RegisterView<GUI::ConvertView>("ConvertView", "DiffusionAddon");
        viewMgr.RegisterView<GUI::UpscaleView>("UpscaleView", "DiffusionAddon");
        viewMgr.RegisterView<GUI::Img2VidView>("Img2VidView", "DiffusionAddon");
        viewMgr.RegisterView<GUI::ModelCacheView>("ModelCacheView", "DiffusionAddon");
        viewMgr.RegisterView<GUI::QueueView>("QueueView", "DiffusionAddon");
        viewMgr.RegisterView<GUI::Txt2VidView>("Txt2VidView", "DiffusionAddon");

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

        auto sdcppSystem = m_entityMgr ? m_entityMgr->GetSystem<ECS::SDCPPSystem>() : nullptr;
        if (sdcppSystem) {
            LogInfo("Stopping SDCPPSystem worker and waiting for tasks to finish...");
            sdcppSystem->Shutdown();
            LogInfo("SDCPPSystem shutdown complete");
        }

        auto fs = m_entityMgr ? m_entityMgr->GetSystem<ECS::FilePathSystem>() : nullptr;
        if (fs) {
            std::string configPath = fs->GetPath("DataPath");
            if (!configPath.empty()) {
                std::string filePath = (std::filesystem::path(configPath) / "paths.json").string();
                fs->SaveToFile(filePath);
            }
        }

        if (m_viewMgr) {
            for (const auto& viewName : m_viewTypeNames) {
                try {
                    m_viewMgr->UnregisterViewType(viewName);
                    LogInfo("Unregistered view type: " + viewName);
                }
                catch (const std::exception& e) {
                    LogError("Failed to unregister view type " + viewName + ": " + e.what());
                }
            }
            m_viewTypeNames.clear();
        }

        if (m_entityMgr) {
            try {
                m_entityMgr->UnregisterSystem<ECS::ModelCacheSystem>();
                LogInfo("Unregistered ModelCacheSystem");
            }
            catch (const std::exception& e) {
                LogError("Failed to unregister ModelCacheSystem: " + std::string(e.what()));
            }
            try {
                m_entityMgr->UnregisterSystem<ECS::SDCPPSystem>();
                LogInfo("Unregistered SDCPPSystem");
            }
            catch (const std::exception& e) {
                LogError("Failed to unregister SDCPPSystem: " + std::string(e.what()));
            }

            for (ComponentTypeID cid : m_componentIds) {
                try {
                    m_entityMgr->UnregisterComponentById(cid);
                    LogInfo("Unregistered component ID: " + std::to_string(cid));
                }
                catch (const std::exception& e) {
                    LogError("Failed to unregister component: " + std::string(e.what()));
                }
            }
            m_componentIds.clear();
        }

        UnregisterEventHandlers();

        Utils::g_FilePathSystem = nullptr;
        m_entityMgr = nullptr;
        m_viewMgr = nullptr;
        m_imguiContext = nullptr;

        LogInfo("DiffusionAddon shutdown complete");
    }

    void OnUpdate(float deltaTime) override {
        if (!m_entityMgr) return;

        auto projSys = m_entityMgr->GetSystem<ANI::ProjectSystem>();
        if (!projSys) return;

        bool isOpen = projSys->IsProjectOpen();

        if (m_wasProjectOpen && !isOpen) {
            LogInfo("Project closing detected - stopping SDCPPSystem worker...");
            auto sdcpp = m_entityMgr->GetSystem<ECS::SDCPPSystem>();
            if (sdcpp) {
                sdcpp->Shutdown();
                LogInfo("SDCPPSystem worker stopped due to project close.");
            }
            auto cache = m_entityMgr->GetSystem<ECS::ModelCacheSystem>();
            if (cache) {
                cache->UnloadAllModels();
                LogInfo("Model cache cleared.");
            }
        }

        m_wasProjectOpen = isOpen;
    }

private:
    std::vector<std::string> m_registeredEvents;
    std::vector<ComponentTypeID> m_componentIds;
    std::vector<std::string> m_viewTypeNames;

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
    bool m_wasProjectOpen;
};

extern "C" {
    PLUGIN_EXPORT Plugins::BasePlugin* CreatePlugin() {
        return new DiffusionAddon();
    }

    PLUGIN_EXPORT void DestroyPlugin(Plugins::BasePlugin* plugin) {
        delete plugin;
    }
}