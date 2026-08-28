#include "ModelCacheView.hpp"
#include "ModelCacheSystem.hpp"

namespace GUI {

    void ModelCacheView::RenderContent() {
        auto cacheSystem = m_entityManager.GetSystem<ECS::ModelCacheSystem>();
        if (!cacheSystem) {
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "ModelCacheSystem not available!");
            return;
        }

        ImGui::Separator();
        RenderLoadFromEntity();
        ImGui::Separator();
        RenderContextTable();
        ImGui::Separator();
        RenderCacheActions();
    }

    void ModelCacheView::RenderLoadFromEntity() {
        ImGui::Text("Load Model from Entity");
        ImGui::InputInt("Entity ID", reinterpret_cast<int*>(&m_loadEntityId));
        ImGui::SameLine();
        if (ImGui::Button("Load")) {
            if (m_loadEntityId != 0 && m_entityManager.IsEntityValid(m_loadEntityId)) {
                nlohmann::json metadata = m_entityManager.SerializeEntity(m_loadEntityId);
                auto cacheSystem = m_entityManager.GetSystem<ECS::ModelCacheSystem>();
                if (cacheSystem) {
                    bool ok = cacheSystem->loadModelFromMetadata(metadata);
                    if (!ok) {
                        std::string error = cacheSystem->getLastError();
                        if (error.find("Insufficient") != std::string::npos) {
                            ShowMemoryErrorDialog(error, cacheSystem->computeKey(metadata));
                        }
                        else {
                            ImGui::OpenPopup("LoadError");
                        }
                    }
                    RefreshContextList();
                }
            }
        }
        ImGui::SameLine();
        HelpMarker("Enter the Entity ID of a Diffusion view or any entity with model components, then click Load.");
    }

    void ModelCacheView::RenderContextTable() {
        auto cacheSystem = m_entityManager.GetSystem<ECS::ModelCacheSystem>();
        if (!cacheSystem) return;

        if (contextDetails.empty()) {
            ImGui::Text("No contexts currently loaded.");
            return;
        }

        size_t totalMemory = 0;
        int activeCount = 0;
        for (const auto& d : contextDetails) {
            totalMemory += d.memoryBytes;
            if (d.activeCount > 0) activeCount++;
        }

        ImGui::Text("Total Contexts: %zu, Active: %d, Total Memory: %s",
            contextDetails.size(), activeCount, FormatMemory(totalMemory).c_str());

        if (ImGui::BeginTable("ContextTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
            ImGui::TableSetupColumn("Model", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableSetupColumn("Memory", ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, 150.0f);
            ImGui::TableHeadersRow();

            for (auto& detail : contextDetails) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();

                bool isSelected = (selectedContextKey == detail.key);
                std::string displayName = ExtractDisplayName(detail.key);
                if (ImGui::Selectable(displayName.c_str(), isSelected, ImGuiSelectableFlags_SpanAllColumns)) {
                    selectedContextKey = detail.key;
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::BeginTooltip();
                    ImGui::Text("Full key: %s", detail.key.c_str());
                    ImGui::EndTooltip();
                }

                ImGui::TableNextColumn();
                ImGui::Text("%s", GetModelType(detail).c_str());

                ImGui::TableNextColumn();
                ImGui::Text("%s", FormatMemory(detail.memoryBytes).c_str());

                ImGui::TableNextColumn();
                if (detail.activeCount > 0) {
                    ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "In Use (%d)", detail.activeCount);
                }
                else {
                    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "Loaded");
                }

                ImGui::TableNextColumn();
                if (ImGui::SmallButton(("Unload##" + detail.key).c_str())) {
                    cacheSystem->UnloadModel(detail.key);
                    RefreshContextList();
                }
                ImGui::SameLine();
                if (ImGui::SmallButton(("Reload##" + detail.key).c_str())) {
                    cacheSystem->reloadModel(detail.key);
                    RefreshContextList();
                }
            }
            ImGui::EndTable();
        }
    }

    void ModelCacheView::RenderCacheActions() {
        auto cacheSystem = m_entityManager.GetSystem<ECS::ModelCacheSystem>();
        if (!cacheSystem) return;

        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Cache Actions");

        if (ImGui::Button("Unload Selected Context")) {
            if (!selectedContextKey.empty()) {
                cacheSystem->UnloadModel(selectedContextKey);
                RefreshContextList();
                selectedContextKey.clear();
            }
        }
        ImGui::SameLine(); HelpMarker("Unloads the selected context if not active.");

        if (ImGui::Button("Unload All Inactive")) {
            ShowConfirmationDialog(ConfirmAction::UnloadAll,
                "Are you sure you want to unload all inactive contexts?\n"
                "Active contexts (in use) will be preserved.");
        }
        ImGui::SameLine(); HelpMarker("Unloads all contexts that are not currently in use.");

        if (ImGui::Button("Force Unload All")) {
            ShowConfirmationDialog(ConfirmAction::ClearAll,
                "Are you sure you want to force unload ALL contexts?\n"
                "This will remove all contexts, even those in use.\n"
                "Use with caution!");
        }
        ImGui::SameLine(); HelpMarker("Forcefully unloads all contexts, regardless of active state.");

        if (ImGui::Button("Refresh List")) {
            RefreshContextList();
        }
    }

    void ModelCacheView::RenderConfirmationDialog() {
        if (!showConfirmDialog) return;
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(400, 0), ImGuiCond_Appearing);
        if (ImGui::Begin("Confirm Action", &showConfirmDialog, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize)) {
            ImGui::TextWrapped("%s", confirmMessage.c_str());
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::SetCursorPosX(ImGui::GetWindowWidth() * 0.5f - 100);
            if (ImGui::Button("Confirm", ImVec2(100, 0))) {
                ExecuteConfirmedAction();
                showConfirmDialog = false;
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(100, 0))) {
                showConfirmDialog = false;
            }
            ImGui::End();
        }
    }

    void ModelCacheView::RenderMemoryErrorDialog() {
        if (!showMemoryErrorDialog) return;
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(450, 0), ImGuiCond_Appearing);
        if (ImGui::Begin("Insufficient Memory", &showMemoryErrorDialog,
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize)) {
            ImGui::TextWrapped("Not enough available memory to load the model.");
            ImGui::TextWrapped("Error: %s", memoryErrorMessage.c_str());
            ImGui::Spacing();
            ImGui::TextWrapped("You can try unloading inactive models to free up memory and retry.");
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::SetCursorPosX(ImGui::GetWindowWidth() * 0.5f - 160);
            if (ImGui::Button("Unload Inactive and Retry", ImVec2(150, 0))) {
                auto cacheSystem = m_entityManager.GetSystem<ECS::ModelCacheSystem>();
                if (cacheSystem) {
                    cacheSystem->UnloadInactiveModels();
                    RefreshContextList();
                    // Retry loading the failed model
                    if (!memoryErrorFailedKey.empty()) {
                        // Find the entity that originally triggered the load; we can't easily get it here.
                        // We'll just refresh the list and let the user retry manually.
                        // Alternatively, we can call loadModelFromMetadata again if we stored the metadata.
                        // Since we don't store it, we'll just close the dialog and let user click Load again.
                        showMemoryErrorDialog = false;
                        memoryErrorRetryPending = false;
                        // Optionally, we could attempt to reload the same metadata if we had it.
                    }
                }
                showMemoryErrorDialog = false;
                memoryErrorRetryPending = false;
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(100, 0))) {
                showMemoryErrorDialog = false;
                memoryErrorRetryPending = false;
            }
            ImGui::End();
        }
    }

    void ModelCacheView::ShowMemoryErrorDialog(const std::string& error, const std::string& failedKey) {
        memoryErrorMessage = error;
        memoryErrorFailedKey = failedKey;
        showMemoryErrorDialog = true;
        memoryErrorRetryPending = false;
    }

    void ModelCacheView::ShowConfirmationDialog(ConfirmAction action, const std::string& message) {
        confirmAction = action;
        confirmMessage = message;
        showConfirmDialog = true;
    }

    void ModelCacheView::ExecuteConfirmedAction() {
        auto cacheSystem = m_entityManager.GetSystem<ECS::ModelCacheSystem>();
        if (!cacheSystem) return;
        switch (confirmAction) {
        case ConfirmAction::ClearAll:
            cacheSystem->UnloadAllModels();
            break;
        case ConfirmAction::UnloadAll:
            cacheSystem->UnloadInactiveModels();
            break;
        case ConfirmAction::ClearSelected:
            if (!selectedContextKey.empty()) {
                cacheSystem->UnloadModel(selectedContextKey);
            }
            break;
        default: break;
        }
        RefreshContextList();
        if (selectedContextKey.empty()) {
        }
    }

    std::string ModelCacheView::ExtractDisplayName(const std::string& key) const {
        size_t pos = key.find('|');
        if (pos != std::string::npos) {
            std::string first = key.substr(0, pos);
            std::filesystem::path p(first);
            return p.filename().string();
        }
        return key;
    }

    std::string ModelCacheView::FormatMemory(size_t bytes) const {
        const char* units[] = { "B", "KB", "MB", "GB" };
        int unitIdx = 0;
        double size = static_cast<double>(bytes);
        while (size >= 1024.0 && unitIdx < 3) {
            size /= 1024.0;
            unitIdx++;
        }
        char buf[64];
        snprintf(buf, sizeof(buf), "%.1f %s", size, units[unitIdx]);
        return std::string(buf);
    }

    std::string ModelCacheView::GetModelType(const ECS::ContextDetail& detail) const {
        return detail.modelType.empty() ? "Unknown" : detail.modelType;
    }

    void ModelCacheView::HelpMarker(const char* desc) {
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted(desc);
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    }

} // namespace GUI