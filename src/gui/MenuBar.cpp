#include "MenuBar.hpp"
#include "../events/Events.hpp"
#include "AniStudio.hpp"

namespace GUI {

	// Global view states instance
	ViewStates g_viewStates;

	void ShowMenuBar(GLFWwindow* window) {
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

				if (ImGui::MenuItem("Settings", nullptr, g_viewStates.settingsWindowOpen)) {
					g_viewStates.settingsWindowOpen = !g_viewStates.settingsWindowOpen;
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Tools")) {
				if (ImGui::MenuItem("Convert Model", nullptr, g_viewStates.convertWindowOpen)) {
					g_viewStates.convertWindowOpen = !g_viewStates.convertWindowOpen;
				}

				ImGui::Separator();

				if (ImGui::MenuItem("Diffusion", nullptr, g_viewStates.diffusionViewOpen)) {
					g_viewStates.diffusionViewOpen = !g_viewStates.diffusionViewOpen;
				}

				if (ImGui::MenuItem("Upscaling", nullptr, g_viewStates.upscaleViewOpen)) {
					g_viewStates.upscaleViewOpen = !g_viewStates.upscaleViewOpen;
				}

				ImGui::Separator();

				if (ImGui::MenuItem("Image View", nullptr, g_viewStates.imageViewOpen)) {
					g_viewStates.imageViewOpen = !g_viewStates.imageViewOpen;
				}

				if (ImGui::MenuItem("Node Graph", nullptr, g_viewStates.nodeGraphViewOpen)) {
					g_viewStates.nodeGraphViewOpen = !g_viewStates.nodeGraphViewOpen;
				}

				if (ImGui::MenuItem("Sequencer", nullptr, g_viewStates.sequencerViewOpen)) {
					g_viewStates.sequencerViewOpen = !g_viewStates.sequencerViewOpen;
				}

				if (ImGui::MenuItem("Node View", nullptr, g_viewStates.nodeViewOpen)) {
					g_viewStates.nodeViewOpen = !g_viewStates.nodeViewOpen;
				}

				if (ImGui::MenuItem("Video View", nullptr, g_viewStates.videoViewOpen)) {
					g_viewStates.videoViewOpen = !g_viewStates.videoViewOpen;
				}

				if (ImGui::MenuItem("Video Sequencer", nullptr, g_viewStates.videoSequencerViewOpen)) {
					g_viewStates.videoSequencerViewOpen = !g_viewStates.videoSequencerViewOpen;
				}

				if (ImGui::MenuItem("Zep Editor", nullptr, g_viewStates.zepViewOpen)) {
					g_viewStates.zepViewOpen = !g_viewStates.zepViewOpen;
				}

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("View")) {
				if (ImGui::MenuItem("View Manager", nullptr, g_viewStates.viewsWindowOpen)) {
					g_viewStates.viewsWindowOpen = !g_viewStates.viewsWindowOpen;
				}

				ImGui::Separator();

				if (ImGui::MenuItem("Debug", nullptr, g_viewStates.debugWindowOpen)) {
					g_viewStates.debugWindowOpen = !g_viewStates.debugWindowOpen;
				}

				ImGui::Separator();

				if (ImGui::MenuItem("Plugins", nullptr, g_viewStates.pluginsWindowOpen)) {
					g_viewStates.pluginsWindowOpen = !g_viewStates.pluginsWindowOpen;
				}

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Help")) {
				if (ImGui::MenuItem("Help", nullptr, g_viewStates.helpWindowOpen)) {
					g_viewStates.helpWindowOpen = !g_viewStates.helpWindowOpen;
				}
				ImGui::EndMenu();
			}

			ImGui::EndMainMenuBar();
		}
	}

	// Individual view management functions that create and manage ECS views
	void ShowOrCreateSettingsView() {
		if (!g_viewStates.settingsWindowOpen) {
			DestroyViewIfClosed(g_viewStates.settingsViewID, g_viewStates.settingsWindowOpen);
			return;
		}

		try {
			auto& viewMgr = ANI::StudioCore::GetViewManager();
			auto& entityMgr = ANI::StudioCore::GetEntityManager();

			// Create view if it doesn't exist
			if (g_viewStates.settingsViewID == 0) {
				g_viewStates.settingsViewID = viewMgr.CreateView();
				viewMgr.AddView<SettingsView>(g_viewStates.settingsViewID, SettingsView(entityMgr));
				viewMgr.GetView<SettingsView>(g_viewStates.settingsViewID).Init();
				std::cout << "[MenuBar] Created SettingsView with ID: " << g_viewStates.settingsViewID << std::endl;
			}
		}
		catch (const std::exception& e) {
			std::cerr << "[MenuBar] Error managing SettingsView: " << e.what() << std::endl;
			g_viewStates.settingsWindowOpen = false;
		}
	}

	void ShowOrCreateDebugView() {
		if (!g_viewStates.debugWindowOpen) {
			DestroyViewIfClosed(g_viewStates.debugViewID, g_viewStates.debugWindowOpen);
			return;
		}

		try {
			auto& viewMgr = ANI::StudioCore::GetViewManager();
			auto& entityMgr = ANI::StudioCore::GetEntityManager();

			if (g_viewStates.debugViewID == 0) {
				g_viewStates.debugViewID = viewMgr.CreateView();
				viewMgr.AddView<DebugView>(g_viewStates.debugViewID, DebugView(entityMgr));
				viewMgr.GetView<DebugView>(g_viewStates.debugViewID).Init();
				std::cout << "[MenuBar] Created DebugView with ID: " << g_viewStates.debugViewID << std::endl;
			}
		}
		catch (const std::exception& e) {
			std::cerr << "[MenuBar] Error managing DebugView: " << e.what() << std::endl;
			g_viewStates.debugWindowOpen = false;
		}
	}

	void ShowOrCreateConvertView() {
		if (!g_viewStates.convertWindowOpen) {
			DestroyViewIfClosed(g_viewStates.convertViewID, g_viewStates.convertWindowOpen);
			return;
		}

		try {
			auto& viewMgr = ANI::StudioCore::GetViewManager();
			auto& entityMgr = ANI::StudioCore::GetEntityManager();

			if (g_viewStates.convertViewID == 0) {
				g_viewStates.convertViewID = viewMgr.CreateView();
				viewMgr.AddView<ConvertView>(g_viewStates.convertViewID, ConvertView(entityMgr));
				viewMgr.GetView<ConvertView>(g_viewStates.convertViewID).Init();
				std::cout << "[MenuBar] Created ConvertView with ID: " << g_viewStates.convertViewID << std::endl;
			}
		}
		catch (const std::exception& e) {
			std::cerr << "[MenuBar] Error managing ConvertView: " << e.what() << std::endl;
			g_viewStates.convertWindowOpen = false;
		}
	}

	void ShowOrCreateViewsView() {
		if (!g_viewStates.viewsWindowOpen) {
			DestroyViewIfClosed(g_viewStates.viewListManagerViewID, g_viewStates.viewsWindowOpen);
			return;
		}

		try {
			auto& viewMgr = ANI::StudioCore::GetViewManager();
			auto& entityMgr = ANI::StudioCore::GetEntityManager();

			if (g_viewStates.viewListManagerViewID == 0) {
				g_viewStates.viewListManagerViewID = viewMgr.CreateView();
				viewMgr.AddView<ViewListManagerView>(g_viewStates.viewListManagerViewID, ViewListManagerView(entityMgr, viewMgr));
				viewMgr.GetView<ViewListManagerView>(g_viewStates.viewListManagerViewID).Init();
				std::cout << "[MenuBar] Created ViewListManagerView with ID: " << g_viewStates.viewListManagerViewID << std::endl;
			}
		}
		catch (const std::exception& e) {
			std::cerr << "[MenuBar] Error managing ViewListManagerView: " << e.what() << std::endl;
			g_viewStates.viewsWindowOpen = false;
		}
	}

	void ShowOrCreatePluginsView() {
		if (!g_viewStates.pluginsWindowOpen) {
			DestroyViewIfClosed(g_viewStates.pluginViewID, g_viewStates.pluginsWindowOpen);
			return;
		}

		try {
			auto& viewMgr = ANI::StudioCore::GetViewManager();
			auto& entityMgr = ANI::StudioCore::GetEntityManager();
			auto& pluginMgr = ANI::StudioCore::GetPluginManager();

			if (g_viewStates.pluginViewID == 0) {
				g_viewStates.pluginViewID = viewMgr.CreateView();
				viewMgr.AddView<PluginView>(g_viewStates.pluginViewID, PluginView(entityMgr, pluginMgr));
				viewMgr.GetView<PluginView>(g_viewStates.pluginViewID).Init();
				std::cout << "[MenuBar] Created PluginView with ID: " << g_viewStates.pluginViewID << std::endl;
			}
		}
		catch (const std::exception& e) {
			std::cerr << "[MenuBar] Error managing PluginView: " << e.what() << std::endl;
			g_viewStates.pluginsWindowOpen = false;
		}
	}

	void ShowOrCreateHelpView() {
		if (!g_viewStates.helpWindowOpen) {
			DestroyViewIfClosed(g_viewStates.helpViewID, g_viewStates.helpWindowOpen);
			return;
		}

		try {
			auto& viewMgr = ANI::StudioCore::GetViewManager();
			auto& entityMgr = ANI::StudioCore::GetEntityManager();

			if (g_viewStates.helpViewID == 0) {
				g_viewStates.helpViewID = viewMgr.CreateView();
				viewMgr.AddView<HelpView>(g_viewStates.helpViewID, HelpView(entityMgr));
				viewMgr.GetView<HelpView>(g_viewStates.helpViewID).Init();
				std::cout << "[MenuBar] Created HelpView with ID: " << g_viewStates.helpViewID << std::endl;
			}
		}
		catch (const std::exception& e) {
			std::cerr << "[MenuBar] Error managing HelpView: " << e.what() << std::endl;
			g_viewStates.helpWindowOpen = false;
		}
	}

	// SPECIAL CASE: DiffusionView with embedded ImageView
	void ShowOrCreateDiffusionView() {
		if (!g_viewStates.diffusionViewOpen) {
			DestroyViewIfClosed(g_viewStates.diffusionViewID, g_viewStates.diffusionViewOpen);
			return;
		}

		try {
			auto& viewMgr = ANI::StudioCore::GetViewManager();
			auto& entityMgr = ANI::StudioCore::GetEntityManager();

			if (g_viewStates.diffusionViewID == 0) {
				// Create a single ViewListID that will contain both views
				g_viewStates.diffusionViewID = viewMgr.CreateView();

				// Add both DiffusionView and ImageView to the same ViewListID
				viewMgr.AddView<DiffusionView>(g_viewStates.diffusionViewID, DiffusionView(entityMgr));
				viewMgr.AddView<ImageView>(g_viewStates.diffusionViewID, ImageView(entityMgr));

				// Initialize both views
				viewMgr.GetView<DiffusionView>(g_viewStates.diffusionViewID).Init();
				viewMgr.GetView<ImageView>(g_viewStates.diffusionViewID).Init();

				std::cout << "[MenuBar] Created DiffusionView + ImageView with shared ID: " << g_viewStates.diffusionViewID << std::endl;
			}
		}
		catch (const std::exception& e) {
			std::cerr << "[MenuBar] Error managing DiffusionView: " << e.what() << std::endl;
			g_viewStates.diffusionViewOpen = false;
		}
	}

	void ShowOrCreateUpscaleView() {
		if (!g_viewStates.upscaleViewOpen) {
			DestroyViewIfClosed(g_viewStates.upscaleViewID, g_viewStates.upscaleViewOpen);
			return;
		}

		try {
			auto& viewMgr = ANI::StudioCore::GetViewManager();
			auto& entityMgr = ANI::StudioCore::GetEntityManager();

			if (g_viewStates.upscaleViewID == 0) {
				g_viewStates.upscaleViewID = viewMgr.CreateView();
				viewMgr.AddView<UpscaleView>(g_viewStates.upscaleViewID, UpscaleView(entityMgr));
				viewMgr.GetView<UpscaleView>(g_viewStates.upscaleViewID).Init();
				std::cout << "[MenuBar] Created UpscaleView with ID: " << g_viewStates.upscaleViewID << std::endl;
			}
		}
		catch (const std::exception& e) {
			std::cerr << "[MenuBar] Error managing UpscaleView: " << e.what() << std::endl;
			g_viewStates.upscaleViewOpen = false;
		}
	}

	void ShowOrCreateImageView() {
		if (!g_viewStates.imageViewOpen) {
			DestroyViewIfClosed(g_viewStates.imageViewID, g_viewStates.imageViewOpen);
			return;
		}

		try {
			auto& viewMgr = ANI::StudioCore::GetViewManager();
			auto& entityMgr = ANI::StudioCore::GetEntityManager();

			if (g_viewStates.imageViewID == 0) {
				g_viewStates.imageViewID = viewMgr.CreateView();
				viewMgr.AddView<ImageView>(g_viewStates.imageViewID, ImageView(entityMgr));
				viewMgr.GetView<ImageView>(g_viewStates.imageViewID).Init();
				std::cout << "[MenuBar] Created standalone ImageView with ID: " << g_viewStates.imageViewID << std::endl;
			}
		}
		catch (const std::exception& e) {
			std::cerr << "[MenuBar] Error managing ImageView: " << e.what() << std::endl;
			g_viewStates.imageViewOpen = false;
		}
	}

	void ShowOrCreateNodeGraphView() {
		if (!g_viewStates.nodeGraphViewOpen) {
			DestroyViewIfClosed(g_viewStates.nodeGraphViewID, g_viewStates.nodeGraphViewOpen);
			return;
		}

		try {
			auto& viewMgr = ANI::StudioCore::GetViewManager();
			auto& entityMgr = ANI::StudioCore::GetEntityManager();

			if (g_viewStates.nodeGraphViewID == 0) {
				g_viewStates.nodeGraphViewID = viewMgr.CreateView();
				viewMgr.AddView<NodeGraphView>(g_viewStates.nodeGraphViewID, NodeGraphView(entityMgr));
				viewMgr.GetView<NodeGraphView>(g_viewStates.nodeGraphViewID).Init();
				std::cout << "[MenuBar] Created NodeGraphView with ID: " << g_viewStates.nodeGraphViewID << std::endl;
			}
		}
		catch (const std::exception& e) {
			std::cerr << "[MenuBar] Error managing NodeGraphView: " << e.what() << std::endl;
			g_viewStates.nodeGraphViewOpen = false;
		}
	}

	void ShowOrCreateSequencerView() {
		if (!g_viewStates.sequencerViewOpen) {
			DestroyViewIfClosed(g_viewStates.sequencerViewID, g_viewStates.sequencerViewOpen);
			return;
		}

		try {
			auto& viewMgr = ANI::StudioCore::GetViewManager();
			auto& entityMgr = ANI::StudioCore::GetEntityManager();

			if (g_viewStates.sequencerViewID == 0) {
				g_viewStates.sequencerViewID = viewMgr.CreateView();
				viewMgr.AddView<SequencerView>(g_viewStates.sequencerViewID, SequencerView(entityMgr));
				viewMgr.GetView<SequencerView>(g_viewStates.sequencerViewID).Init();
				std::cout << "[MenuBar] Created SequencerView with ID: " << g_viewStates.sequencerViewID << std::endl;
			}
		}
		catch (const std::exception& e) {
			std::cerr << "[MenuBar] Error managing SequencerView: " << e.what() << std::endl;
			g_viewStates.sequencerViewOpen = false;
		}
	}

	void ShowOrCreateNodeView() {
		if (!g_viewStates.nodeViewOpen) {
			DestroyViewIfClosed(g_viewStates.nodeViewID, g_viewStates.nodeViewOpen);
			return;
		}

		try {
			auto& viewMgr = ANI::StudioCore::GetViewManager();
			auto& entityMgr = ANI::StudioCore::GetEntityManager();

			if (g_viewStates.nodeViewID == 0) {
				g_viewStates.nodeViewID = viewMgr.CreateView();
				viewMgr.AddView<NodeView>(g_viewStates.nodeViewID, NodeView(entityMgr));
				viewMgr.GetView<NodeView>(g_viewStates.nodeViewID).Init();
				std::cout << "[MenuBar] Created NodeView with ID: " << g_viewStates.nodeViewID << std::endl;
			}
		}
		catch (const std::exception& e) {
			std::cerr << "[MenuBar] Error managing NodeView: " << e.what() << std::endl;
			g_viewStates.nodeViewOpen = false;
		}
	}

	void ShowOrCreateVideoView() {
		if (!g_viewStates.videoViewOpen) {
			DestroyViewIfClosed(g_viewStates.videoViewID, g_viewStates.videoViewOpen);
			return;
		}

		try {
			auto& viewMgr = ANI::StudioCore::GetViewManager();
			auto& entityMgr = ANI::StudioCore::GetEntityManager();

			if (g_viewStates.videoViewID == 0) {
				g_viewStates.videoViewID = viewMgr.CreateView();
				viewMgr.AddView<VideoView>(g_viewStates.videoViewID, VideoView(entityMgr));
				viewMgr.GetView<VideoView>(g_viewStates.videoViewID).Init();
				std::cout << "[MenuBar] Created VideoView with ID: " << g_viewStates.videoViewID << std::endl;
			}
		}
		catch (const std::exception& e) {
			std::cerr << "[MenuBar] Error managing VideoView: " << e.what() << std::endl;
			g_viewStates.videoViewOpen = false;
		}
	}

	void ShowOrCreateVideoSequencerView() {
		if (!g_viewStates.videoSequencerViewOpen) {
			DestroyViewIfClosed(g_viewStates.videoSequencerViewID, g_viewStates.videoSequencerViewOpen);
			return;
		}

		try {
			auto& viewMgr = ANI::StudioCore::GetViewManager();
			auto& entityMgr = ANI::StudioCore::GetEntityManager();

			if (g_viewStates.videoSequencerViewID == 0) {
				g_viewStates.videoSequencerViewID = viewMgr.CreateView();
				// Note: You had VideoView registered twice with different names
				// Assuming you want a separate VideoSequencerView class, or change this to VideoView
				viewMgr.AddView<VideoView>(g_viewStates.videoSequencerViewID, VideoView(entityMgr));
				viewMgr.GetView<VideoView>(g_viewStates.videoSequencerViewID).Init();
				std::cout << "[MenuBar] Created VideoSequencerView with ID: " << g_viewStates.videoSequencerViewID << std::endl;
			}
		}
		catch (const std::exception& e) {
			std::cerr << "[MenuBar] Error managing VideoSequencerView: " << e.what() << std::endl;
			g_viewStates.videoSequencerViewOpen = false;
		}
	}

	void ShowOrCreateZepView() {
		if (!g_viewStates.zepViewOpen) {
			DestroyViewIfClosed(g_viewStates.zepViewID, g_viewStates.zepViewOpen);
			return;
		}

		try {
			auto& viewMgr = ANI::StudioCore::GetViewManager();
			auto& entityMgr = ANI::StudioCore::GetEntityManager();

			if (g_viewStates.zepViewID == 0) {
				g_viewStates.zepViewID = viewMgr.CreateView();
				viewMgr.AddView<ZepView>(g_viewStates.zepViewID, ZepView(entityMgr));
				viewMgr.GetView<ZepView>(g_viewStates.zepViewID).Init();
				std::cout << "[MenuBar] Created ZepView with ID: " << g_viewStates.zepViewID << std::endl;
			}
		}
		catch (const std::exception& e) {
			std::cerr << "[MenuBar] Error managing ZepView: " << e.what() << std::endl;
			g_viewStates.zepViewOpen = false;
		}
	}

	// Helper functions for view lifecycle management
	void DestroyViewIfClosed(GUI::ViewListID& viewID, bool& isOpen) {
		if (viewID != 0 && !isOpen) {
			try {
				auto& viewMgr = ANI::StudioCore::GetViewManager();
				viewMgr.DestroyView(viewID);
				std::cout << "[MenuBar] Destroyed view with ID: " << viewID << std::endl;
				viewID = 0;
			}
			catch (const std::exception& e) {
				std::cerr << "[MenuBar] Error destroying view: " << e.what() << std::endl;
				viewID = 0; // Reset anyway to prevent issues
			}
		}
	}

	void DestroyAllClosedViews() {
		DestroyViewIfClosed(g_viewStates.settingsViewID, g_viewStates.settingsWindowOpen);
		DestroyViewIfClosed(g_viewStates.debugViewID, g_viewStates.debugWindowOpen);
		DestroyViewIfClosed(g_viewStates.convertViewID, g_viewStates.convertWindowOpen);
		DestroyViewIfClosed(g_viewStates.viewListManagerViewID, g_viewStates.viewsWindowOpen);
		DestroyViewIfClosed(g_viewStates.pluginViewID, g_viewStates.pluginsWindowOpen);
		DestroyViewIfClosed(g_viewStates.helpViewID, g_viewStates.helpWindowOpen);

		DestroyViewIfClosed(g_viewStates.diffusionViewID, g_viewStates.diffusionViewOpen);
		DestroyViewIfClosed(g_viewStates.upscaleViewID, g_viewStates.upscaleViewOpen);
		DestroyViewIfClosed(g_viewStates.imageViewID, g_viewStates.imageViewOpen);
		DestroyViewIfClosed(g_viewStates.nodeGraphViewID, g_viewStates.nodeGraphViewOpen);
		DestroyViewIfClosed(g_viewStates.sequencerViewID, g_viewStates.sequencerViewOpen);
		DestroyViewIfClosed(g_viewStates.nodeViewID, g_viewStates.nodeViewOpen);
		DestroyViewIfClosed(g_viewStates.videoViewID, g_viewStates.videoViewOpen);
		DestroyViewIfClosed(g_viewStates.videoSequencerViewID, g_viewStates.videoSequencerViewOpen);
		DestroyViewIfClosed(g_viewStates.zepViewID, g_viewStates.zepViewOpen);
	}

	// Helper function to manage all active views - call this from your main render loop
	void RenderAllViews() {
		// Manage ECS-based views - these create/destroy views in the ViewManager
		ShowOrCreateSettingsView();
		ShowOrCreateDebugView();
		ShowOrCreateConvertView();
		ShowOrCreateViewsView();
		ShowOrCreatePluginsView();
		ShowOrCreateHelpView();
		ShowOrCreateDiffusionView();  // Contains both DiffusionView and ImageView
		ShowOrCreateUpscaleView();
		ShowOrCreateImageView();
		ShowOrCreateNodeGraphView();
		ShowOrCreateSequencerView();
		ShowOrCreateNodeView();
		ShowOrCreateVideoView();
		ShowOrCreateVideoSequencerView();
		ShowOrCreateZepView();

		// Clean up any views that were closed
		DestroyAllClosedViews();
	}

} // namespace GUI