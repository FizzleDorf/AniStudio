#pragma once
#include "Base/BaseView.hpp"
#include "ImNodeFlow.h"
#include "ECS.h"
#include <string>
#include <memory>
#include <functional>
#include <vector>

namespace GUI {

    // Forward declarations for node types
    class NumberNode;
    class OperationNode;

    class NodeView : public BaseView {
    public:
        NodeView(ECS::EntityManager& entityMgr);
        ~NodeView() = default;

        // Delete copy constructor and assignment operator
        NodeView(const NodeView&) = delete;
        NodeView& operator=(const NodeView&) = delete;

        // Move constructor and assignment operator
        NodeView(NodeView&&) = default;
        NodeView& operator=(NodeView&&) = default;

        // Overrides from BaseView
        void Init() override;
        void Render() override;
        nlohmann::json Serialize() const override;
        void Deserialize(const nlohmann::json& j) override;

    private:
        // Node editor instance
        std::unique_ptr<ImFlow::ImNodeFlow> m_nodeFlow;

        // Node registry
        std::vector<std::shared_ptr<ImFlow::BaseNode>> m_registeredNodes;

        // Helper methods
        void RegisterNodeTypes();
        void SetupContext();
    };

    // Basic number node that can output a value
    class NumberNode : public ImFlow::BaseNode {
    public:
        NumberNode();
        void draw() override;

    private:
        float m_value = 0.0f;
        std::shared_ptr<ImFlow::OutPin<float>> m_output;
    };

    // Operation node that takes inputs and produces an output
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