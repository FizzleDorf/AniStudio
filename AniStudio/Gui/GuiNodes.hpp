#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <nlohmann/json.hpp> // Include for JSON

class Node {
public:
    // Constructor
    Node() = default;

    // Adding and updating inputs
    void AddFloatInput(const std::string& key, float value);
    void AddStringInput(const std::string& key, const std::string& value);
    void AddArrayInput(const std::string& key, const std::vector<std::string>& arrayValue);

    // Setters and getters
    const std::string& GetClassType() const;
    void SetClassType(const std::string& type);

    const std::string& GetTitle() const;
    void SetTitle(const std::string& title);

    const std::unordered_map<std::string, float>& GetFloatInputs() const;
    const std::unordered_map<std::string, std::string>& GetStringInputs() const;
    const std::unordered_map<std::string, std::vector<std::string>>& GetArrayInputs() const;

    void UpdateValue(const std::string& key, float value);
    void UpdateValue(const std::string& key, const std::string& value);
    void UpdateArrayValue(const std::string& key, const std::vector<std::string>& arrayValue);

    // JSON handling
    void LoadFromJSON(const nlohmann::json& jsonData);
    nlohmann::json SaveToJSON() const;

private:
    std::string classType;
    std::string title;
    std::unordered_map<std::string, float> floatInputs;
    std::unordered_map<std::string, std::string> stringInputs;
    std::unordered_map<std::string, std::vector<std::string>> arrayInputs;
};
