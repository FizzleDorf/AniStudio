#include "NodeView.hpp"
#include "imgui.h"
#include <iostream>

namespace GUI {

    NodeView::NodeView(ECS::EntityManager& entityMgr)
        : BaseView(entityMgr)
    {
        viewName = "Node Editor";
        m_nodeFlow = std::make_unique<ImFlow::ImNodeFlow>("NodeEditor");

        // Set style properties
        auto& style = m_nodeFlow->getStyle();
        style.grid_size = 32.0f;
        style.grid_subdivisions = 4.0f;
        style.colors.background = IM_COL32(40, 40, 40, 255);
        style.colors.grid = IM_COL32(100, 100, 100, 40);
        style.colors.subGrid = IM_COL32(80, 80, 80, 20);
    }

    void NodeView::Init() {
        SetupContext();
        RegisterNodeTypes();

        // Add a few example nodes to start with
        auto numberNode = m_nodeFlow->addNode<NumberNode>(ImVec2(100, 100));
        m_registeredNodes.push_back(numberNode);

        // Create operation node at a different position
        auto opNode = m_nodeFlow->addNode<OperationNode>(ImVec2(300, 150));
        m_registeredNodes.push_back(opNode);

        // Add a second operation node to demonstrate connections
        auto opNode2 = m_nodeFlow->addNode<OperationNode>(ImVec2(500, 250));
        m_registeredNodes.push_back(opNode2);

        // Example connection between nodes
        auto outPin = numberNode->outPin("Value");
        auto inPin = opNode->inPin("A");
        if (outPin && inPin) {
            outPin->createLink(inPin);
        }
    }

    void NodeView::Render() {
        ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
        if (ImGui::Begin(viewName.c_str())) {
            // Process node editor
            m_nodeFlow->update();

            // Display node count in the top corner
            ImGui::SetCursorPos(ImVec2(10, 10));
            ImGui::Text("Nodes: %d", m_nodeFlow->getNodesCount());
            ImGui::SameLine(200);
            ImGui::Text("Pan with middle mouse, zoom with scroll wheel");
        }
        ImGui::End();
    }

    nlohmann::json NodeView::Serialize() const {
        nlohmann::json j = BaseView::Serialize();
        // TODO: Implement serialization of nodes and connections
        return j;
    }

    void NodeView::Deserialize(const nlohmann::json& j) {
        BaseView::Deserialize(j);
        // TODO: Implement deserialization of nodes and connections
    }

    void NodeView::RegisterNodeTypes() {
        // Register factory functions for node types if needed
    }

    void NodeView::SetupContext() {
        // Setup right-click context menu
        m_nodeFlow->rightClickPopUpContent([this](ImFlow::BaseNode* hoveredNode) {
            if (hoveredNode) {
                // Node-specific context menu
                ImGui::Text("Node: %s", hoveredNode->getName().c_str());
                ImGui::Separator();

                if (ImGui::MenuItem("Delete Node")) {
                    hoveredNode->destroy();
                }
            }
            else {
                // General context menu for creating new nodes
                ImGui::Text("Create New Node");
                ImGui::Separator();

                if (ImGui::MenuItem("Number Node")) {
                    auto pos = m_nodeFlow->screen2grid(ImGui::GetMousePos());
                    auto node = m_nodeFlow->addNode<NumberNode>(pos);
                    m_registeredNodes.push_back(node);
                }

                if (ImGui::MenuItem("Add Operation Node")) {
                    auto pos = m_nodeFlow->screen2grid(ImGui::GetMousePos());
                    auto node = m_nodeFlow->addNode<OperationNode>(pos);
                    m_registeredNodes.push_back(node);
                }
            }
            });

        // Setup dropped link popup
        m_nodeFlow->droppedLinkPopUpContent([this](ImFlow::Pin* dragged) {
            ImGui::Text("Create node for %s", dragged->getName().c_str());
            ImGui::Separator();

            if (dragged->getType() == ImFlow::PinType_Output) {
                if (ImGui::MenuItem("Create Operation Node")) {
                    auto pos = m_nodeFlow->screen2grid(ImGui::GetMousePos());
                    auto node = m_nodeFlow->addNode<OperationNode>(pos);
                    m_registeredNodes.push_back(node);

                    // Connect the dragged output to the first input of our new node
                    auto inputPin = node->inPin("A");
                    if (inputPin) {
                        dragged->createLink(inputPin);
                    }
                }
            }
            else {
                if (ImGui::MenuItem("Create Number Node")) {
                    auto pos = m_nodeFlow->screen2grid(ImGui::GetMousePos());
                    auto node = m_nodeFlow->addNode<NumberNode>(pos);
                    m_registeredNodes.push_back(node);

                    // Connect the new node's output to the dragged input
                    auto outputPin = node->outPin("Value");
                    if (outputPin) {
                        outputPin->createLink(dragged);
                    }
                }
            }
            });
    }

    // NumberNode implementation
    NumberNode::NumberNode() {
        setTitle("Number");
        setStyle(ImFlow::NodeStyle::cyan());

        // Create an output pin
        m_output = addOUT<float>("Value");
        m_output->behaviour([this]() { return m_value; });
    }

    void NumberNode::draw() {
        // Slider for adjusting the value
        ImGui::PushItemWidth(120);
        ImGui::SliderFloat("##value", &m_value, 0.0f, 100.0f, "%.2f");
        ImGui::PopItemWidth();

        // Display the current value
        ImGui::Text("Value: %.2f", m_value);
    }

    // OperationNode implementation
    OperationNode::OperationNode(Operation op) : m_operation(op) {
        // Set title based on operation
        switch (m_operation) {
        case Add: setTitle("Add"); break;
        case Subtract: setTitle("Subtract"); break;
        case Multiply: setTitle("Multiply"); break;
        case Divide: setTitle("Divide"); break;
        }

        // Set node style
        switch (m_operation) {
        case Add: setStyle(ImFlow::NodeStyle::green()); break;
        case Subtract: setStyle(ImFlow::NodeStyle::red()); break;
        case Multiply: setStyle(ImFlow::NodeStyle::brown()); break;
        case Divide: setStyle(ImFlow::NodeStyle::cyan()); break;
        }

        // Create input pins - these need to be created permanently to allow connections
        addIN<float>("A", 0.0f, ImFlow::ConnectionFilter::Numbers(), ImFlow::PinStyle::blue());
        addIN<float>("B", 0.0f, ImFlow::ConnectionFilter::Numbers(), ImFlow::PinStyle::blue());

        // Create an output pin with the calculation behavior
        m_output = addOUT<float>("Result");
        m_output->behaviour([this]() {
            // Get input values (with defaults)
            float a = 0.0f;
            float b = 0.0f;

            try {
                a = getInVal<float>("A");
            }
            catch (...) {
                // Use default if pin not found
            }

            try {
                b = getInVal<float>("B");
            }
            catch (...) {
                // Use default if pin not found
            }

            // Perform operation
            switch (m_operation) {
            case Add: return a + b;
            case Subtract: return a - b;
            case Multiply: return a * b;
            case Divide: return b != 0.0f ? a / b : 0.0f;
            default: return 0.0f;
            }
            });
    }

    void OperationNode::draw() {
        // Display the calculation
        const char* opSymbol = "+";
        switch (m_operation) {
        case Add: opSymbol = "+"; break;
        case Subtract: opSymbol = "-"; break;
        case Multiply: opSymbol = "Å~"; break;
        case Divide: opSymbol = "ÅÄ"; break;
        }

        float a = 0.0f;
        float b = 0.0f;

        try {
            a = getInVal<float>("A");
            b = getInVal<float>("B");
        }
        catch (...) {
            // Handle any exceptions from missing pins
        }

        // Calculate result
        float result = 0.0f;
        switch (m_operation) {
        case Add: result = a + b; break;
        case Subtract: result = a - b; break;
        case Multiply: result = a * b; break;
        case Divide: result = b != 0.0f ? a / b : 0.0f; break;
        }

        // Show operation and result
        ImGui::Text("%.2f %s %.2f = %.2f", a, opSymbol, b, result);
    }

} // namespace GUI