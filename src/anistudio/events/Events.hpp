#pragma once
#include "ECS.h"
#include "ViewTypes.hpp"
#include "FilePaths.hpp"
#include "systems.h"
#include <GLFW/glfw3.h>
#include <functional>
#include <queue>
#include <string>

// Forward declarations
namespace ANI {
	class Core;
	extern Core &appCore;
	void WindowCloseCallback(GLFWwindow *window);
}

namespace GUI {
	class ViewManager;
}

namespace ANI {
	class ProjectManager;
}

namespace ANI {

	enum class EventType {
		// Application
		Quit,
		NewProject,
		OpenProject,

		// Diffusion
		InferenceRequest,
		Img2ImgRequest,
		UpscaleRequest,
		T2VInferenceRequest,
		ConvertToGGUF,

		// Queue Controls
		ClearInferenceQueue,
		PauseInference,
		ResumeInference,
		StopCurrentTask,

		// IO Events
		LoadImageEvent,
		SaveImageEvent,
		RemoveImageEvent,

		LoadVideoEvent,
		SaveVideoEvent
	};

	enum class ViewEventType {
		AddView,
		RemoveView,
		SetActiveWorkspace,
		CreateWorkspace,
		DeleteWorkspace
	};

	struct Event {
		EventType type;
		ECS::EntityID entityID;
	};

	struct ViewEvent {
		ViewEventType type;
		GUI::WorkspaceID workspaceID;  // The workspace/view ID
		std::string viewTypeName;

		// Additional data for specific events
		std::string workspaceName; // For CreateWorkspace

		// Legacy compatibility (deprecated)
		GUI::WorkspaceID viewID() const { return workspaceID; }
	};

	class Events {
	public:
		~Events();
		Events(const Events &) = delete;
		Events &operator=(const Events &) = delete;

		static Events &Ref() {
			static Events instance;
			return instance;
		}

		void Poll();
		void Init(GLFWwindow *window);
		void QueueEvent(const Event &event);
		void ProcessEvents();

		// ViewEvent functions
		void QueueViewEvent(const ViewEvent &event);
		void ProcessViewEvents();

		// Set managers (called by StudioCore)
		void SetManagers(GUI::ViewManager* viewMgr, ANI::ProjectManager* projectMgr);

		// Helper functions for view events
		void RequestAddView(GUI::WorkspaceID workspaceID, const std::string& viewTypeName) {
			ViewEvent event;
			event.type = ViewEventType::AddView;
			event.workspaceID = workspaceID;
			event.viewTypeName = viewTypeName;
			QueueViewEvent(event);
		}

		void RequestRemoveView(GUI::WorkspaceID workspaceID, const std::string& viewTypeName) {
			ViewEvent event;
			event.type = ViewEventType::RemoveView;
			event.workspaceID = workspaceID;
			event.viewTypeName = viewTypeName;
			QueueViewEvent(event);
		}

		void RequestSetActiveWorkspace(GUI::WorkspaceID workspaceID) {
			ViewEvent event;
			event.type = ViewEventType::SetActiveWorkspace;
			event.workspaceID = workspaceID;
			QueueViewEvent(event);
		}

		void RequestCreateWorkspace(const std::string& workspaceName = "New Workspace") {
			ViewEvent event;
			event.type = ViewEventType::CreateWorkspace;
			event.workspaceName = workspaceName;
			QueueViewEvent(event);
		}

		void RequestDeleteWorkspace(GUI::WorkspaceID workspaceID) {
			ViewEvent event;
			event.type = ViewEventType::DeleteWorkspace;
			event.workspaceID = workspaceID;
			QueueViewEvent(event);
		}

	private:
		Events();
		std::queue<Event> eventQueue;
		std::queue<ViewEvent> viewEventQueue;

		// Manager references (set by StudioCore)
		GUI::ViewManager* viewManager = nullptr;
		ANI::ProjectManager* projectManager = nullptr;
	};

} // namespace ANI