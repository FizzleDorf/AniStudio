#pragma once
#include "GUI.h"
#include <string>
#include <vector>
#include <memory>

namespace ECS {
    class EntityManager;
    class SettingsSystem;
    class BaseSettingsTab;
}

namespace GUI {

    class SettingsView {
    public:
        SettingsView();
        virtual ~SettingsView() = default;

        void SetImGuiContext(ImGuiContext* context);
        void SetEntityManager(ECS::EntityManager& mgr);
        void SetTabs(const std::vector<std::unique_ptr<ECS::BaseSettingsTab>>* tabs) { m_tabs = tabs; }

        void Show();
        void Hide();
        bool IsVisible() const { return m_open; }

        void Render();

    private:
        ECS::EntityManager* m_entityManager = nullptr;
        ECS::SettingsSystem* m_settingsSystem = nullptr;
        const std::vector<std::unique_ptr<ECS::BaseSettingsTab>>* m_tabs = nullptr;
        ImGuiContext* imguiContext = nullptr;

        bool m_open = false;
        bool showSaveNotification = false;

        char filterBuffer[256] = "";

        void RenderMainContent();
        void RenderTabsAndContent();
        void RenderActionButtons();
    };

}