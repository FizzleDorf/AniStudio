#pragma once

#include "ExampleComponent.hpp"
#include "ECS.h"
#include "GUI.h"
#include <imgui.h>

namespace GUI {

class ExampleView : public BaseView {
public:
    
    explicit ExampleView(ECS::EntityManager &entityMgr) : BaseView(entityMgr) { viewName = "Example Plugin View"; }

    void Render() override {
        ImGui::Begin(viewName.c_str());

        if (ImGui::Button("Create Counter")) {
            auto entity = mgr.AddNewEntity();
            mgr.AddComponent<ExampleComponent>(entity);
        }

        ImGui::Separator();

        for (const auto &entity : mgr.GetAllEntities()) {
            if (mgr.HasComponent<ExampleComponent>(entity)) {
                auto &counter = mgr.GetComponent<ExampleComponent>(entity);

                std::string label = "Counter " + std::to_string(entity);
                if (ImGui::CollapsingHeader(label.c_str())) {
                    // Display counter value
                    ImGui::Text("Count: %d", counter.count);

                    // Manual controls
                    if (ImGui::Button("Increment")) {
                        counter.count++;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Decrement")) {
                        counter.count--;
                    }

                    // Auto increment toggle and rate
                    ImGui::Checkbox("Auto Increment", &counter.autoIncrement);
                    if (counter.autoIncrement) {
                        ImGui::SliderFloat("Updates per second", &counter.updateRate, 0.1f, 10.0f);
                    }

                    // Reset button
                    if (ImGui::Button("Reset")) {
                        counter.count = 0;
                    }

                    // Delete entity button
                    ImGui::SameLine();
                    if (ImGui::Button("Delete")) {
                        mgr.DestroyEntity(entity);
                    }
                }
            }
        }

        ImGui::End();
    }
};

} // namespace ExamplePlugin