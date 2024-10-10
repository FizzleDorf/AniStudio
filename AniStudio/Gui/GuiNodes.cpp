#include "GuiNodes.hpp"

// Adding and updating inputs
void Node::AddFloatInput(const std::string& key, float value) {
    floatInputs[key] = value;
}

void Node::AddStringInput(const std::string& key, const std::string& value) {
    stringInputs[key] = value;
}

void Node::AddArrayInput(const std::string& key, const std::vector<std::string>& arrayValue) {
    arrayInputs[key] = arrayValue;
}

const std::string& Node::GetClassType() const {
    return classType;
}

void Node::SetClassType(const std::string& type) {
    classType = type;
}

const std::string& Node::GetTitle() const {
    return title;
}

void Node::SetTitle(const std::string& title) {
    this->title = title;
}

const std::unordered_map<std::string, float>& Node::GetFloatInputs() const {
    return floatInputs;
}

const std::unordered_map<std::string, std::string>& Node::GetStringInputs() const {
    return stringInputs;
}

const std::unordered_map<std::string, std::vector<std::string>>& Node::GetArrayInputs() const {
    return arrayInputs;
}

void Node::UpdateValue(const std::string& key, float value) {
    floatInputs[key] = value;
}

void Node::UpdateValue(const std::string& key, const std::string& value) {
    stringInputs[key] = value;
}

void Node::UpdateArrayValue(const std::string& key, const std::vector<std::string>& arrayValue) {
    arrayInputs[key] = arrayValue;
}

// JSON handling
void Node::LoadFromJSON(const nlohmann::json& jsonData) {
    if (jsonData.contains("inputs")) {
        const auto& inputs = jsonData["inputs"];
        for (auto& [key, value] : inputs.items()) {
            if (value.is_number_float()) {
                floatInputs[key] = value.get<float>();
            }
            else if (value.is_string()) {
                stringInputs[key] = value.get<std::string>();
            }
            else if (value.is_array()) {
                std::vector<std::string> arrayValues;
                for (const auto& item : value) {
                    if (item.is_string()) {
                        arrayValues.push_back(item.get<std::string>());
                    }
                }
                arrayInputs[key] = arrayValues;
            }
        }
    }

    if (jsonData.contains("class_type")) {
        classType = jsonData["class_type"].get<std::string>();
    }

    if (jsonData.contains("_meta") && jsonData["_meta"].contains("title")) {
        title = jsonData["_meta"]["title"].get<std::string>();
    }
}

nlohmann::json Node::SaveToJSON() const {
    nlohmann::json jsonData;

    // Save inputs
    nlohmann::json inputsJson;
    for (const auto& [key, value] : floatInputs) {
        inputsJson[key] = value;
    }
    for (const auto& [key, value] : stringInputs) {
        inputsJson[key] = value;
    }
    for (const auto& [key, array] : arrayInputs) {
        nlohmann::json arrayJson = array;
        inputsJson[key] = arrayJson;
    }
    jsonData["inputs"] = inputsJson;

    // Save class type
    jsonData["class_type"] = classType;

    // Save title
    jsonData["_meta"]["title"] = title;

    return jsonData;
}
