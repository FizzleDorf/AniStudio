#ifndef NODEGRAPHVIEW_HPP
#define NODEGRAPHVIEW_HPP

#include "Base/BaseView.hpp"
#include "NodeGraphComponents/NodeComponents.hpp"
#include "node_utils.hpp"
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
                    // ImGui::OpenPopup("New Node Type");
                    auto node = CreateMathNode(ImGui::GetMousePos());
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
        using namespace ax::NodeEditor::Utilities;

        auto entities = mgr.GetAllEntities();
        for (auto entity : entities) {
            if (!mgr.HasComponent<ECS::NodeComponent>(entity))
                continue;

            auto &node = mgr.GetComponent<ECS::NodeComponent>(entity);

            ed::BeginNode(ed::NodeId(static_cast<intptr_t>(entity)));
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 8));

            // Header
            BeginVertical("header");
            {
                ImGui::TextUnformatted(node.name.c_str());
                ImGui::Spacing();
            }
            EndVertical();

            // Content
            BeginHorizontal("content");
            {
                const float MIN_WIDTH = 180.0f; // Minimum node width
                float currentWidth = ImGui::GetWindowWidth();
                float requiredWidth = MIN_WIDTH;

                // Calculate required width based on pin names
                for (const auto &pin : node.inputs) {
                    requiredWidth =
                        std::max(requiredWidth,
                                 ImGui::CalcTextSize(pin.name.c_str()).x + 50.0f); // 50 for pin circle and spacing
                }
                for (const auto &pin : node.outputs) {
                    requiredWidth = std::max(requiredWidth, ImGui::CalcTextSize(pin.name.c_str()).x + 50.0f);
                }

                // Input pins on left
                BeginVertical("inputs");
                {
                    for (size_t i = 0; i < node.inputs.size(); i++) {
                        RenderPin(entity, node.inputs[i], i, true);
                    }
                }
                EndVertical();

                // Spacing between input and output pins
                ImGui::Dummy(ImVec2(std::max(requiredWidth - 100.0f, 20.0f), 1.0f));
                ImGui::SameLine();

                // Output pins on right
                BeginVertical("outputs");
                {
                    for (size_t i = 0; i < node.outputs.size(); i++) {
                        RenderPin(entity, node.outputs[i], i, false);
                    }
                }
                EndVertical();
            }
            EndHorizontal();

            ImGui::PopStyleVar();
            ed::EndNode();

            // Update node position
            ImVec2 pos = ed::GetNodePosition(ed::NodeId(static_cast<intptr_t>(entity)));
            node.position = glm::vec2(pos.x, pos.y);
        }
    }

    void RenderPin(ECS::EntityID nodeId, const ECS::Pin &pin, size_t pinIdx, bool isInput) {
        using namespace ax::NodeEditor::Utilities;

        const float PIN_CIRCLE_RADIUS = 4.0f;
        BeginHorizontal(pin.name.c_str());

        ed::BeginPin(ed::PinId(GeneratePinId(nodeId, pinIdx, isInput)),
                     isInput ? ed::PinKind::Input : ed::PinKind::Output);

        if (isInput) {
            // Input pin: Circle then text
            ImGui::BeginGroup();
            {
                ImVec2 cursorPos = ImGui::GetCursorScreenPos();
                ImDrawList *drawList = ImGui::GetWindowDrawList();

                // Circle at text baseline
                float circleY = cursorPos.y + ImGui::GetTextLineHeight() / 2;
                ImVec2 circlePos(cursorPos.x + PIN_CIRCLE_RADIUS, circleY);

                drawList->AddCircleFilled(circlePos, PIN_CIRCLE_RADIUS, pin.GetColor());
                drawList->AddCircle(circlePos, PIN_CIRCLE_RADIUS + 0.5f, IM_COL32(100, 100, 100, 255));

                // Text after circle
                ImGui::Dummy(ImVec2(PIN_CIRCLE_RADIUS * 2 + 4, ImGui::GetTextLineHeight()));
                ImGui::SameLine();
                ImGui::TextUnformatted(pin.name.c_str());
            }
            ImGui::EndGroup();
        } else {
            // Output pin: Text then circle
            ImGui::BeginGroup();
            {
                ImVec2 textSize = ImGui::CalcTextSize(pin.name.c_str());
                ImVec2 cursorPos = ImGui::GetCursorScreenPos();
                ImDrawList *drawList = ImGui::GetWindowDrawList();

                // Text first
                ImGui::TextUnformatted(pin.name.c_str());
                ImGui::SameLine();

                // Circle at text baseline
                float circleY = cursorPos.y + ImGui::GetTextLineHeight() / 2;
                ImVec2 circlePos(cursorPos.x + textSize.x + 4 + PIN_CIRCLE_RADIUS, circleY);

                drawList->AddCircleFilled(circlePos, PIN_CIRCLE_RADIUS, pin.GetColor());
                drawList->AddCircle(circlePos, PIN_CIRCLE_RADIUS + 0.5f, IM_COL32(100, 100, 100, 255));

                ImGui::Dummy(ImVec2(PIN_CIRCLE_RADIUS * 2 + 4, ImGui::GetTextLineHeight()));
            }
            ImGui::EndGroup();
        }

        ed::EndPin();
        EndHorizontal();
    }

    void RenderLinks() {
        auto entities = mgr.GetAllEntities();
        for (auto entity : entities) {
            if (!mgr.HasComponent<ECS::LinkComponent>(entity))
                continue;

            auto &link = mgr.GetComponent<ECS::LinkComponent>(entity);

            // Validate nodes
            if (!mgr.HasComponent<ECS::NodeComponent>(link.startNode) ||
                !mgr.HasComponent<ECS::NodeComponent>(link.endNode)) {
                mgr.DestroyEntity(entity);
                continue;
            }

            auto &startNode = mgr.GetComponent<ECS::NodeComponent>(link.startNode);
            auto &endNode = mgr.GetComponent<ECS::NodeComponent>(link.endNode);

            // Validate pin indices
            if (link.startPinIndex >= startNode.outputs.size() || link.endPinIndex >= endNode.inputs.size()) {
                mgr.DestroyEntity(entity);
                continue;
            }

            // Create link
            ed::PinId startPinId = ed::PinId(GeneratePinId(link.startNode, link.startPinIndex, false));
            ed::PinId endPinId = ed::PinId(GeneratePinId(link.endNode, link.endPinIndex, true));

            ImColor linkColor = ImColor(startNode.outputs[link.startPinIndex].GetColor());

            // Draw link shadow/outline
            if (link.isSelected) {
                ed::Link(ed::LinkId(static_cast<intptr_t>(entity) | 0x80000000), // Different ID for shadow
                         startPinId, endPinId, ImColor(255, 255, 255, 100), link.thickness + 2.0f);
            }

            // Draw main link
            ed::Link(ed::LinkId(static_cast<intptr_t>(entity)), startPinId, endPinId, linkColor, link.thickness);
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