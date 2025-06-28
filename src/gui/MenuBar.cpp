#include "MenuBar.hpp"
#include "../events/Events.hpp"
#include "AniStudio.hpp"

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

	// CRITICAL FIX: Only ONE ViewListID for diffusion workspace (contains both DiffusionView AND ImageView)
	static GUI::ViewListID settingsViewID = 0;
	static GUI::ViewListID debugViewID = 0;
	static GUI::ViewListID convertViewID = 0;
	static GUI::ViewListID viewListManagerViewID = 0;
	static GUI::ViewListID pluginViewID = 0;
	static GUI::ViewListID helpViewID = 0;

	// ONE ViewListID for diffusion workspace (will contain BOTH view types)
	static GUI::ViewListID diffusionViewID = 0;        // Contains DiffusionView + ImageView

	static GUI::ViewListID upscaleViewID = 0;
	static GUI::ViewListID imageViewID = 0;
	static GUI::ViewListID nodeGraphViewID = 0;
	static GUI::ViewListID sequencerViewID = 0;
	static GUI::ViewListID nodeViewID = 0;
	static GUI::ViewListID videoViewID = 0;
	static GUI::ViewListID videoSequencerViewID = 0;
	static GUI::ViewListID zepViewID = 0;

	// Helper function with robust error handling
	static void DestroyViewIfClosed(GUI::ViewListID& viewID, bool isOpen, GUI::ViewManager& viewManager) {
		if (viewID != 0 && !isOpen) {
			try {
				std::cout << "[MenuBar] Destroying ViewListID: " << viewID << std::endl;
				viewManager.DestroyView(viewID);
				viewID = 0;
				std::cout << "[MenuBar] ViewListID destroyed successfully" << std::endl;
			}
			catch (const std::exception& e) {
				std::cerr << "[MenuBar] Error destroying ViewListID " << viewID << ": " << e.what() << std::endl;
				viewID = 0; // Reset anyway to prevent repeated attempts
			}
		}
	}

	// Macro to reduce boilerplate for simple single-component views
#define CREATE_SIMPLE_VIEW(viewType, viewOpen, viewID, viewClass) \
		if (viewOpen && viewID == 0) { \
			try { \
				std::cout << "[MenuBar] Creating " #viewClass "..." << std::endl; \
				viewID = viewManager.CreateView(); \
				viewManager.AddView<viewClass>(viewID, viewClass(entityManager)); \
				viewManager.GetView<viewClass>(viewID).Init(); \
				std::cout << "[MenuBar] " #viewClass " created successfully with ViewListID: " << viewID << std::endl; \
			} \
			catch (const std::exception& e) { \
				std::cerr << "[MenuBar] FAILED to create " #viewClass ": " << e.what() << std::endl; \
				if (viewID != 0) { \
					try { viewManager.DestroyView(viewID); } catch (...) {} \
					viewID = 0; \
				} \
				viewOpen = false; \
			} \
		} \
		DestroyViewIfClosed(viewID, viewOpen, viewManager);

	static void UpdateECSViews(GUI::ViewManager& viewManager, ECS::EntityManager& entityManager) {
		try {
			// ========================================================================
			// ECS-STYLE: Create ONE ViewListID with MULTIPLE view components
			// ========================================================================
			if (diffusionViewOpen && diffusionViewID == 0) {
				std::cout << "[MenuBar] Creating ECS-style diffusion workspace..." << std::endl;

				try {
					// Step 1: Create ONE ViewListID (like creating one entity)
					diffusionViewID = viewManager.CreateView();
					std::cout << "[MenuBar] Created ViewListID: " << diffusionViewID << std::endl;

					// Step 2: Add DiffusionView component to this ViewListID
					std::cout << "[MenuBar] Adding DiffusionView component..." << std::endl;
					viewManager.AddView<DiffusionView>(diffusionViewID, DiffusionView(entityManager));
					viewManager.GetView<DiffusionView>(diffusionViewID).Init();
					std::cout << "[MenuBar] DiffusionView component added and initialized" << std::endl;

					// Step 3: Add ImageView component to THE SAME ViewListID
					std::cout << "[MenuBar] Adding ImageView component to same ViewListID..." << std::endl;
					viewManager.AddView<ImageView>(diffusionViewID, ImageView(entityManager));
					viewManager.GetView<ImageView>(diffusionViewID).Init();
					std::cout << "[MenuBar] ImageView component added and initialized" << std::endl;

					std::cout << "[MenuBar] ECS-style diffusion workspace created successfully!" << std::endl;
					std::cout << "  ViewListID: " << diffusionViewID << std::endl;
					std::cout << "  Components: DiffusionView + ImageView" << std::endl;

					// Optional: Verify both components exist
					if (viewManager.HasView<DiffusionView>(diffusionViewID) && viewManager.HasView<ImageView>(diffusionViewID)) {
						std::cout << "[MenuBar] Verified: Both view components exist on ViewListID " << diffusionViewID << std::endl;
					}
					else {
						std::cerr << "[MenuBar] Warning: Component verification failed!" << std::endl;
					}
				}
				catch (const std::exception& e) {
					std::cerr << "[MenuBar] FAILED to create ECS-style diffusion workspace: " << e.what() << std::endl;

					// Cleanup on failure
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

			// When diffusion is closed, destroy the ViewListID (destroys BOTH components automatically)
			if (!diffusionViewOpen && diffusionViewID != 0) {
				std::cout << "[MenuBar] Destroying diffusion workspace (removes both components)..." << std::endl;
				viewManager.DestroyView(diffusionViewID);  // This removes BOTH DiffusionView AND ImageView
				diffusionViewID = 0;
			}

			// ========================================================================
			// SINGLE-COMPONENT VIEWS (Traditional Style)
			// ========================================================================

			// Settings View
			CREATE_SIMPLE_VIEW("Settings", settingsWindowOpen, settingsViewID, SettingsView);

			// Debug View
			CREATE_SIMPLE_VIEW("Debug", debugWindowOpen, debugViewID, DebugView);

			// Convert View
			CREATE_SIMPLE_VIEW("Convert", convertWindowOpen, convertViewID, ConvertView);

			// View Manager View (requires both ViewManager and EntityManager)
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
			DestroyViewIfClosed(viewListManagerViewID, viewsWindowOpen, viewManager);

			// Plugin View (requires PluginManager)
			if (pluginsWindowOpen && pluginViewID == 0) {
				try {
					std::cout << "[MenuBar] Creating PluginView..." << std::endl;
					auto& pluginMgr = ANI::StudioCore::GetPluginManager();
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
			DestroyViewIfClosed(pluginViewID, pluginsWindowOpen, viewManager);

			// Help View
			CREATE_SIMPLE_VIEW("Help", helpWindowOpen, helpViewID, HelpView);

			// Tool Views
			CREATE_SIMPLE_VIEW("Upscale", upscaleViewOpen, upscaleViewID, UpscaleView);
			CREATE_SIMPLE_VIEW("Image", imageViewOpen, imageViewID, ImageView);
			CREATE_SIMPLE_VIEW("NodeGraph", nodeGraphViewOpen, nodeGraphViewID, NodeGraphView);
			CREATE_SIMPLE_VIEW("Sequencer", sequencerViewOpen, sequencerViewID, SequencerView);
			CREATE_SIMPLE_VIEW("Node", nodeViewOpen, nodeViewID, NodeView);
			CREATE_SIMPLE_VIEW("Video", videoViewOpen, videoViewID, VideoView);
			CREATE_SIMPLE_VIEW("Zep", zepViewOpen, zepViewID, ZepView);

			// Video Sequencer View (uses VideoView class)
			if (videoSequencerViewOpen && videoSequencerViewID == 0) {
				try {
					std::cout << "[MenuBar] Creating VideoSequencerView..." << std::endl;
					videoSequencerViewID = viewManager.CreateView();
					viewManager.AddView<VideoView>(videoSequencerViewID, VideoView(entityManager));
					viewManager.GetView<VideoView>(videoSequencerViewID).Init();
					std::cout << "[MenuBar] VideoSequencerView created successfully with ViewListID: " << videoSequencerViewID << std::endl;
				}
				catch (const std::exception& e) {
					std::cerr << "[MenuBar] FAILED to create VideoSequencerView: " << e.what() << std::endl;
					if (videoSequencerViewID != 0) {
						try { viewManager.DestroyView(videoSequencerViewID); }
						catch (...) {}
						videoSequencerViewID = 0;
					}
					videoSequencerViewOpen = false;
				}
			}
			DestroyViewIfClosed(videoSequencerViewID, videoSequencerViewOpen, viewManager);

			// ========================================================================
			// OPTIONAL: Dynamic view component management (like ECS systems)
			// ========================================================================

			// Add/remove components dynamically with hotkeys
			if (ImGui::IsKeyPressed(ImGuiKey_F1)) {
				// Add a debug component to the diffusion workspace
				if (diffusionViewID != 0 && !viewManager.HasView<DebugView>(diffusionViewID)) {
					try {
						viewManager.AddView<DebugView>(diffusionViewID, DebugView(entityManager));
						viewManager.GetView<DebugView>(diffusionViewID).Init();
						std::cout << "[MenuBar] Added DebugView component to diffusion workspace!" << std::endl;
					}
					catch (const std::exception& e) {
						std::cerr << "[MenuBar] Failed to add DebugView: " << e.what() << std::endl;
					}
				}
			}

			if (ImGui::IsKeyPressed(ImGuiKey_F2)) {
				// Remove the debug component from diffusion workspace
				if (diffusionViewID != 0 && viewManager.HasView<DebugView>(diffusionViewID)) {
					try {
						viewManager.RemoveView<DebugView>(diffusionViewID);
						std::cout << "[MenuBar] Removed DebugView component from diffusion workspace!" << std::endl;
					}
					catch (const std::exception& e) {
						std::cerr << "[MenuBar] Failed to remove DebugView: " << e.what() << std::endl;
					}
				}
			}

		}
		catch (const std::exception& e) {
			std::cerr << "[MenuBar] CRITICAL ERROR in UpdateECSViews: " << e.what() << std::endl;

			// In case of critical error, try to log the ViewManager state for debugging
			try {
				viewManager.DebugPrintState();
			}
			catch (...) {
				std::cerr << "[MenuBar] Could not print ViewManager debug state" << std::endl;
			}
		}
	}

	void ShowMenuBar(GLFWwindow* window, GUI::ViewManager& viewManager, ECS::EntityManager& entityManager) {
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

				if (ImGui::MenuItem("Video Sequencer", nullptr, videoSequencerViewOpen)) {
					videoSequencerViewOpen = !videoSequencerViewOpen;
				}

				if (ImGui::MenuItem("Zep Editor", nullptr, zepViewOpen)) {
					zepViewOpen = !zepViewOpen;
				}

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("View")) {
				if (ImGui::MenuItem("View Manager", nullptr, viewsWindowOpen)) {
					viewsWindowOpen = !viewsWindowOpen;
				}

				ImGui::Separator();

				if (ImGui::MenuItem("Debug", nullptr, debugWindowOpen)) {
					debugWindowOpen = !debugWindowOpen;
				}

				ImGui::Separator();

				if (ImGui::MenuItem("Plugins", nullptr, pluginsWindowOpen)) {
					pluginsWindowOpen = !pluginsWindowOpen;
				}

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Help")) {
				if (ImGui::MenuItem("Help", nullptr, helpWindowOpen)) {
					helpWindowOpen = !helpWindowOpen;
				}
				ImGui::Separator();
				ImGui::Text("Hotkeys:");
				ImGui::Text("F1: Add Debug to Diffusion");
				ImGui::Text("F2: Remove Debug from Diffusion");
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Debug")) {
				if (ImGui::MenuItem("Print ViewManager State")) {
					viewManager.DebugPrintState();
				}
				if (ImGui::MenuItem("Print EntityManager State")) {
					entityManager.DebugPrintRegisteredComponents();
				}
				ImGui::EndMenu();
			}

			ImGui::EndMainMenuBar();
		}

		// Update ECS views based on menu states
		UpdateECSViews(viewManager, entityManager);
	}

#undef CREATE_SIMPLE_VIEW

} // namespace GUI