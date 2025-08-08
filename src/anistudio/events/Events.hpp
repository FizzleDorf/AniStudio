#pragma once
#include "ECS.h"
#include "GUI.h"
#include <systems.h>
#include "FilePaths.hpp"
#include <GLFW/glfw3.h>
#include <functional>
#include <queue>
#include <string>

// Forward declaration
namespace ANI {
	class Core;
	extern Core &appCore;
	void WindowCloseCallback(GLFWwindow *window);
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
		RemoveView
	};

	struct Event {
		EventType type;
		ECS::EntityID entityID;
	};

	struct ViewEvent {
		ViewEventType type;
		GUI::WorkspaceID viewID;
		std::string viewTypeName;
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

		// Helper functions for view events
		void RequestAddView(const std::string& viewTypeName) {
			ViewEvent event;
			event.type = ViewEventType::AddView;
			event.viewID = 0;
			event.viewTypeName = viewTypeName;
			QueueViewEvent(event);
		}

		void RequestRemoveView(GUI::WorkspaceID viewID, const std::string& viewTypeName) {
			ViewEvent event;
			event.type = ViewEventType::RemoveView;
			event.viewID = viewID;
			event.viewTypeName = viewTypeName;
			QueueViewEvent(event);
		}

	private:
		Events();
		std::queue<Event> eventQueue;
		std::queue<ViewEvent> viewEventQueue;
	};

} // namespace ANI