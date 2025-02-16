#pragma once
#include "Base/BaseComponent.hpp"
#include <glm/glm.hpp>
#include <imgui.h>
#include <imgui_node_editor.h>
#include <string>
#include <variant>
#include <vector>

namespace ed = ax::NodeEditor;

namespace ECS {

// Pin value types
struct ObjectData {
    std::string className;
    std::string instanceName;
    EntityID referenceEntity = 0;
};

struct FunctionData {
    std::string functionName;
    std::vector<std::string> parameters;
    std::string returnType;
};

struct DelegateData {
    std::string delegateName;
    std::vector<std::string> parameters;
    std::vector<EntityID> boundEntities;
};

// Helper function to convert ImU32 to ImVec4
inline ImVec4 ColorU32ToVec4(ImU32 color) {
    return ImVec4(((color >> IM_COL32_R_SHIFT) & 0xFF) / 255.0f, ((color >> IM_COL32_G_SHIFT) & 0xFF) / 255.0f,
                  ((color >> IM_COL32_B_SHIFT) & 0xFF) / 255.0f, ((color >> IM_COL32_A_SHIFT) & 0xFF) / 255.0f);
}

// Helper function to convert ImVec4 to ImU32
inline ImU32 ColorVec4ToU32(const ImVec4 &color) { return ImColor(color); }

// Pin value variant type
using PinValue = std::variant<std::monostate, // Flow (no value)
                              bool,           // Bool
                              int,            // Int
                              float,          // Float
                              std::string,    // String
                              ObjectData,     // Object reference
                              FunctionData,   // Function reference
                              DelegateData    // Delegate reference
                              >;

// Pin structure
struct Pin {
    enum class Type {
        Flow,     // Execution flow
        Bool,     // Boolean value
        Int,      // Integer value
        Float,    // Float value
        String,   // String value
        Object,   // Object reference
        Function, // Function reference
        Delegate  // Delegate/Event reference
    };

    std::string name;
    Type type = Type::Flow;
    PinValue value;
    bool isConnected = false;

    ImVec4 GetColorVec4() const { return ColorU32ToVec4(GetColor()); }

    ImU32 GetColor() const {
        switch (type) {
        case Type::Flow:
            return IM_COL32(255, 255, 255, 255); // White
        case Type::Bool:
            return IM_COL32(220, 48, 48, 255); // Red
        case Type::Int:
            return IM_COL32(68, 201, 156, 255); // Turquoise
        case Type::Float:
            return IM_COL32(147, 226, 74, 255); // Green
        case Type::String:
            return IM_COL32(124, 21, 153, 255); // Purple
        case Type::Object:
            return IM_COL32(30, 144, 255, 255); // Dodger Blue
        case Type::Function:
            return IM_COL32(255, 165, 0, 255); // Orange
        case Type::Delegate:
            return IM_COL32(218, 112, 214, 255); // Orchid
        default:
            return IM_COL32(128, 128, 128, 255); // Gray
        }
    }

    bool IsCompatibleWith(const Pin &other) const {
        // Flow pins can only connect to other flow pins
        if (type == Type::Flow || other.type == Type::Flow) {
            return type == other.type;
        }

        // Type compatibility rules
        switch (type) {
        case Type::Bool:
            return other.type == Type::Bool;
        case Type::Int:
            return other.type == Type::Int || other.type == Type::Float;
        case Type::Float:
            return other.type == Type::Float || other.type == Type::Int;
        case Type::String:
            return other.type == Type::String;
        case Type::Object:
            return other.type == Type::Object;
        case Type::Function:
            return other.type == Type::Function;
        case Type::Delegate:
            return other.type == Type::Delegate;
        default:
            return false;
        }
    }
};

enum class NodeType { Blueprint, Simple, Tree, Comment, Houdini };

struct NodeComponent : public BaseComponent {
    NodeComponent() { compName = "NodeComponent"; }

    std::string name;
    NodeType type = NodeType::Simple;
    glm::vec2 position{0.0f, 0.0f};
    glm::vec2 size{150.0f, 100.0f};
    ImU32 color = IM_COL32(60, 60, 60, 255);
    bool dragging = false;
    std::string state;

    // Input and output pins
    std::vector<Pin> inputs;
    std::vector<Pin> outputs;

    void AddInput(const std::string &name, Pin::Type type) { inputs.push_back({name, type}); }

    void AddOutput(const std::string &name, Pin::Type type) { outputs.push_back({name, type}); }

    nlohmann::json Serialize() const override {
        nlohmann::json j = BaseComponent::Serialize();
        j["name"] = name;
        j["type"] = static_cast<int>(type);
        j["position"] = {position.x, position.y};
        j["size"] = {size.x, size.y};
        j["color"] = color;
        j["state"] = state;

        j["inputs"] = nlohmann::json::array();
        j["outputs"] = nlohmann::json::array();

        for (const auto &pin : inputs) {
            nlohmann::json pinJson;
            pinJson["name"] = pin.name;
            pinJson["type"] = static_cast<int>(pin.type);
            pinJson["isConnected"] = pin.isConnected;
            j["inputs"].push_back(pinJson);
        }

        for (const auto &pin : outputs) {
            nlohmann::json pinJson;
            pinJson["name"] = pin.name;
            pinJson["type"] = static_cast<int>(pin.type);
            pinJson["isConnected"] = pin.isConnected;
            j["outputs"].push_back(pinJson);
        }

        return j;
    }

    void Deserialize(const nlohmann::json &j) override {
        BaseComponent::Deserialize(j);
        name = j["name"];
        type = static_cast<NodeType>(j["type"]);
        position = {j["position"][0], j["position"][1]};
        size = {j["size"][0], j["size"][1]};
        color = j["color"];
        state = j["state"];

        inputs.clear();
        outputs.clear();

        for (const auto &pinJson : j["inputs"]) {
            Pin pin;
            pin.name = pinJson["name"];
            pin.type = static_cast<Pin::Type>(pinJson["type"]);
            pin.isConnected = pinJson["isConnected"];
            inputs.push_back(pin);
        }

        for (const auto &pinJson : j["outputs"]) {
            Pin pin;
            pin.name = pinJson["name"];
            pin.type = static_cast<Pin::Type>(pinJson["type"]);
            pin.isConnected = pinJson["isConnected"];
            outputs.push_back(pin);
        }
    }
};

struct LinkComponent : public BaseComponent {
    LinkComponent() { compName = "LinkComponent"; }

    EntityID startNode = 0;
    int startPinIndex = -1;
    EntityID endNode = 0;
    int endPinIndex = -1;
    bool isSelected = false;
    float thickness = 1.0f;

    ImU32 GetColor(const NodeComponent &startNode, const NodeComponent &endNode) const {
        if (startPinIndex >= 0 && startPinIndex < startNode.outputs.size() && endPinIndex >= 0 &&
            endPinIndex < endNode.inputs.size()) {
            return startNode.outputs[startPinIndex].GetColor();
        }
        return IM_COL32(255, 255, 255, 255);
    }

    bool IsValid(const NodeComponent &startNode, const NodeComponent &endNode) const {
        if (startPinIndex >= 0 && startPinIndex < startNode.outputs.size() && endPinIndex >= 0 &&
            endPinIndex < endNode.inputs.size()) {
            return startNode.outputs[startPinIndex].IsCompatibleWith(endNode.inputs[endPinIndex]);
        }
        return false;
    }
};

} // namespace ECS