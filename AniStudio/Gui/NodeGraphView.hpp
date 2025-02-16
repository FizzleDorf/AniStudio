#ifndef NODEGRAPHVIEW_HPP
#define NODEGRAPHVIEW_HPP

#include "Base/BaseView.hpp"
#include "NodeGraphComponents/NodeComponents.hpp"
#include <glm/glm.hpp>
#include <imgui_node_editor.h>
#include <iostream>

namespace ed = ax::NodeEditor;

namespace GUI {

class NodeGraphView : public BaseView {
public:
    NodeGraphView(ECS::EntityManager &entityMgr) : BaseView(entityMgr), m_Editor(nullptr), isInitialized(false) {
        viewName = "NodeGraphView";
    }

    ~NodeGraphView() { Cleanup(); }

    void Init() override {
        if (isInitialized)
            return;

        try {
            ed::Config config;
            config.SettingsFile = nullptr;
            m_Editor = ed::CreateEditor(&config);

            if (!m_Editor) {
                throw std::runtime_error("Failed to create node editor context");
            }

            CreateExampleNodes();
            isInitialized = true;
        } catch (const std::exception &e) {
            std::cerr << "NodeGraphView initialization failed: " << e.what() << std::endl;
            Cleanup();
        }
    }

    void Cleanup() {
        isInitialized = false;
        if (m_Editor) {
            ed::DestroyEditor(m_Editor);
            m_Editor = nullptr;
        }
        CleanupEntities();
    }

    void Render() override {
        if (!isInitialized || !m_Editor) {
            Init();
            return;
        }

        try {
            ImGui::Begin("Node Graph", nullptr, ImGuiWindowFlags_MenuBar);
            RenderMenuBar();

            ed::SetCurrentEditor(m_Editor);
            ed::Begin("Node Editor");

            RenderNodes();
            RenderLinks();
            HandleInteractions();

            ed::End();
            ed::SetCurrentEditor(nullptr);

            ImGui::End();
        } catch (const std::exception &e) {
            std::cerr << "NodeGraphView render error: " << e.what() << std::endl;
            isInitialized = false;
        }
    }

private:
    void RenderMenuBar() {
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("New Node")) {
                    ImGui::OpenPopup("New Node Type");
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        if (ImGui::BeginPopup("New Node Type")) {
            if (ImGui::MenuItem("Math Node")) {
                auto node = CreateMathNode(ImGui::GetMousePos());
            }
            if (ImGui::MenuItem("String Node")) {
                auto node = CreateStringNode(ImGui::GetMousePos());
            }
            if (ImGui::MenuItem("Flow Control")) {
                auto node = CreateFlowNode(ImGui::GetMousePos());
            }
            ImGui::EndPopup();
        }
    }

    void RenderNodes() {
        auto entities = mgr.GetAllEntities();
        for (auto entity : entities) {
            if (!mgr.HasComponent<ECS::NodeComponent>(entity))
                continue;

            auto &node = mgr.GetComponent<ECS::NodeComponent>(entity);
            ed::BeginNode(ed::NodeId(static_cast<intptr_t>(entity)));

            // Node header
            ImGui::BeginGroup();
            ImGui::TextUnformatted(node.name.c_str());
            ImGui::Spacing();

            // Content with pins
            ImGui::BeginGroup();

            // Input pins on left
            ImGui::BeginGroup();
            for (size_t i = 0; i < node.inputs.size(); i++) {
                RenderPin(entity, node.inputs[i], i, true);
            }
            ImGui::EndGroup();

            ImGui::SameLine(150);

            // Output pins on right
            ImGui::BeginGroup();
            for (size_t i = 0; i < node.outputs.size(); i++) {
                RenderPin(entity, node.outputs[i], i, false);
            }
            ImGui::EndGroup();

            ImGui::EndGroup();
            ImGui::EndGroup();

            ed::EndNode();

            // Update node position
            ImVec2 pos = ed::GetNodePosition(ed::NodeId(static_cast<intptr_t>(entity)));
            node.position = glm::vec2(pos.x, pos.y);
        }
    }

    void RenderPin(ECS::EntityID nodeId, const ECS::Pin &pin, size_t pinIdx, bool isInput) {
        ed::BeginPin(ed::PinId(GeneratePinId(nodeId, pinIdx, isInput)),
                     isInput ? ed::PinKind::Input : ed::PinKind::Output);

        ImGui::PushStyleColor(ImGuiCol_Text, pin.GetColor());
        ImGui::Text("%s", pin.name.c_str());
        ImGui::PopStyleColor();

        ed::EndPin();
    }

    void RenderLinks() {
        auto entities = mgr.GetAllEntities();
        for (auto entity : entities) {
            if (!mgr.HasComponent<ECS::LinkComponent>(entity))
                continue;

            auto &link = mgr.GetComponent<ECS::LinkComponent>(entity);

            // Get the connected nodes
            if (!mgr.HasComponent<ECS::NodeComponent>(link.startNode) ||
                !mgr.HasComponent<ECS::NodeComponent>(link.endNode)) {
                mgr.DestroyEntity(entity);
                continue;
            }

            auto &startNode = mgr.GetComponent<ECS::NodeComponent>(link.startNode);
            auto &endNode = mgr.GetComponent<ECS::NodeComponent>(link.endNode);

            if (link.startPinIndex >= startNode.outputs.size() || link.endPinIndex >= endNode.inputs.size()) {
                mgr.DestroyEntity(entity);
                continue;
            }

            // Draw the link
            ed::Link(ed::LinkId(static_cast<intptr_t>(entity)),
                     ed::PinId(GeneratePinId(link.startNode, link.startPinIndex, false)),
                     ed::PinId(GeneratePinId(link.endNode, link.endPinIndex, true)),
                     ColorU32ToVec4(link.GetColor(startNode, endNode)), link.thickness);
        }
    }

    void HandleInteractions() {
        HandleLinkCreation();
        HandleDeletion();
    }

    void HandleLinkCreation() {
        ed::PinId startPinId, endPinId;
        if (ed::QueryNewLink(&startPinId, &endPinId)) {
            if (startPinId && endPinId) {
                if (CanCreateLink(startPinId, endPinId)) {
                    if (ed::AcceptNewItem()) {
                        CreateLink(startPinId, endPinId);
                    }
                } else {
                    ed::RejectNewItem();
                }
            }
        }
    }

    bool CanCreateLink(ed::PinId startPinId, ed::PinId endPinId) {
        auto [startNodeId, startPinIdx, startIsInput] = DecodePinId(startPinId.Get());
        auto [endNodeId, endPinIdx, endIsInput] = DecodePinId(endPinId.Get());

        // Must connect input to output
        if (startIsInput == endIsInput)
            return false;

        // Get the nodes
        if (!mgr.HasComponent<ECS::NodeComponent>(startNodeId) || !mgr.HasComponent<ECS::NodeComponent>(endNodeId))
            return false;

        auto &startNode = mgr.GetComponent<ECS::NodeComponent>(startNodeId);
        auto &endNode = mgr.GetComponent<ECS::NodeComponent>(endNodeId);

        // Ensure pin indices are valid
        if (startIsInput) {
            if (startPinIdx >= startNode.inputs.size() || endPinIdx >= endNode.outputs.size())
                return false;
            return startNode.inputs[startPinIdx].IsCompatibleWith(endNode.outputs[endPinIdx]);
        } else {
            if (startPinIdx >= startNode.outputs.size() || endPinIdx >= endNode.inputs.size())
                return false;
            return startNode.outputs[startPinIdx].IsCompatibleWith(endNode.inputs[endPinIdx]);
        }
    }

    void CreateLink(ed::PinId startPinId, ed::PinId endPinId) {
        auto [startNodeId, startPinIdx, startIsInput] = DecodePinId(startPinId.Get());
        auto [endNodeId, endPinIdx, endIsInput] = DecodePinId(endPinId.Get());

        // Ensure we connect output to input
        if (startIsInput) {
            std::swap(startNodeId, endNodeId);
            std::swap(startPinIdx, endPinIdx);
        }

        auto linkEntity = mgr.AddNewEntity();
        mgr.AddComponent<ECS::LinkComponent>(linkEntity);
        auto &link = mgr.GetComponent<ECS::LinkComponent>(linkEntity);

        link.startNode = startNodeId;
        link.startPinIndex = startPinIdx;
        link.endNode = endNodeId;
        link.endPinIndex = endPinIdx;
    }

    void HandleDeletion() {
        ed::NodeId nodeId;
        while (ed::QueryDeletedNode(&nodeId)) {
            if (ed::AcceptDeletedItem()) {
                mgr.DestroyEntity(static_cast<ECS::EntityID>(nodeId.Get()));
            }
        }

        ed::LinkId linkId;
        while (ed::QueryDeletedLink(&linkId)) {
            if (ed::AcceptDeletedItem()) {
                mgr.DestroyEntity(static_cast<ECS::EntityID>(linkId.Get()));
            }
        }
    }

    // Node creation helpers
    ECS::EntityID CreateMathNode(const ImVec2 &pos) {
        auto entity = mgr.AddNewEntity();
        mgr.AddComponent<ECS::NodeComponent>(entity);
        auto &node = mgr.GetComponent<ECS::NodeComponent>(entity);

        node.name = "Math Node";
        node.position = glm::vec2(pos.x, pos.y);

        node.inputs = {{"Value A", ECS::Pin::Type::Float}, {"Value B", ECS::Pin::Type::Float}};

        node.outputs = {{"Result", ECS::Pin::Type::Float}};

        return entity;
    }

    ECS::EntityID CreateStringNode(const ImVec2 &pos) {
        auto entity = mgr.AddNewEntity();
        mgr.AddComponent<ECS::NodeComponent>(entity);
        auto &node = mgr.GetComponent<ECS::NodeComponent>(entity);

        node.name = "String Node";
        node.position = glm::vec2(pos.x, pos.y);

        node.inputs = {{"Text In", ECS::Pin::Type::String}};

        node.outputs = {{"Modified", ECS::Pin::Type::String}};

        return entity;
    }

    ECS::EntityID CreateFlowNode(const ImVec2 &pos) {
        auto entity = mgr.AddNewEntity();
        mgr.AddComponent<ECS::NodeComponent>(entity);
        auto &node = mgr.GetComponent<ECS::NodeComponent>(entity);

        node.name = "Flow Control";
        node.position = glm::vec2(pos.x, pos.y);

        node.inputs = {{"In", ECS::Pin::Type::Flow}};

        node.outputs = {{"True", ECS::Pin::Type::Flow}, {"False", ECS::Pin::Type::Flow}};

        return entity;
    }

    void CreateExampleNodes() {
        CreateMathNode(ImVec2(100, 100));
        CreateStringNode(ImVec2(400, 100));
        CreateFlowNode(ImVec2(250, 300));
    }

    void CleanupEntities() {
        auto entities = mgr.GetAllEntities();
        for (auto entity : entities) {
            if (mgr.HasComponent<ECS::LinkComponent>(entity) || mgr.HasComponent<ECS::NodeComponent>(entity)) {
                mgr.DestroyEntity(entity);
            }
        }
    }

    // Pin ID encoding/decoding helpers
    intptr_t GeneratePinId(ECS::EntityID nodeId, size_t pinIdx, bool isInput) {
        return (static_cast<intptr_t>(nodeId) << 16) | (pinIdx << 1) | (isInput ? 1 : 0);
    }

    std::tuple<ECS::EntityID, size_t, bool> DecodePinId(intptr_t id) {
        return {
            static_cast<ECS::EntityID>(id >> 16),    // nodeId
            static_cast<size_t>((id >> 1) & 0x7FFF), // pinIdx
            (id & 1) == 1                            // isInput
        };
    }

private:
    ed::EditorContext *m_Editor;
    bool isInitialized;
};

} // namespace GUI

#endif // NODEGRAPHVIEW_HPP