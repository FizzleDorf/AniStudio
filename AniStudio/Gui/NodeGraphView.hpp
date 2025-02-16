#ifndef NODEGRAPHVIEW_HPP
#define NODEGRAPHVIEW_HPP

#include "Base/BaseView.hpp"
#include "NodeGraphComponents/NodeComponents.hpp"
#include "node_utils.hpp"
#include <glm/glm.hpp>
#include <imgui_node_editor.h>
#include <tuple>

namespace ed = ax::NodeEditor;

namespace GUI {

class NodeGraphView : public BaseView {
public:
    NodeGraphView(ECS::EntityManager &entityMgr);
    ~NodeGraphView();

    void Init() override;
    void Cleanup();
    void Render() override;

private:
    void RenderMenuBar();
    void RenderNodes();
    void RenderPin(ECS::EntityID nodeId, const ECS::Pin &pin, size_t pinIdx, bool isInput);
    void RenderLinks();
    void HandleInteractions();

    // Node creation helpers
    ECS::EntityID CreateMathNode(const ImVec2 &pos);
    ECS::EntityID CreateStringNode(const ImVec2 &pos);
    ECS::EntityID CreateFlowNode(const ImVec2 &pos);
    void CreateExampleNodes();

    // Link management
    void CreateLink(ed::PinId startPinId, ed::PinId endPinId);
    bool CanCreateLink(ed::PinId startPinId, ed::PinId endPinId);
    void DeleteLink(ed::LinkId linkId);
    void HandleDeletion();
    void DeleteNode(ECS::EntityID nodeId);
    void CleanupEntities();

    // Pin ID utilities
    intptr_t GeneratePinId(ECS::EntityID nodeId, size_t pinIdx, bool isInput);
    std::tuple<ECS::EntityID, size_t, bool> DecodePinId(intptr_t id);

private:
    ed::EditorContext *m_Editor;
    bool isInitialized;
};

} // namespace GUI

#endif // NODEGRAPHVIEW_HPP