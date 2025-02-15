#pragma once
#include "Base/BaseView.hpp"
#include "NodeGraphComponents/NodeComponents.hpp"
#include <imgui_node_editor.h>
#include <memory>
#include <string>

namespace ed = ax::NodeEditor;

namespace GUI {

class NodeGraphView : public BaseView {
public:
    NodeGraphView(ECS::EntityManager &entityMgr) : BaseView(entityMgr), m_Editor(nullptr) {
        viewName = "NodeGraphView";
        mgr.RegisterSystem<ECS::NodeGraphSystem>();
    }

    ~NodeGraphView() {
        if (m_Editor) {
            ed::DestroyEditor(m_Editor);
            m_Editor = nullptr;
        }
    }

    void Init() override {
        ed::Config config;
        config.SettingsFile = "nodegraph.json";
        m_Editor = ed::CreateEditor(&config);
    }

    void Render() override {
        if (!m_Editor)
            return;

        ImGui::Begin("Node Graph");

        ed::SetCurrentEditor(m_Editor);
        ed::Begin("Node Editor");

        // Get all entities from EntityManager
        std::vector<ECS::EntityID> entities = mgr.GetAllEntities();

        // Render all nodes
        for (ECS::EntityID entity : entities) {
            if (mgr.HasComponent<ECS::NodeComponent>(entity)) {
                RenderNode(entity);
            }
        }

        // Render all links
        for (ECS::EntityID entity : entities) {
            if (mgr.HasComponent<ECS::LinkComponent>(entity)) {
                RenderLink(entity);
            }
        }

        HandleInteraction();

        ed::End();
        ed::SetCurrentEditor(nullptr);

        ImGui::End();
    }

    ECS::EntityID CreateNode(const std::string &name, const ImVec2 &position) {
        ECS::EntityID entity = mgr.AddNewEntity();
        mgr.AddComponent<ECS::NodeComponent>(entity);

        auto &node = mgr.GetComponent<ECS::NodeComponent>(entity);
        node.name = name;
        node.position = glm::vec2(position.x, position.y);

        return entity;
    }

    ECS::EntityID AddPin(ECS::EntityID nodeEntity, const std::string &name, ECS::PinComponent::Type type,
                         ECS::PinComponent::Kind kind) {
        if (!mgr.HasComponent<ECS::NodeComponent>(nodeEntity))
            return 0;

        ECS::EntityID pinEntity = mgr.AddNewEntity();
        mgr.AddComponent<ECS::PinComponent>(pinEntity);

        auto &pin = mgr.GetComponent<ECS::PinComponent>(pinEntity);
        pin.name = name;
        pin.type = type;
        pin.kind = kind;
        pin.nodeEntity = nodeEntity;

        return pinEntity;
    }

private:
    void RenderNode(ECS::EntityID entity) {
        auto &node = mgr.GetComponent<ECS::NodeComponent>(entity);

        ed::BeginNode(ed::NodeId(static_cast<intptr_t>(entity)));

        ImGui::BeginGroup();

        // Node header
        ImGui::TextUnformatted(node.name.c_str());
        ImGui::Spacing();

        // Node content - pins
        ImGui::BeginGroup();

        // Get all entities to find pins
        std::vector<ECS::EntityID> entities = mgr.GetAllEntities();

        // Input pins
        for (ECS::EntityID pinEntity : entities) {
            if (mgr.HasComponent<ECS::PinComponent>(pinEntity)) {
                auto &pin = mgr.GetComponent<ECS::PinComponent>(pinEntity);
                if (pin.nodeEntity == entity && pin.kind == ECS::PinComponent::Kind::Input) {
                    RenderPin(pinEntity, pin);
                }
            }
        }

        ImGui::EndGroup();

        ImGui::SameLine();

        // Output pins
        ImGui::BeginGroup();
        for (ECS::EntityID pinEntity : entities) {
            if (mgr.HasComponent<ECS::PinComponent>(pinEntity)) {
                auto &pin = mgr.GetComponent<ECS::PinComponent>(pinEntity);
                if (pin.nodeEntity == entity && pin.kind == ECS::PinComponent::Kind::Output) {
                    RenderPin(pinEntity, pin);
                }
            }
        }
        ImGui::EndGroup();

        ImGui::EndGroup();

        ed::EndNode();

        // Handle node dragging
        if (ed::GetNodePosition(ed::NodeId(static_cast<intptr_t>(entity))) !=
            ImVec2(node.position.x, node.position.y)) {
            ImVec2 newPos = ed::GetNodePosition(ed::NodeId(static_cast<intptr_t>(entity)));
            node.position = glm::vec2(newPos.x, newPos.y);
        }
    }

    void RenderPin(ECS::EntityID entity, ECS::PinComponent &pin) {
        ed::BeginPin(ed::PinId(static_cast<intptr_t>(entity)),
                     pin.kind == ECS::PinComponent::Kind::Input ? ed::PinKind::Input : ed::PinKind::Output);

        // Draw pin label with simple arrow indicator
        if (pin.kind == ECS::PinComponent::Kind::Input) {
            ImGui::Text("-> %s", pin.name.c_str());
        } else {
            ImGui::Text("%s ->", pin.name.c_str());
        }

        ed::EndPin();

        // Store current pin position
        pin.position = glm::vec2(ImGui::GetCursorScreenPos().x, ImGui::GetCursorScreenPos().y);
    }

    void RenderLink(ECS::EntityID entity) {
        auto &link = mgr.GetComponent<ECS::LinkComponent>(entity);

        ed::Link(ed::LinkId(static_cast<intptr_t>(entity)), ed::PinId(static_cast<intptr_t>(link.startPin)),
                 ed::PinId(static_cast<intptr_t>(link.endPin)));
    }

    void HandleInteraction() {
        // Handle link creation
        if (ed::BeginCreate()) {
            ed::PinId startPinId, endPinId;
            if (ed::QueryNewLink(&startPinId, &endPinId)) {
                if (startPinId && endPinId) {
                    if (ed::AcceptNewItem()) {
                        // Create new link entity
                        ECS::EntityID linkEntity = mgr.AddNewEntity();
                        mgr.AddComponent<ECS::LinkComponent>(linkEntity);

                        auto &link = mgr.GetComponent<ECS::LinkComponent>(linkEntity);
                        link.startPin = reinterpret_cast<ECS::EntityID>(startPinId.AsPointer());
                        link.endPin = reinterpret_cast<ECS::EntityID>(endPinId.AsPointer());
                    }
                }
            }
        }
        ed::EndCreate();

        // Handle deletion
        if (ed::BeginDelete()) {
            // Handle node deletion
            ed::NodeId nodeId;
            while (ed::QueryDeletedNode(&nodeId)) {
                if (ed::AcceptDeletedItem()) {
                    mgr.DestroyEntity(reinterpret_cast<ECS::EntityID>(nodeId.AsPointer()));
                }
            }

            // Handle link deletion
            ed::LinkId linkId;
            while (ed::QueryDeletedLink(&linkId)) {
                if (ed::AcceptDeletedItem()) {
                    mgr.DestroyEntity(reinterpret_cast<ECS::EntityID>(linkId.AsPointer()));
                }
            }
        }
        ed::EndDelete();
    }

    ed::EditorContext *m_Editor;
};

} // namespace GUI