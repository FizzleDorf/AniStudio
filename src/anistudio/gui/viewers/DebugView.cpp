#include "DebugView.hpp"
#include "../Events/Events.hpp"

namespace GUI {

	void DebugView::Init() {
		RefreshEntities();
	}

	void DebugView::RefreshEntities() {
		entities = mgr.GetAllEntities();
		entityIndex = entities.empty() ? -1 : static_cast<int>(entities.size()) - 1;
	}

	void DebugView::Render() {
		RenderEntityPanel();
		RenderSystemPanel();
	}

	void DebugView::RenderEntityPanel() {
		if (ImGui::Begin(GetWindowTitle().c_str(), &windowOpen)) {

			if (!windowOpen) {
				ANI::Events::Ref().RequestRemoveView(GetID(), viewName);
				ImGui::End();
				return;
			}

			if (ImGui::Button("Refresh Entities")) {
				RefreshEntities();
			}

			ImGui::SameLine();
			ImGui::Text("Total Entities: %zu", entities.size());

			ImGui::Separator();

			// Show entities in a scrollable region
			if (ImGui::BeginChild("EntityList", ImVec2(0, -200), true)) {
				for (size_t i = 0; i < entities.size(); ++i) {
					ECS::EntityID entity = entities[i];
					bool isSelected = (entity == selectedEntity);

					if (ImGui::Selectable((std::string("Entity ") + std::to_string(entity)).c_str(), isSelected)) {
						selectedEntity = entity;
						entityIndex = static_cast<int>(i);
					}

					// Show component details when expanded
					if (ImGui::TreeNode((std::string("Entity Details: ") + std::to_string(entity)).c_str())) {

						// Get regular components
						auto components = mgr.GetEntityComponents(entity);

						ImGui::Text("Components (%zu):", components.size());
						ImGui::Indent();

						for (auto compType : components) {
							std::string componentName = mgr.GetComponentNameById(compType);

							// Check if this is a plugin component
							bool isPluginComponent = mgr.HasPluginComponent(entity, compType);

							if (isPluginComponent) {
								ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 1.0f, 0.5f, 1.0f)); // Green for plugin components
								ImGui::Text("[PLUGIN] %s (ID: %u)", componentName.c_str(), compType);
								ImGui::PopStyleColor();

								// Try to show plugin component data
								void* pluginComponent = mgr.GetPluginComponent(entity, compType);
								if (pluginComponent) {
									// For ExampleComponent specifically, show its data
									if (componentName == "ExampleComponent") {
										// We know the structure of ExampleComponent
										struct ExampleComponentData {
											ECS::EntityID entityID;
											std::string message;
											float value;
										};

										ExampleComponentData* exampleComp = static_cast<ExampleComponentData*>(pluginComponent);
										ImGui::Indent();
										ImGui::Text("Entity ID: %zu", exampleComp->entityID);
										ImGui::Text("Message: %s", exampleComp->message.c_str());
										ImGui::Text("Value: %.2f", exampleComp->value);
										ImGui::Unindent();
									}
									else {
										ImGui::Indent();
										ImGui::Text("Plugin component data available");
										ImGui::Unindent();
									}
								}
							}
							else {
								ImGui::Text("Component: %s (ID: %u)", componentName.c_str(), compType);
							}
						}

						ImGui::Unindent();
						ImGui::TreePop();
					}
				}
			}
			ImGui::EndChild();

			// Entity creation/deletion controls
			ImGui::Separator();

			if (ImGui::Button("Create New Entity")) {
				ECS::EntityID newEntity = mgr.AddNewEntity();
				RefreshEntities();
				selectedEntity = newEntity;
				std::cout << "[DebugView] Created new entity: " << newEntity << std::endl;
			}

			ImGui::SameLine();

			if (ImGui::Button("Delete Selected Entity") && selectedEntity != static_cast<ECS::EntityID>(-1)) {
				std::cout << "[DebugView] Deleting entity: " << selectedEntity << std::endl;
				mgr.DestroyEntity(selectedEntity);
				selectedEntity = static_cast<ECS::EntityID>(-1);
				RefreshEntities();
			}

			// Show selected entity info
			if (selectedEntity != static_cast<ECS::EntityID>(-1)) {
				ImGui::Separator();
				ImGui::Text("Selected Entity: %zu", selectedEntity);

				auto components = mgr.GetEntityComponents(selectedEntity);
				ImGui::Text("Total Components: %zu", components.size());

				// Count plugin vs regular components
				int regularComponents = 0;
				int pluginComponents = 0;

				for (auto compType : components) {
					if (mgr.HasPluginComponent(selectedEntity, compType)) {
						pluginComponents++;
					}
					else {
						regularComponents++;
					}
				}

				ImGui::Text("Regular Components: %d", regularComponents);
				ImGui::Text("Plugin Components: %d", pluginComponents);
			}
		}
		ImGui::End();
	}

	void DebugView::RenderSystemPanel() {
		if (ImGui::Begin("Active Systems", nullptr)) {

			ImGui::Text("Registered Systems");
			ImGui::Separator();

			// Show regular systems
			const auto& systems = mgr.GetRegisteredSystems();
			ImGui::Text("Regular Systems (%zu):", systems.size());

			for (const auto &[id, aniSystem] : systems) {
				if (ImGui::TreeNode((std::string("System ") + std::to_string(id) + " " + aniSystem->GetSystemName()).c_str())) {
					ImGui::Text("System ID: %zu", id);
					ImGui::Text("System Name: %s", aniSystem->GetSystemName().c_str());

					// We can't directly access the entities member since it's protected
					// Instead, we'll show that the system exists and is active
					ImGui::Text("Status: Active");

					ImGui::TreePop();
				}
			}

			ImGui::Separator();

			// Show plugin systems - we need to add a method to get these
			ImGui::Text("Plugin Systems:");
			ImGui::Text("(Plugin system info not directly accessible)");
			ImGui::Text("Check console output for plugin system updates");

			ImGui::Separator();

			// System controls
			if (ImGui::Button("Refresh Systems")) {
				std::cout << "[DebugView] System refresh requested" << std::endl;
			}

			ImGui::SameLine();

			if (ImGui::Button("Print Registry Debug Info")) {
				mgr.DebugPrintRegisteredComponents();
			}
		}
		ImGui::End();
	}

} // namespace GUI