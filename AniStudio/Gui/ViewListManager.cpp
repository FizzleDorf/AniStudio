#include "ViewListManager.hpp"
#include <filesystem>
#include <fstream>

namespace GUI {

void ViewListManager::Init() {
    LoadTemplates();

    // Load last selected template
    std::ifstream file(lastSelectedPath);
    if (file.is_open()) {
        nlohmann::json j;
        file >> j;
        if (j.contains("lastSelected")) {
            selectedTemplate = j["lastSelected"];
            LoadViewList(selectedTemplate);
        }
    }
}

void ViewListManager::Render() {
    ImGui::Begin("View List Manager");

    RenderTemplateList();
    ImGui::Separator();
    RenderViewTypeSelector();
    ImGui::Separator();
    RenderCurrentViewList();
    ImGui::Separator();
    RenderTemplateControls();

    ImGui::End();
}

void ViewListManager::RenderTemplateList() {
    ImGui::Text("Templates");
    if (ImGui::BeginListBox("##Templates", ImVec2(-1, 100))) {
        for (const auto &temp : templates) {
            bool isSelected = (selectedTemplate == temp.name);
            if (ImGui::Selectable(temp.name.c_str(), isSelected)) {
                selectedTemplate = temp.name;
                LoadViewList(selectedTemplate);

                // Save last selected template
                nlohmann::json j;
                j["lastSelected"] = selectedTemplate;
                std::ofstream file(lastSelectedPath);
                file << j.dump(4);
            }

            if (temp.isDefault) {
                ImGui::SameLine();
                ImGui::TextDisabled("(Default)");
            }
        }
        ImGui::EndListBox();
    }
}

void ViewListManager::RenderViewTypeSelector() {
    if (selectedTemplate.empty())
        return;

    ImGui::Text("Available Views");
    ImGui::BeginChild("AvailableViews", ImVec2(-1, 150), true);

    /*for (const auto &viewType : viewMgr.GetAllRegisteredViewTypes()) {
        if (ImGui::Button(viewType.c_str())) {
            auto it = std::find_if(templates.begin(), templates.end(),
                                   [this](const ViewListTemplate &t) { return t.name == selectedTemplate; });

            if (it != templates.end() && !it->isDefault) {
                it->viewTypes.push_back(viewType);
                SaveViewList(selectedTemplate);
            }
        }
    }*/
    ImGui::EndChild();
}

void ViewListManager::RenderCurrentViewList() {
    if (selectedTemplate.empty())
        return;

    auto it = std::find_if(templates.begin(), templates.end(),
                           [this](const ViewListTemplate &t) { return t.name == selectedTemplate; });

    if (it != templates.end()) {
        ImGui::Text("Current Views");
        ImGui::BeginChild("CurrentViews", ImVec2(-1, 150), true);

        for (size_t i = 0; i < it->viewTypes.size(); ++i) {
            const auto &viewType = it->viewTypes[i];
            ImGui::Text("%s", viewType);

            ImGui::SameLine();
            if (ImGui::Button(("Erase##" + std::to_string(i)).c_str())) {
                it->viewTypes.erase(it->viewTypes.begin() + i);
                SaveViewList(selectedTemplate);
                LoadViewList(selectedTemplate); // Refresh views
                break;
            }
        }
        ImGui::EndChild();
    }
}

void ViewListManager::RenderTemplateControls() {
    ImGui::Text("Manage Templates");

    static char nameBuffer[256] = "";
    ImGui::InputText("New Template Name", nameBuffer, sizeof(nameBuffer));
    if (ImGui::Button("Create New Template")) {
        std::string newName(nameBuffer);
        if (!newName.empty()) {
            ViewListTemplate newTemplate;
            newTemplate.name = newName;
            newTemplate.isDefault = false;
            templates.push_back(newTemplate);
            SaveDefaultTemplates();
            memset(nameBuffer, 0, sizeof(nameBuffer));
        }
    }

    if (!selectedTemplate.empty()) {
        auto it = std::find_if(templates.begin(), templates.end(),
                               [this](const ViewListTemplate &t) { return t.name == selectedTemplate; });

        if (it != templates.end() && !it->isDefault) {
            if (ImGui::Button("Delete Template")) {
                DeleteViewList(selectedTemplate);
            }
        }
    }
}

void ViewListManager::SaveViewList(const std::string &name) {
    auto it =
        std::find_if(templates.begin(), templates.end(), [&name](const ViewListTemplate &t) { return t.name == name; });

    if (it != templates.end() && !it->isDefault) {
        SaveDefaultTemplates();
    }
}

void ViewListManager::LoadViewList(const std::string &name) {
    auto it =
        std::find_if(templates.begin(), templates.end(), [&name](const ViewListTemplate &t) { return t.name == name; });

    if (it != templates.end()) {
        // Clear current views from the manager
        auto views = viewMgr.GetAllViews();
        for (auto view : views) {
            viewMgr.DestroyView(view);
        }

        // Add views from template
        for (auto viewType : it->viewTypes) {
            auto viewId = viewMgr.CreateView();
            // Need to map viewType to actual view class and add
            // This would need a registration system for view types
        }
    }
}

void ViewListManager::DeleteViewList(const std::string &name) {
    auto it = std::find_if(templates.begin(), templates.end(),
                           [&name](const ViewListTemplate &t) { return t.name == name && !t.isDefault; });

    if (it != templates.end()) {
        templates.erase(it);
        SaveDefaultTemplates();
        selectedTemplate.clear();
    }
}

void ViewListManager::CreateDefaultTemplates() {
    templates = defaultTemplates;
    SaveDefaultTemplates();
}

void ViewListManager::SaveDefaultTemplates() {
    std::filesystem::create_directories("../data/templates");

    nlohmann::json j;
    for (const auto &temp : templates) {
        nlohmann::json templateJson;
        templateJson["name"] = temp.name;
        templateJson["isDefault"] = temp.isDefault;
        templateJson["viewTypes"] = temp.viewTypes;
        j["templates"].push_back(templateJson);
    }

    std::ofstream file(templatesPath);
    file << j.dump(4);
}

void ViewListManager::LoadTemplates() {
    std::ifstream file(templatesPath);
    if (!file.is_open()) {
        CreateDefaultTemplates();
        return;
    }

    try {
        nlohmann::json j;
        file >> j;

        templates.clear();
        for (const auto &templateJson : j["templates"]) {
            ViewListTemplate temp;
            temp.name = templateJson["name"];
            temp.isDefault = templateJson["isDefault"];
            temp.viewTypes = templateJson["viewTypes"].get<std::vector<ViewTypeID>>();
            templates.push_back(temp);
        }
    } catch (...) {
        CreateDefaultTemplates();
    }
}

} // namespace GUI