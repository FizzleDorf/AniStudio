/*
 * ExampleView.hpp - Example plugin view for AniStudio
 *
 * This demonstrates how to create a custom view in a plugin that:
 * - Inherits from BaseView properly
 * - Implements GetMetadataJSON() for registration
 * - Shows various ImGui widgets and functionality
 * - Integrates with the ECS system
 */

#pragma once

#include "BaseView.hpp"
#include "ECS.h"
#include <imgui.h>
#include <string>
#include <vector>

namespace ExamplePluginViews {

	class ExampleView : public GUI::BaseView {
	public:
		// Constructor takes EntityManager reference (required by BaseView)
		ExampleView(ECS::EntityManager& entityMgr)
			: BaseView(entityMgr), counter(0), showDemo(false) {
			viewName = "Example Plugin View";
		}

		// REQUIRED: Static metadata method for registration
		static constexpr const char* GetMetadataJSON() {
			return R"({
                "displayName": "Example View",
                "category": "Example Plugin",
                "description": "Demonstrates plugin view creation with various ImGui widgets."
            })";
		}

		// Initialize the view
		void Init() override {
			// Add some demo text to the log
			logMessages.push_back("Example view initialized!");
			logMessages.push_back("This view demonstrates plugin integration.");
			logMessages.push_back("Try clicking the buttons and interacting with widgets.");
		}

		// Update logic (called every frame)
		void Update(const float deltaT) override {
			// Update any time-based logic here
			animationTime += deltaT;
		}

		// Render the ImGui interface
		void Render() override {
			if (ImGui::Begin(viewName.c_str())) {
				RenderHeader();
				RenderButtons();
				RenderInputWidgets();
				RenderEntitySection();
				RenderLogSection();

				if (showDemo) {
					ImGui::ShowDemoWindow(&showDemo);
				}
			}
			ImGui::End();
		}

	private:
		void RenderHeader() {
			ImGui::Text("Welcome to the Example Plugin View!");
			ImGui::Separator();

			// Show some animated text
			float phase = sinf(animationTime * 2.0f) * 0.5f + 0.5f;
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, phase, phase, 1.0f));
			ImGui::Text("Animated text using deltaTime: %.2f", animationTime);
			ImGui::PopStyleColor();

			ImGui::Spacing();
		}

		void RenderButtons() {
			if (ImGui::Button("Click Me!")) {
				counter++;
				logMessages.push_back("Button clicked! Count: " + std::to_string(counter));
			}

			ImGui::SameLine();
			if (ImGui::Button("Show ImGui Demo")) {
				showDemo = !showDemo;
			}

			ImGui::SameLine();
			if (ImGui::Button("Clear Log")) {
				logMessages.clear();
			}

			ImGui::Text("Button click count: %d", counter);
			ImGui::Spacing();
		}

		void RenderInputWidgets() {
			ImGui::Text("Input Widgets:");

			// Text input
			static char textBuffer[256] = "Enter text here...";
			if (ImGui::InputText("Text Input", textBuffer, sizeof(textBuffer))) {
				// Text changed
			}

			// Slider
			static float sliderValue = 50.0f;
			ImGui::SliderFloat("Slider", &sliderValue, 0.0f, 100.0f);

			// Color picker
			static float color[3] = { 1.0f, 0.5f, 0.0f };
			ImGui::ColorEdit3("Color", color);

			// Checkbox
			static bool checkboxValue = true;
			ImGui::Checkbox("Enable something", &checkboxValue);

			ImGui::Spacing();
		}

		void RenderEntitySection() {
			ImGui::Text("ECS Integration:");
			ImGui::Text("Entity Manager: %p", &mgr);

			if (ImGui::Button("Create Entity")) {
				ECS::EntityID newEntity = mgr.AddNewEntity();
				logMessages.push_back("Created entity ID: " + std::to_string(newEntity));
			}

			ImGui::SameLine();
			if (ImGui::Button("Count Entities")) {
				// This would require a method to count entities
				logMessages.push_back("Entity counting not implemented in this example");
			}

			ImGui::Spacing();
		}

		void RenderLogSection() {
			ImGui::Text("Event Log:");

			// Scrollable log area
			if (ImGui::BeginChild("LogArea", ImVec2(0, 150), true)) {
				for (const auto& message : logMessages) {
					ImGui::Text("%s", message.c_str());
				}

				// Auto-scroll to bottom
				if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
					ImGui::SetScrollHereY(1.0f);
				}
			}
			ImGui::EndChild();
		}

	private:
		int counter;
		bool showDemo;
		float animationTime = 0.0f;
		std::vector<std::string> logMessages;
	};

} // namespace ExamplePluginViews