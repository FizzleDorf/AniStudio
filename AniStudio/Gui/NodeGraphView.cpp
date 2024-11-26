#include "NodeGraphView.hpp"
#include <imgui.h>

namespace ed = ax::NodeEditor;

NodeGraphView::NodeGraphView() {
    // Constructor could be empty, just used for initialization
}

NodeGraphView::~NodeGraphView() {
    // Cleanup when destroying the view
    Cleanup();
}

void NodeGraphView::Initialize() {
    // Initialize the editor context
    ed::Config config;
    config.SettingsFile = "Simple.json"; // Optional: Save the settings in a file
    m_Context = ed::CreateEditor(&config);
}

void NodeGraphView::Cleanup() {
    // Destroy the editor context when the view stops
    if (m_Context) {
        ed::DestroyEditor(m_Context);
        m_Context = nullptr;
    }
}

void NodeGraphView::Render() {
    // Set the current editor context to render nodes
    ed::SetCurrentEditor(m_Context);

    // Begin the node editor window
    ed::Begin("Node Graph Editor", ImVec2(0.0f, 0.0f));

    // Example: Creating a node with pins
    int uniqueId = 1;
    ed::BeginNode(uniqueId++);
    ImGui::Text("Node A");

    // Create an input pin
    ed::BeginPin(uniqueId++, ed::PinKind::Input);
    ImGui::Text("-> In");
    ed::EndPin();

    // Create an output pin
    ImGui::SameLine();
    ed::BeginPin(uniqueId++, ed::PinKind::Output);
    ImGui::Text("Out ->");
    ed::EndPin();

    ed::EndNode(); // End node

    // Finish rendering the node editor
    ed::End();

    // Reset the editor context (if necessary)
    ed::SetCurrentEditor(nullptr);
}
