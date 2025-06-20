/*
		d8888          d8b  .d8888b.  888                  888 d8b
	   d88888          Y8P d88P  Y88b 888                  888 Y8P
	  d88P888              Y88b.      888                  888
	 d88P 888 88888b.  888  "Y888b.   888888 888  888  .d88888 888  .d88b.
	d88P  888 888 "88b 888     "Y88b. 888    888  888 d88" 888 888 d88""88b
   d88P   888 888  888 888       "888 888    888  888 888  888 888 888  888
  d8888888888 888  888 888 Y88b  d88P Y88b.  Y88b 888 Y88b 888 888 Y88..88P
 d88P     888 888  888 888  "Y8888P"   "Y888  "Y88888  "Y88888 888  "Y88P"

 * This file is part of AniStudio.
 * Copyright (C) 2025 FizzleDorf (AnimAnon)
 *
 * This software is dual-licensed under the GNU Lesser General Public License v3.0 (LGPL-3.0)
 * and a commercial license. You may choose to use it under either license.
 *
 * For the LGPL-3.0, see the LICENSE-LGPL-3.0.txt file in the repository.
 * For commercial license iformation, please contact legal@kframe.ai.
 */

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