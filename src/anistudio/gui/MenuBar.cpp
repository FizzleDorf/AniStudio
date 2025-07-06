// FIXED MenuBar.cpp - Use StudioCore Instance Instead of Static Methods

#include "MenuBar.hpp"
#include "../events/Events.hpp"
#include "AniStudio.hpp"
#include "AllViews.h"

namespace GUI {
	// Static variables for menu state
	static bool settingsWindowOpen = false;
	static bool debugWindowOpen = false;
	static bool convertWindowOpen = false;
	static bool viewsWindowOpen = false;
	static bool pluginsWindowOpen = false;
	static bool helpWindowOpen = false;

	// Tool view states
	static bool diffusionViewOpen = false;
	static bool upscaleViewOpen = false;
	static bool imageViewOpen = false;
	static bool nodeGraphViewOpen = false;
	static bool sequencerViewOpen = false;
	static bool nodeViewOpen = false;
	static bool videoViewOpen = false;
	static bool videoSequencerViewOpen = false;
	static bool zepViewOpen = false;

	// View IDs
	static GUI::ViewListID settingsViewID = 0;
	static GUI::ViewListID debugViewID = 0;
	static GUI::ViewListID convertViewID = 0;
	static GUI::ViewListID viewListManagerViewID = 0;
	static GUI::ViewListID pluginViewID = 0;
	static GUI::ViewListID helpViewID = 0;
	static GUI::ViewListID diffusionViewID = 0;
	static GUI::ViewListID upscaleViewID = 0;
	static GUI::ViewListID imageViewID = 0;
	static GUI::ViewListID nodeGraphViewID = 0;
	static GUI::ViewListID sequencerViewID = 0;
	static GUI::ViewListID nodeViewID = 0;
	static GUI::ViewListID videoViewID = 0;
	static GUI::ViewListID videoSequencerViewID = 0;
	static GUI::ViewListID zepViewID = 0;

	static void UpdateECSViews(ANI::StudioCore& studioCore) {
		// Get everything from StudioCore
		GUI::ViewManager& viewManager = studioCore.GetViewManager();
		ECS::EntityManager& entityManager = studioCore.GetEntityManager();

		try {
			// ========================================================================
			// ECS-STYLE: Create ONE ViewListID with MULTIPLE view components
			// ========================================================================
			if (diffusionViewOpen && diffusionViewID == 0) {
				std::cout << "[MenuBar] Creating ECS-style diffusion workspace..." << std::endl;

				try {
					diffusionViewID = viewManager.CreateView();
					std::cout << "[MenuBar] Created ViewListID: " << diffusionViewID << std::endl;

					std::cout << "[MenuBar] Adding DiffusionView component..." << std::endl;
					viewManager.AddView<DiffusionView>(diffusionViewID, DiffusionView(entityManager));
					viewManager.GetView<DiffusionView>(diffusionViewID).Init();
					std::cout << "[MenuBar] DiffusionView component added and initialized" << std::endl;

					std::cout << "[MenuBar] ECS-style diffusion workspace created successfully!" << std::endl;
					std::cout << "  ViewListID: " << diffusionViewID << std::endl;
				}
				catch (const std::exception& e) {
					std::cerr << "[MenuBar] FAILED to create ECS-style diffusion workspace: " << e.what() << std::endl;

					if (diffusionViewID != 0) {
						try {
							std::cout << "[MenuBar] Attempting cleanup of failed ViewListID " << diffusionViewID << std::endl;
							viewManager.DestroyView(diffusionViewID);
							std::cout << "[MenuBar] Cleanup successful" << std::endl;
						}
						catch (const std::exception& cleanupError) {
							std::cerr << "[MenuBar] Cleanup also failed: " << cleanupError.what() << std::endl;
						}
						diffusionViewID = 0;
					}

					diffusionViewOpen = false;
				}
			}

			// When diffusion is closed, destroy the ViewListID
			if (!diffusionViewOpen && diffusionViewID != 0) {
				std::cout << "[MenuBar] Destroying diffusion workspace..." << std::endl;
				viewManager.DestroyView(diffusionViewID);
				diffusionViewID = 0;
			}

			// ========================================================================
			// INDIVIDUAL VIEWS - NO MORE MACRO BULLSHIT
			// ========================================================================

			// Settings View
			if (settingsWindowOpen && settingsViewID == 0) {
				try {
					std::cout << "[MenuBar] Creating SettingsView..." << std::endl;
					settingsViewID = viewManager.CreateView();
					viewManager.AddView<SettingsView>(settingsViewID, SettingsView(entityManager));
					viewManager.GetView<SettingsView>(settingsViewID).Init();
					std::cout << "[MenuBar] SettingsView created successfully with ViewListID: " << settingsViewID << std::endl;
				}
				catch (const std::exception& e) {
					std::cerr << "[MenuBar] FAILED to create SettingsView: " << e.what() << std::endl;
					if (settingsViewID != 0) {
						try { viewManager.DestroyView(settingsViewID); }
						catch (...) {}
						settingsViewID = 0;
					}
					settingsWindowOpen = false;
				}
			}
			if (!settingsWindowOpen && settingsViewID != 0) {
				try {
					viewManager.DestroyView(settingsViewID);
					settingsViewID = 0;
				}
				catch (...) {}
			}

			// Debug View
			if (debugWindowOpen && debugViewID == 0) {
				try {
					std::cout << "[MenuBar] Creating DebugView..." << std::endl;
					debugViewID = viewManager.CreateView();
					viewManager.AddView<DebugView>(debugViewID, DebugView(entityManager));
					viewManager.GetView<DebugView>(debugViewID).Init();
					std::cout << "[MenuBar] DebugView created successfully with ViewListID: " << debugViewID << std::endl;
				}
				catch (const std::exception& e) {
					std::cerr << "[MenuBar] FAILED to create DebugView: " << e.what() << std::endl;
					if (debugViewID != 0) {
						try { viewManager.DestroyView(debugViewID); }
						catch (...) {}
						debugViewID = 0;
					}
					debugWindowOpen = false;
				}
			}
			if (!debugWindowOpen && debugViewID != 0) {
				try {
					viewManager.DestroyView(debugViewID);
					debugViewID = 0;
				}
				catch (...) {}
			}

			// Convert View
			if (convertWindowOpen && convertViewID == 0) {
				try {
					std::cout << "[MenuBar] Creating ConvertView..." << std::endl;
					convertViewID = viewManager.CreateView();
					viewManager.AddView<ConvertView>(convertViewID, ConvertView(entityManager));
					viewManager.GetView<ConvertView>(convertViewID).Init();
					std::cout << "[MenuBar] ConvertView created successfully with ViewListID: " << convertViewID << std::endl;
				}
				catch (const std::exception& e) {
					std::cerr << "[MenuBar] FAILED to create ConvertView: " << e.what() << std::endl;
					if (convertViewID != 0) {
						try { viewManager.DestroyView(convertViewID); }
						catch (...) {}
						convertViewID = 0;
					}
					convertWindowOpen = false;
				}
			}
			if (!convertWindowOpen && convertViewID != 0) {
				try {
					viewManager.DestroyView(convertViewID);
					convertViewID = 0;
				}
				catch (...) {}
			}

			// IMAGE VIEW - EXPLICIT IMPLEMENTATION
			if (imageViewOpen && imageViewID == 0) {
				try {
					std::cout << "[MenuBar] Creating ImageView..." << std::endl;
					imageViewID = viewManager.CreateView();
					viewManager.AddView<ImageView>(imageViewID, ImageView(entityManager));
					viewManager.GetView<ImageView>(imageViewID).Init();
					std::cout << "[MenuBar] ImageView created successfully with ViewListID: " << imageViewID << std::endl;
				}
				catch (const std::exception& e) {
					std::cerr << "[MenuBar] FAILED to create ImageView: " << e.what() << std::endl;
					if (imageViewID != 0) {
						try { viewManager.DestroyView(imageViewID); }
						catch (...) {}
						imageViewID = 0;
					}
					imageViewOpen = false;
				}
			}
			// ONLY destroy when the window is actually closed
			if (!imageViewOpen && imageViewID != 0) {
				try {
					std::cout << "[MenuBar] Destroying ImageView (window closed)" << std::endl;
					viewManager.DestroyView(imageViewID);
					imageViewID = 0;
				}
				catch (const std::exception& e) {
					std::cerr << "[MenuBar] Error destroying ImageView: " << e.what() << std::endl;
					imageViewID = 0;
				}
			}

			// Upscale View
			if (upscaleViewOpen && upscaleViewID == 0) {
				try {
					std::cout << "[MenuBar] Creating UpscaleView..." << std::endl;
					upscaleViewID = viewManager.CreateView();
					viewManager.AddView<UpscaleView>(upscaleViewID, UpscaleView(entityManager));
					viewManager.GetView<UpscaleView>(upscaleViewID).Init();
					std::cout << "[MenuBar] UpscaleView created successfully with ViewListID: " << upscaleViewID << std::endl;
				}
				catch (const std::exception& e) {
					std::cerr << "[MenuBar] FAILED to create UpscaleView: " << e.what() << std::endl;
					if (upscaleViewID != 0) {
						try { viewManager.DestroyView(upscaleViewID); }
						catch (...) {}
						upscaleViewID = 0;
					}
					upscaleViewOpen = false;
				}
			}
			if (!upscaleViewOpen && upscaleViewID != 0) {
				try {
					viewManager.DestroyView(upscaleViewID);
					upscaleViewID = 0;
				}
				catch (...) {}
			}

			// NodeGraph View
			if (nodeGraphViewOpen && nodeGraphViewID == 0) {
				try {
					std::cout << "[MenuBar] Creating NodeGraphView..." << std::endl;
					nodeGraphViewID = viewManager.CreateView();
					viewManager.AddView<NodeGraphView>(nodeGraphViewID, NodeGraphView(entityManager));
					viewManager.GetView<NodeGraphView>(nodeGraphViewID).Init();
					std::cout << "[MenuBar] NodeGraphView created successfully with ViewListID: " << nodeGraphViewID << std::endl;
				}
				catch (const std::exception& e) {
					std::cerr << "[MenuBar] FAILED to create NodeGraphView: " << e.what() << std::endl;
					if (nodeGraphViewID != 0) {
						try { viewManager.DestroyView(nodeGraphViewID); }
						catch (...) {}
						nodeGraphViewID = 0;
					}
					nodeGraphViewOpen = false;
				}
			}
			if (!nodeGraphViewOpen && nodeGraphViewID != 0) {
				try {
					viewManager.DestroyView(nodeGraphViewID);
					nodeGraphViewID = 0;
				}
				catch (...) {}
			}

			// Sequencer View
			if (sequencerViewOpen && sequencerViewID == 0) {
				try {
					std::cout << "[MenuBar] Creating SequencerView..." << std::endl;
					sequencerViewID = viewManager.CreateView();
					viewManager.AddView<SequencerView>(sequencerViewID, SequencerView(entityManager));
					viewManager.GetView<SequencerView>(sequencerViewID).Init();
					std::cout << "[MenuBar] SequencerView created successfully with ViewListID: " << sequencerViewID << std::endl;
				}
				catch (const std::exception& e) {
					std::cerr << "[MenuBar] FAILED to create SequencerView: " << e.what() << std::endl;
					if (sequencerViewID != 0) {
						try { viewManager.DestroyView(sequencerViewID); }
						catch (...) {}
						sequencerViewID = 0;
					}
					sequencerViewOpen = false;
				}
			}
			if (!sequencerViewOpen && sequencerViewID != 0) {
				try {
					viewManager.DestroyView(sequencerViewID);
					sequencerViewID = 0;
				}
				catch (...) {}
			}

			// Node View
			if (nodeViewOpen && nodeViewID == 0) {
				try {
					std::cout << "[MenuBar] Creating NodeView..." << std::endl;
					nodeViewID = viewManager.CreateView();
					viewManager.AddView<NodeView>(nodeViewID, NodeView(entityManager));
					viewManager.GetView<NodeView>(nodeViewID).Init();
					std::cout << "[MenuBar] NodeView created successfully with ViewListID: " << nodeViewID << std::endl;
				}
				catch (const std::exception& e) {
					std::cerr << "[MenuBar] FAILED to create NodeView: " << e.what() << std::endl;
					if (nodeViewID != 0) {
						try { viewManager.DestroyView(nodeViewID); }
						catch (...) {}
						nodeViewID = 0;
					}
					nodeViewOpen = false;
				}
			}
			if (!nodeViewOpen && nodeViewID != 0) {
				try {
					viewManager.DestroyView(nodeViewID);
					nodeViewID = 0;
				}
				catch (...) {}
			}

			// Video View
			if (videoViewOpen && videoViewID == 0) {
				try {
					std::cout << "[MenuBar] Creating VideoView..." << std::endl;
					videoViewID = viewManager.CreateView();
					viewManager.AddView<VideoView>(videoViewID, VideoView(entityManager));
					viewManager.GetView<VideoView>(videoViewID).Init();
					std::cout << "[MenuBar] VideoView created successfully with ViewListID: " << videoViewID << std::endl;
				}
				catch (const std::exception& e) {
					std::cerr << "[MenuBar] FAILED to create VideoView: " << e.what() << std::endl;
					if (videoViewID != 0) {
						try { viewManager.DestroyView(videoViewID); }
						catch (...) {}
						videoViewID = 0;
					}
					videoViewOpen = false;
				}
			}
			if (!videoViewOpen && videoViewID != 0) {
				try {
					viewManager.DestroyView(videoViewID);
					videoViewID = 0;
				}
				catch (...) {}
			}

			// ViewListManager View
			if (viewsWindowOpen && viewListManagerViewID == 0) {
				try {
					std::cout << "[MenuBar] Creating ViewListManagerView..." << std::endl;
					viewListManagerViewID = viewManager.CreateView();
					viewManager.AddView<ViewListManagerView>(viewListManagerViewID, ViewListManagerView(entityManager, viewManager));
					viewManager.GetView<ViewListManagerView>(viewListManagerViewID).Init();
					std::cout << "[MenuBar] ViewListManagerView created successfully with ViewListID: " << viewListManagerViewID << std::endl;
				}
				catch (const std::exception& e) {
					std::cerr << "[MenuBar] FAILED to create ViewListManagerView: " << e.what() << std::endl;
					if (viewListManagerViewID != 0) {
						try { viewManager.DestroyView(viewListManagerViewID); }
						catch (...) {}
						viewListManagerViewID = 0;
					}
					viewsWindowOpen = false;
				}
			}
			if (!viewsWindowOpen && viewListManagerViewID != 0) {
				try {
					viewManager.DestroyView(viewListManagerViewID);
					viewListManagerViewID = 0;
				}
				catch (...) {}
			}

			// Plugin View
			if (pluginsWindowOpen && pluginViewID == 0) {
				try {
					std::cout << "[MenuBar] Creating PluginView..." << std::endl;
					// FIXED: Use studioCore instance instead of static method
					auto& pluginMgr = studioCore.GetPluginManager();
					pluginViewID = viewManager.CreateView();
					viewManager.AddView<PluginView>(pluginViewID, PluginView(entityManager, pluginMgr));
					viewManager.GetView<PluginView>(pluginViewID).Init();
					std::cout << "[MenuBar] PluginView created successfully with ViewListID: " << pluginViewID << std::endl;
				}
				catch (const std::exception& e) {
					std::cerr << "[MenuBar] FAILED to create PluginView: " << e.what() << std::endl;
					if (pluginViewID != 0) {
						try { viewManager.DestroyView(pluginViewID); }
						catch (...) {}
						pluginViewID = 0;
					}
					pluginsWindowOpen = false;
				}
			}
			if (!pluginsWindowOpen && pluginViewID != 0) {
				try {
					viewManager.DestroyView(pluginViewID);
					pluginViewID = 0;
				}
				catch (...) {}
			}

		}
		catch (const std::exception& e) {
			std::cerr << "[MenuBar] CRITICAL ERROR in UpdateECSViews: " << e.what() << std::endl;
		}
	}

	void ShowMenuBar(ANI::StudioCore& studioCore) {
		if (ImGui::BeginMainMenuBar()) {
			if (ImGui::BeginMenu("File")) {
				if (ImGui::MenuItem("New")) {
					ANI::Event event;
					event.type = ANI::EventType::NewProject;
					ANI::Events::Ref().QueueEvent(event);
				}
				if (ImGui::MenuItem("Open")) {
					ANI::Event event;
					event.type = ANI::EventType::OpenProject;
					ANI::Events::Ref().QueueEvent(event);
				}
				ImGui::MenuItem("Save");
				if (ImGui::MenuItem("Exit")) {
					ANI::Event event;
					event.type = ANI::EventType::Quit;
					ANI::Events::Ref().QueueEvent(event);
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Edit")) {
				ImGui::MenuItem("Undo");
				ImGui::MenuItem("Redo");
				ImGui::Separator();
				if (ImGui::MenuItem("Settings", nullptr, settingsWindowOpen)) {
					settingsWindowOpen = !settingsWindowOpen;
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Tools")) {
				if (ImGui::MenuItem("Convert Model", nullptr, convertWindowOpen)) {
					convertWindowOpen = !convertWindowOpen;
				}

				ImGui::Separator();

				if (ImGui::MenuItem("Diffusion (ECS-Style)", nullptr, diffusionViewOpen)) {
					diffusionViewOpen = !diffusionViewOpen;
				}

				if (ImGui::MenuItem("Upscaling", nullptr, upscaleViewOpen)) {
					upscaleViewOpen = !upscaleViewOpen;
				}

				ImGui::Separator();

				if (ImGui::MenuItem("Image View", nullptr, imageViewOpen)) {
					imageViewOpen = !imageViewOpen;
				}

				if (ImGui::MenuItem("Node Graph", nullptr, nodeGraphViewOpen)) {
					nodeGraphViewOpen = !nodeGraphViewOpen;
				}

				if (ImGui::MenuItem("Sequencer", nullptr, sequencerViewOpen)) {
					sequencerViewOpen = !sequencerViewOpen;
				}

				if (ImGui::MenuItem("Node View", nullptr, nodeViewOpen)) {
					nodeViewOpen = !nodeViewOpen;
				}

				if (ImGui::MenuItem("Video View", nullptr, videoViewOpen)) {
					videoViewOpen = !videoViewOpen;
				}

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("View")) {
				if (ImGui::MenuItem("Debug", nullptr, debugWindowOpen)) {
					debugWindowOpen = !debugWindowOpen;
				}
				if (ImGui::MenuItem("Views", nullptr, viewsWindowOpen)) {
					viewsWindowOpen = !viewsWindowOpen;
				}
				if (ImGui::MenuItem("Plugins", nullptr, pluginsWindowOpen)) {
					pluginsWindowOpen = !pluginsWindowOpen;
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Help")) {
				if (ImGui::MenuItem("Help", nullptr, helpWindowOpen)) {
					helpWindowOpen = !helpWindowOpen;
				}
				ImGui::MenuItem("About");
				ImGui::EndMenu();
			}

			ImGui::EndMainMenuBar();
		}

		// CLEAN: Pass only StudioCore - it has everything we need
		UpdateECSViews(studioCore);
	}

} // namespace GUI