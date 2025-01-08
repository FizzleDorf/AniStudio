#pragma once
#include "GUI/Base/BaseView.hpp"
#include "ECS/Base/EntityManager.hpp"
#include "ExampleComponent.hpp"
#include <imgui.h>

using namespace GUI;

namespace ExamplePlugin {

class ExampleView : public BaseView {
public:
    void Render() override {
        ImGui::Begin("Example Plugin View");

        // Add entity button
        if (ImGui::Button("Add Entity with ExampleComponent")) {
            auto entity = ECS::mgr.AddNewEntity();
            ECS::mgr.AddComponent<ExampleComponent>(entity);
        }

        // Display entities with ExampleComponent
        for (const auto &entity : ECS::mgr.GetAllEntities()) {
            if (ECS::mgr.HasComponent<ExampleComponent>(entity)) {
                auto &comp = ECS::mgr.GetComponent<ExampleComponent>(entity);

                std::string label = "Entity " + std::to_string(entity);
                if (ImGui::TreeNode(label.c_str())) {
                    ImGui::SliderFloat("Value", &comp.value, 0.0f, 1.0f);

                    static char buffer[256];
                    strcpy_s(buffer, comp.text.c_str());
                    if (ImGui::InputText("Text", buffer, sizeof(buffer))) {
                        comp.text = buffer;
                    }

                    ImGui::TreePop();
                }
            }
        }

        ImGui::End();
    }
};

} // namespace ExamplePlugin