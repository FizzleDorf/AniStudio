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
		std::string viewName = "Base_View";

		BaseView(ECS::EntityManager &entityMgr) : mgr(entityMgr) {}
		virtual ~BaseView() {}

		static constexpr const char* GetMetadataJSON() {
			return R"({
                "displayName": "BaseView",
                "category": "Hidden", 
                "description": "Base view class."
            })";
		}

		template<typename T>
		static ViewMetadata GetMetadataFor() {
			try {
				nlohmann::json j = nlohmann::json::parse(T::GetMetadataJSON());
				return j.get<ViewMetadata>();
			}
			catch (const std::exception& e) {
				ViewMetadata meta;
				meta.displayName = "Unknown";
				meta.category = "Unknown";
				meta.description = "";
				return meta;
			}
		}

		static ViewMetadata GetMetadata() {
			return GetMetadataFor<BaseView>();
		}

		inline const WorkspaceID GetID() const { return workspaceID; }

		virtual void Init() {}
		virtual void Update(const float deltaT) {}

		virtual void Render() {
			if (ImGui::Begin(GetWindowTitle().c_str(), &windowOpen)) {
				ImGui::Text("Default BaseView Content");
				ImGui::Text("ViewID: %zu", workspaceID);
				ImGui::Text("This view should override RenderContent()");
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
		bool windowOpen = true;

		virtual std::string GetWindowTitle() const {
			return viewName + "##" + std::to_string(workspaceID);
		}

	private:
		friend class ViewManager;
		WorkspaceID workspaceID;
	};

} // namespace GUI