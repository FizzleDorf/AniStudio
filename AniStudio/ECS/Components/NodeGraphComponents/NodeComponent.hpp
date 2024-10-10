
#include "BaseComponent.hpp"
#include <string>
#include <vector>
#include <variant>

using NodeDataType = std::variant<int, float, std::string, bool>;

struct NodeComponent : public ECS::BaseComponent {
    std::string nodeName;
    std::vector<NodeDataType> inputs;
    std::vector<NodeDataType> outputs;

    NodeComponent(const std::string& name, const std::vector<NodeDataType>& in, const std::vector<NodeDataType>& out)
        : nodeName(name), inputs(in), outputs(out) {}
};
