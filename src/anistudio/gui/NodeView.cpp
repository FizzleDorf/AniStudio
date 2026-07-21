#include "NodeView.hpp"
#include "imgui.h"
#include <iostream>
#include "Events.hpp"

namespace GUI {

    void NodeView::Init() {
        SetupContext();
        RegisterNodeTypes();

        auto numberNode = m_nodeFlow->addNode<NumberNode>(ImVec2(100, 100));
        m_registeredNodes.push_back(numberNode);

        auto opNode = m_nodeFlow->addNode<OperationNode>(ImVec2(300, 150));
        m_registeredNodes.push_back(opNode);

        auto opNode2 = m_nodeFlow->addNode<OperationNode>(ImVec2(500, 250));
        m_registeredNodes.push_back(opNode2);

        auto outPin = numberNode->outPin("Value");
        auto inPin = opNode->inPin("A");
        if (outPin && inPin) {
            outPin->createLink(inPin);
        }
    }

    void NodeView::Render() {
        std::string windowName = GetWindowTitle();
        bool windowOpen = true;

        ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
        if (ImGui::Begin(windowName.c_str(), &windowOpen)) {

            if (!windowOpen) {
                std::unordered_map<std::string, std::any> eventData;
                eventData["workspaceID"] = GetID();
                eventData["viewTypeName"] = viewName;
                ANI::Events::Ref().QueueEventWithData("RemoveView", eventData);
                ImGui::End();
                return;
            }

            m_nodeFlow->update();

            ImGui::SetCursorPos(ImVec2(10, 10));
            ImGui::Text("Nodes: %d", m_nodeFlow->getNodesCount());
            ImGui::SameLine(200);
            ImGui::Text("Pan with middle mouse, zoom with scroll wheel");
        }
        ImGui::End();
    }

    nlohmann::json NodeView::Serialize() const {
        nlohmann::json j = BaseView::Serialize();
        return j;
    }

    void NodeView::Deserialize(const nlohmann::json& j) {
        BaseView::Deserialize(j);
    }

    void NodeView::RegisterNodeTypes() {
    }

    void NodeView::SetupContext() {
        m_nodeFlow->rightClickPopUpContent([this](ImFlow::BaseNode* hoveredNode) {
            if (hoveredNode) {
                ImGui::Text("Node: %s", hoveredNode->getName().c_str());
                ImGui::Separator();

                if (ImGui::MenuItem("Delete Node")) {
                    hoveredNode->destroy();
                }
            }
            else {
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

        m_nodeFlow->droppedLinkPopUpContent([this](ImFlow::Pin* dragged) {
            ImGui::Text("Create node for %s", dragged->getName().c_str());
            ImGui::Separator();

            if (dragged->getType() == ImFlow::PinType_Output) {
                if (ImGui::MenuItem("Create Operation Node")) {
                    auto pos = m_nodeFlow->screen2grid(ImGui::GetMousePos());
                    auto node = m_nodeFlow->addNode<OperationNode>(pos);
                    m_registeredNodes.push_back(node);

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

                    auto outputPin = node->outPin("Value");
                    if (outputPin) {
                        outputPin->createLink(dragged);
                    }
                }
            }
            });
    }

    NumberNode::NumberNode() {
        setTitle("Number");
        setStyle(ImFlow::NodeStyle::cyan());

        m_output = addOUT<float>("Value");
        m_output->behaviour([this]() { return m_value; });
    }

    void NumberNode::draw() {
        ImGui::PushItemWidth(120);
        ImGui::SliderFloat("##value", &m_value, 0.0f, 100.0f, "%.2f");
        ImGui::PopItemWidth();

        ImGui::Text("Value: %.2f", m_value);
    }

    OperationNode::OperationNode(Operation op) : m_operation(op) {
        switch (m_operation) {
        case Add: setTitle("Add"); break;
        case Subtract: setTitle("Subtract"); break;
        case Multiply: setTitle("Multiply"); break;
        case Divide: setTitle("Divide"); break;
        }

        switch (m_operation) {
        case Add: setStyle(ImFlow::NodeStyle::green()); break;
        case Subtract: setStyle(ImFlow::NodeStyle::red()); break;
        case Multiply: setStyle(ImFlow::NodeStyle::brown()); break;
        case Divide: setStyle(ImFlow::NodeStyle::cyan()); break;
        }

        addIN<float>("A", 0.0f, ImFlow::ConnectionFilter::Numbers(), ImFlow::PinStyle::blue());
        addIN<float>("B", 0.0f, ImFlow::ConnectionFilter::Numbers(), ImFlow::PinStyle::blue());

        m_output = addOUT<float>("Result");
        m_output->behaviour([this]() {
            float a = 0.0f;
            float b = 0.0f;

            try {
                a = getInVal<float>("A");
            }
            catch (...) {
            }

            try {
                b = getInVal<float>("B");
            }
            catch (...) {
            }

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
        const char* opSymbol = "+";
        switch (m_operation) {
        case Add: opSymbol = "+"; break;
        case Subtract: opSymbol = "-"; break;
        case Multiply: opSymbol = "*"; break;
        case Divide: opSymbol = "/"; break;
        }

        float a = 0.0f;
        float b = 0.0f;

        try {
            a = getInVal<float>("A");
            b = getInVal<float>("B");
        }
        catch (...) {
        }

        float result = 0.0f;
        switch (m_operation) {
        case Add: result = a + b; break;
        case Subtract: result = a - b; break;
        case Multiply: result = a * b; break;
        case Divide: result = b != 0.0f ? a / b : 0.0f; break;
        }

        ImGui::Text("%.2f %s %.2f = %.2f", a, opSymbol, b, result);
    }

} // namespace GUI