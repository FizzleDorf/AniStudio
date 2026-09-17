#include "ModelCacheView.hpp"
#include "ModelCacheSystem.hpp"
#include "SDCPPParamFill.hpp"

namespace GUI {

    // =========================================================================
    // Refresh
    // =========================================================================
    void ModelCacheView::RefreshLists() {
        auto cacheSystem = m_entityManager.GetSystem<ECS::ModelCacheSystem>();
        if (!cacheSystem) return;

        contextDetails = cacheSystem->GetContextDetails();
        upscalerDetails = cacheSystem->GetUpscalerDetails();

        // Drop selections that no longer exist.
        if (!selectedContextKey.empty()) {
            auto it = std::find_if(contextDetails.begin(), contextDetails.end(),
                [this](const ECS::ContextDetail& d) { return d.key == selectedContextKey; });
            if (it == contextDetails.end()) selectedContextKey.clear();
        }
        if (!selectedUpscalerKey.empty()) {
            auto it = std::find_if(upscalerDetails.begin(), upscalerDetails.end(),
                [this](const ECS::UpscalerDetail& d) { return d.key == selectedUpscalerKey; });
            if (it == upscalerDetails.end()) selectedUpscalerKey.clear();
        }
    }

    // =========================================================================
    // Layout
    // =========================================================================
    void ModelCacheView::RenderContent() {
        auto cacheSystem = m_entityManager.GetSystem<ECS::ModelCacheSystem>();
        if (!cacheSystem) {
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "ModelCacheSystem not available!");
            return;
        }

        // Tree takes the top portion, controls below. Use a fixed split via
        // BeginChild so the controls always sit at the bottom.
        const float controlsHeight = 90.0f;
        float treeHeight = ImGui::GetContentRegionAvail().y - controlsHeight;
        if (treeHeight < 100.0f) treeHeight = 100.0f;

        ImGui::BeginChild("##CacheTree", ImVec2(0, treeHeight), true);
        RenderTree();
        ImGui::EndChild();

        ImGui::Separator();
        RenderControls();
    }

    // =========================================================================
    // Tree
    // =========================================================================
    void ModelCacheView::RenderTree() {
        size_t ctxMem = 0;
        for (const auto& d : contextDetails) ctxMem += d.memoryBytes;
        size_t upMem = 0;
        for (const auto& d : upscalerDetails) upMem += d.memoryBytes;

        ImGuiTreeNodeFlags groupFlags =
            ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth;

        // ---- Generation Contexts ----
        std::string ctxLabel = "Generation Contexts (" + std::to_string(contextDetails.size()) + ")";
        if (ImGui::TreeNodeEx("##ctxGroup", groupFlags, "%s", ctxLabel.c_str())) {
            if (contextDetails.empty()) {
                ImGui::TextDisabled("  (none)");
            }
            else {
                ImGui::TextDisabled("  Total memory: %s", FormatMemory(ctxMem).c_str());
                for (const auto& d : contextDetails) RenderContextLeaf(d);
            }
            ImGui::TreePop();
        }

        ImGui::Spacing();

        // ---- Upscalers ----
        std::string upLabel = "Upscalers (" + std::to_string(upscalerDetails.size()) + ")";
        if (ImGui::TreeNodeEx("##upGroup", groupFlags, "%s", upLabel.c_str())) {
            if (upscalerDetails.empty()) {
                ImGui::TextDisabled("  (none)");
            }
            else {
                ImGui::TextDisabled("  Total memory: %s", FormatMemory(upMem).c_str());
                for (const auto& d : upscalerDetails) RenderUpscalerLeaf(d);
            }
            ImGui::TreePop();
        }
    }

    void ModelCacheView::RenderContextLeaf(const ECS::ContextDetail& d) {
        ImGui::PushID(d.key.c_str());

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf
            | ImGuiTreeNodeFlags_NoTreePushOnOpen
            | ImGuiTreeNodeFlags_SpanAvailWidth;
        if (selectedContextKey == d.key) flags |= ImGuiTreeNodeFlags_Selected;

        std::string label = ExtractDisplayName(d.key)
            + "  [" + GetModelType(d) + "]"
            + "  " + FormatMemory(d.memoryBytes);

        bool opened = ImGui::TreeNodeEx("##leaf", flags, "%s", label.c_str());
        (void)opened;

        if (ImGui::IsItemClicked()) {
            selectedContextKey = d.key;
            selectedUpscalerKey.clear();
        }

        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("Key: %s", d.key.c_str());
            ImGui::Text("Type: %s", GetModelType(d).c_str());
            ImGui::Text("Memory: %s", FormatMemory(d.memoryBytes).c_str());
            if (d.activeCount > 0)
                ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f),
                    "Status: In Use (%d)", d.activeCount);
            else
                ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "Status: Loaded");
            ImGui::EndTooltip();
        }

        // Right-click context menu
        if (ImGui::BeginPopupContextItem("##ctxMenu")) {
            selectedContextKey = d.key;
            selectedUpscalerKey.clear();

            if (ImGui::MenuItem("Reload", nullptr, false, d.activeCount == 0)) {
                auto cache = m_entityManager.GetSystem<ECS::ModelCacheSystem>();
                if (cache) cache->reloadModel(d.key);
                RefreshLists();
            }
            if (ImGui::MenuItem("Unload", nullptr, false, d.activeCount == 0)) {
                auto cache = m_entityManager.GetSystem<ECS::ModelCacheSystem>();
                if (cache) cache->UnloadModel(d.key);
                RefreshLists();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Copy Key")) {
                ImGui::SetClipboardText(d.key.c_str());
            }
            if (ImGui::MenuItem("Details...")) {
                ShowDetailsForContext(d.key);
            }
            ImGui::EndPopup();
        }

        ImGui::PopID();
    }

    void ModelCacheView::RenderUpscalerLeaf(const ECS::UpscalerDetail& d) {
        ImGui::PushID(d.key.c_str());

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf
            | ImGuiTreeNodeFlags_NoTreePushOnOpen
            | ImGuiTreeNodeFlags_SpanAvailWidth;
        if (selectedUpscalerKey == d.key) flags |= ImGuiTreeNodeFlags_Selected;

        std::string label = ExtractDisplayName(d.key)
            + "  " + FormatMemory(d.memoryBytes);

        bool opened = ImGui::TreeNodeEx("##leaf", flags, "%s", label.c_str());
        (void)opened;

        if (ImGui::IsItemClicked()) {
            selectedUpscalerKey = d.key;
            selectedContextKey.clear();
        }

        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("Key: %s", d.key.c_str());
            ImGui::Text("Memory: %s", FormatMemory(d.memoryBytes).c_str());
            if (d.activeCount > 0)
                ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f),
                    "Status: In Use (%d)", d.activeCount);
            else
                ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "Status: Loaded");
            ImGui::EndTooltip();
        }

        if (ImGui::BeginPopupContextItem("##upMenu")) {
            selectedUpscalerKey = d.key;
            selectedContextKey.clear();

            if (ImGui::MenuItem("Reload", nullptr, false, d.activeCount == 0)) {
                auto cache = m_entityManager.GetSystem<ECS::ModelCacheSystem>();
                if (cache) cache->reloadUpscaler(d.key);
                RefreshLists();
            }
            if (ImGui::MenuItem("Unload", nullptr, false, d.activeCount == 0)) {
                auto cache = m_entityManager.GetSystem<ECS::ModelCacheSystem>();
                if (cache) cache->UnloadUpscaler(d.key);
                RefreshLists();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Copy Key")) {
                ImGui::SetClipboardText(d.key.c_str());
            }
            if (ImGui::MenuItem("Details...")) {
                ShowDetailsForUpscaler(d.key);
            }
            ImGui::EndPopup();
        }

        ImGui::PopID();
    }

    // =========================================================================
    // Controls
    // =========================================================================
    void ModelCacheView::RenderControls() {
        auto cacheSystem = m_entityManager.GetSystem<ECS::ModelCacheSystem>();
        if (!cacheSystem) return;

        // ---- Row 1: Load from entity ----
        ImGui::Text("Load Model from Entity");
        ImGui::SetNextItemWidth(120.0f);
        ImGui::InputInt("##entityId", reinterpret_cast<int*>(&m_loadEntityId));
        ImGui::SameLine();
        if (ImGui::Button("Load")) {
            if (m_loadEntityId != 0 && m_entityManager.IsEntityValid(m_loadEntityId)) {
                bool ok = SDCPP::PreloadEntity(m_entityManager, m_loadEntityId, *cacheSystem);
                if (!ok) {
                    ImGui::OpenPopup("LoadError");
                }
                RefreshLists();
            }
        }
        ImGui::SameLine();
        HelpMarker("Enter an Entity ID that has model components, then click Load to preload it into the cache.");

        if (ImGui::BeginPopup("LoadError")) {
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Load failed");
            ImGui::Separator();
            ImGui::TextWrapped("%s", cacheSystem->getLastError().c_str());
            ImGui::EndPopup();
        }

        ImGui::Spacing();

        // ---- Row 2: Actions ----
        bool hasCtxSel = !selectedContextKey.empty();
        bool hasUpSel = !selectedUpscalerKey.empty();
        bool hasSel = hasCtxSel || hasUpSel;

        if (!hasSel) ImGui::BeginDisabled();
        if (ImGui::Button("Unload Selected")) {
            ShowConfirm(hasCtxSel ? ConfirmAction::UnloadSelectedContext
                : ConfirmAction::UnloadSelectedUpscaler,
                "Unload the selected entry? If it is currently in use, "
                "it will be marked for unload and freed when the last "
                "handle releases.");
        }
        if (!hasSel) ImGui::EndDisabled();
        ImGui::SameLine();

        if (ImGui::Button("Unload All Inactive")) {
            ShowConfirm(ConfirmAction::UnloadAll,
                "Unload all inactive contexts and upscalers?\n"
                "Entries currently in use will be preserved.");
        }
        ImGui::SameLine();

        if (ImGui::Button("Force Unload All")) {
            ShowConfirm(ConfirmAction::ClearAll,
                "Force unload ALL contexts and upscalers?\n"
                "This includes entries currently in use.\n"
                "Use with caution!");
        }
        ImGui::SameLine();

        if (ImGui::Button("Refresh")) {
            RefreshLists();
        }
        ImGui::SameLine();
        HelpMarker("Refresh the cache list. Also refreshes automatically every 2s.");

        // ---- Row 3: summary ----
        size_t ctxMem = 0;
        for (const auto& d : contextDetails) ctxMem += d.memoryBytes;
        size_t upMem = 0;
        for (const auto& d : upscalerDetails) upMem += d.memoryBytes;

        int ctxActive = 0;
        for (const auto& d : contextDetails) if (d.activeCount > 0) ctxActive++;

        ImGui::Spacing();
        ImGui::TextDisabled(
            "Contexts: %zu (%d active, %s) | Upscalers: %zu (%s)",
            contextDetails.size(), ctxActive, FormatMemory(ctxMem).c_str(),
            upscalerDetails.size(), FormatMemory(upMem).c_str());
    }

    // =========================================================================
    // Confirmation dialog
    // =========================================================================
    void ModelCacheView::ShowConfirm(ConfirmAction a, const std::string& msg) {
        confirmAction = a;
        confirmMessage = msg;
        showConfirmDialog = true;
    }

    void ModelCacheView::RenderConfirmationDialog() {
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(420, 0), ImGuiCond_Appearing);
        if (ImGui::Begin("Confirm Action", &showConfirmDialog,
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize)) {
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

    void ModelCacheView::ExecuteConfirmedAction() {
        auto cache = m_entityManager.GetSystem<ECS::ModelCacheSystem>();
        if (!cache) return;

        switch (confirmAction) {
        case ConfirmAction::ClearAll:
            cache->UnloadAllModels();
            break;
        case ConfirmAction::UnloadAll:
            cache->UnloadInactiveModels();
            break;
        case ConfirmAction::UnloadSelectedContext:
            if (!selectedContextKey.empty())
                cache->UnloadModel(selectedContextKey);
            break;
        case ConfirmAction::UnloadSelectedUpscaler:
            if (!selectedUpscalerKey.empty())
                cache->UnloadUpscaler(selectedUpscalerKey);
            break;
        case ConfirmAction::None:
        default:
            break;
        }
        RefreshLists();
    }

    // =========================================================================
    // Details dialog
    // =========================================================================
    void ModelCacheView::ShowDetailsForContext(const std::string& key) {
        auto cache = m_entityManager.GetSystem<ECS::ModelCacheSystem>();
        if (!cache) return;

        detailsTitle = "Context Details";
        detailsRows.clear();
        detailsRows.emplace_back("Key", key);

        for (const auto& d : contextDetails) {
            if (d.key == key) {
                detailsRows.emplace_back("Display Name", d.displayName);
                detailsRows.emplace_back("Model Type", d.modelType);
                detailsRows.emplace_back("Memory", FormatMemory(d.memoryBytes));
                detailsRows.emplace_back("Active Count", std::to_string(d.activeCount));
                detailsRows.emplace_back("In Use", d.isInUse ? "yes" : "no");
                break;
            }
        }

        showDetailsDialog = true;
    }

    void ModelCacheView::ShowDetailsForUpscaler(const std::string& key) {
        detailsTitle = "Upscaler Details";
        detailsRows.clear();
        detailsRows.emplace_back("Key", key);

        for (const auto& d : upscalerDetails) {
            if (d.key == key) {
                detailsRows.emplace_back("Display Name", d.displayName);
                detailsRows.emplace_back("Memory", FormatMemory(d.memoryBytes));
                detailsRows.emplace_back("Active Count", std::to_string(d.activeCount));
                detailsRows.emplace_back("In Use", d.isInUse ? "yes" : "no");
                break;
            }
        }

        showDetailsDialog = true;
    }

    void ModelCacheView::RenderDetailsDialog() {
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(500, 300), ImGuiCond_Appearing);
        if (ImGui::Begin(detailsTitle.c_str(), &showDetailsDialog,
            ImGuiWindowFlags_NoCollapse)) {

            if (ImGui::BeginTable("##details", 2,
                ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
                ImGui::TableSetupColumn("Field", ImGuiTableColumnFlags_WidthFixed, 140.0f);
                ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableHeadersRow();

                for (const auto& [k, v] : detailsRows) {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(k.c_str());
                    ImGui::TableNextColumn();
                    ImGui::TextWrapped("%s", v.c_str());
                }
                ImGui::EndTable();
            }

            ImGui::Spacing();
            if (ImGui::Button("Copy All")) {
                std::string all;
                for (const auto& [k, v] : detailsRows) {
                    all += k + ": " + v + "\n";
                }
                ImGui::SetClipboardText(all.c_str());
            }
            ImGui::SameLine();
            if (ImGui::Button("Close")) {
                showDetailsDialog = false;
            }
            ImGui::End();
        }
    }

    // =========================================================================
    // Helpers
    // =========================================================================
    std::string ModelCacheView::ExtractDisplayName(const std::string& key) const {
        // Keys look like "model=C:\foo\bar.safetensors|vae=...|". Extract
        // the first "name=value" pair's value, then take the filename.
        size_t eq = key.find('=');
        size_t pipe = key.find('|');
        if (eq != std::string::npos && pipe != std::string::npos && pipe > eq) {
            std::string first = key.substr(eq + 1, pipe - eq - 1);
            std::filesystem::path p(first);
            return p.filename().string();
        }
        if (eq != std::string::npos) {
            std::filesystem::path p(key.substr(eq + 1));
            return p.filename().string();
        }
        return key;
    }

    std::string ModelCacheView::FormatMemory(size_t bytes) const {
        const char* units[] = { "B", "KB", "MB", "GB", "TB" };
        int unitIdx = 0;
        double size = static_cast<double>(bytes);
        while (size >= 1024.0 && unitIdx < 4) {
            size /= 1024.0;
            unitIdx++;
        }
        char buf[64];
        snprintf(buf, sizeof(buf), "%.1f %s", size, units[unitIdx]);
        return std::string(buf);
    }

    std::string ModelCacheView::GetModelType(const ECS::ContextDetail& d) const {
        return d.modelType.empty() ? "Unknown" : d.modelType;
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