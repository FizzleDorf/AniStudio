#include "BasePlugin.hpp"
#include "EntityManager.hpp"
#include "ViewManager.hpp"
#include "Events.hpp"
#include "DiffusionCallbackUtils.hpp"
#include "SettingsSystem.hpp"
#include "FilePathComponent.hpp"
#include "EngineContext.hpp"
#include "StudioContext.hpp"
#include "WindowState.hpp"
#include <thread>
#include <chrono>
#include "SDCPPComponents.h"
#include "SDCPPSystem.hpp"
#include "ModelCacheSystem.hpp"
#include "SDCPPViews.h"
#include "PreviewView.hpp"
#include "ProjectSystem.hpp"

#include <iostream>
#include <filesystem>
#include "MissingPathsPopup.hpp"
#include "SDCPPSettingsTab.hpp"
#include "SDCPPSettingsComponent.hpp"
#include "BaseSettingsTab.hpp"

using namespace ECS;

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

        auto settings = entityMgr.GetSystem<ECS::SettingsSystem>();
        if (!settings) {
            LogError("SettingsSystem not found! Cannot initialize.");
            return false;
        }
        LogInfo("SettingsSystem obtained.");

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
            if (!settings->HasPath(key)) {
                settings->SetPath(key, "");
            }
        }

        std::string modelRoot = settings->GetPath("ModelRoot");
        if (modelRoot.empty()) {
            modelRoot = (std::filesystem::current_path() / "models").string();
        }

        settings->RegisterPathDefault("ModelRoot", modelRoot);

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
            std::string defaultPath = (std::filesystem::path(modelRoot) / subdir).string();
            settings->RegisterPathDefault(key, defaultPath);
        }

        Utils::CheckMissingPaths(settings.get());

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

        m_componentIds.push_back(entityMgr.RegisterComponent<ECS::HighNoiseSamplerComponent>("HighNoiseSampler"));

        m_componentIds.push_back(entityMgr.RegisterComponent<ECS::ADetailerComponent>("ADetailer"));
        m_componentIds.push_back(entityMgr.RegisterComponent<ECS::ChromaComponent>("Chroma"));
        m_componentIds.push_back(entityMgr.RegisterComponent<ECS::StackedIdEmbedComponent>("StackedIdEmbed"));
        m_componentIds.push_back(entityMgr.RegisterComponent<ECS::EasyCacheComponent>("EasyCache"));

        m_componentIds.push_back(entityMgr.RegisterComponent<ECS::ConversionComponent>("Conversion"));

        m_componentIds.push_back(entityMgr.RegisterComponent<ECS::ControlNetImageComponent>("ControlNetImage"));
        m_componentIds.push_back(entityMgr.RegisterComponent<ECS::PhotoMakerImageComponent>("PhotoMakerImage"));

        m_componentIds.push_back(entityMgr.RegisterComponent<ECS::ControlFramesComponent>("ControlFrames"));

        m_componentIds.push_back(entityMgr.RegisterComponent<ECS::SDCPPSettingsComponent>("SDCPP"));

        entityMgr.RegisterSystem<ECS::ModelCacheSystem>();
        LogInfo("Registered ModelCacheSystem");

        entityMgr.RegisterSystem<ECS::SDCPPSystem>();
        LogInfo("Registered SDCPPSystem");

        m_entityMgr = &entityMgr;

        EntityID settingsEntity = settings->GetSettingsEntity();
        if (entityMgr.IsEntityValid(settingsEntity) &&
            !entityMgr.HasComponent<ECS::SDCPPSettingsComponent>(settingsEntity)) {
            entityMgr.AddComponent<ECS::SDCPPSettingsComponent>(settingsEntity);
            LogInfo("Added SDCPPSettingsComponent to global settings entity");
        }

        if (entityMgr.HasComponent<ECS::SDCPPSettingsComponent>(settingsEntity)) {
            auto& sdcppComp = entityMgr.GetComponent<ECS::SDCPPSettingsComponent>(settingsEntity);
            settings->LoadComponentSettings(sdcppComp);
            GUI::DiffusionCallbackUtils::SetLogLevel(sdcppComp.log_level);
        }

        GUI::DiffusionCallbackUtils::InitializeCallbacks();

        RegisterEventHandlers();

        ANI::Events::Ref().RegisterEventWithData("ProjectOpened",
            [this](const std::any& data) {
                if (m_entityMgr) {
                    auto settings = m_entityMgr->GetSystem<ECS::SettingsSystem>();
                    if (settings) {
                        Utils::CheckMissingPaths(settings.get());
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

        m_viewMgr = &viewMgr;

        auto settings = entityMgr.GetSystem<ECS::SettingsSystem>();
        if (settings) {
            EntityID settingsEntity = settings->GetSettingsEntity();
            if (entityMgr.IsEntityValid(settingsEntity) &&
                entityMgr.HasComponent<ECS::SDCPPSettingsComponent>(settingsEntity)) {
                auto& sdcppComp = entityMgr.GetComponent<ECS::SDCPPSettingsComponent>(settingsEntity);

                settings->LoadComponentSettings(sdcppComp);
                settings->TrackSettingsComponent(
                    entityMgr.GetComponentTypeIdByName("SDCPP"));

                settings->SetApplyCallback(
                    entityMgr.GetComponentTypeIdByName("SDCPP"),
                    [](ECS::BaseComponent& c) {
                        auto& s = static_cast<ECS::SDCPPSettingsComponent&>(c);
                        GUI::DiffusionCallbackUtils::SetLogLevel(s.log_level);
                    });

                auto studioCtx = GetStudioContext();
                if (studioCtx && studioCtx->addSettingsTab) {
                    studioCtx->addSettingsTab(
                        std::make_unique<ECS::SDCPPSettingsTab>(sdcppComp));
                    LogInfo("Registered SDCPP settings tab with StudioCore");
                }
                else {
                    LogError("StudioContext has no addSettingsTab hook; SDCPP tab not registered");
                }
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
        m_viewTypeNames.push_back("PreviewView");

        viewMgr.RegisterView<GUI::Txt2ImgView>("Txt2ImgView", "DiffusionAddon");
        viewMgr.RegisterView<GUI::Img2ImgView>("Img2ImgView", "DiffusionAddon");
        viewMgr.RegisterView<GUI::EditView>("EditView", "DiffusionAddon");
        viewMgr.RegisterView<GUI::ConvertView>("ConvertView", "DiffusionAddon");
        viewMgr.RegisterView<GUI::UpscaleView>("UpscaleView", "DiffusionAddon");
        viewMgr.RegisterView<GUI::Img2VidView>("Img2VidView", "DiffusionAddon");
        viewMgr.RegisterView<GUI::ModelCacheView>("ModelCacheView", "DiffusionAddon");
        viewMgr.RegisterView<GUI::QueueView>("QueueView", "DiffusionAddon");
        viewMgr.RegisterView<GUI::Txt2VidView>("Txt2VidView", "DiffusionAddon");
        viewMgr.RegisterView<GUI::PreviewView>("PreviewView", "DiffusionAddon");

        LogInfo("Views registered via direct ViewManager");
        return true;
    }

    void SetImGuiContext(ImGuiContext* context) override {
        m_imguiContext = context;
        LogInfo("ImGui context set for DiffusionAddon");
    }

    bool HasValidImGuiContext() const override {
        return m_imguiContext != nullptr;
    }

    void OnShutdown() override {
        LogInfo("SHUTTING DOWN DIFFUSION ADDON");

        auto sdcppSystem = m_entityMgr ? m_entityMgr->GetSystem<ECS::SDCPPSystem>() : nullptr;
        if (sdcppSystem) {
            sdcppSystem->Shutdown();
        }

        if (m_entityMgr) {
            if (auto settings = m_entityMgr->GetSystem<ECS::SettingsSystem>()) {
                EntityID settingsEntity = settings->GetSettingsEntity();
                if (m_entityMgr->IsEntityValid(settingsEntity) &&
                    m_entityMgr->HasComponent<ECS::SDCPPSettingsComponent>(settingsEntity)) {
                    auto& sdcppComp = m_entityMgr->GetComponent<ECS::SDCPPSettingsComponent>(settingsEntity);
                    settings->SaveComponentSettings(sdcppComp);
                }
            }
        }

        if (m_viewMgr) {
            for (const auto& viewName : m_viewTypeNames) {
                try {
                    m_viewMgr->UnregisterViewType(viewName);
                }
                catch (const std::exception& e) {
                    LogError("Failed to unregister view type " + viewName + ": " + e.what());
                }
            }
            m_viewTypeNames.clear();
        }

        if (m_entityMgr) {
            try { m_entityMgr->UnregisterSystem<ECS::ModelCacheSystem>(); }
            catch (const std::exception&) {}
            try { m_entityMgr->UnregisterSystem<ECS::SDCPPSystem>(); }
            catch (const std::exception&) {}

            for (ComponentTypeID cid : m_componentIds) {
                try { m_entityMgr->UnregisterComponentById(cid); }
                catch (const std::exception&) {}
            }
            m_componentIds.clear();
        }

        UnregisterEventHandlers();

        m_entityMgr = nullptr;
        m_viewMgr = nullptr;
        m_imguiContext = nullptr;

        LogInfo("DiffusionAddon shutdown complete");
    }

    void OnUpdate(float deltaTime) override {
        if (!m_entityMgr) return;

        auto projSys = m_entityMgr->GetSystem<ECS::ProjectSystem>();
        if (!projSys) return;

        bool isOpen = projSys->IsProjectOpen();

        if (m_wasProjectOpen && !isOpen) {
            auto sdcpp = m_entityMgr->GetSystem<ECS::SDCPPSystem>();
            if (sdcpp) sdcpp->Shutdown();

            auto cache = m_entityMgr->GetSystem<ECS::ModelCacheSystem>();
            if (cache) cache->UnloadAllModels();
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
            "QueueDiffusionTask", "RemoveFromDiffusionQueue", "MoveInDiffusionQueue",
            "StopCurrentDiffusionTask", "CancelCurrentDiffusionTask",
            "ClearDiffusionQueue", "ClearAllDiffusionTasks",
            "PauseDiffusionWorker", "ResumeDiffusionWorker"
        };

        events.RegisterEventWithData("QueueDiffusionTask", [this](const std::any& d) { OnQueueDiffusionTask(d); });
        events.RegisterEventWithData("RemoveFromDiffusionQueue", [this](const std::any& d) { OnRemoveFromDiffusionQueue(d); });
        events.RegisterEventWithData("MoveInDiffusionQueue", [this](const std::any& d) { OnMoveInDiffusionQueue(d); });
        events.RegisterEvent("StopCurrentDiffusionTask", [this]() { OnStopCurrentDiffusionTask(); });
        events.RegisterEvent("CancelCurrentDiffusionTask", [this]() { OnCancelCurrentDiffusionTask(); });
        events.RegisterEvent("ClearDiffusionQueue", [this]() { OnClearDiffusionQueue(); });
        events.RegisterEvent("ClearAllDiffusionTasks", [this]() { OnClearAllDiffusionTasks(); });
        events.RegisterEvent("PauseDiffusionWorker", [this]() { OnPauseDiffusionWorker(); });
        events.RegisterEvent("ResumeDiffusionWorker", [this]() { OnResumeDiffusionWorker(); });
    }

    void UnregisterEventHandlers() {
        auto& events = ANI::Events::Ref();
        for (const auto& eventName : m_registeredEvents) {
            try { events.UnregisterEvent(eventName); }
            catch (const std::exception&) {}
        }
        m_registeredEvents.clear();
    }

    void OnQueueDiffusionTask(const std::any& data) {
        try {
            auto t = std::any_cast<std::pair<ECS::EntityID, ECS::SDCPPSystem::TaskType>>(data);
            auto system = m_entityMgr->GetSystem<ECS::SDCPPSystem>();
            if (system) system->QueueTask(t.first, t.second);
        }
        catch (const std::bad_any_cast& e) {
            LogError("Invalid data type for QueueDiffusionTask event: " + std::string(e.what()));
        }
    }

    void OnRemoveFromDiffusionQueue(const std::any& data) {
        try {
            auto index = std::any_cast<size_t>(data);
            auto system = m_entityMgr->GetSystem<ECS::SDCPPSystem>();
            if (system) system->RemoveFromQueue(index);
        }
        catch (const std::bad_any_cast& e) {
            LogError("Invalid data type for RemoveFromDiffusionQueue event: " + std::string(e.what()));
        }
    }

    void OnMoveInDiffusionQueue(const std::any& data) {
        try {
            auto idx = std::any_cast<std::pair<size_t, size_t>>(data);
            auto system = m_entityMgr->GetSystem<ECS::SDCPPSystem>();
            if (system) system->MoveInQueue(idx.first, idx.second);
        }
        catch (const std::bad_any_cast& e) {
            LogError("Invalid data type for MoveInDiffusionQueue event: " + std::string(e.what()));
        }
    }

    void OnStopCurrentDiffusionTask() {
        auto system = m_entityMgr->GetSystem<ECS::SDCPPSystem>();
        if (system) system->StopCurrentTask();
    }

    void OnCancelCurrentDiffusionTask() {
        auto system = m_entityMgr->GetSystem<ECS::SDCPPSystem>();
        if (system) system->CancelCurrentTask();
    }

    void OnClearDiffusionQueue() {
        auto system = m_entityMgr->GetSystem<ECS::SDCPPSystem>();
        if (system) system->ClearQueuedTasks();
    }

    void OnClearAllDiffusionTasks() {
        auto system = m_entityMgr->GetSystem<ECS::SDCPPSystem>();
        if (system) system->ClearAllTasks();
    }

    void OnPauseDiffusionWorker() {
        auto system = m_entityMgr->GetSystem<ECS::SDCPPSystem>();
        if (system) system->PauseWorker();
    }

    void OnResumeDiffusionWorker() {
        auto system = m_entityMgr->GetSystem<ECS::SDCPPSystem>();
        if (system) system->ResumeWorker();
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