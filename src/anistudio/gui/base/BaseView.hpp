// BaseView.hpp - Enhanced version with window close detection
#pragma once
#include "ECS.h"
#include "ViewTypes.hpp"
#include <imgui.h>
#include <nlohmann/json.hpp>
#include <string>
#include <functional>

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

		BaseView(ECS::EntityManager &entityMgr) : mgr(entityMgr), m_shouldClose(false) {}
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

		// Check if view should be closed (window X was clicked)
		inline bool ShouldClose() const { return m_shouldClose; }

		// Set close callback - called when view needs to be closed
		inline void SetCloseCallback(std::function<void(WorkspaceID)> callback) {
			m_closeCallback = callback;
		}

		virtual void Init() {}
		virtual void Update(const float deltaT) {}

		// Enhanced Render method that handles window close detection
		virtual void Render() {
			std::string windowName = GetWindowTitle();
			bool windowOpen = true;

			// Begin window with close button enabled
			if (ImGui::Begin(windowName.c_str(), &windowOpen)) {
				// Check if window was closed via X button
				if (!windowOpen && !m_shouldClose) {
					m_shouldClose = true;
					// Trigger close callback if set
					if (m_closeCallback) {
						m_closeCallback(workspaceID);
					}
				}

				// Call derived class render implementation
				RenderContent();
			}
			ImGui::End();

			// Reset close flag if window is still open
			if (windowOpen) {
				m_shouldClose = false;
			}
		}

		// Derived classes override this instead of Render()
		virtual void RenderContent() {
			ImGui::Text("Default BaseView Content");
			ImGui::Text("ViewID: %zu", workspaceID);
			ImGui::Text("This view should override RenderContent()");
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

		// Generate window title with unique ID
		virtual std::string GetWindowTitle() const {
			return viewName + "##" + std::to_string(workspaceID);
		}

	private:
		friend class ViewManager;
		WorkspaceID workspaceID;
		bool m_shouldClose;
		std::function<void(WorkspaceID)> m_closeCallback;
	};

} // namespace GUI