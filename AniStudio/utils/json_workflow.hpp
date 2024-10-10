#ifndef JSON_WORKFLOW_HPP
#define JSON_WORKFLOW_HPP

#include <nlohmann/json.hpp>
#include <NodeData.h>
#include <fstream>
#include <stdexcept>

using json = nlohmann::json;

// Function to parse JSON into nodes
inline std::unordered_map<int, NodeData> ParseJsonToNodes(const json& j) {
    std::unordered_map<int, NodeData> nodes;

    for (auto& [id, node_data] : j.items()) {
        NodeData node;
        node.class_type = node_data.value().at("class_type").get<std::string>();
        node.title = node_data.value().at("_meta").at("title").get<std::string>();

        // Read parameters based on type
        for (auto& [key, value] : node_data.value().at("inputs").items()) {
            if (value.is_string()) {
                node.string_params[key] = value.get<std::string>();
            }
            else if (value.is_number_integer()) {
                node.int_params[key] = value.get<int>();
            }
            else if (value.is_number_float()) {
                node.float_params[key] = value.get<float>();
            }
        }

        nodes[std::stoi(id)] = node;
    }

    return nodes;
}

// Load nodes from a JSON file
inline std::unordered_map<int, NodeData> LoadNodesFromJson(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + path);
    }

    json j;
    file >> j;
    file.close();

    return ParseJsonToNodes(j);
}

// Save nodes to a JSON file
inline void SaveNodesToJson(const std::string& path, const std::unordered_map<int, NodeData>& nodes) {
    json j;

    for (const auto& [id, node] : nodes) {
        json node_data;
        node_data["class_type"] = node.class_type;
        node_data["_meta"]["title"] = node.title;

        // Add parameters to JSON
        json inputs;
        for (const auto& [key, value] : node.string_params) {
            inputs[key] = value;
        }
        for (const auto& [key, value] : node.int_params) {
            inputs[key] = value;
        }
        for (const auto& [key, value] : node.float_params) {
            inputs[key] = value;
        }

        node_data["inputs"] = inputs;
        j[std::to_string(id)] = node_data;
    }

    std::ofstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + path);
    }

    file << j.dump(4);  // Pretty print with an indent of 4 spaces
    file.close();
}

#endif // JSON_WORKFLOW_HPP
