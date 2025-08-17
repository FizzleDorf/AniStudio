#include <GL/glew.h>
#include "Events.hpp"
#include "ViewManager.hpp"
#include "GUI.h"
#include <iostream>
#include "../core/Core.hpp"

namespace ANI {

	Events::Events() {}

	Events::~Events() {}

	void Events::Init(GLFWwindow *window) {
		glfwSetWindowCloseCallback(window, WindowCloseCallback);
	}

	void Events::QueueEvent(const Event &event) {
		eventQueue.push(event);
	}

	void Events::QueueViewEvent(const ViewEvent &event) {
		viewEventQueue.push(event);
	}

	void Events::Poll() {
		// Poll and handle events (inputs, window resize, etc.)
		glfwPollEvents();

		// Process all pending events after polling
		ProcessEvents();

		// Process view events
		ProcessViewEvents();
	}

	void Events::ProcessViewEvents() {
		while (!viewEventQueue.empty()) {
			ViewEvent event = viewEventQueue.front();
			viewEventQueue.pop();

			// Access ViewManager through the existing appCore reference
			auto& viewManager = appCore.GetViewManager();

			switch (event.type) {
			case ViewEventType::AddView: {
				std::cout << "[Events] Adding view: " << event.viewTypeName
					<< " to workspace: " << event.workspaceID << std::endl;

				try {
					// Get the view type ID from the name
					GUI::ViewTypeID viewType = viewManager.GetViewType(event.viewTypeName);

					// Ensure the workspace exists
					auto allWorkspaces = viewManager.GetAllWorkspaces();
					if (std::find(allWorkspaces.begin(), allWorkspaces.end(), event.workspaceID) == allWorkspaces.end()) {
						std::cerr << "[Events] Workspace " << event.workspaceID << " does not exist!" << std::endl;
						break;
					}

					// Add the view type to the specified workspace
					viewManager.AddViewByType(event.workspaceID, viewType);

					std::cout << "[Events] Successfully added view: " << event.viewTypeName
						<< " to workspace: " << event.workspaceID << std::endl;
				}
				catch (const std::exception& e) {
					std::cerr << "[Events] Failed to add view: " << event.viewTypeName
						<< " - " << e.what() << std::endl;
				}
				break;
			}

			case ViewEventType::RemoveView: {
				std::cout << "[Events] Removing view: " << event.viewTypeName
					<< " from workspace: " << event.workspaceID << std::endl;

				try {
					// Get the view type ID from the name
					GUI::ViewTypeID viewType = viewManager.GetViewType(event.viewTypeName);

					// Remove the view type from the specified workspace
					viewManager.RemoveViewByType(event.workspaceID, viewType);

					std::cout << "[Events] Successfully removed view: " << event.viewTypeName
						<< " from workspace: " << event.workspaceID << std::endl;
				}
				catch (const std::exception& e) {
					std::cerr << "[Events] Failed to remove view: " << event.viewTypeName
						<< " - " << e.what() << std::endl;
				}
				break;
			}

			case ViewEventType::SetActiveWorkspace: {
				std::cout << "[Events] Setting active workspace to: " << event.workspaceID << std::endl;

				// Verify the workspace exists
				auto allWorkspaces = viewManager.GetAllWorkspaces();
				if (std::find(allWorkspaces.begin(), allWorkspaces.end(), event.workspaceID) != allWorkspaces.end()) {
					// Notify any systems that need to know about workspace changes
					// For now, just log the change
					std::cout << "[Events] Active workspace set to: " << event.workspaceID << std::endl;
				}
				else {
					std::cerr << "[Events] Cannot set active workspace - ID " << event.workspaceID << " does not exist" << std::endl;
				}
				break;
			}

			case ViewEventType::CreateWorkspace: {
				std::cout << "[Events] Creating new workspace: " << event.workspaceName << std::endl;

				try {
					GUI::WorkspaceID newWorkspaceID = viewManager.CreateView();
					std::cout << "[Events] Created new workspace: " << newWorkspaceID
						<< " (" << event.workspaceName << ")" << std::endl;
				}
				catch (const std::exception& e) {
					std::cerr << "[Events] Failed to create workspace: " << e.what() << std::endl;
				}
				break;
			}

			case ViewEventType::DeleteWorkspace: {
				std::cout << "[Events] Deleting workspace: " << event.workspaceID << std::endl;

				try {
					// Check if this is the last workspace
					auto allWorkspaces = viewManager.GetAllWorkspaces();
					if (allWorkspaces.size() <= 1) {
						std::cerr << "[Events] Cannot delete the last workspace" << std::endl;
						break;
					}

					// Verify the workspace exists
					if (std::find(allWorkspaces.begin(), allWorkspaces.end(), event.workspaceID) == allWorkspaces.end()) {
						std::cerr << "[Events] Cannot delete workspace - ID " << event.workspaceID << " does not exist" << std::endl;
						break;
					}

					// Delete the workspace
					viewManager.DestroyView(event.workspaceID);
					std::cout << "[Events] Successfully deleted workspace: " << event.workspaceID << std::endl;
				}
				catch (const std::exception& e) {
					std::cerr << "[Events] Failed to delete workspace: " << e.what() << std::endl;
				}
				break;
			}

			default:
				std::cerr << "[Events] Unknown view event type" << std::endl;
				break;
			}
		}
	}

	// Handle events based on its EventType
	void Events::ProcessEvents() {
		while (!eventQueue.empty()) {
			Event event = eventQueue.front();
			eventQueue.pop();

			switch (event.type) {

			case EventType::Quit: {
				appCore.Quit();
				break;
			}

			case EventType::InferenceRequest: {
				std::cout << "Handling InferenceRequest event for Entity ID: " << event.entityID << '\n';

				auto sdcppSystem = appCore.GetEntityManager().GetSystem<ECS::SDCPPSystem>();
				if (sdcppSystem) {
					std::cout << "SDCPPSystem is registered." << std::endl;
					sdcppSystem->QueueTask(event.entityID, ECS::SDCPPSystem::TaskType::Inference);
				}
				else {
					std::cerr << "SDCPPSystem is not registered." << std::endl;
				}
				break;
			}

			case EventType::Img2ImgRequest: {
				std::cout << "Handling Img2Img event for Entity ID: " << event.entityID << '\n';

				auto sdcppSystem = appCore.GetEntityManager().GetSystem<ECS::SDCPPSystem>();
				if (sdcppSystem) {
					std::cout << "SDCPPSystem is registered." << std::endl;
					sdcppSystem->QueueTask(event.entityID, ECS::SDCPPSystem::TaskType::Img2Img);
				}
				else {
					std::cerr << "SDCPPSystem is not registered." << std::endl;
				}
				break;
			}

			case EventType::UpscaleRequest: {
				std::cout << "Handling Upscale request for Entity ID: " << event.entityID << '\n';

				auto sdcppSystem = appCore.GetEntityManager().GetSystem<ECS::SDCPPSystem>();
				if (sdcppSystem) {
					std::cout << "SDCPPSystem is registered, queueing upscale task." << std::endl;
					sdcppSystem->QueueTask(event.entityID, ECS::SDCPPSystem::TaskType::Upscaling);
				}
				else {
					std::cerr << "SDCPPSystem is not registered, cannot process upscale request." << std::endl;
				}
				break;
			}

			case EventType::ConvertToGGUF: {
				std::cout << "Handling Convert event for Entity ID: " << event.entityID << '\n';

				auto sdcppSystem = appCore.GetEntityManager().GetSystem<ECS::SDCPPSystem>();
				if (sdcppSystem) {
					std::cout << "SDCPPSystem is registered." << std::endl;
					sdcppSystem->QueueTask(event.entityID, ECS::SDCPPSystem::TaskType::Conversion);
				}
				else {
					std::cerr << "SDCPPSystem is not registered." << std::endl;
				}
				break;
			}

			case EventType::PauseInference: {
				auto sdcppSystem = appCore.GetEntityManager().GetSystem<ECS::SDCPPSystem>();
				if (sdcppSystem) {
					sdcppSystem->PauseWorker();
				}
				break;
			}

			case EventType::ResumeInference: {
				auto sdcppSystem = appCore.GetEntityManager().GetSystem<ECS::SDCPPSystem>();
				if (sdcppSystem) {
					sdcppSystem->ResumeWorker();
				}
				break;
			}

			case EventType::StopCurrentTask: {
				auto sdcppSystem = appCore.GetEntityManager().GetSystem<ECS::SDCPPSystem>();
				if (sdcppSystem) {
					sdcppSystem->StopCurrentTask();
				}
				break;
			}

			case EventType::ClearInferenceQueue: {
				auto sdcppSystem = appCore.GetEntityManager().GetSystem<ECS::SDCPPSystem>();
				if (sdcppSystem) {
					sdcppSystem->ClearQueue();
				}
				break;
			}

			case EventType::SaveImageEvent: {
				std::cout << "Handling SaveImage event for Entity ID: " << event.entityID << " to path: " << '\n';

				auto imageSystem = appCore.GetEntityManager().GetSystem<ECS::ImageSystem>();
				if (imageSystem) {
					//imageSystem->QueueSaveImage(event.entityID);
				}
				else {
					std::cerr << "ImageSystem is not registered." << std::endl;
				}
				break;
			}

			case EventType::LoadImageEvent: {
				std::cout << "Handling LoadImage event for Entity ID: " << event.entityID << " from path: " << '\n';

				auto imageSystem = appCore.GetEntityManager().GetSystem<ECS::ImageSystem>();
				if (imageSystem) {
					//imageSystem->QueueLoadImage(event.entityID);
				}
				else {
					std::cerr << "ImageSystem is not registered." << std::endl;
				}
				break;
			}

			case EventType::RemoveImageEvent: {
				std::cout << "Handling RemoveImage event for Entity ID: " << event.entityID << '\n';

				auto imageSystem = appCore.GetEntityManager().GetSystem<ECS::ImageSystem>();
				if (imageSystem) {
					//imageSystem->QueueRemoveImage(event.entityID);
				}
				else {
					std::cerr << "ImageSystem is not registered." << std::endl;
				}
				break;
			}

			default:
				std::cerr << "Unknown event type" << std::endl;
				break;
			}
		}
	}

} // namespace ANI