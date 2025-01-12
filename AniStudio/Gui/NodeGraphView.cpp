#include "NodeGraphView.hpp"

namespace ed = ax::NodeEditor;

namespace GUI {

NodeGraphView::NodeGraphView(ECS::EntityManager &entityMgr) : BaseView(entityMgr), m_Context(nullptr) { Initialize(); }

NodeGraphView::~NodeGraphView() { Cleanup(); }

void NodeGraphView::Initialize() {
    ed::Config config;
    config.SettingsFile = nullptr; // Disable settings file
    m_Context = ed::CreateEditor(&config);
}

void NodeGraphView::Cleanup() {
    if (m_Context) {
        ed::DestroyEditor(m_Context);
        m_Context = nullptr;
    }
}

void NodeGraphView::Render() {
    if (!m_Context)
        return;

    // Just create a basic ImGui window first
    ImGui::Begin("Node Graph");

    // Set and begin editor with minimal configuration
    ed::SetCurrentEditor(m_Context);
    ed::Begin("Simple Editor");

    // Just draw a single static node for testing
    ed::BeginNode(1);
    ImGui::Text("Test Node");
    ed::EndNode();

    // End editor and window
    ed::End();
    ed::SetCurrentEditor(nullptr);

    ImGui::End();
}
} // namespace GUI