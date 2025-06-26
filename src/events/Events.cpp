#include "Events.hpp"
#include "AllViews.h"
#include "PluginManager.hpp"
#include "GUI.h"  // Add this include for ViewListID and GUI namespace
#include <iostream>
#include "../engine/Engine.hpp"

namespace ANI {

	Events::Events() {}

	Events::~Events() {}

	void Events::Init(GLFWwindow *window) {
		// Set the close callback to the static function that invokes Engine::Quit()
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

	static GUI::ViewListID debugID = 0;      // Add GUI:: namespace prefix
	static GUI::ViewListID settingsID = 0;  // Add GUI:: namespace prefix
	static GUI::ViewListID viewsID = 0;     // Add GUI:: namespace prefix
	static GUI::ViewListID pluginsID = 0;   // Add GUI:: namespace prefix
	static GUI::ViewListID helpID = 0;      // Add GUI:: namespace prefix

	// Handle events based on its EventType
	void Events::ProcessEvents() {
		while (!eventQueue.empty()) {
			Event event = eventQueue.front();
			eventQueue.pop();

			switch (event.type) {

			case EventType::Quit: {
				Core.Quit();
				break;
			}

			case EventType::OpenSettings: {
				auto &vMgr = Core.GetViewManager();
				GUI::ViewListID id = vMgr.CreateView();
				vMgr.AddView<GUI::SettingsView>(id, GUI::SettingsView(Core.GetEntityManager()));
				vMgr.GetView<GUI::SettingsView>(id).Init();
				settingsID = id;
				break;
			}

			case EventType::CloseSettings: {
				auto &vMgr = Core.GetViewManager();
				if (vMgr.HasView<GUI::SettingsView>(settingsID)) {
					vMgr.DestroyView(settingsID);
				}
				break;
			}

			case EventType::OpenDebug: {
				auto &vMgr = Core.GetViewManager();
				GUI::ViewListID id = vMgr.CreateView();
				vMgr.AddView<GUI::DebugView>(id, GUI::DebugView(Core.GetEntityManager()));
				vMgr.GetView<GUI::DebugView>(id).Init();
				debugID = id;
				break;
			}

			case EventType::CloseDebug: {
				auto &vMgr = Core.GetViewManager();
				if (vMgr.HasView<GUI::DebugView>(debugID)) {
					vMgr.DestroyView(debugID);
				}
				break;
			}

			case EventType::OpenConvert: {
				auto &vMgr = Core.GetViewManager();
				GUI::ViewListID id = vMgr.CreateView();
				vMgr.AddView<GUI::ConvertView>(id, GUI::ConvertView(Core.GetEntityManager()));
				vMgr.GetView<GUI::ConvertView>(id).Init();
				viewsID = id;
				break;
			}

			case EventType::CloseConvert: {
				auto &vMgr = Core.GetViewManager();
				if (vMgr.HasView<GUI::ConvertView>(viewsID)) {
					vMgr.DestroyView(viewsID);
				}
				break;
			}

			case EventType::OpenViews: {
				auto &vMgr = Core.GetViewManager();
				GUI::ViewListID id = vMgr.CreateView();
				vMgr.AddView<GUI::ViewListManagerView>(id, GUI::ViewListManagerView(Core.GetEntityManager(), vMgr));
				vMgr.GetView<GUI::ViewListManagerView>(id).Init();
				viewsID = id;
				break;
			}

			case EventType::CloseViews: {
				auto &vMgr = Core.GetViewManager();
				if (vMgr.HasView<GUI::ViewListManagerView>(viewsID)) {
					vMgr.DestroyView(viewsID);
				}
				break;
			}

			case EventType::OpenPlugins: {
				auto &vMgr = Core.GetViewManager();
				auto id = vMgr.CreateView();
				vMgr.AddView<GUI::PluginView>(id, GUI::PluginView(Core.GetEntityManager(), Core.GetPluginManager()));
				vMgr.GetView<GUI::PluginView>(id).Init();
				pluginsID = id;
				break;
			}

			case EventType::ClosePlugins: {
				auto &vMgr = Core.GetViewManager();
				if (vMgr.HasView<GUI::PluginView>(pluginsID)) {
					vMgr.DestroyView(pluginsID);
				}
				break;
			}

			case EventType::OpenHelp: {
				auto &vMgr = Core.GetViewManager();
				auto id = vMgr.CreateView();
				vMgr.AddView<GUI::HelpView>(id, GUI::HelpView(Core.GetEntityManager()));
				vMgr.GetView<GUI::HelpView>(id).Init();
				helpID = id;
				break;
			}

			case EventType::CloseHelp: {
				auto &vMgr = Core.GetViewManager();
				if (vMgr.HasView<GUI::HelpView>(helpID)) {
					vMgr.DestroyView(helpID);
				}
				break;
			}

			case EventType::InferenceRequest: {
				std::cout << "Handling InferenceRequest event for Entity ID: " << event.entityID << '\n';

				auto sdcppSystem = Core.GetEntityManager().GetSystem<ECS::SDCPPSystem>();
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

				auto sdcppSystem = Core.GetEntityManager().GetSystem<ECS::SDCPPSystem>();
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

				auto sdcppSystem = Core.GetEntityManager().GetSystem<ECS::SDCPPSystem>();
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

				auto sdcppSystem = Core.GetEntityManager().GetSystem<ECS::SDCPPSystem>();
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
				auto sdcppSystem = Core.GetEntityManager().GetSystem<ECS::SDCPPSystem>();
				if (sdcppSystem) {
					sdcppSystem->PauseWorker();
				}
				break;
			}

			case EventType::ResumeInference: {
				auto sdcppSystem = Core.GetEntityManager().GetSystem<ECS::SDCPPSystem>();
				if (sdcppSystem) {
					sdcppSystem->ResumeWorker();
				}
				break;
			}

			case EventType::StopCurrentTask: {
				auto sdcppSystem = Core.GetEntityManager().GetSystem<ECS::SDCPPSystem>();
				if (sdcppSystem) {
					sdcppSystem->StopCurrentTask();
				}
				break;
			}

			case EventType::ClearInferenceQueue: {
				auto sdcppSystem = Core.GetEntityManager().GetSystem<ECS::SDCPPSystem>();
				if (sdcppSystem) {
					sdcppSystem->ClearQueue();
				}
				break;
			}

			case EventType::SaveImageEvent: {
				std::cout << "Handling SaveImage event for Entity ID: " << event.entityID << " to path: " << '\n';

				auto imageSystem = Core.GetEntityManager().GetSystem<ECS::ImageSystem>();
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

				auto imageSystem = Core.GetEntityManager().GetSystem<ECS::ImageSystem>();
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

				auto imageSystem = Core.GetEntityManager().GetSystem<ECS::ImageSystem>();
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