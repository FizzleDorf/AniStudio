/*
 * FIXED BaseView.hpp - Using YOUR ORIGINAL virtual function names
 * The issue was Render() being pure virtual = 0 when your derived classes DO implement it
 */

#pragma once
#include "ECS.h"
#include "ViewTypes.hpp"
#include <imgui.h>
#include <nlohmann/json.hpp>
#include <string>

namespace GUI {
	class BaseView {
	public:
		std::string viewName = "Base_View";
		std::string category = "";

		BaseView(ECS::EntityManager &entityMgr) : mgr(entityMgr) {}
		virtual ~BaseView() {}

		inline const ViewListID GetID() const { return viewID; }

		virtual void Init() {}
		virtual void Update(const float deltaT) {}

		// CRITICAL FIX: Remove the "= 0" - make it virtual, not pure virtual
		// Your derived classes DO implement this method
		virtual void Render() {
			// Default implementation for safety
			if (ImGui::Begin(viewName.c_str())) {
				ImGui::Text("Default BaseView Render");
				ImGui::Text("ViewID: %zu", viewID);
				ImGui::Text("This view should override Render()");
			}
			ImGui::End();
		}

		virtual void HandleInput(int key, int action) {}

		virtual nlohmann::json Serialize() const {
			nlohmann::json j;
			j["viewName"] = viewName;
			return j;
		}

		virtual void Deserialize(const nlohmann::json &j) {
			if (j.contains("viewName"))
				viewName = j["viewName"];
		}

	protected:
		ECS::EntityManager &mgr;

	private:
		friend class ViewManager;
		ViewListID viewID;
	};
} // namespace GUI