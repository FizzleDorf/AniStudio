#pragma once

#include "GUI.h"
#include "BaseTabObject.hpp"
#include <string>
#include <vector>
#include <set>
#include <memory>

namespace ECS { class EntityManager; }
namespace Settings { class SettingsManager; }

namespace GUI {

    class SettingsView {
    public:
        SettingsView();
        virtual ~SettingsView() = default;

        void SetImGuiContext(ImGuiContext* context);
        void SetEntityManager(ECS::EntityManager& mgr);
        void Render();
        void Show() { showPopup = true; }
        void Hide() { showPopup = false; }
        bool IsVisible() const { return showPopup; }

    private:
        std::unique_ptr<Settings::SettingsManager> settingsManager;

        bool showPopup;
        bool showSavePopup;
        bool showUnsavedChangesDialog;
        bool pendingClose;

        float filterListWidth;
        std::set<std::string> availableCategories;
        std::set<std::string> selectedCategories;
        std::string lastSelectedCategory;
        std::string currentActiveTab;

        ImGuiContext* imguiContext;
        bool settingsLoaded;
        ECS::EntityManager* m_entityManager = nullptr;

        void RegisterCoreTabs();
        void UpdateCategoriesForActiveTab(const std::string& activeTabName);
        std::vector<std::string> GetCategoriesFromTab(Settings::BaseTabObject* tab);
        void RenderFilteredTabContent(Settings::BaseTabObject* tab, const std::set<std::string>& selectedCategories);
        void HandleCategorySelection(const std::string& categoryName, bool ctrlHeld, bool shiftHeld);
        void SelectAllCategories();
        void DeselectAll();
        void HandlePopupClose();
        void LoadAllSettingsWithContext();

        void RenderMainContent();
        void RenderFilterList();
        void RenderSelectedCategoriesContent();
        void RenderActionButtons();
        void RenderSaveConfirmationPopup();
        void RenderUnsavedChangesDialog();
    };

} // namespace GUI