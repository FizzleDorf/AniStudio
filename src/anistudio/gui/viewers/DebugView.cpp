#include "DebugView.hpp"
#include "../Events/Events.hpp"

namespace GUI {

void DebugView::Init() { RefreshEntities(); }

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
		}

		if (ImGui::Button("Refresh Entities")) {
			RefreshEntities();
		}

		for (size_t i = 0; i < entities.size(); ++i) {
			EntityID entity = entities[i];
			bool isSelected = (entity == selectedEntity);

			if (ImGui::Selectable((std::string("Entity ") + std::to_string(entity)).c_str(), isSelected)) {
				selectedEntity = entity;
				entityIndex = static_cast<int>(i);
			}

			if (ImGui::TreeNode((std::string("Entity Details: ") + std::to_string(entity)).c_str())) {
				auto components = mgr.GetEntityComponents(entity);
				for (auto compType : components) {
					ImGui::Text("Component ID: %u", compType);
					ImGui::SameLine();
					ImGui::Text("Component Name: %u", compType);
				}
				ImGui::TreePop();
			}
		}
	}
    ImGui::End();
}

void DebugView::RenderSystemPanel() {
    ImGui::Begin("Active Systems");

    for (const auto &[id, aniSystem] : mgr.GetRegisteredSystems()) {
        if (ImGui::TreeNode((std::string("System ") + std::to_string(id) + " " + aniSystem->GetSystemName()).c_str())) {

            ImGui::TreePop();
        }
    }

    ImGui::End();
}
} // namespace GUI