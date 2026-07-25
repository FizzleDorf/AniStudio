#pragma once
#include "GUI.h"
#include <string>
#include <memory>

namespace ECS {
    class EntityManager;
    class SettingsSystem;
    class FilePathSystem;
}

namespace GUI {

    class SettingsView {
    public:
        SettingsView();
        virtual ~SettingsView() = default;

        void SetImGuiContext(ImGuiContext* context);
        void SetEntityManager(ECS::EntityManager& mgr);

        void Show() { showPopup = true; }
        void Hide() { showPopup = false; }
        bool IsVisible() const { return showPopup; }

        void Render();

    private:
        ECS::EntityManager* m_entityManager = nullptr;
        ECS::SettingsSystem* m_settingsSystem = nullptr;
        ImGuiContext* imguiContext = nullptr;

        bool showPopup = false;
        bool showUnsavedChangesDialog = false;
        bool pendingClose = false;
        bool settingsLoaded = false;
        bool showSaveNotification = false;

        std::string currentActiveTab;

        char filterBuffer[256] = "";

        void HandlePopupClose();
        void RenderMainContent();
        void RenderTabsAndContent();
        void RenderActionButtons();
        void RenderUnsavedChangesDialog();
    };

}