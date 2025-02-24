#pragma once

#include "BaseSystem.hpp"
#include "NodeComponents/NodeComponents.hpp"
#include <queue>
#include <unordered_map>
#include <vector>

namespace ECS {

class NodeSystem : public BaseSystem {
public:
    NodeSystem(EntityManager &entityMgr) : BaseSystem(entityMgr) {
        sysName = "NodeSystem";
        AddComponentSignature<NodeComponent>();
    }

    ~NodeSystem() = default;

    void Start() override {}

    void Update(const float deltaT) override {
        if (isDirty) {
            executionOrder = GetExecutionOrder();
            isDirty = false;
        }

        for (EntityID entity : executionOrder) {
            ExecuteNode(entity);
        }
    }

    // Create a node from any component type
    EntityID CreateNode(ComponentTypeID typeId, const glm::vec2 &pos) {
        EntityID entity = mgr.AddNewEntity();

        // Add NodeComponent first
        mgr.AddComponent<NodeComponent>(entity);
        auto &node = mgr.GetComponent<NodeComponent>(entity);

        // Set initial position and other node properties
        node.position = pos;
        node.name = mgr.GetComponentName(typeId);

        // Add the actual component through EntityManager
        mgr.AddComponent(entity, typeId);

        isDirty = true;
        return entity;
    }

    // Create a connection between nodes
    bool ConnectNodes(EntityID sourceNode, size_t sourcePin, EntityID targetNode, size_t targetPin) {
        if (!ValidateConnection(sourceNode, sourcePin, targetNode, targetPin)) {
            return false;
        }

        EntityID linkEntity = mgr.AddNewEntity();
        mgr.AddComponent<LinkComponent>(linkEntity);
        auto &link = mgr.GetComponent<LinkComponent>(linkEntity);

        link.startNode = sourceNode;
        link.startPinIndex = sourcePin;
        link.endNode = targetNode;
        link.endPinIndex = targetPin;

        // Update pin connection states
        auto &sourceNodeComp = mgr.GetComponent<NodeComponent>(sourceNode);
        auto &targetNodeComp = mgr.GetComponent<NodeComponent>(targetNode);

        sourceNodeComp.outputs[sourcePin].isConnected = true;
        sourceNodeComp.outputs[sourcePin].targetEntityId = targetNode;

        targetNodeComp.inputs[targetPin].isConnected = true;
        targetNodeComp.inputs[targetPin].sourceEntityId = sourceNode;

        isDirty = true;
        return true;
    }

    // Remove a connection
    void DisconnectNodes(EntityID sourceNode, size_t sourcePin, EntityID targetNode, size_t targetPin) {
        // Find and remove the link entity
        auto linkEntity = FindLinkEntity(sourceNode, sourcePin, targetNode, targetPin);
        if (linkEntity != INVALID_ENTITY) {
            mgr.DestroyEntity(linkEntity);
        }

        // Update pin connection states
        if (mgr.HasComponent<NodeComponent>(sourceNode)) {
            auto &sourceNodeComp = mgr.GetComponent<NodeComponent>(sourceNode);
            if (sourcePin < sourceNodeComp.outputs.size()) {
                sourceNodeComp.outputs[sourcePin].isConnected = false;
                sourceNodeComp.outputs[sourcePin].targetEntityId = INVALID_ENTITY;
            }
        }

        if (mgr.HasComponent<NodeComponent>(targetNode)) {
            auto &targetNodeComp = mgr.GetComponent<NodeComponent>(targetNode);
            if (targetPin < targetNodeComp.inputs.size()) {
                targetNodeComp.inputs[targetPin].isConnected = false;
                targetNodeComp.inputs[targetPin].sourceEntityId = INVALID_ENTITY;
            }
        }

        isDirty = true;
    }

    // Delete a node and all its connections
    void DeleteNode(EntityID nodeId) {
        if (!mgr.HasComponent<NodeComponent>(nodeId)) {
            return;
        }

        // Find and remove all connected links
        auto entities = mgr.GetAllEntities();
        for (auto entity : entities) {
            if (!mgr.HasComponent<LinkComponent>(entity)) {
                continue;
            }

            auto &link = mgr.GetComponent<LinkComponent>(entity);
            if (link.startNode == nodeId || link.endNode == nodeId) {
                mgr.DestroyEntity(entity);
            }
        }

        // Remove the node entity itself
        mgr.DestroyEntity(nodeId);
        isDirty = true;
    }

private:
    bool isDirty = true;
    std::vector<EntityID> executionOrder;
    static constexpr EntityID INVALID_ENTITY = static_cast<EntityID>(-1);

    bool ValidateConnection(EntityID sourceNode, size_t sourcePin, EntityID targetNode, size_t targetPin) {
        if (!mgr.HasComponent<NodeComponent>(sourceNode) || !mgr.HasComponent<NodeComponent>(targetNode)) {
            return false;
        }

        auto &sourceNodeComp = mgr.GetComponent<NodeComponent>(sourceNode);
        auto &targetNodeComp = mgr.GetComponent<NodeComponent>(targetNode);

        // Check pin indices are valid
        if (sourcePin >= sourceNodeComp.outputs.size() || targetPin >= targetNodeComp.inputs.size()) {
            return false;
        }

        // Check pins aren't already connected
        if (sourceNodeComp.outputs[sourcePin].isConnected || targetNodeComp.inputs[targetPin].isConnected) {
            return false;
        }

        // Check pin types are compatible
        return sourceNodeComp.outputs[sourcePin].type == targetNodeComp.inputs[targetPin].type;
    }

    EntityID FindLinkEntity(EntityID sourceNode, size_t sourcePin, EntityID targetNode, size_t targetPin) {
        auto entities = mgr.GetAllEntities();
        for (auto entity : entities) {
            if (!mgr.HasComponent<LinkComponent>(entity)) {
                continue;
            }

            auto &link = mgr.GetComponent<LinkComponent>(entity);
            if (link.startNode == sourceNode && link.startPinIndex == sourcePin && link.endNode == targetNode &&
                link.endPinIndex == targetPin) {
                return entity;
            }
        }
        return INVALID_ENTITY;
    }

    std::vector<EntityID> GetExecutionOrder() {
        std::vector<EntityID> sorted;
        std::unordered_map<EntityID, int> inDegree;
        std::queue<EntityID> queue;

        // Calculate in-degrees
        for (EntityID entity : entities) {
            if (!mgr.HasComponent<NodeComponent>(entity)) {
                continue;
            }

            auto &node = mgr.GetComponent<NodeComponent>(entity);
            inDegree[entity] = 0;

            for (const auto &pin : node.inputs) {
                if (pin.isConnected) {
                    inDegree[entity]++;
                }
            }

            if (inDegree[entity] == 0) {
                queue.push(entity);
            }
        }

        // Topological sort
        while (!queue.empty()) {
            EntityID current = queue.front();
            queue.pop();
            sorted.push_back(current);

            auto &node = mgr.GetComponent<NodeComponent>(current);
            for (const auto &pin : node.outputs) {
                if (pin.isConnected) {
                    EntityID target = pin.targetEntityId;
                    inDegree[target]--;
                    if (inDegree[target] == 0) {
                        queue.push(target);
                    }
                }
            }
        }

        return sorted;
    }

    void ExecuteNode(EntityID entity) {
        auto components = mgr.GetEntityComponents(entity);
        for (auto compType : components) {
            mgr.ExecuteComponent(entity, compType);
        }
    }
};

} // namespace ECS