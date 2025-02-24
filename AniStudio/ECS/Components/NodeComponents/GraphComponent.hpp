
#include "BaseComponent.hpp"
#include <string>
#include <variant>
#include <vector>

struct GraphComponent : public ECS::BaseComponent {
    std::string graphName;
    std::vector<int> nodeIDs;

    GraphComponent();
    void addNode(int nodeID);
    void removeNode(int nodeID);
    void clearNodes();
    void loadJson();
    void saveJson();
};
