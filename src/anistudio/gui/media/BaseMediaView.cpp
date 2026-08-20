#include "BaseMediaView.hpp"
#include "ClipboardUtilities.hpp"
#include "ImageHistoryView.hpp"
#include "VideoHistoryView.hpp"
#include "DragDropUtils.hpp"
#include <imgui.h>
#include <algorithm>
#include <iostream>

namespace GUI {

    BaseMediaView::BaseMediaView(ECS::EntityManager& mgr, ViewManager& vm)
        : BaseView(mgr, vm), selectedEntityID(0), index(0), zoom(1.0f), offsetX(0.0f), offsetY(0.0f),
        lastEntityCount(0), historyViewVisible(false), historyWorkspaceID(0),
        contextMenuUtils(std::make_unique<Utils::ContextMenuUtils>(mgr)), isDragging(false) {
    }

    BaseMediaView::~BaseMediaView() {
        if (historyViewVisible && historyWorkspaceID != 0) {
            GetViewManager().DestroyView(historyWorkspaceID);
        }
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
        }
        else {
            index = 0;
            selectedEntityID = mediaEntities.empty() ? 0 : mediaEntities[0];
        }
    }

    void BaseMediaView::UpdateSelectionAfterRemoval(ECS::EntityID removedEntity) {
        if (selectedEntityID == removedEntity) {
            if (!mediaEntities.empty()) {
                int newIndex = std::min(index, static_cast<int>(mediaEntities.size()) - 1);
                if (newIndex < 0) newIndex = 0;
                selectedEntityID = mediaEntities[newIndex];
                index = newIndex;
            }
            else {
                selectedEntityID = 0;
                index = 0;
            }
        }
    }

    void BaseMediaView::ToggleHistoryView(bool show) {
        if (show && !historyViewVisible) {
            std::string viewType = GetHistoryViewTypeName();
            if (viewType.empty()) return;
            historyWorkspaceID = GetViewManager().CreateViewByName(viewType, m_entityManager);
            if (historyWorkspaceID != 0) {
                historyViewVisible = true;
                BaseView* historyView = &GetViewManager().GetView<BaseView>(historyWorkspaceID);
                if (historyView) {
                    if (auto* imgHist = dynamic_cast<ImageHistoryView*>(historyView)) {
                        imgHist->SetParentViewWorkspace(GetID());
                    }
                    else if (auto* vidHist = dynamic_cast<VideoHistoryView*>(historyView)) {
                        vidHist->SetParentViewWorkspace(GetID());
                    }
                }
            }
        }
        else if (!show && historyViewVisible && historyWorkspaceID != 0) {
            GetViewManager().DestroyView(historyWorkspaceID);
            historyWorkspaceID = 0;
            historyViewVisible = false;
        }
    }

    bool BaseMediaView::IsHistoryVisible() const {
        return historyViewVisible;
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
                            LoadMedia({ comp.filePath });
                        }
                    }
                    else if (m_entityManager.HasComponent<ECS::VideoComponent>(entity)) {
                        auto& comp = m_entityManager.GetComponent<ECS::VideoComponent>(entity);
                        if (!comp.filePath.empty()) {
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

} // namespace GUI