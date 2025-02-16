#include "NodeGraphView.hpp"
#include <iostream>

using ax::NodeEditor::Utilities::BeginHorizontal;
using ax::NodeEditor::Utilities::BeginVertical;
using ax::NodeEditor::Utilities::EndHorizontal;
using ax::NodeEditor::Utilities::EndVertical;
using ax::NodeEditor::Utilities::Spring;

namespace GUI {

NodeGraphView::NodeGraphView(ECS::EntityManager &entityMgr)
    : BaseView(entityMgr), m_Editor(nullptr), isInitialized(false) {
    viewName = "NodeGraphView";
}

NodeGraphView::~NodeGraphView() { Cleanup(); }

void NodeGraphView::Init() {
    if (isInitialized)
        return;

    try {
        ed::Config config;
        config.SettingsFile = nullptr;
        m_Editor = ed::CreateEditor(&config);
        ed::SetCurrentEditor(m_Editor);
        if (!m_Editor) {
            throw std::runtime_error("Failed to create node editor context");
        }

        // CreateExampleNodes();
        isInitialized = true;
    } catch (const std::exception &e) {
        std::cerr << "NodeGraphView initialization failed: " << e.what() << std::endl;
        Cleanup();
    }
}

void NodeGraphView::Cleanup() {
    isInitialized = false;
    if (m_Editor) {
        ed::DestroyEditor(m_Editor);
        m_Editor = nullptr;
    }
    CleanupEntities();
}

void NodeGraphView::Render() {
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
        ImGui::End();
    } catch (const std::exception &e) {
        std::cerr << "NodeGraphView render error: " << e.what() << std::endl;
        isInitialized = false;
        ed::SetCurrentEditor(nullptr);
    }
}

void NodeGraphView::RenderMenuBar() {
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New Node")) {
                auto node = CreateMathNode(ImGui::GetMousePos());
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
}

void NodeGraphView::RenderNodes() {

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
            // Input pins on left
            BeginVertical("inputs");
            {
                for (size_t i = 0; i < node.inputs.size(); i++) {
                    RenderPin(entity, node.inputs[i], i, true);
                }
            }
            EndVertical();

            // Spacing between pins
            Spring(1);

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

void NodeGraphView::RenderPin(ECS::EntityID nodeId, const ECS::Pin &pin, size_t pinIdx, bool isInput) {

    ed::BeginPin(ed::PinId(GeneratePinId(nodeId, pinIdx, isInput)), isInput ? ed::PinKind::Input : ed::PinKind::Output);

    BeginHorizontal(pin.name.c_str());

    if (isInput) {
        // Socket
        ImGui::Dummy(ImVec2(8, 8));
        auto drawList = ImGui::GetWindowDrawList();
        auto pos = ImGui::GetCursorScreenPos();
        drawList->AddCircleFilled(ImVec2(pos.x - 4, pos.y + 4), 4.0f, pin.GetColor());

        Spring(0);
        ImGui::TextUnformatted(pin.name.c_str());
    } else {
        ImGui::TextUnformatted(pin.name.c_str());
        Spring(0);

        // Socket
        ImGui::Dummy(ImVec2(8, 8));
        auto drawList = ImGui::GetWindowDrawList();
        auto pos = ImGui::GetCursorScreenPos();
        drawList->AddCircleFilled(ImVec2(pos.x - 4, pos.y + 4), 4.0f, pin.GetColor());
    }

    EndHorizontal();
    ed::EndPin();
}

void NodeGraphView::RenderLinks() {
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

        // Draw link
        ed::Link(ed::LinkId(static_cast<intptr_t>(entity)),
                 ed::PinId(GeneratePinId(link.startNode, link.startPinIndex, false)),
                 ed::PinId(GeneratePinId(link.endNode, link.endPinIndex, true)),
                 ImColor(startNode.outputs[link.startPinIndex].GetColor()), 2.0f);
    }
}

void NodeGraphView::HandleInteractions() {
    // Keyboard & Mouse navigation is handled by ImGui Node Editor already

    // Handle link creation
    if (ed::BeginCreate()) {
        ed::PinId startPinId, endPinId;
        if (ed::QueryNewLink(&startPinId, &endPinId)) {
            // Handle dragging from start pin
            if (startPinId && !endPinId) {
                // We're dragging a new link - show preview
                // First decode the pin information
                auto [nodeId, pinIdx, isInput] = DecodePinId(startPinId.Get());
                if (mgr.HasComponent<ECS::NodeComponent>(nodeId)) {
                    auto &node = mgr.GetComponent<ECS::NodeComponent>(nodeId);
                    // Get color from the correct pin array based on input/output
                    if (isInput && pinIdx < node.inputs.size()) {
                        ed::Link(ed::LinkId(0), startPinId, endPinId, ImColor(node.inputs[pinIdx].GetColor()), 2.0f);
                    } else if (!isInput && pinIdx < node.outputs.size()) {
                        ed::Link(ed::LinkId(0), startPinId, endPinId, ImColor(node.outputs[pinIdx].GetColor()), 2.0f);
                    }
                }
            }

            // Link is complete - try to create it
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
    ed::EndCreate();

    // Handle deletion
    if (ed::BeginDelete()) {
        ed::LinkId linkId;
        while (ed::QueryDeletedLink(&linkId)) {
            if (ed::AcceptDeletedItem()) {
                DeleteLink(linkId);
            }
        }

        ed::NodeId nodeId;
        while (ed::QueryDeletedNode(&nodeId)) {
            if (ed::AcceptDeletedItem()) {
                DeleteNode(static_cast<ECS::EntityID>(nodeId.Get()));
            }
        }
    }
    ed::EndDelete();
}

bool NodeGraphView::CanCreateLink(ed::PinId startPinId, ed::PinId endPinId) {
    auto [startNodeId, startPinIdx, startIsInput] = DecodePinId(startPinId.Get());
    auto [endNodeId, endPinIdx, endIsInput] = DecodePinId(endPinId.Get());

    // Must connect input to output
    if (startIsInput == endIsInput)
        return false;

    // Can't connect to same node
    if (startNodeId == endNodeId)
        return false;

    // Get the nodes
    if (!mgr.HasComponent<ECS::NodeComponent>(startNodeId) || !mgr.HasComponent<ECS::NodeComponent>(endNodeId))
        return false;

    auto &startNode = mgr.GetComponent<ECS::NodeComponent>(startNodeId);
    auto &endNode = mgr.GetComponent<ECS::NodeComponent>(endNodeId);

    // Always make startPin be the output
    if (startIsInput) {
        std::swap(startNodeId, endNodeId);
        std::swap(startPinIdx, endPinIdx);
        std::swap(startNode, endNode);
        std::swap(startIsInput, endIsInput);
    }

    // Validate pin indices and types
    if (startPinIdx >= startNode.outputs.size() || endPinIdx >= endNode.inputs.size())
        return false;

    return startNode.outputs[startPinIdx].IsCompatibleWith(endNode.inputs[endPinIdx]);
}

void NodeGraphView::CreateLink(ed::PinId startPinId, ed::PinId endPinId) {
    auto startInfo = DecodePinId(startPinId.Get());
    auto endInfo = DecodePinId(endPinId.Get());

    auto linkEntity = mgr.AddNewEntity();
    mgr.AddComponent<ECS::LinkComponent>(linkEntity);
    auto &link = mgr.GetComponent<ECS::LinkComponent>(linkEntity);

    // Make sure start is output and end is input
    if (std::get<2>(startInfo)) { // if startPin is input
        std::swap(startInfo, endInfo);
    }

    link.startNode = std::get<0>(startInfo);
    link.startPinIndex = std::get<1>(startInfo);
    link.endNode = std::get<0>(endInfo);
    link.endPinIndex = std::get<1>(endInfo);
    link.thickness = 2.0f;

    // Update pin connection states
    auto &startNode = mgr.GetComponent<ECS::NodeComponent>(link.startNode);
    auto &endNode = mgr.GetComponent<ECS::NodeComponent>(link.endNode);

    startNode.outputs[link.startPinIndex].isConnected = true;
    endNode.inputs[link.endPinIndex].isConnected = true;
}

void NodeGraphView::DeleteLink(ed::LinkId linkId) {
    auto linkEntity = static_cast<ECS::EntityID>(linkId.Get());
    if (!mgr.HasComponent<ECS::LinkComponent>(linkEntity))
        return;

    auto &link = mgr.GetComponent<ECS::LinkComponent>(linkEntity);

    // Update connection states of pins
    if (mgr.HasComponent<ECS::NodeComponent>(link.startNode)) {
        auto &startNode = mgr.GetComponent<ECS::NodeComponent>(link.startNode);
        if (link.startPinIndex < startNode.outputs.size()) {
            startNode.outputs[link.startPinIndex].isConnected = false;
        }
    }
    if (mgr.HasComponent<ECS::NodeComponent>(link.endNode)) {
        auto &endNode = mgr.GetComponent<ECS::NodeComponent>(link.endNode);
        if (link.endPinIndex < endNode.inputs.size()) {
            endNode.inputs[link.endPinIndex].isConnected = false;
        }
    }

    mgr.DestroyEntity(linkEntity);
}

ECS::EntityID NodeGraphView::CreateMathNode(const ImVec2 &pos) {
    auto entity = mgr.AddNewEntity();
    mgr.AddComponent<ECS::NodeComponent>(entity);
    auto &node = mgr.GetComponent<ECS::NodeComponent>(entity);

    node.name = "Math Node";
    node.position = glm::vec2(pos.x, pos.y);
    node.size = glm::vec2(150.0f, 100.0f);

    node.inputs = {{"Value A", ECS::Pin::Type::Float}, {"Value B", ECS::Pin::Type::Float}};
    node.outputs = {{"Result", ECS::Pin::Type::Float}};

    ed::SetNodePosition(ed::NodeId(static_cast<intptr_t>(entity)), pos);
    return entity;
}

ECS::EntityID NodeGraphView::CreateStringNode(const ImVec2 &pos) {
    auto entity = mgr.AddNewEntity();
    mgr.AddComponent<ECS::NodeComponent>(entity);
    auto &node = mgr.GetComponent<ECS::NodeComponent>(entity);

    node.name = "String Node";
    node.position = glm::vec2(pos.x, pos.y);
    node.size = glm::vec2(150.0f, 100.0f);

    node.inputs = {{"Text In", ECS::Pin::Type::String}};
    node.outputs = {{"Modified", ECS::Pin::Type::String}};

    ed::SetNodePosition(ed::NodeId(static_cast<intptr_t>(entity)), pos);
    return entity;
}

ECS::EntityID NodeGraphView::CreateFlowNode(const ImVec2 &pos) {
    auto entity = mgr.AddNewEntity();
    mgr.AddComponent<ECS::NodeComponent>(entity);
    auto &node = mgr.GetComponent<ECS::NodeComponent>(entity);

    node.name = "Flow Control";
    node.position = glm::vec2(pos.x, pos.y);
    node.size = glm::vec2(150.0f, 100.0f);

    node.inputs = {{"In", ECS::Pin::Type::Flow}};
    node.outputs = {{"True", ECS::Pin::Type::Flow}, {"False", ECS::Pin::Type::Flow}};

    ed::SetNodePosition(ed::NodeId(static_cast<intptr_t>(entity)), pos);
    return entity;
}

void NodeGraphView::CreateExampleNodes() {
    CreateMathNode(ImVec2(100, 100));
    CreateStringNode(ImVec2(400, 100));
    CreateFlowNode(ImVec2(250, 300));
}

void NodeGraphView::CleanupEntities() {
    auto entities = mgr.GetAllEntities();
    for (auto entity : entities) {
        if (mgr.HasComponent<ECS::LinkComponent>(entity) || mgr.HasComponent<ECS::NodeComponent>(entity)) {
            mgr.DestroyEntity(entity);
        }
    }
}

std::tuple<ECS::EntityID, size_t, bool> NodeGraphView::DecodePinId(intptr_t id) {
    return {
        static_cast<ECS::EntityID>(id >> 16),    // nodeId
        static_cast<size_t>((id >> 1) & 0x7FFF), // pinIdx
        (id & 1) == 1                            // isInput
    };
}

intptr_t NodeGraphView::GeneratePinId(ECS::EntityID nodeId, size_t pinIdx, bool isInput) {
    return (static_cast<intptr_t>(nodeId) << 16) | (pinIdx << 1) | (isInput ? 1 : 0);
}

void NodeGraphView::DeleteNode(ECS::EntityID nodeId) {
    if (!mgr.HasComponent<ECS::NodeComponent>(nodeId))
        return;

    // Find and delete all connected links
    auto entities = mgr.GetAllEntities();
    for (auto entity : entities) {
        if (!mgr.HasComponent<ECS::LinkComponent>(entity))
            continue;

        auto &link = mgr.GetComponent<ECS::LinkComponent>(entity);
        if (link.startNode == nodeId || link.endNode == nodeId) {
            mgr.DestroyEntity(entity);
        }
    }

    mgr.DestroyEntity(nodeId);
}

} // namespace GUI