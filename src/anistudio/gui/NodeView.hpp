#pragma once

#include "GUI.h"
#include "ImNodeFlow.h"
#include "ECS.h"
#include <string>
#include <memory>
#include <functional>
#include <vector>

namespace GUI {

    class NumberNode;
    class OperationNode;

    class NodeView : public BaseView {
    public:
        static constexpr const char* GetMetadataJSON() {
            return R"({
            "displayName": "Node View",
            "category": "Tools",
            "description": "Moar Nodes."
        })";
        }

        NodeView(ECS::EntityManager& mgr, ViewManager& vm)
            : BaseView(mgr, vm) {
            viewName = "NodeView";
            m_nodeFlow = std::make_unique<ImFlow::ImNodeFlow>("NodeView");

            auto& style = m_nodeFlow->getStyle();
            style.grid_size = 32.0f;
            style.grid_subdivisions = 4.0f;
            style.colors.background = IM_COL32(40, 40, 40, 255);
            style.colors.grid = IM_COL32(100, 100, 100, 40);
            style.colors.subGrid = IM_COL32(80, 80, 80, 20);
        }
        ~NodeView() = default;

        NodeView(const NodeView&) = delete;
        NodeView& operator=(const NodeView&) = delete;

        NodeView(NodeView&&) = default;
        NodeView& operator=(NodeView&&) = default;

        void Init() override;
        void Render() override;
        nlohmann::json Serialize() const override;
        void Deserialize(const nlohmann::json& j) override;

    private:
        std::unique_ptr<ImFlow::ImNodeFlow> m_nodeFlow;
        std::vector<std::shared_ptr<ImFlow::BaseNode>> m_registeredNodes;

        void RegisterNodeTypes();
        void SetupContext();
    };

    class NumberNode : public ImFlow::BaseNode {
    public:
        NumberNode();
        void draw() override;

    private:
        float m_value = 0.0f;
        std::shared_ptr<ImFlow::OutPin<float>> m_output;
    };

    class OperationNode : public ImFlow::BaseNode {
    public:
        enum Operation {
            Add,
            Subtract,
            Multiply,
            Divide
        };

        OperationNode(Operation op = Add);
        void draw() override;

    private:
        Operation m_operation;
        std::shared_ptr<ImFlow::OutPin<float>> m_output;
    };

} // namespace GUI