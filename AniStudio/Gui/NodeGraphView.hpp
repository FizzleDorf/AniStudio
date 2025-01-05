#ifndef NODEGRAPHVIEW_HPP
#define NODEGRAPHVIEW_HPP

#include "Base/BaseView.hpp"
#include <imgui_node_editor.h>
#include <string>

namespace ed = ax::NodeEditor;

namespace GUI {
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
} // namespace GUI

#endif // NODEGRAPHVIEW_HPP
