#pragma once
#include "Base/BaseView.hpp"
#include "NodeGraphComponents/NodeComponents.hpp"
#include <imgui_node_editor.h>
#include <memory>
#include <string>
#include <unordered_map>

namespace ed = ax::NodeEditor;

namespace GUI {

class NodeGraphView : public BaseView {
public:
    NodeGraphView(ECS::EntityManager &entityMgr) : BaseView(entityMgr), m_Editor(nullptr), isInitialized(false) {
        viewName = "NodeGraphView";
    }

    ~NodeGraphView() { Cleanup(); }

    void Init() override {
        if (isInitialized) {
            return;
        }

        try {
            // Register the system first
            //mgr.RegisterSystem<ECS::NodeGraphSystem>();

            // Initialize editor
            ed::Config config;
            config.SettingsFile = nullptr; // Don't save settings for now to avoid file issues
            m_Editor = ed::CreateEditor(&config);

            if (!m_Editor) {
                throw std::runtime_error("Failed to create node editor context");
            }

            isInitialized = true;
        } catch (const std::exception &e) {
            std::cerr << "NodeGraphView initialization failed: " << e.what() << std::endl;
            Cleanup();
        }
    }

    void Cleanup() {
        isInitialized = false;

        if (m_Editor) {
            ed::DestroyEditor(m_Editor);
            m_Editor = nullptr;
        }

        // Clean up any remaining entities
        CleanupEntities();
    }

    void Render() override {
        if (!isInitialized || !m_Editor) {
            if (!isInitialized) {
                Init();
            }
            return;
        }

        try {
            ImGui::Begin("Node Graph", nullptr, ImGuiWindowFlags_MenuBar);

            if (ImGui::BeginMenuBar()) {
                if (ImGui::BeginMenu("File")) {
                    if (ImGui::MenuItem("New Node")) {
                        CreateNode("New Node", ImGui::GetMousePos());
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndMenuBar();
            }

            ed::SetCurrentEditor(m_Editor);
            ed::Begin("Node Editor");

            // Safe rendering of existing elements
            RenderExistingNodes();
            RenderExistingLinks();

            // Handle interactions
            if (ed::BeginCreate()) {
                HandleLinkCreation();
            }
            ed::EndCreate();

            if (ed::BeginDelete()) {
                HandleDeletion();
            }
            ed::EndDelete();

            ed::End();
            ed::SetCurrentEditor(nullptr);

            ImGui::End();
        } catch (const std::exception &e) {
            std::cerr << "NodeGraphView render error: " << e.what() << std::endl;
            isInitialized = false; // Force reinitialization next frame
        }
    }

private:
    void RenderExistingNodes() {
        auto entities = mgr.GetAllEntities();

        for (auto entity : entities) {
            if (!mgr.HasComponent<ECS::NodeComponent>(entity)) {
                continue;
            }

            try {
                auto &node = mgr.GetComponent<ECS::NodeComponent>(entity);

                ed::BeginNode(ed::NodeId(static_cast<intptr_t>(entity)));

                ImGui::BeginGroup();

                // Node header
                ImGui::TextUnformatted(node.name.c_str());
                ImGui::Spacing();

                // Content
                ImGui::BeginGroup();

                // Pins
                if (mgr.HasComponent<ECS::PinComponent>(entity)) {
                    auto &pin = mgr.GetComponent<ECS::PinComponent>(entity);
                    RenderPin(entity, pin);
                }

                ImGui::EndGroup();

                ImGui::EndGroup();

                ed::EndNode();

                // Update position if changed
                ImVec2 pos = ed::GetNodePosition(ed::NodeId(static_cast<intptr_t>(entity)));
                node.position = glm::vec2(pos.x, pos.y);
            } catch (const std::exception &e) {
                std::cerr << "Error rendering node " << entity << ": " << e.what() << std::endl;
                continue;
            }
        }
    }

    void RenderPin(ECS::EntityID entity, ECS::PinComponent &pin) {
        ed::BeginPin(ed::PinId(static_cast<intptr_t>(entity)),
                     pin.kind == ECS::PinComponent::Kind::Input ? ed::PinKind::Input : ed::PinKind::Output);

        ImGui::Text("%s", pin.name.c_str());

        ed::EndPin();
    }

    void RenderExistingLinks() {
        auto entities = mgr.GetAllEntities();

        for (auto entity : entities) {
            if (!mgr.HasComponent<ECS::LinkComponent>(entity)) {
                continue;
            }

            try {
                auto &link = mgr.GetComponent<ECS::LinkComponent>(entity);

                // Validate pins exist before drawing link
                if (!mgr.HasComponent<ECS::PinComponent>(link.startPin) ||
                    !mgr.HasComponent<ECS::PinComponent>(link.endPin)) {
                    mgr.DestroyEntity(entity); // Clean up invalid link
                    continue;
                }

                ed::Link(ed::LinkId(static_cast<intptr_t>(entity)), ed::PinId(static_cast<intptr_t>(link.startPin)),
                         ed::PinId(static_cast<intptr_t>(link.endPin)));
            } catch (const std::exception &e) {
                std::cerr << "Error rendering link " << entity << ": " << e.what() << std::endl;
                continue;
            }
        }
    }

    void HandleLinkCreation() {
        ed::PinId startPinId, endPinId;
        if (ed::QueryNewLink(&startPinId, &endPinId)) {
            if (startPinId && endPinId) {
                if (ed::AcceptNewItem()) {
                    CreateLink(startPinId, endPinId);
                }
            }
        }
    }

    void HandleDeletion() {
        // Handle node deletion
        ed::NodeId nodeId;
        while (ed::QueryDeletedNode(&nodeId)) {
            if (ed::AcceptDeletedItem()) {
                DeleteNode(static_cast<ECS::EntityID>(nodeId.Get()));
            }
        }

        // Handle link deletion
        ed::LinkId linkId;
        while (ed::QueryDeletedLink(&linkId)) {
            if (ed::AcceptDeletedItem()) {
                mgr.DestroyEntity(static_cast<ECS::EntityID>(linkId.Get()));
            }
        }
    }

    ECS::EntityID CreateNode(const std::string &name, const ImVec2 &position) {
        try {
            auto entity = mgr.AddNewEntity();
            mgr.AddComponent<ECS::NodeComponent>(entity);

            auto &node = mgr.GetComponent<ECS::NodeComponent>(entity);
            node.name = name;
            node.position = glm::vec2(position.x, position.y);

            // Add a default input and output pin
            CreatePin(entity, "In", ECS::PinComponent::Type::Flow, ECS::PinComponent::Kind::Input);
            CreatePin(entity, "Out", ECS::PinComponent::Type::Flow, ECS::PinComponent::Kind::Output);

            return entity;
        } catch (const std::exception &e) {
            std::cerr << "Error creating node: " << e.what() << std::endl;
            return 0;
        }
    }

    void CreateLink(ed::PinId startId, ed::PinId endId) {
        try {
            auto startPinEntity = static_cast<ECS::EntityID>(startId.Get());
            auto endPinEntity = static_cast<ECS::EntityID>(endId.Get());

            // Validate pins
            if (!mgr.HasComponent<ECS::PinComponent>(startPinEntity) ||
                !mgr.HasComponent<ECS::PinComponent>(endPinEntity)) {
                return;
            }

            auto entity = mgr.AddNewEntity();
            mgr.AddComponent<ECS::LinkComponent>(entity);

            auto &link = mgr.GetComponent<ECS::LinkComponent>(entity);
            link.startPin = startPinEntity;
            link.endPin = endPinEntity;
        } catch (const std::exception &e) {
            std::cerr << "Error creating link: " << e.what() << std::endl;
        }
    }

    ECS::EntityID CreatePin(ECS::EntityID nodeEntity, const std::string &name, ECS::PinComponent::Type type,
                            ECS::PinComponent::Kind kind) {
        try {
            auto entity = mgr.AddNewEntity();
            mgr.AddComponent<ECS::PinComponent>(entity);

            auto &pin = mgr.GetComponent<ECS::PinComponent>(entity);
            pin.name = name;
            pin.type = type;
            pin.kind = kind;
            pin.nodeEntity = nodeEntity;

            return entity;
        } catch (const std::exception &e) {
            std::cerr << "Error creating pin: " << e.what() << std::endl;
            return 0;
        }
    }

    void DeleteNode(ECS::EntityID nodeEntity) {
        try {
            // First delete all associated pins
            auto entities = mgr.GetAllEntities();
            for (auto entity : entities) {
                if (mgr.HasComponent<ECS::PinComponent>(entity)) {
                    auto &pin = mgr.GetComponent<ECS::PinComponent>(entity);
                    if (pin.nodeEntity == nodeEntity) {
                        DeleteLinksConnectedToPin(entity);
                        mgr.DestroyEntity(entity);
                    }
                }
            }

            // Then delete the node itself
            mgr.DestroyEntity(nodeEntity);
        } catch (const std::exception &e) {
            std::cerr << "Error deleting node: " << e.what() << std::endl;
        }
    }

    void DeleteLinksConnectedToPin(ECS::EntityID pinEntity) {
        try {
            auto entities = mgr.GetAllEntities();
            for (auto entity : entities) {
                if (!mgr.HasComponent<ECS::LinkComponent>(entity)) {
                    continue;
                }

                auto &link = mgr.GetComponent<ECS::LinkComponent>(entity);
                if (link.startPin == pinEntity || link.endPin == pinEntity) {
                    mgr.DestroyEntity(entity);
                }
            }
        } catch (const std::exception &e) {
            std::cerr << "Error deleting links: " << e.what() << std::endl;
        }
    }

    void CleanupEntities() {
        try {
            auto entities = mgr.GetAllEntities();

            // First clean up all links
            for (auto entity : entities) {
                if (mgr.HasComponent<ECS::LinkComponent>(entity)) {
                    mgr.DestroyEntity(entity);
                }
            }

            // Then clean up all pins
            for (auto entity : entities) {
                if (mgr.HasComponent<ECS::PinComponent>(entity)) {
                    mgr.DestroyEntity(entity);
                }
            }

            // Finally clean up all nodes
            for (auto entity : entities) {
                if (mgr.HasComponent<ECS::NodeComponent>(entity)) {
                    mgr.DestroyEntity(entity);
                }
            }
        } catch (const std::exception &e) {
            std::cerr << "Error during cleanup: " << e.what() << std::endl;
        }
    }

private:
    ed::EditorContext *m_Editor;
    bool isInitialized;
};

} // namespace GUI