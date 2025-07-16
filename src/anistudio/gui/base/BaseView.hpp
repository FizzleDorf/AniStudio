// BaseView.hpp - Fixed version
#pragma once
#include "ECS.h"
#include "ViewTypes.hpp"
#include <imgui.h>
#include <nlohmann/json.hpp>
#include <string>

namespace GUI {

	struct ViewMetadata {
		std::string displayName;
		std::string category;
		std::string description;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(ViewMetadata, displayName, category, description)
	};

	class BaseView {
	public:
		// Instance members
		std::string viewName = "Base_View";

		BaseView(ECS::EntityManager &entityMgr) : mgr(entityMgr) {}
		virtual ~BaseView() {}

		// Static metadata - each derived class overrides this
		static constexpr const char* GetMetadataJSON() {
			return R"({
                "displayName": "BaseView",
                "category": "Hidden", 
                "description": "Base view class."
            })";
		}

		// Template method that calls the child view's GetMetadataJSON
		template<typename T>
		static ViewMetadata GetMetadataFor() {
			try {
				nlohmann::json j = nlohmann::json::parse(T::GetMetadataJSON());
				return j.get<ViewMetadata>();
			}
			catch (const std::exception& e) {
				// Fallback to default
				ViewMetadata meta;
				meta.displayName = "Unknown";
				meta.category = "Unknown";
				meta.description = "";
				return meta;
			}
		}

		// This method is only used by BaseView itself
		static ViewMetadata GetMetadata() {
			return GetMetadataFor<BaseView>();
		}

		inline const WorkspaceID GetID() const { return workspaceID; }

		virtual void Init() {}
		virtual void Update(const float deltaT) {}

		virtual void Render() {
			if (ImGui::Begin(viewName.c_str())) {
				ImGui::Text("Default BaseView Render");
				ImGui::Text("ViewID: %zu", workspaceID);
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
		WorkspaceID workspaceID;
	};

} // namespace GUI