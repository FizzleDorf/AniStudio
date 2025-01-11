#pragma once
#include "ECS.h"
#include "GUI.h"
#include "ExampleComponent.hpp"
#include <imgui.h>

namespace ExamplePlugin {

class ExampleView : public GUI::BaseView {
public:
    ExampleView() { viewName = "Example Plugin View"; }

    void Render() override {
        ImGui::Begin(viewName.c_str());

        if (ImGui::Button("Create Counter")) {
            auto entity = ECS::mgr.AddNewEntity();
            ECS::mgr.AddComponent<ExampleComponent>(entity);
        }

        ImGui::Separator();

        // Display all counter entities
        for (const auto &entity : ECS::mgr.GetAllEntities()) {
            if (ECS::mgr.HasComponent<ExampleComponent>(entity)) {
                auto &counter = ECS::mgr.GetComponent<ExampleComponent>(entity);

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
                        ECS::mgr.DestroyEntity(entity);
                    }
                }
            }
        }

        ImGui::End();
    }
};

} // namespace CounterPlugin