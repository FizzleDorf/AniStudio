// NodeData.h
#pragma once
#include <string>
#include <unordered_map>

struct NodeData {
    std::string class_type;
    std::unordered_map<std::string, std::string> string_params;
    std::unordered_map<std::string, int> int_params;
    std::unordered_map<std::string, float> float_params;
};