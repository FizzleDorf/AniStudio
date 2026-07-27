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

        void Show();
        void Hide();
        bool IsVisible() const { return popupOpen; }

        void Render();

    private:
        ECS::EntityManager* m_entityManager = nullptr;
        ECS::SettingsSystem* m_settingsSystem = nullptr;
        ImGuiContext* imguiContext = nullptr;

        bool popupOpen = false;
        bool settingsLoaded = false;
        bool showSaveNotification = false;

        std::string currentActiveTab;
        char filterBuffer[256] = "";

        void RenderMainContent();
        void RenderTabsAndContent();
        void RenderActionButtons();
    };

}