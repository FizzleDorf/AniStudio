#include "DebugView.hpp"
#include "Events.hpp"
#include "NetClient.hpp"

#include <imgui.h>

namespace GUI {

    void DebugView::Init() {
        RefreshEntities();
    }

    void DebugView::RefreshEntities() {
        entities = m_entityManager.GetAllEntities();
        entityIndex = entities.empty() ? -1 : static_cast<int>(entities.size()) - 1;
    }

    void DebugView::Render() {
        if (m_netClient) {
            RenderClientPanel();
            return;
        }

        RenderEntityPanel();
        RenderSystemPanel();
    }

    void DebugView::RenderClientPanel() {
        auto& client = *m_netClient;

        if (ImGui::Begin(GetWindowTitle().c_str(), &windowOpen)) {
            ImGui::TextColored(ImVec4(0.5f, 0.9f, 1.0f, 1.0f), "NETWORKED CLIENT");
            ImGui::Separator();

            uint64_t localSession = 0;
            uint64_t projectID = 0;
            size_t   entityCount = 0;
            std::vector<std::pair<uint64_t, std::pair<float, float>>> presences;

            {
                std::lock_guard<std::mutex> lk(client.Mirror().mtx);
                localSession = client.Mirror().localSessionID;
                projectID = client.Mirror().projectID;
                entityCount = client.Mirror().components.size();
                presences.reserve(client.Mirror().presences.size());
                for (auto& [sid, p] : client.Mirror().presences) {
                    presences.emplace_back(sid, std::make_pair(p.x, p.y));
                }
            }

            ImGui::Text("Local session: %llu", (unsigned long long)localSession);
            ImGui::Text("Project:       %llu", (unsigned long long)projectID);
            ImGui::Text("Entities seen: %zu", entityCount);

            ImGui::Separator();
            ImGui::Text("Presences (%zu):", presences.size());

            if (ImGui::BeginChild("PresenceList", ImVec2(0, 120), true)) {
                for (auto& [sid, pos] : presences) {
                    const bool isSelf = (sid == localSession);
                    if (isSelf) {
                        ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f),
                            "Session %llu (you)  ->  (%.1f, %.1f)",
                            (unsigned long long)sid, pos.first, pos.second);
                    }
                    else {
                        ImGui::Text("Session %llu        ->  (%.1f, %.1f)",
                            (unsigned long long)sid, pos.first, pos.second);
                    }
                }
            }
            ImGui::EndChild();

            ImGui::Separator();
            ImGui::Text("Send cursor update:");

            static float x = 0.f, y = 0.f;
            bool moved = false;
            moved |= ImGui::SliderFloat("X", &x, 0.f, 1920.f);
            moved |= ImGui::SliderFloat("Y", &y, 0.f, 1080.f);
            if (moved) {
                client.SendMoveCursor(x, y);
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

    void DebugView::RenderEntityPanel() {
        if (ImGui::Begin(GetWindowTitle().c_str(), &windowOpen)) {

            if (ImGui::Button("Refresh Entities")) {
                RefreshEntities();
            }

            ImGui::SameLine();
            ImGui::Text("Total Entities: %zu", entities.size());

            ImGui::Separator();

            if (ImGui::BeginChild("EntityList", ImVec2(0, -200), true)) {
                for (size_t i = 0; i < entities.size(); ++i) {
                    ECS::EntityID entity = entities[i];
                    bool isSelected = (entity == selectedEntity);

                    if (ImGui::Selectable((std::string("Entity ") + std::to_string(entity)).c_str(), isSelected)) {
                        selectedEntity = entity;
                        entityIndex = static_cast<int>(i);
                    }

                    if (ImGui::TreeNode((std::string("Entity Details: ") + std::to_string(entity)).c_str())) {

                        auto components = m_entityManager.GetEntityComponents(entity);

                        ImGui::Text("Components (%zu):", components.size());
                        ImGui::Indent();

                        for (auto compType : components) {
                            std::string componentName = m_entityManager.GetComponentNameById(compType);

                            bool isPluginComponent = m_entityManager.HasPluginComponent(entity, compType);

                            if (isPluginComponent) {
                                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 1.0f, 0.5f, 1.0f));
                                ImGui::Text("[PLUGIN] %s (ID: %u)", componentName.c_str(), compType);
                                ImGui::PopStyleColor();

                                void* pluginComponent = m_entityManager.GetPluginComponent(entity, compType);
                                if (pluginComponent) {
                                    if (componentName == "ExampleComponent") {
                                        struct ExampleComponentData {
                                            ECS::EntityID entityID;
                                            std::string message;
                                            float value;
                                        };

                                        ExampleComponentData* exampleComp = static_cast<ExampleComponentData*>(pluginComponent);
                                        ImGui::Indent();
                                        ImGui::Text("Entity ID: %zu", exampleComp->entityID);
                                        ImGui::Text("Message: %s", exampleComp->message.c_str());
                                        ImGui::Text("Value: %.2f", exampleComp->value);
                                        ImGui::Unindent();
                                    }
                                    else {
                                        ImGui::Indent();
                                        ImGui::Text("Plugin component data available");
                                        ImGui::Unindent();
                                    }
                                }
                            }
                            else {
                                ImGui::Text("Component: %s (ID: %u)", componentName.c_str(), compType);
                            }
                        }

                        ImGui::Unindent();
                        ImGui::TreePop();
                    }
                }
            }
            ImGui::EndChild();

            ImGui::Separator();

            if (ImGui::Button("Create New Entity")) {
                ECS::EntityID newEntity = m_entityManager.AddNewEntity();
                RefreshEntities();
                selectedEntity = newEntity;
                std::cout << "[DebugView] Created new entity: " << newEntity << std::endl;
            }

            ImGui::SameLine();

            if (ImGui::Button("Delete Selected Entity") && selectedEntity != static_cast<ECS::EntityID>(-1)) {
                std::cout << "[DebugView] Deleting entity: " << selectedEntity << std::endl;
                m_entityManager.DestroyEntity(selectedEntity);
                selectedEntity = static_cast<ECS::EntityID>(-1);
                RefreshEntities();
            }

            if (selectedEntity != static_cast<ECS::EntityID>(-1)) {
                ImGui::Separator();
                ImGui::Text("Selected Entity: %zu", selectedEntity);

                auto components = m_entityManager.GetEntityComponents(selectedEntity);
                ImGui::Text("Total Components: %zu", components.size());

                int regularComponents = 0;
                int pluginComponents = 0;

                for (auto compType : components) {
                    if (m_entityManager.HasPluginComponent(selectedEntity, compType)) {
                        pluginComponents++;
                    }
                    else {
                        regularComponents++;
                    }
                }

                ImGui::Text("Regular Components: %d", regularComponents);
                ImGui::Text("Plugin Components: %d", pluginComponents);
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

    void DebugView::RenderSystemPanel() {
        if (ImGui::Begin("Active Systems", nullptr)) {

            ImGui::Text("Registered Systems");
            ImGui::Separator();

            const auto& systems = m_entityManager.GetRegisteredSystems();
            ImGui::Text("Regular Systems (%zu):", systems.size());

            for (const auto& [id, aniSystem] : systems) {
                if (ImGui::TreeNode((std::string("System ") + std::to_string(id) + " " + aniSystem->GetSystemName()).c_str())) {
                    ImGui::Text("System ID: %zu", id);
                    ImGui::Text("System Name: %s", aniSystem->GetSystemName().c_str());
                    ImGui::Text("Status: Active");
                    ImGui::TreePop();
                }
            }

            ImGui::Separator();

            ImGui::Text("Plugin Systems:");
            ImGui::Text("(Plugin system info not directly accessible)");
            ImGui::Text("Check console output for plugin system updates");

            ImGui::Separator();

            if (ImGui::Button("Refresh Systems")) {
                std::cout << "[DebugView] System refresh requested" << std::endl;
            }

            ImGui::SameLine();

            if (ImGui::Button("Print Registry Debug Info")) {
                m_entityManager.DebugPrintRegisteredComponents();
            }
        }
        ImGui::End();
    }

} // namespace GUI