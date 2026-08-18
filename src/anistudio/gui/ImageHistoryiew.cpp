#include "ImageHistoryView.hpp"
#include "Events.hpp"
#include <imgui.h>
#include <algorithm>
#include <iostream>

namespace GUI {

    ImageHistoryView::ImageHistoryView(ECS::EntityManager& mgr, ViewManager& vm)
        : BaseView(mgr, vm), imageSystem(nullptr), parentWorkspaceID(0), selectedEntityID(0) {
        viewName = "ImageHistoryView";
    }

    void ImageHistoryView::Init() {
        imageSystem = m_entityManager.GetSystem<ECS::ImageSystem>();
        if (!imageSystem) {
            m_entityManager.RegisterSystem<ECS::ImageSystem>();
            imageSystem = m_entityManager.GetSystem<ECS::ImageSystem>();
        }
        if (imageSystem) {
            imageSystem->RegisterImageAddedCallback([this](ECS::EntityID entity) { OnImageAdded(entity); });
            imageSystem->RegisterImageRemovedCallback([this](ECS::EntityID entity) { OnImageRemoved(entity); });
        }
        RefreshEntities();
    }

    void ImageHistoryView::Update(float deltaT) {}

    void ImageHistoryView::Render() {
        ImGui::Begin("Image History", &windowOpen);
        if (imageEntities.empty()) {
            ImGui::Text("No images loaded.");
            ImGui::End();
            return;
        }
        float currentRowWidth = 0.0f;
        for (size_t i = 0; i < imageEntities.size(); ++i) {
            ECS::EntityID entityID = imageEntities[i];
            if (!m_entityManager.IsEntityValid(entityID) || !m_entityManager.HasComponent<ECS::ImageComponent>(entityID))
                continue;
            const auto& imageComp = m_entityManager.GetComponent<ECS::ImageComponent>(entityID);
            ImGui::BeginGroup();
            if (entityID == selectedEntityID) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
            }
            float aspectRatio = (imageComp.width > 0 && imageComp.height > 0) ?
                static_cast<float>(imageComp.width) / static_cast<float>(imageComp.height) : 1.0f;
            ImVec2 maxSize(128.0f, 128.0f);
            ImVec2 imageSize;
            if (aspectRatio > 1.0f) imageSize = ImVec2(maxSize.x, maxSize.x / aspectRatio);
            else imageSize = ImVec2(maxSize.y * aspectRatio, maxSize.y);
            std::string label = std::to_string(i) + ": " + imageComp.fileName;
            ImGui::Text("%s", label.c_str());
            if (entityID == selectedEntityID) ImGui::PopStyleColor();
            if (imageComp.textureID != 0) {
                if (ImGui::ImageButton(("##img" + std::to_string(i)).c_str(),
                    (ImTextureID)(intptr_t)imageComp.textureID, imageSize)) {
                    SelectImage(entityID);
                }
            }
            else {
                if (ImGui::Button(("Select##" + std::to_string(i)).c_str(), imageSize)) {
                    SelectImage(entityID);
                }
            }
            ImGui::EndGroup();
            float buttonWidth = imageSize.x + ImGui::GetStyle().ItemSpacing.x;
            currentRowWidth += buttonWidth;
            if (i < imageEntities.size() - 1) {
                float nextButtonWidth = std::min(maxSize.x, maxSize.y * aspectRatio) + ImGui::GetStyle().ItemSpacing.x;
                if (currentRowWidth + nextButtonWidth > ImGui::GetContentRegionAvail().x) {
                    ImGui::NewLine();
                    currentRowWidth = 0.0f;
                }
                else {
                    ImGui::SameLine();
                }
            }
        }
        ImGui::End();
        if (!windowOpen) {
            std::unordered_map<std::string, std::any> eventData;
            eventData["workspaceID"] = GetID();
            eventData["viewTypeName"] = viewName;
            ANI::Events::Ref().QueueEventWithData("RemoveView", eventData);
        }
    }

    void ImageHistoryView::RefreshEntities() {
        imageEntities.clear();
        if (!imageSystem) return;
        for (auto entityID : imageSystem->GetAllImageEntities()) {
            imageEntities.push_back(entityID);
        }
    }

    void ImageHistoryView::OnImageAdded(ECS::EntityID entity) {
        RefreshEntities();
    }

    void ImageHistoryView::OnImageRemoved(ECS::EntityID entity) {
        RefreshEntities();
        if (selectedEntityID == entity) {
            selectedEntityID = imageEntities.empty() ? 0 : imageEntities[0];
        }
    }

    void ImageHistoryView::SelectImage(ECS::EntityID entity) {
        selectedEntityID = entity;
        if (parentWorkspaceID != 0) {
            std::unordered_map<std::string, std::any> eventData;
            eventData["workspaceID"] = parentWorkspaceID;
            eventData["entityID"] = entity;
            ANI::Events::Ref().QueueEventWithData("SelectMediaEntity", eventData);
        }
    }

    void ImageHistoryView::SetParentViewWorkspace(WorkspaceID parentWorkspace) {
        parentWorkspaceID = parentWorkspace;
    }

} // namespace GUI