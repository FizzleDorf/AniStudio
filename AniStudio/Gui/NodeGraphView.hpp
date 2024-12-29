#ifndef NODEGRAPHVIEW_HPP
#define NODEGRAPHVIEW_HPP

#include "BaseView.hpp"
#include <imgui_node_editor.h>
#include <string>

namespace ed = ax::NodeEditor;

class NodeGraphView : public BaseView {
public:
    NodeGraphView();  // Constructor to initialize editor context
    ~NodeGraphView(); // Destructor to clean up resources

    // Initialize the node editor context
    void Initialize();

    // Clean up when the editor is stopped
    void Cleanup();

    // Main render loop for drawing the node editor
    void Render();

private:
    // Editor context to manage the node graph
    ed::EditorContext *m_Context;
};

#endif // NODEGRAPHVIEW_HPP
