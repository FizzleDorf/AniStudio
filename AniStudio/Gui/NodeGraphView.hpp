#ifndef NODEGRAPHVIEW_HPP
#define NODEGRAPHVIEW_HPP

#include "BaseView.hpp"
#include <imgui_node_editor.h>
#include <string>

namespace ed = ax::NodeEditor;

class NodeGraphView : public BaseView {
public:
    NodeGraphView();
    ~NodeGraphView();

    void Initialize();

    void Cleanup();

    void Render();

private:
    ed::EditorContext *m_Context;
};

#endif // NODEGRAPHVIEW_HPP
