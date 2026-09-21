#include "StudioRegistration.hpp"

#include "EntityManager.hpp"
#include "ViewManager.hpp"
#include "StudioContext.hpp"
#include "Log.hpp"

#include "AniStudioComponents.hpp"
#include "AniStudioSystems.hpp"
#include "AniStudioViews.hpp"

#include "FilePathSystem.hpp"
#include "BaseView.hpp"
#include "StudioPluginManager.hpp"
#include "PlaybackStateComponent.hpp"
#include "VideoAudioSystem.hpp"
#include "AVStreamingSystem.hpp"

#include <memory>

namespace ANI::Registration {

    // Components
    void RegisterComponents(ECS::EntityManager& entityMgr) {
        entityMgr.RegisterComponent<ECS::ImGuiStyleSettingsComponent>("ImGuiStyleSettings");
        entityMgr.RegisterComponent<ECS::ImGuiRenderSettingsComponent>("ImGuiRenderSettings");
        entityMgr.RegisterComponent<ECS::FontSettingsComponent>("FontSettings");
        entityMgr.RegisterComponent<ECS::TextEditorSettingsComponent>("TextEditorSettings");
        entityMgr.RegisterComponent<ECS::TextureComponent>("TextureComponent");
        entityMgr.RegisterComponent<ECS::PlaybackStateComponent>("PlaybackState");
        entityMgr.RegisterComponent<ECS::PreviewImageComponent>("PreviewImage");

        ANI_LOG_INFO("All GUI components registered");
    }

    // Systems
    void RegisterSystems(ECS::EntityManager& entityMgr,
        GUI::ViewManager* viewManager,
        void* windowHandle,
        StudioContext* studioContext) {
        entityMgr.RegisterSystem<ECS::TextureSystem>();
        entityMgr.RegisterSystem<ECS::SettingsSystem>();
        entityMgr.RegisterSystem<ECS::ProjectSystem>();
        entityMgr.RegisterSystem<ECS::AudioPlaybackSystem>();
        entityMgr.RegisterSystem<ECS::VideoPlaybackSystem>();
        entityMgr.RegisterSystem<ECS::VideoAudioSystem>();
        entityMgr.RegisterSystem<ECS::AVStreamingSystem>();
        entityMgr.RegisterSystem<ECS::MediaEngineSystem>();

        // post-registration wiring
        if (auto projectSystem = entityMgr.GetSystem<ECS::ProjectSystem>()) {
            projectSystem->SetWindowHandle(windowHandle);
            if (viewManager) {
                projectSystem->SetViewManager(viewManager);
            }

            if (auto fileSys = entityMgr.GetSystem<ECS::FilePathSystem>()) {
                std::string defaultPath = fileSys->GetPath("DefaultProject");
                if (!defaultPath.empty()) {
                    projectSystem->SetDefaultProjectPath(defaultPath);
                }
            }
        }

        ANI_LOG_INFO("All GUI systems registered");
    }

    // Views
    void RegisterViews(ECS::EntityManager& entityMgr,
        GUI::ViewManager& viewManager,
        Plugins::StudioPluginManager* pluginManager) {
        viewManager.RegisterView<GUI::DebugView>("DebugView");
        viewManager.RegisterView<GUI::ImageView>("ImageView");
        viewManager.RegisterView<GUI::VideoView>("VideoView");
        viewManager.RegisterView<GUI::HelpView>("HelpView");
        viewManager.RegisterView<GUI::TextEditorView>("TextEditor");
        viewManager.RegisterView<GUI::MediaHistoryView>("MediaHistoryView");
        viewManager.RegisterView<GUI::AssetsView>("AssetsView");
        viewManager.RegisterView<GUI::MetadataView>("MetadataView");
        viewManager.RegisterView<GUI::AudioView>("AudioView");

        if (pluginManager) {
            auto* pluginMgr = pluginManager;
            viewManager.RegisterViewWithFactory("PluginView", "Tools",
                [pluginMgr](ECS::EntityManager& mgr, GUI::ViewManager& vm) -> std::unique_ptr<GUI::BaseView> {
                    return std::make_unique<GUI::PluginView>(mgr, vm, *pluginMgr);
                },
                []() -> GUI::ViewMetadata {
                    return GUI::BaseView::GetMetadataFor<GUI::PluginView>();
                }
            );
        }

        viewManager.RegisterViewWithFactory("WorkspaceView", "Views",
            [](ECS::EntityManager& mgr, GUI::ViewManager& vm) -> std::unique_ptr<GUI::BaseView> {
                return std::make_unique<GUI::WorkspaceView>(mgr, vm);
            },
            []() -> GUI::ViewMetadata { return GUI::WorkspaceView::GetMetadata(); }
        );

        ANI_LOG_INFO("All views registered");
    }

} // namespace ANI::Registration