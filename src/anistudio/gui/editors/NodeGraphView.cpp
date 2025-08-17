#include "NodeGraphView.hpp"
#include "../events/Events.hpp"

namespace ed = ax::NodeEditor;

namespace GUI {

NodeGraphView::NodeGraphView(ECS::EntityManager &entityMgr) : BaseView(entityMgr), m_Context(nullptr) {
	viewName = "NodeGraphView";
}

NodeGraphView::~NodeGraphView() { Cleanup(); }

void NodeGraphView::Init() {
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

	std::string windowName = GetWindowTitle();
	bool windowOpen = true;

    // Just create a basic ImGui window first
	if (ImGui::Begin(windowName.c_str(), &windowOpen)) {
		if (!windowOpen) {
			ANI::Events::Ref().RequestRemoveView(GetID(), viewName);
		}

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
	}
    ImGui::End();
}
} // namespace GUI