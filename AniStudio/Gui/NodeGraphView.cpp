#include "NodeGraphView.hpp"
#include <iostream>

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
            if (ImGui::MenuItem("Input Image Node")) {
                CreateInputImageNode(ImGui::GetMousePos());
            }
            if (ImGui::MenuItem("Output Image Node")) {
                CreateOutputImageNode(ImGui::GetMousePos());
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
        NodeEditorUtils::BeginVertical("header"); // Use simplified namespace
        {
            ImGui::TextUnformatted(node.name.c_str());
            ImGui::Spacing();
        }
        NodeEditorUtils::EndVertical(); // Use simplified namespace

        // Content
        NodeEditorUtils::BeginHorizontal("content"); // Use simplified namespace
        {
            // Input pins on left
            NodeEditorUtils::BeginVertical("inputs"); // Use simplified namespace
            {
                for (size_t i = 0; i < node.inputs.size(); i++) {
                    RenderPin(node.inputs[i], i, true);
                }
            }
            NodeEditorUtils::EndVertical(); // Use simplified namespace

            // Spacing between pins
            NodeEditorUtils::Spring(1); // Use simplified namespace

            // Output pins on right
            NodeEditorUtils::BeginVertical("outputs"); // Use simplified namespace
            {
                for (size_t i = 0; i < node.outputs.size(); i++) {
                    RenderPin(node.outputs[i], i, false);
                }
            }
            NodeEditorUtils::EndVertical(); // Use simplified namespace
        }
        NodeEditorUtils::EndHorizontal(); // Use simplified namespace

        ImGui::PopStyleVar();
        ed::EndNode();

        // Update node position
        ImVec2 pos = ed::GetNodePosition(ed::NodeId(static_cast<intptr_t>(entity)));
        node.position = glm::vec2(pos.x, pos.y);
    }
}

void NodeGraphView::RenderPin(const ECS::Pin &pin, size_t pinIndex, bool isInput) {
    // Set the pin color based on its type
    ImU32 pinColor = pin.GetColor();

    // Begin the pin group
    ax::NodeEditor::BeginPin(pinIndex, isInput ? ax::NodeEditor::PinKind::Input : ax::NodeEditor::PinKind::Output);

    // Draw the pin as a circle
    NodeEditorUtils::BeginHorizontal(pin.name.c_str()); // Use simplified namespace
    if (isInput) {
        // Draw the pin on the left side for inputs
        ax::NodeEditor::PinPivotAlignment(ImVec2(0.0f, 0.5f));
        ax::NodeEditor::PinPivotSize(ImVec2(0.0f, 0.0f));
    } else {
        // Draw the pin on the right side for outputs
        ax::NodeEditor::PinPivotAlignment(ImVec2(1.0f, 0.5f));
        ax::NodeEditor::PinPivotSize(ImVec2(0.0f, 0.0f));
    }

    // Draw the pin circle
    ImGui::PushStyleColor(ImGuiCol_Button, pinColor);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, pinColor);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, pinColor);
    ImGui::Button("", ImVec2(10, 10)); // Small circle for the pin
    ImGui::PopStyleColor(3);

    // Draw the pin label
    if (isInput) {
        NodeEditorUtils::Spring(0.0f);
        ImGui::TextUnformatted(pin.name.c_str());
    } else {
        ImGui::TextUnformatted(pin.name.c_str());
        NodeEditorUtils::Spring(0.0f);
    }

    NodeEditorUtils::EndHorizontal();

    // End the pin group
    ax::NodeEditor::EndPin();
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
    // Handle link creation
    if (ed::BeginCreate()) {
        ed::PinId startPinId, endPinId;
        if (ed::QueryNewLink(&startPinId, &endPinId)) {
            if (startPinId && endPinId && CanCreateLink(startPinId, endPinId)) {
                if (ed::AcceptNewItem()) {
                    CreateLink(startPinId, endPinId);
                }
            } else {
                ed::RejectNewItem();
            }
        }
        ed::EndCreate();
    }

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
        ed::EndDelete();
    }

    // Handle context menu
    if (ed::ShowBackgroundContextMenu()) {
        ImGui::OpenPopup("NodeContextMenu");
    }

    if (ImGui::BeginPopup("NodeContextMenu")) {
        if (ImGui::MenuItem("Create Input Image Node")) {
            CreateInputImageNode(ImGui::GetMousePos());
        }
        if (ImGui::MenuItem("Create Output Image Node")) {
            CreateOutputImageNode(ImGui::GetMousePos());
        }
        ImGui::EndPopup();
    }
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

ECS::EntityID NodeGraphView::CreateInputImageNode(const ImVec2 &pos) {
    auto entity = mgr.AddNewEntity();
    mgr.AddComponent<ECS::NodeComponent>(entity);
    auto &node = mgr.GetComponent<ECS::NodeComponent>(entity);

    node.name = "Input Image Node";
    node.position = glm::vec2(pos.x, pos.y);
    node.size = glm::vec2(150.0f, 100.0f);

    // Define input and output pins
    node.inputs = {};                                  // No inputs for this node
    node.outputs = {{"Image", ECS::Pin::Type::Image}}; // Output an image

    // Set the node position in the editor
    ed::SetNodePosition(ed::NodeId(static_cast<intptr_t>(entity)), pos);
    return entity;
}

ECS::EntityID NodeGraphView::CreateOutputImageNode(const ImVec2 &pos) {
    auto entity = mgr.AddNewEntity();
    mgr.AddComponent<ECS::NodeComponent>(entity);
    auto &node = mgr.GetComponent<ECS::NodeComponent>(entity);

    node.name = "Output Image Node";
    node.position = glm::vec2(pos.x, pos.y);
    node.size = glm::vec2(150.0f, 100.0f);

    // Define input and output pins
    node.inputs = {{"Image", ECS::Pin::Type::Image}}; // Input an image
    node.outputs = {};                                // No outputs for this node

    // Set the node position in the editor
    ed::SetNodePosition(ed::NodeId(static_cast<intptr_t>(entity)), pos);
    return entity;
}
} // namespace GUI