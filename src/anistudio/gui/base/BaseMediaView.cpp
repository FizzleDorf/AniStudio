#include "BaseMediaView.hpp"
#include "ClipboardUtilities.hpp"
#include "ImageHistoryView.hpp"
#include "VideoHistoryView.hpp"
#include <imgui.h>
#include <algorithm>
#include <iostream>

namespace GUI {

    BaseMediaView::BaseMediaView(ECS::EntityManager& mgr, ViewManager& vm)
        : BaseView(mgr, vm), selectedEntityID(0), index(0), zoom(1.0f), offsetX(0.0f), offsetY(0.0f),
        lastEntityCount(0), historyViewVisible(false), historyWorkspaceID(0) {
    }

    BaseMediaView::~BaseMediaView() {
        if (historyViewVisible && historyWorkspaceID != 0) {
            viewManager.DestroyView(historyWorkspaceID);
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
            historyWorkspaceID = viewManager.CreateViewByName(viewType, m_entityManager);
            if (historyWorkspaceID != 0) {
                historyViewVisible = true;
                auto* historyView = viewManager.GetView<BaseView>(historyWorkspaceID);
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
            viewManager.DestroyView(historyWorkspaceID);
            historyWorkspaceID = 0;
            historyViewVisible = false;
        }
    }

    bool BaseMediaView::IsHistoryVisible() const {
        return historyViewVisible;
    }

    void BaseMediaView::RenderImageContextMenu(ECS::EntityID entityID) {
        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            ImGui::OpenPopup(("ContextMenu_" + std::to_string(entityID)).c_str());
        }
        if (ImGui::BeginPopup(("ContextMenu_" + std::to_string(entityID)).c_str())) {
            if (ImGui::MenuItem("Copy Entity")) {
                GUI::Clipboard::CopyEntity(m_entityManager, entityID);
            }
            if (ImGui::MenuItem("Copy Component")) {
                GUI::Clipboard::CopyComponent(m_entityManager, entityID, "ImageComponent");
            }
            if (ImGui::MenuItem("Copy Image Metadata")) {
                const auto& imageComp = m_entityManager.GetComponent<ECS::ImageComponent>(entityID);
                GUI::Clipboard::CopyImageMetadata(m_entityManager, imageComp.filePath);
            }
            ImGui::EndPopup();
        }
    }

} // namespace GUI