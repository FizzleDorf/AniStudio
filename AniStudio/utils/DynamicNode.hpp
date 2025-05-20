/*
		d8888          d8b  .d8888b.  888                  888 d8b
	   d88888          Y8P d88P  Y88b 888                  888 Y8P
	  d88P888              Y88b.      888                  888
	 d88P 888 88888b.  888  "Y888b.   888888 888  888  .d88888 888  .d88b.
	d88P  888 888 "88b 888     "Y88b. 888    888  888 d88" 888 888 d88""88b
   d88P   888 888  888 888       "888 888    888  888 888  888 888 888  888
  d8888888888 888  888 888 Y88b  d88P Y88b.  Y88b 888 Y88b 888 888 Y88..88P
 d88P     888 888  888 888  "Y8888P"   "Y888  "Y88888  "Y88888 888  "Y88P"

 * This file is part of AniStudio.
 * Copyright (C) 2025 FizzleDorf (AnimAnon)
 *
 * This software is dual-licensed under the GNU Lesser General Public License v3.0 (LGPL-3.0)
 * and a commercial license. You may choose to use it under either license.
 *
 * For the LGPL-3.0, see the LICENSE-LGPL-3.0.txt file in the repository.
 * For commercial license iformation, please contact legal@kframe.ai.
 */

// DynamicNode.hpp
#pragma once

#include <ImNodeFlow.h>
#include <ImJSchema.h>
#include "NodeComponent.hpp"
#include "EntityManager.hpp"

class DynamicNode : public ImFlow::BaseNode {
public:
    DynamicNode(EntityID entityId, EntityManager& entityManager)
        : m_entityId(entityId), m_entityManager(entityManager) {

        // Setup from component schemas
        setupFromComponents();
    }

    void draw() override {
        // Get node component
        auto* nodeComp = m_entityManager.GetComponent<ECS::NodeComponent>(m_entityId);
        if (!nodeComp) return;

        // Draw pins and content
        drawInputPins();
        drawComponentUI();
        drawOutputPins();

        // Update node position back to component
        nodeComp->position = getPos();

        // Execute if needed
        if (nodeComp->needsExecution) {
            executeComponents();
            nodeComp->needsExecution = false;
        }
    }

    EntityID getEntityId() const { return m_entityId; }

private:
    // Setup node from components
    void setupFromComponents() {
        auto* nodeComp = m_entityManager.GetComponent<ECS::NodeComponent>(m_entityId);
        if (!nodeComp) return;

        // Set basic properties
        setTitle(nodeComp->title);
        setPos(nodeComp->position);

        // Find functional component and use its schema to set up node
        for (const auto& compName : m_entityManager.GetComponentNamesForEntity(m_entityId)) {
            auto* comp = m_entityManager.GetComponentByName(compName, m_entityId);
            if (comp && comp != nodeComp) {
                // Get node schema from component
                auto nodeSchema = comp->getNodeSchema();

                // Update node component from schema
                nodeComp->UpdateFromSchema(nodeSchema);

                // Set style based on category
                if (comp->compCategory == "Image") {
                    setStyle(ImFlow::NodeStyle::brown());
                }
                else if (comp->compCategory == "Math") {
                    setStyle(ImFlow::NodeStyle::green());
                }
                else {
                    setStyle(ImFlow::NodeStyle::cyan());
                }

                break;
            }
        }
    }

    // Draw input pins
    void drawInputPins() {
        auto* nodeComp = m_entityManager.GetComponent<ECS::NodeComponent>(m_entityId);
        if (!nodeComp) return;

        for (const auto& input : nodeComp->inputs) {
            // Draw appropriate pin based on data type
            if (input.dataType == "float") {
                drawTypedInputPin<float>(input.id, input.displayName, 0.0f);
            }
            else if (input.dataType == "int") {
                drawTypedInputPin<int>(input.id, input.displayName, 0);
            }
            else if (input.dataType == "bool") {
                drawTypedInputPin<bool>(input.id, input.displayName, false);
            }
            else if (input.dataType == "string") {
                drawTypedInputPin<std::string>(input.id, input.displayName, "");
            }
            else if (input.dataType == "image") {
                drawTypedInputPin<std::string>(input.id, input.displayName, "");
            }
        }
    }

    // Draw UI using ImJSchema
    void drawComponentUI() {
        static nlohmann::json dataCache;
        static nlohmann::json uiCache;

        // Initialize data cache if needed
        if (dataCache.is_null()) {
            dataCache = nlohmann::json::object();

            // Get values from all components
            for (const auto& compName : m_entityManager.GetComponentNamesForEntity(m_entityId)) {
                auto* comp = m_entityManager.GetComponentByName(compName, m_entityId);
                if (comp && comp->compName != "Node_Component") {
                    nlohmann::json compData = comp->Serialize();

                    // Extract values from serialized component
                    if (compData.contains(comp->compName) && compData[comp->compName].is_object()) {
                        for (auto& [key, value] : compData[comp->compName].items()) {
                            dataCache[key] = value;
                        }
                    }
                }
            }
        }

        // Find component with schema
        for (const auto& compName : m_entityManager.GetComponentNamesForEntity(m_entityId)) {
            auto* comp = m_entityManager.GetComponentByName(compName, m_entityId);
            if (comp && comp->compName != "Node_Component" && !comp->schema.is_null()) {
                // Get UI schema
                auto uiSchema = comp->getUISchema();

                // Draw UI using ImJSchema
                bool changed = ImJSchema::drawSchemaWidget(
                    comp->compName.c_str(),
                    dataCache,
                    uiSchema,
                    uiCache
                );

                // If changed, update component(s) and mark for execution
                if (changed) {
                    // Create update data
                    nlohmann::json updateData;
                    updateData[comp->compName] = nlohmann::json::object();

                    // Copy values to update data
                    for (auto& [key, value] : dataCache.items()) {
                        // Skip inputs and outputs
                        if (isInputOrOutput(key)) continue;

                        updateData[comp->compName][key] = value;
                    }

                    // Update component
                    comp->Deserialize(updateData);

                    // Mark for execution
                    auto* nodeComp = m_entityManager.GetComponent<ECS::NodeComponent>(m_entityId);
                    if (nodeComp) {
                        nodeComp->needsExecution = true;
                    }
                }

                break;
            }
        }
    }

    // Check if field is an input or output
    bool isInputOrOutput(const std::string& key) {
        auto* nodeComp = m_entityManager.GetComponent<ECS::NodeComponent>(m_entityId);
        if (!nodeComp) return false;

        // Check inputs
        for (const auto& input : nodeComp->inputs) {
            if (input.id == key) return true;
        }

        // Check outputs
        for (const auto& output : nodeComp->outputs) {
            if (output.id == key) return true;
        }

        return false;
    }

    // Draw output pins
    void drawOutputPins() {
        auto* nodeComp = m_entityManager.GetComponent<ECS::NodeComponent>(m_entityId);
        if (!nodeComp) return;

        for (const auto& output : nodeComp->outputs) {
            if (output.dataType == "float") {
                drawTypedOutputPin<float>(output.id, output.displayName);
            }
            else if (output.dataType == "int") {
                drawTypedOutputPin<int>(output.id, output.displayName);
            }
            else if (output.dataType == "bool") {
                drawTypedOutputPin<bool>(output.id, output.displayName);
            }
            else if (output.dataType == "string") {
                drawTypedOutputPin<std::string>(output.id, output.displayName);
            }
            else if (output.dataType == "image") {
                drawTypedOutputPin<std::string>(output.id, output.displayName);
            }
        }
    }

    // Draw typed input pin
    template<typename T>
    void drawTypedInputPin(const std::string& id, const std::string& displayName, const T& defaultValue) {
        auto* nodeComp = m_entityManager.GetComponent<ECS::NodeComponent>(m_entityId);
        if (!nodeComp) return;

        // Find the input pin
        ECS::NodePin* inputPin = nullptr;
        for (auto& input : nodeComp->inputs) {
            if (input.id == id) {
                inputPin = &input;
                break;
            }
        }

        if (!inputPin) return;

        // Get value from connected pin or use default
        T value = getInputValue<T>(*inputPin, defaultValue);

        // Draw pin and get value
        const T& pinValue = showIN<T>(id, value, ImFlow::ConnectionFilter::SameType());

        // Store value in data cache
        m_dataCache[id] = pinValue;
    }

    // Draw typed output pin
    template<typename T>
    void drawTypedOutputPin(const std::string& id, const std::string& displayName) {
        showOUT<T>(id, [this, id]() -> T {
            // Return value from data cache or default
            if (m_dataCache.contains(id)) {
                try {
                    return m_dataCache[id].get<T>();
                }
                catch (...) {
                    // Type conversion error
                    return T{};
                }
            }
            return T{};
            });
    }

    // Get value from connected input pin
    template<typename T>
    T getInputValue(const ECS::NodePin& input, const T& defaultValue) {
        // Check data cache first
        if (m_dataCache.contains(input.id)) {
            try {
                return m_dataCache[input.id].get<T>();
            }
            catch (...) {
                // Type conversion error
            }
        }

        // If connected to another node, get value from there
        if (input.connectedEntity != 0 && !input.connectedOutputId.empty()) {
            // In a real implementation, this would get values from a global cache
            // This is a placeholder
            return defaultValue;
        }

        return defaultValue;
    }

    // Execute all components on this entity
    void executeComponents() {
        auto* nodeComp = m_entityManager.GetComponent<ECS::NodeComponent>(m_entityId);
        if (!nodeComp) return;

        // Execute for each component type
        for (const auto& compName : m_entityManager.GetComponentNamesForEntity(m_entityId)) {
            auto* comp = m_entityManager.GetComponentByName(compName, m_entityId);
            if (comp && comp->compName != "Node_Component") {
                // Here you'd have component-specific execution logic
                // or use a registry of execution functions

                // For now, we'll just handle some known types
                if (comp->compName == "Sampler") {
                    // Process stable diffusion
                    auto* samplerComp = static_cast<ECS::SamplerComponent*>(comp);

                    // Get input values
                    std::string prompt = m_dataCache.value("prompt", "");
                    std::string negPrompt = m_dataCache.value("negative_prompt", "");

                    // Run stable diffusion (placeholder)
                    std::string resultImage = ""; // In real impl, this would be actual SD output

                    // Store output
                    m_dataCache["image"] = resultImage;
                }
            }
        }

        // Mark connected nodes for execution
        markConnectedNodesForExecution();
    }

    // Mark connected nodes for execution
    void markConnectedNodesForExecution() {
        for (auto entityId : m_entityManager.GetEntitiesWithComponent<ECS::NodeComponent>()) {
            if (entityId == m_entityId) continue;

            auto* nodeComp = m_entityManager.GetComponent<ECS::NodeComponent>(entityId);
            if (!nodeComp) continue;

            // Check if connected to this node
            bool connected = false;
            for (auto& input : nodeComp->inputs) {
                if (input.connectedEntity == m_entityId) {
                    connected = true;
                    break;
                }
            }

            // Mark for execution if connected
            if (connected) {
                nodeComp->needsExecution = true;
            }
        }
    }

    EntityID m_entityId;
    EntityManager& m_entityManager;
    nlohmann::json m_dataCache; // Cache for input/output values
};