#include "BaseMediaView.hpp"
#include "ClipboardUtilities.hpp"
#include "MediaHistoryView.hpp"
#include "MetadataView.hpp"
#include "DragDropUtils.hpp"
#include "ViewManager.hpp"
#include "IconFonts.hpp"
#include "Log.hpp"

#include <imgui.h>
#include <algorithm>

namespace GUI {

    BaseMediaView::BaseMediaView(ECS::EntityManager& mgr, ViewManager& vm)
        : BaseView(mgr, vm), selectedEntityID(0), index(0), zoom(1.0f), offsetX(0.0f), offsetY(0.0f),
        lastEntityCount(0), contextMenuUtils(std::make_unique<Utils::ContextMenuUtils>(mgr)),
        isDragging(false) {
        ANI_LOG_DEBUG("Constructed");
    }

    BaseMediaView::~BaseMediaView() {
    }

    void BaseMediaView::SetZoom(float newZoom) {
        zoom = std::clamp(newZoom, 0.1f, 5.0f);
    }

    void BaseMediaView::DrawGrid(int width, int height) {
        if (width <= 0 || height <= 0) return;
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        const float gridStep = 100.0f * zoom;
        const ImVec2 windowPos = ImGui::GetWindowPos();
        const ImVec2 windowSize = ImGui::GetWindowSize();
        const ImVec2 contentMin = windowPos;
        const ImVec2 contentMax = ImVec2(windowPos.x + windowSize.x, windowPos.y + windowSize.y);
        float startX = contentMin.x - fmodf(ImGui::GetScrollX(), gridStep);
        float startY = contentMin.y - fmodf(ImGui::GetScrollY(), gridStep);
        for (float x = startX; x < contentMax.x; x += gridStep) {
            draw_list->AddLine(ImVec2(x, contentMin.y), ImVec2(x, contentMax.y), IM_COL32(255, 255, 255, 50));
        }
        for (float y = startY; y < contentMax.y; y += gridStep) {
            draw_list->AddLine(ImVec2(contentMin.x, y), ImVec2(contentMax.x, y), IM_COL32(255, 255, 255, 50));
        }
    }

    std::string BaseMediaView::TruncateFilename(const std::string& filename, float maxTextWidth) {
        if (filename.empty()) return "Unknown";
        float textWidth = ImGui::CalcTextSize(filename.c_str()).x;
        if (textWidth <= maxTextWidth) return filename;
        std::string truncated = "...";
        for (int i = static_cast<int>(filename.length()) - 1; i >= 0; --i) {
            truncated.insert(3, 1, filename[i]);
            if (ImGui::CalcTextSize(truncated.c_str()).x > maxTextWidth) {
                truncated.erase(3, 1);
                return truncated;
            }
        }
        return truncated;
    }

    ECS::EntityID BaseMediaView::GetSelectedEntity() const {
        return selectedEntityID;
    }

    void BaseMediaView::SetSelectedEntity(ECS::EntityID entity) {
        selectedEntityID = entity;
        auto it = std::find(mediaEntities.begin(), mediaEntities.end(), entity);
        if (it != mediaEntities.end()) {
            index = static_cast<int>(std::distance(mediaEntities.begin(), it));
            ANI_LOG_TRACE("Selected entity %u at index %d", entity, index);
        }
        else {
            index = 0;
            selectedEntityID = mediaEntities.empty() ? 0 : mediaEntities[0];
            if (selectedEntityID == 0) {
                ANI_LOG_TRACE("SetSelectedEntity: entity %u not found, list empty", entity);
            }
            else {
                ANI_LOG_TRACE("SetSelectedEntity: entity %u not in list, fell back to %u",
                    entity, selectedEntityID);
            }
        }
    }

    void BaseMediaView::UpdateSelectionAfterRemoval(ECS::EntityID removedEntity) {
        if (selectedEntityID == removedEntity) {
            if (!mediaEntities.empty()) {
                int newIndex = std::min(index, static_cast<int>(mediaEntities.size()) - 1);
                if (newIndex < 0) newIndex = 0;
                selectedEntityID = mediaEntities[newIndex];
                index = newIndex;
                ANI_LOG_TRACE("Selection moved to entity %u after removal of %u",
                    selectedEntityID, removedEntity);
            }
            else {
                selectedEntityID = 0;
                index = 0;
                ANI_LOG_TRACE("Selection cleared after removal of %u", removedEntity);
            }
        }
    }

    void BaseMediaView::ToggleHistoryView(bool show) {
        auto& vm = GetViewManager();
        std::string viewType = GetHistoryViewTypeName();
        if (viewType.empty()) {
            ANI_LOG_WARN("ToggleHistoryView: empty history view type name");
            return;
        }

        bool exists = IsHistoryVisible();

        if (show && !exists) {
            try {
                ViewTypeID histType = vm.GetViewType(viewType);
                vm.AddViewByType(GetID(), histType);
                ANI_LOG_DEBUG("Added history view: %s", viewType.c_str());
            }
            catch (const std::exception& e) {
                ANI_LOG_ERROR("Failed to add history view: %s", e.what());
            }
        }
        else if (!show && exists) {
            try {
                ViewTypeID histType = vm.GetViewType(viewType);
                vm.RemoveViewByType(GetID(), histType);
                ANI_LOG_DEBUG("Removed history view: %s", viewType.c_str());
            }
            catch (const std::exception& e) {
                ANI_LOG_ERROR("Failed to remove history view: %s", e.what());
            }
        }
    }

    void BaseMediaView::RenderMediaContextMenu(ECS::EntityID entityID) {
        if (entityID != 0 && m_entityManager.IsEntityValid(entityID)) {
            contextMenuUtils->RenderEntityContextMenu(entityID);
        }
    }

    void BaseMediaView::RenderMediaContextMenuForPath(const std::string& filePath) {
        std::string ext = std::filesystem::path(filePath).extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (GUI::DragDrop::IsImageFile(filePath)) {
            contextMenuUtils->RenderImageContextMenuWithPath(filePath);
        }
        else if (GUI::DragDrop::IsVideoFile(filePath)) {
            contextMenuUtils->RenderVideoContextMenuWithPath(filePath);
        }
    }

    void BaseMediaView::HandleFileDropTarget() {
        std::vector<std::string> files;
        if (GUI::DragDrop::AcceptFileDrop(files)) {
            if (!files.empty()) {
                ANI_LOG_DEBUG("File drop accepted (%zu file(s))", files.size());
                LoadMedia(files);
            }
        }
    }

    void BaseMediaView::HandleEntityDropTarget() {
        ECS::EntityID droppedEntity;
        if (GUI::DragDrop::AcceptEntityDrop(droppedEntity)) {
            if (m_entityManager.IsEntityValid(droppedEntity)) {
                if (m_entityManager.HasComponent<ECS::ImageComponent>(droppedEntity) ||
                    m_entityManager.HasComponent<ECS::VideoComponent>(droppedEntity)) {
                    SetSelectedEntity(droppedEntity);
                    ANI_LOG_TRACE("Entity drop selected entity %u", droppedEntity);
                }
            }
        }
    }

    void BaseMediaView::HandleClipboardPaste() {
        if (!ImGui::IsWindowFocused()) return;
        ImGuiIO& io = ImGui::GetIO();
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_V)) {
            std::vector<std::string> filePaths;
            if (Clipboard::PasteMediaFromClipboard(filePaths)) {
                if (!filePaths.empty()) {
                    ANI_LOG_DEBUG("Pasted %zu file path(s) from clipboard", filePaths.size());
                    LoadMedia(filePaths);
                    return;
                }
            }
            if (Clipboard::HasEntity()) {
                ECS::EntityID entity = Clipboard::GetCopiedEntity();
                if (entity != 0 && m_entityManager.IsEntityValid(entity)) {
                    if (m_entityManager.HasComponent<ECS::ImageComponent>(entity)) {
                        auto& comp = m_entityManager.GetComponent<ECS::ImageComponent>(entity);
                        if (!comp.filePath.empty()) {
                            ANI_LOG_DEBUG("Pasted image entity %u", entity);
                            LoadMedia({ comp.filePath });
                        }
                    }
                    else if (m_entityManager.HasComponent<ECS::VideoComponent>(entity)) {
                        auto& comp = m_entityManager.GetComponent<ECS::VideoComponent>(entity);
                        if (!comp.filePath.empty()) {
                            ANI_LOG_DEBUG("Pasted video entity %u", entity);
                            LoadMedia({ comp.filePath });
                        }
                    }
                }
            }
        }
    }

    bool BaseMediaView::IsAltKeyDown() const {
        ImGuiIO& io = ImGui::GetIO();
        return io.KeyAlt;
    }

    void BaseMediaView::SendSelectedToMetadataView() {
        std::string filePath = GetSelectedFilePath();
        if (filePath.empty()) {
            ANI_LOG_WARN("SendSelectedToMetadataView: no file selected");
            return;
        }

        auto& vm = GetViewManager();
        WorkspaceID currentWorkspace = GetID();

        bool hasMetadataView = false;
        const auto& signatures = vm.GetWorkspaceSignatures();
        auto sigIt = signatures.find(currentWorkspace);
        if (sigIt != signatures.end()) {
            try {
                ViewTypeID metaType = vm.GetViewType("MetadataView");
                if (sigIt->second->count(metaType) > 0) {
                    hasMetadataView = true;
                }
            }
            catch (...) {}
        }

        if (!hasMetadataView) {
            try {
                ViewTypeID metaType = vm.GetViewType("MetadataView");
                vm.AddViewByType(currentWorkspace, metaType);
                ANI_LOG_DEBUG("Added MetadataView to workspace %zu", currentWorkspace);
            }
            catch (const std::exception& e) {
                ANI_LOG_ERROR("Failed to add MetadataView: %s", e.what());
                return;
            }
        }

        auto& ws = vm.GetWorkspaces();
        auto wsIt = ws.find(currentWorkspace);
        if (wsIt != ws.end()) {
            for (auto& [typeID, viewPtr] : wsIt->second) {
                if (auto* metaView = dynamic_cast<MetadataView*>(viewPtr.get())) {
                    try {
                        metaView->LoadFromFile(filePath);
                        ANI_LOG_DEBUG("Sent file to MetadataView: %s", filePath.c_str());
                    }
                    catch (const std::exception& e) {
                        ANI_LOG_ERROR("Failed to load metadata for %s: %s",
                            filePath.c_str(), e.what());
                    }
                    return;
                }
            }
        }
        ANI_LOG_WARN("MetadataView not found in workspace after adding");
    }

    void BaseMediaView::RenderToolbar() {
    }

    void BaseMediaView::RenderControls() {
    }

    void BaseMediaView::RenderSelector() {
    }

} // namespace GUI