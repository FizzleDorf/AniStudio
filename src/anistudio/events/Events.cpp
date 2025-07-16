#include "Events.hpp"
#include "AllViews.h"
#include "PluginManager.hpp"
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

	void Events::Poll() {
		// Poll and handle events (inputs, window resize, etc.)
		glfwPollEvents();

		// Process all pending events after polling
		ProcessEvents();
	}

	static GUI::WorkspaceID debugID = 0;   
	static GUI::WorkspaceID settingsID = 0;
	static GUI::WorkspaceID viewsID = 0;   
	static GUI::WorkspaceID pluginsID = 0; 
	static GUI::WorkspaceID helpID = 0;    

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