#pragma once

#include "NodeView.hpp"
#include "ImageSystem.hpp"
#include "ImageComponent.hpp"
#include "PromptComponent.hpp"
#include "SamplerComponent.hpp"
#include "ModelComponents.hpp"
#include "LatentComponent.hpp"
#include "filepaths.hpp"
#include "Events/Events.hpp"
#include <string>
#include <functional>

namespace GUI {

    // Simple node definition for the placeholder
    class SimpleNodeDefinition : public NodeDefinition {
    public:
        SimpleNodeDefinition(const std::string& typeName, const std::string& category)
            : NodeDefinition(typeName, category) {
        }

        void Execute(NodeInstance* instance, ECS::EntityManager& mgr) override {
            // Just pass through input to output
            if (!instance) return;

            for (const auto& input : GetInputs()) {
                for (const auto& output : GetOutputs()) {
                    instance->SetOutputValue(output.first, instance->GetInputValue(input.first));
                }
            }
        }
    };

    // Base class for Input Nodes
    class InputNodeBase : public NodeDefinition {
    public:
        InputNodeBase(const std::string& typeName, const std::string& category)
            : NodeDefinition(typeName, category) {
            // Common setup for input nodes
            AddOutput("Value", "any");
        }

        void Execute(NodeInstance* instance, ECS::EntityManager& mgr) override {
            // Implementation in derived classes
        }
    };

    // Float Input Node
    class FloatInputNode : public InputNodeBase {
    public:
        FloatInputNode(const std::string& typeName, const std::string& category)
            : InputNodeBase(typeName, category) {
        }

        std::shared_ptr<NodeInstance> CreateInstance() const override {
            auto instance = std::make_shared<NodeInstance>(this);
            instance->SetOutputValue("Value", 0.0f);
            return instance;
        }

        void Execute(NodeInstance* instance, ECS::EntityManager& mgr) override {
            // No execution needed - value is set directly in the UI
        }
    };

    // String Input Node
    class StringInputNode : public InputNodeBase {
    public:
        StringInputNode(const std::string& typeName, const std::string& category)
            : InputNodeBase(typeName, category) {
        }

        std::shared_ptr<NodeInstance> CreateInstance() const override {
            auto instance = std::make_shared<NodeInstance>(this);
            instance->SetOutputValue("Value", std::string());
            return instance;
        }

        void Execute(NodeInstance* instance, ECS::EntityManager& mgr) override {
            // No execution needed - value is set directly in the UI
        }
    };

    // Entity Input Node
    class EntityInputNode : public NodeDefinition {
    public:
        EntityInputNode(const std::string& typeName, const std::string& category)
            : NodeDefinition(typeName, category) {
            AddOutput("Entity", "entity");
        }

        std::shared_ptr<NodeInstance> CreateInstance() const override {
            auto instance = std::make_shared<NodeInstance>(this);
            instance->SetOutputValue("Entity", 0); // Default to invalid entity ID
            return instance;
        }

        void Execute(NodeInstance* instance, ECS::EntityManager& mgr) override {
            // Create a new entity and set it as output
            ECS::EntityID entity = mgr.AddNewEntity();
            instance->SetOutputValue("Entity", entity);
        }
    };

    // Entity Output Node
    class EntityOutputNode : public NodeDefinition {
    public:
        EntityOutputNode(const std::string& typeName, const std::string& category)
            : NodeDefinition(typeName, category) {
            AddInput("Entity", "entity");
        }

        void Execute(NodeInstance* instance, ECS::EntityManager& mgr) override {
            // Get the input entity
            nlohmann::json entityJson = instance->GetInputValue("Entity");
            if (entityJson.is_number()) {
                ECS::EntityID entity = entityJson.get<ECS::EntityID>();

                // Do something with the entity (e.g., log it)
                std::cout << "Entity Output: " << entity << std::endl;
            }
        }
    };

    // Math Nodes

    // Add Node
    class AddNode : public NodeDefinition {
    public:
        AddNode(const std::string& typeName, const std::string& category)
            : NodeDefinition(typeName, category) {
            AddInput("A", "float");
            AddInput("B", "float");
            AddOutput("Result", "float");
        }

        void Execute(NodeInstance* instance, ECS::EntityManager& mgr) override {
            float a = 0.0f, b = 0.0f;

            // Get input values
            nlohmann::json aJson = instance->GetInputValue("A");
            nlohmann::json bJson = instance->GetInputValue("B");

            if (aJson.is_number()) {
                a = aJson.get<float>();
            }

            if (bJson.is_number()) {
                b = bJson.get<float>();
            }

            // Calculate result
            float result = a + b;

            // Set output value
            instance->SetOutputValue("Result", result);
        }
    };

    // Multiply Node
    class MultiplyNode : public NodeDefinition {
    public:
        MultiplyNode(const std::string& typeName, const std::string& category)
            : NodeDefinition(typeName, category) {
            AddInput("A", "float");
            AddInput("B", "float");
            AddOutput("Result", "float");
        }

        void Execute(NodeInstance* instance, ECS::EntityManager& mgr) override {
            float a = 0.0f, b = 0.0f;

            // Get input values
            nlohmann::json aJson = instance->GetInputValue("A");
            nlohmann::json bJson = instance->GetInputValue("B");

            if (aJson.is_number()) {
                a = aJson.get<float>();
            }

            if (bJson.is_number()) {
                b = bJson.get<float>();
            }

            // Calculate result
            float result = a * b;

            // Set output value
            instance->SetOutputValue("Result", result);
        }
    };

    // Image nodes

    // Load Image Node
    class LoadImageNode : public NodeDefinition {
    public:
        LoadImageNode(const std::string& typeName, const std::string& category)
            : NodeDefinition(typeName, category) {
            AddInput("Path", "string");
            AddOutput("Entity", "entity");
            AddOutput("Image", "image");
        }

        void Execute(NodeInstance* instance, ECS::EntityManager& mgr) override {
            // Get input path
            nlohmann::json pathJson = instance->GetInputValue("Path");
            std::string path;

            if (pathJson.is_string()) {
                path = pathJson.get<std::string>();
            }
            else {
                // Default path if none provided
                path = std::string(filePaths.defaultProjectPath) + "/image.png";
            }

            // Create entity with image component
            ECS::EntityID entity = mgr.AddNewEntity();
            auto& imageComp = mgr.AddComponent<ECS::ImageComponent>(entity);

            // Set file info
            imageComp.filePath = path;
            std::filesystem::path fsPath(path);
            imageComp.fileName = fsPath.filename().string();

            // Queue load operation via event
            ANI::Event event;
            event.type = ANI::EventType::LoadImageEvent;
            event.entityID = entity;
            ANI::Events::Ref().QueueEvent(event);

            // Set outputs
            instance->SetOutputValue("Entity", entity);
            instance->SetOutputValue("Image", entity); // Use entity ID to reference the image
        }
    };

    // Save Image Node
    class SaveImageNode : public NodeDefinition {
    public:
        SaveImageNode(const std::string& typeName, const std::string& category)
            : NodeDefinition(typeName, category) {
            AddInput("Image", "image");
            AddInput("Path", "string");
            AddOutput("Success", "boolean");
        }

        void Execute(NodeInstance* instance, ECS::EntityManager& mgr) override {
            // Get input image entity
            nlohmann::json imageJson = instance->GetInputValue("Image");
            ECS::EntityID entity = 0;

            if (imageJson.is_number()) {
                entity = imageJson.get<ECS::EntityID>();
            }

            // Get path
            nlohmann::json pathJson = instance->GetInputValue("Path");
            std::string path;

            if (pathJson.is_string()) {
                path = pathJson.get<std::string>();
            }
            else {
                // Default path if none provided
                path = std::string(filePaths.defaultProjectPath) + "/saved_image.png";
            }

            bool success = false;

            // Check if entity has image component
            if (entity != 0 && mgr.HasComponent<ECS::ImageComponent>(entity)) {
                // Queue save operation via event
                ANI::Event event;
                event.type = ANI::EventType::SaveImageEvent;
                event.entityID = entity;
                ANI::Events::Ref().QueueEvent(event);

                success = true;
            }

            // Set output
            instance->SetOutputValue("Success", success);
        }
    };

    // Text to Image Node
    class Text2ImageNode : public NodeDefinition {
    public:
        Text2ImageNode(const std::string& typeName, const std::string& category)
            : NodeDefinition(typeName, category) {
            AddInput("Prompt", "string");
            AddInput("Negative Prompt", "string");
            AddInput("Width", "integer");
            AddInput("Height", "integer");
            AddInput("Steps", "integer");
            AddInput("CFG Scale", "float");
            AddInput("Seed", "integer");
            AddOutput("Entity", "entity");
            AddOutput("Image", "image");
        }

        void Execute(NodeInstance* instance, ECS::EntityManager& mgr) override {
            // Create a new entity for the generation
            ECS::EntityID entity = mgr.AddNewEntity();

            // Set up components with default values
            auto& promptComp = mgr.AddComponent<ECS::PromptComponent>(entity);
            auto& samplerComp = mgr.AddComponent<ECS::SamplerComponent>(entity);
            auto& modelComp = mgr.AddComponent<ECS::ModelComponent>(entity);
            auto& latentComp = mgr.AddComponent<ECS::LatentComponent>(entity);
            auto& imageComp = mgr.AddComponent<ECS::ImageComponent>(entity);

            // Get input values
            nlohmann::json promptJson = instance->GetInputValue("Prompt");
            if (promptJson.is_string()) {
                promptComp.posPrompt = promptJson.get<std::string>();
                strncpy(promptComp.PosBuffer, promptComp.posPrompt.c_str(), sizeof(promptComp.PosBuffer) - 1);
                promptComp.PosBuffer[sizeof(promptComp.PosBuffer) - 1] = '\0';
            }

            nlohmann::json negPromptJson = instance->GetInputValue("Negative Prompt");
            if (negPromptJson.is_string()) {
                promptComp.negPrompt = negPromptJson.get<std::string>();
                strncpy(promptComp.NegBuffer, promptComp.negPrompt.c_str(), sizeof(promptComp.NegBuffer) - 1);
                promptComp.NegBuffer[sizeof(promptComp.NegBuffer) - 1] = '\0';
            }

            nlohmann::json widthJson = instance->GetInputValue("Width");
            if (widthJson.is_number()) {
                latentComp.latentWidth = widthJson.get<int>();
            }

            nlohmann::json heightJson = instance->GetInputValue("Height");
            if (heightJson.is_number()) {
                latentComp.latentHeight = heightJson.get<int>();
            }

            nlohmann::json stepsJson = instance->GetInputValue("Steps");
            if (stepsJson.is_number()) {
                samplerComp.steps = stepsJson.get<int>();
            }

            nlohmann::json cfgJson = instance->GetInputValue("CFG Scale");
            if (cfgJson.is_number()) {
                samplerComp.cfg = cfgJson.get<float>();
            }

            nlohmann::json seedJson = instance->GetInputValue("Seed");
            if (seedJson.is_number()) {
                samplerComp.seed = seedJson.get<int>();
            }

            // Set image output path
            imageComp.fileName = "generated_image.png";
            imageComp.filePath = filePaths.defaultProjectPath;

            // Set default model if needed (should be customizable in a real implementation)
            if (modelComp.modelPath.empty()) {
                // Set default model path based on your environment
                modelComp.modelPath = filePaths.checkpointDir + "/model.safetensors";
            }

            // Queue inference task
            ANI::Event event;
            event.type = ANI::EventType::InferenceRequest;
            event.entityID = entity;
            ANI::Events::Ref().QueueEvent(event);

            // Set outputs
            instance->SetOutputValue("Entity", entity);
            instance->SetOutputValue("Image", entity);
        }
    };

} // namespace GUI