//#pragma once
//#include "Base/BaseSystem.hpp"
//#include "NodeGraphComponents/NodeComponents.hpp"
//#include <imgui_node_editor.h>
//
//namespace ed = ax::NodeEditor;
//
//namespace ECS {
//
//class NodeGraphSystem : public BaseSystem {
//public:
//    NodeGraphSystem(EntityManager &entityMgr) : BaseSystem(entityMgr) {
//        sysName = "NodeGraphSystem";
//        AddComponentSignature<NodeComponent>();
//        AddComponentSignature<PinComponent>();
//        AddComponentSignature<LinkComponent>();
//    }
//
//    void Start() override {
//        ed::Config config;
//        config.SettingsFile = "NodeGraph.json";
//        m_Editor = ed::CreateEditor(&config);
//    }
//
//    void Update(const float deltaT) override {
//        ed::SetCurrentEditor(m_Editor);
//        ed::Begin("Node Editor");
//
//        // Draw nodes
//        for (auto entity : entities) {
//            if (mgr.HasComponent<NodeComponent>(entity)) {
//                auto &node = mgr.GetComponent<NodeComponent>(entity);
//                drawNode(entity, node);
//            }
//        }
//
//        // Handle interactions
//        handleInteractions();
//
//        ed::End();
//    }
//
//    void Destroy() override {
//        if (m_Editor) {
//            ed::DestroyEditor(m_Editor);
//            m_Editor = nullptr;
//        }
//    }
//
//private:
//    void drawNode(EntityID entity, NodeComponent &node) {
//        ed::NodeId nodeId = ed::NodeId(static_cast<intptr_t>(entity));
//        ed::BeginNode(nodeId);
//
//        ImGui::BeginGroup(); // Begin node group
//
//        // Header
//        ImGui::BeginGroup(); // Begin header group
//        ImGui::Text("%s", node.name.c_str());
//        ImGui::EndGroup(); // End header group
//
//        ImGui::BeginGroup(); // Begin pins group
//
//        // Input pins
//        for (auto pinEntity : entities) {
//            if (mgr.HasComponent<PinComponent>(pinEntity)) {
//                auto &pin = mgr.GetComponent<PinComponent>(pinEntity);
//                if (pin.nodeEntity == entity && pin.kind == PinComponent::Kind::Input) {
//                    drawPin(pinEntity, pin);
//                }
//            }
//        }
//
//        // Output pins
//        for (auto pinEntity : entities) {
//            if (mgr.HasComponent<PinComponent>(pinEntity)) {
//                auto &pin = mgr.GetComponent<PinComponent>(pinEntity);
//                if (pin.nodeEntity == entity && pin.kind == PinComponent::Kind::Output) {
//                    drawPin(pinEntity, pin);
//                }
//            }
//        }
//
//        ImGui::EndGroup(); // End pins group
//        ImGui::EndGroup(); // End node group
//
//        ed::EndNode();
//
//        if (ImGui::IsItemHovered() && ImGui::IsMouseDragging(0)) {
//            node.dragging = true;
//            ImVec2 delta = ImGui::GetIO().MouseDelta;
//            node.position.x += delta.x;
//            node.position.y += delta.y;
//        } else {
//            node.dragging = false;
//        }
//    }
//
//    void drawPin(EntityID entity, PinComponent &pin) {
//        ed::PinId pinId = ed::PinId(static_cast<intptr_t>(entity));
//        ed::BeginPin(pinId, pin.kind == PinComponent::Kind::Input ? ed::PinKind::Input : ed::PinKind::Output);
//
//        if (pin.kind == PinComponent::Kind::Input) {
//            ImGui::Text("-> %s", pin.name.c_str());
//        } else {
//            ImGui::Text("%s <-", pin.name.c_str());
//        }
//
//        ed::EndPin();
//
//        // Convert ImVec2 to glm::vec2 for pin position
//        ImVec2 screenPos = ImGui::GetCursorScreenPos();
//        pin.position = glm::vec2(screenPos.x, screenPos.y);
//    }
//
//    void handleInteractions() {
//        if (ed::BeginCreate()) {
//            ed::PinId startPinId, endPinId;
//            if (ed::QueryNewLink(&startPinId, &endPinId)) {
//                if (canCreateLink(startPinId, endPinId)) {
//                    if (ed::AcceptNewItem()) {
//                        createLink(startPinId, endPinId);
//                    }
//                }
//            }
//        }
//        ed::EndCreate();
//
//        if (ed::BeginDelete()) {
//            ed::NodeId nodeId;
//            while (ed::QueryDeletedNode(&nodeId)) {
//                if (ed::AcceptDeletedItem()) {
//                    EntityID entityId = reinterpret_cast<EntityID>(nodeId.AsPointer());
//                    mgr.DestroyEntity(entityId);
//                }
//            }
//
//            ed::LinkId linkId;
//            while (ed::QueryDeletedLink(&linkId)) {
//                if (ed::AcceptDeletedItem()) {
//                    EntityID entityId = reinterpret_cast<EntityID>(linkId.AsPointer());
//                    mgr.DestroyEntity(entityId);
//                }
//            }
//        }
//        ed::EndDelete();
//    }
//
//    bool canCreateLink(ed::PinId startPinId, ed::PinId endPinId) {
//        if (!startPinId || !endPinId)
//            return false;
//
//        EntityID startEntity = reinterpret_cast<EntityID>(startPinId.AsPointer());
//        EntityID endEntity = reinterpret_cast<EntityID>(endPinId.AsPointer());
//
//        if (!mgr.HasComponent<PinComponent>(startEntity) || !mgr.HasComponent<PinComponent>(endEntity))
//            return false;
//
//        auto &startPin = mgr.GetComponent<PinComponent>(startEntity);
//        auto &endPin = mgr.GetComponent<PinComponent>(endEntity);
//
//        if (startPin.nodeEntity == endPin.nodeEntity)
//            return false;
//        if (startPin.kind == endPin.kind)
//            return false;
//        if (startPin.type != endPin.type)
//            return false;
//
//        return true;
//    }
//
//    void createLink(ed::PinId startPinId, ed::PinId endPinId) {
//        EntityID entity = mgr.AddNewEntity();
//        mgr.AddComponent<LinkComponent>(entity);
//        auto &link = mgr.GetComponent<LinkComponent>(entity);
//        link.startPin = reinterpret_cast<EntityID>(startPinId.AsPointer());
//        link.endPin = reinterpret_cast<EntityID>(endPinId.AsPointer());
//    }
//
//    ed::EditorContext *m_Editor = nullptr;
//};
//
//} // namespace ECS