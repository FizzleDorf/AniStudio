// SettingsView.cpp
#include "SettingsView.hpp"
#include "PathsSettings.hpp"
#include "GeneralSettings.hpp"
#include "ImGuiStyleSettings.hpp"
#include "ImGuiRenderSettings.hpp"
#include "SettingsManager.hpp"
#include <iostream>
#include <algorithm>
#include <imgui_internal.h>

namespace GUI {

    SettingsView::SettingsView()
        : showPopup(false), showSavePopup(false),
        showUnsavedChangesDialog(false), pendingClose(false),
        filterListWidth(250.0f), currentActiveTab(""),
        imguiContext(nullptr), settingsLoaded(false) {

        std::cout << "[SettingsView] Constructor - Creating SettingsManager..." << std::endl;
        settingsManager = std::make_unique<Settings::SettingsManager>();

        std::cout << "[SettingsView] Registering core tabs..." << std::endl;
        RegisterCoreTabs();

        const auto& tabs = settingsManager->GetTabs();
        if (!tabs.empty()) {
            std::cout << "[SettingsView] First tab: " << tabs[0]->GetTabName() << std::endl;
            UpdateCategoriesForActiveTab(tabs[0]->GetTabName());
        }

        std::cout << "[SettingsView] Initialized (settings will load when context is set)" << std::endl;
    }

    void SettingsView::SetImGuiContext(ImGuiContext* context) {
        imguiContext = context;
        std::cout << "[SettingsView] ImGui context set to: " << imguiContext << std::endl;

        if (!imguiContext) {
            std::cerr << "[SettingsView] Warning: ImGui context is null!" << std::endl;
            return;
        }

        // Set context for all ImGui-related tabs
        for (const auto& tab : settingsManager->GetTabs()) {
            std::string tabName = tab->GetTabName();
            if (tabName == "ImGui Style") {
                auto* styleTab = dynamic_cast<Settings::ImGuiStyleSettingsTab*>(tab.get());
                if (styleTab) {
                    styleTab->SetImGuiContext(imguiContext);
                    std::cout << "[SettingsView] Set ImGui context for ImGui Style tab" << std::endl;
                }
            }
            else if (tabName == "ImGui Render") {
                auto* renderTab = dynamic_cast<Settings::ImGuiRenderSettingsTab*>(tab.get());
                if (renderTab) {
                    renderTab->SetImGuiContext(imguiContext);
                    std::cout << "[SettingsView] Set ImGui context for ImGui Render tab" << std::endl;
                }
            }
        }

        // Now load settings with the context set
        LoadAllSettingsWithContext();
    }

    void SettingsView::LoadAllSettingsWithContext() {
        if (settingsLoaded) {
            return;
        }

        if (!imguiContext) {
            std::cerr << "[SettingsView] Cannot load settings: ImGui context is null!" << std::endl;
            return;
        }

        std::cout << "[SettingsView] Loading all settings with ImGui context..." << std::endl;

        ImGui::SetCurrentContext(imguiContext);

        try {
            settingsManager->LoadAllSettings();
            std::cout << "[SettingsView] Settings loaded successfully" << std::endl;
            settingsLoaded = true;
        }
        catch (const std::exception& e) {
            std::cerr << "[SettingsView] Error loading settings: " << e.what() << std::endl;
        }
    }

    void SettingsView::RegisterCoreTabs() {
        std::cout << "[SettingsView] Registering core tabs..." << std::endl;

        try {
            auto generalTab = std::make_unique<Settings::GeneralSettingsTab>();
            settingsManager->RegisterTab(std::move(generalTab));
            std::cout << "[SettingsView] Registered General tab" << std::endl;

            auto pathsTab = std::make_unique<Settings::PathsSettingsTab>();
            settingsManager->RegisterTab(std::move(pathsTab));
            std::cout << "[SettingsView] Registered Paths tab" << std::endl;

            auto styleTab = std::make_unique<Settings::ImGuiStyleSettingsTab>();
            settingsManager->RegisterTab(std::move(styleTab));
            std::cout << "[SettingsView] Registered ImGui Style tab" << std::endl;

            auto renderTab = std::make_unique<Settings::ImGuiRenderSettingsTab>();
            settingsManager->RegisterTab(std::move(renderTab));
            std::cout << "[SettingsView] Registered ImGui Render tab" << std::endl;

            std::cout << "[SettingsView] All core tabs registered successfully" << std::endl;
        }
        catch (const std::exception& e) {
            std::cerr << "[SettingsView] Exception registering core tabs: " << e.what() << std::endl;
        }
    }

    void SettingsView::UpdateCategoriesForActiveTab(const std::string& activeTabName) {
        availableCategories.clear();

        for (const auto& tab : settingsManager->GetTabs()) {
            if (tab->GetTabName() == activeTabName) {
                auto tabCategories = GetCategoriesFromTab(tab.get());
                for (const auto& category : tabCategories) {
                    availableCategories.insert(category);
                }
                break;
            }
        }

        SelectAllCategories();
    }

    std::vector<std::string> SettingsView::GetCategoriesFromTab(Settings::BaseTabObject* tab) {
        auto tabCategories = tab->GetCategories();
        if (!tabCategories.empty()) {
            return tabCategories;
        }

        std::string tabName = tab->GetTabName();

        if (tabName == "General") {
            return { "Startup Settings", "Auto-Save Settings", "Confirmation Dialogs", "Performance Settings", "Logging Settings" };
        }
        else if (tabName == "Paths") {
            return { "General Paths", "Model Paths" };
        }
        else if (tabName == "ImGui Style") {
            return { "Style Presets", "Font Settings", "Size Settings", "Border Settings", "Rounding Settings", "Color Settings" };
        }
        else if (tabName == "ImGui Render") {
            return { "Display Settings", "Input Settings", "Window Behavior", "Navigation Settings", "Docking Settings", "Multi-Viewport Settings", "Memory & Performance", "Input Text Settings" };
        }

        return {};
    }

    void SettingsView::RenderFilteredTabContent(Settings::BaseTabObject* tab, const std::set<std::string>& selectedCategories) {
        if (!tab) return;

        std::string tabName = tab->GetTabName();
        if ((tabName == "ImGui Style" || tabName == "ImGui Render") && imguiContext) {
            ImGui::SetCurrentContext(imguiContext);
        }

        tab->RenderFilteredUI(selectedCategories);
    }

    void SettingsView::Render() {
        if (!showPopup) return;

        if (!imguiContext) {
            std::cerr << "[SettingsView] Cannot render: ImGui context is null!" << std::endl;
            return;
        }

        ImGui::SetCurrentContext(imguiContext);
        ImGui::OpenPopup("Settings##SettingsPopup");

        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(1200, 700), ImGuiCond_Appearing);

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking |
            ImGuiWindowFlags_Modal;

        bool isOpen = true;
        if (ImGui::BeginPopupModal("Settings##SettingsPopup", &isOpen, flags)) {
            RenderMainContent();
            ImGui::EndPopup();
        }

        if (!isOpen || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            HandlePopupClose();
        }

        RenderSaveConfirmationPopup();
        RenderUnsavedChangesDialog();
    }

    void SettingsView::HandlePopupClose() {
        if (settingsManager->HasAnyUnsavedChanges()) {
            showUnsavedChangesDialog = true;
            pendingClose = true;
        }
        else {
            showPopup = false;
        }
    }

    void SettingsView::RenderMainContent() {
        const auto& tabs = settingsManager->GetTabs();
        if (tabs.empty()) {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "No settings tabs available!");
            return;
        }

        ImGui::BeginChild("SettingsSplit", ImVec2(0, -50), false);

        ImGui::BeginChild("FilterList", ImVec2(filterListWidth, 0), true);
        RenderFilterList();
        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::BeginChild("ContentArea", ImVec2(0, 0), true);
        RenderSelectedCategoriesContent();
        ImGui::EndChild();

        ImGui::EndChild();

        ImGui::Separator();
        RenderActionButtons();
    }

    void SettingsView::RenderFilterList() {
        ImGui::Text("Settings Categories");
        ImGui::Separator();

        if (ImGui::Button("Show All Categories", ImVec2(-1, 0))) {
            SelectAllCategories();
        }

        if (ImGui::Button("Deselect All", ImVec2(-1, 0))) {
            DeselectAll();
        }

        ImGui::Separator();

        if (availableCategories.empty()) {
            ImGui::TextDisabled("No categories available");
        }
        else {
            for (const auto& category : availableCategories) {
                bool isSelected = selectedCategories.find(category) != selectedCategories.end();

                ImGui::PushStyleColor(ImGuiCol_Text, isSelected ?
                    ImVec4(0.4f, 0.8f, 1.0f, 1.0f) : ImVec4(0.8f, 0.8f, 0.8f, 1.0f));

                bool clicked = ImGui::Selectable(
                    category.c_str(),
                    isSelected,
                    ImGuiSelectableFlags_AllowDoubleClick
                );

                ImGui::PopStyleColor();

                if (clicked) {
                    HandleCategorySelection(category,
                        ImGui::GetIO().KeyCtrl, ImGui::GetIO().KeyShift);
                }
            }
        }

        ImGui::Separator();
        ImGui::Text("Selected: %zu categories", selectedCategories.size());
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f),
            "Ctrl+Click: Multi-select");
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f),
            "Shift+Click: Range select");
    }

    void SettingsView::RenderSelectedCategoriesContent() {
        const auto& tabs = settingsManager->GetTabs();

        if (ImGui::BeginTabBar("SettingsTabs", ImGuiTabBarFlags_None)) {
            for (const auto& tab : tabs) {
                std::string tabTitle = tab->GetTabName() + " Settings";
                std::string tabName = tab->GetTabName();

                if (ImGui::BeginTabItem(tabTitle.c_str())) {
                    if (currentActiveTab != tabName) {
                        currentActiveTab = tabName;
                        UpdateCategoriesForActiveTab(tabName);
                    }

                    ImGui::BeginChild("TabContent", ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);
                    RenderFilteredTabContent(tab.get(), selectedCategories);
                    ImGui::EndChild();
                    ImGui::EndTabItem();
                }
            }
            ImGui::EndTabBar();
        }

        if (currentActiveTab.empty() && !tabs.empty()) {
            currentActiveTab = tabs[0]->GetTabName();
            UpdateCategoriesForActiveTab(currentActiveTab);
        }
    }

    void SettingsView::HandleCategorySelection(const std::string& categoryName, bool ctrlHeld, bool shiftHeld) {
        if (!ctrlHeld && !shiftHeld) {
            selectedCategories.clear();
            selectedCategories.insert(categoryName);
            lastSelectedCategory = categoryName;
        }
        else if (ctrlHeld) {
            if (selectedCategories.count(categoryName)) {
                selectedCategories.erase(categoryName);
            }
            else {
                selectedCategories.insert(categoryName);
            }
            lastSelectedCategory = categoryName;
        }
        else if (shiftHeld && !lastSelectedCategory.empty()) {
            std::vector<std::string> categoryOrder(availableCategories.begin(), availableCategories.end());
            std::sort(categoryOrder.begin(), categoryOrder.end());

            auto startIt = std::find(categoryOrder.begin(), categoryOrder.end(), lastSelectedCategory);
            auto endIt = std::find(categoryOrder.begin(), categoryOrder.end(), categoryName);

            if (startIt != categoryOrder.end() && endIt != categoryOrder.end()) {
                if (startIt > endIt) std::swap(startIt, endIt);

                for (auto it = startIt; it <= endIt; ++it) {
                    selectedCategories.insert(*it);
                }
            }
        }
    }

    void SettingsView::SelectAllCategories() {
        selectedCategories = availableCategories;
    }

    void SettingsView::DeselectAll() {
        selectedCategories.clear();
    }

    void SettingsView::RenderActionButtons() {
        if (settingsManager->HasAnyUnsavedChanges()) {
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "* Unsaved changes");
            ImGui::SameLine();
        }

        if (ImGui::Button("Apply")) {
            if (settingsManager->SaveAllSettings()) {
                showSavePopup = true;
            }
        }

        ImGui::SameLine();

        if (ImGui::Button("Save and Close")) {
            if (settingsManager->SaveAllSettings()) {
                showPopup = false;
                ImGui::CloseCurrentPopup();
            }
        }

        if (settingsManager->HasAnyUnsavedChanges()) {
            ImGui::SameLine();
            if (ImGui::Button("Cancel Changes")) {
                settingsManager->RestoreAllFromBackups();
            }

            ImGui::SameLine();
            if (ImGui::Button("Reset to Defaults")) {
                settingsManager->ResetAllToDefaults();
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Close")) {
            HandlePopupClose();
        }
    }

    void SettingsView::RenderSaveConfirmationPopup() {
        if (showSavePopup) {
            ImGui::OpenPopup("Settings Saved");
        }

        if (ImGui::BeginPopupModal("Settings Saved", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("All settings have been saved successfully.");
            ImGui::Separator();
            if (ImGui::Button("OK")) {
                showSavePopup = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }

    void SettingsView::RenderUnsavedChangesDialog() {
        if (showUnsavedChangesDialog) {
            ImGui::OpenPopup("Unsaved Changes");
        }

        if (ImGui::BeginPopupModal("Unsaved Changes", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("You have unsaved changes. What would you like to do?");
            ImGui::Separator();

            if (ImGui::Button("Apply and Close", ImVec2(120, 0))) {
                if (settingsManager->SaveAllSettings()) {
                    showUnsavedChangesDialog = false;
                    showPopup = false;
                    pendingClose = false;
                    ImGui::CloseCurrentPopup();
                }
            }

            ImGui::SameLine();
            if (ImGui::Button("Discard Changes", ImVec2(120, 0))) {
                settingsManager->RestoreAllFromBackups();
                showUnsavedChangesDialog = false;
                showPopup = false;
                pendingClose = false;
                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                showUnsavedChangesDialog = false;
                pendingClose = false;
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }

} // namespace GUI