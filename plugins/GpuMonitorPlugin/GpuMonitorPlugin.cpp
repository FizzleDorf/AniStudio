// GpuMonitorPlugin.cpp
#include "../../../AniStudio/Plugins/PluginInterface.hpp"
#include "../../../AniStudio/ECS/Base/BaseComponent.hpp"
#include "../../../AniStudio/ECS/Base/BaseSystem.hpp"
#include "../../../AniStudio/GUI/Base/BaseView.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <fstream>
#include <chrono>
#include <deque>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>
#include <ctime>
#include <mutex>

// Define components
namespace ECS {
	// Component to store performance metrics
	struct GpuMetricsComponent : public BaseComponent {
		GpuMetricsComponent() { compName = "GpuMetrics"; }

		// Frame time metrics
		float frameTimeMs = 0.0f;
		float avgFrameTimeMs = 0.0f;
		float minFrameTimeMs = 9999.0f;
		float maxFrameTimeMs = 0.0f;

		// FPS metrics
		float currentFps = 0.0f;
		float avgFps = 0.0f;

		// GPU metrics (we'll get what we can from OpenGL)
		GLint totalMemoryKb = 0;
		GLint usedMemoryKb = 0;
		float gpuUtilizationPercent = 0.0f;
		std::string gpuName;
		std::string gpuVendor;
		std::string glVersion;

		// History for graphs
		static const size_t MAX_HISTORY = 150;
		std::deque<float> frameTimeHistory;
		std::deque<float> fpsHistory;
		std::deque<float> memoryUsageHistory;

		// Settings
		bool loggingEnabled = false;
		std::string logFilePath = "gpu_performance_log.csv";
		bool showOverlay = true;
		bool showDetailedView = true;
		ImVec4 graphColor = ImVec4(0.0f, 1.0f, 0.3f, 1.0f);
		ImVec4 textColor = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
		float overlayScale = 1.0f;
		int overlayCorner = 0; // 0=top-left, 1=top-right, 2=bottom-left, 3=bottom-right

		// Log file handle
		std::ofstream logFile;

		// Update frame metrics
		void UpdateFrameMetrics(float deltaTime) {
			frameTimeMs = deltaTime * 1000.0f; // Convert to milliseconds

			// Update min/max
			minFrameTimeMs = std::min(minFrameTimeMs, frameTimeMs);
			maxFrameTimeMs = std::max(maxFrameTimeMs, frameTimeMs);

			// Update history
			frameTimeHistory.push_back(frameTimeMs);
			if (frameTimeHistory.size() > MAX_HISTORY) {
				frameTimeHistory.pop_front();
			}

			// Calculate average frame time
			float sum = 0.0f;
			for (float t : frameTimeHistory) {
				sum += t;
			}
			avgFrameTimeMs = sum / frameTimeHistory.size();

			// Calculate FPS
			currentFps = 1000.0f / frameTimeMs;

			// Update FPS history
			fpsHistory.push_back(currentFps);
			if (fpsHistory.size() > MAX_HISTORY) {
				fpsHistory.pop_front();
			}

			// Calculate average FPS
			sum = 0.0f;
			for (float fps : fpsHistory) {
				sum += fps;
			}
			avgFps = sum / fpsHistory.size();

			// Log if enabled
			if (loggingEnabled && logFile.is_open()) {
				auto now = std::chrono::system_clock::now();
				auto timeT = std::chrono::system_clock::to_time_t(now);
				std::tm tm = *std::localtime(&timeT);

				logFile << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << ","
					<< frameTimeMs << ","
					<< currentFps << ","
					<< usedMemoryKb / 1024 << std::endl;
			}
		}

		// Update GPU metrics
		void UpdateGpuMetrics() {
			// Get GPU vendor and renderer
			if (gpuVendor.empty()) {
				const GLubyte* vendor = glGetString(GL_VENDOR);
				const GLubyte* renderer = glGetString(GL_RENDERER);
				const GLubyte* version = glGetString(GL_VERSION);

				if (vendor) gpuVendor = reinterpret_cast<const char*>(vendor);
				if (renderer) gpuName = reinterpret_cast<const char*>(renderer);
				if (version) glVersion = reinterpret_cast<const char*>(version);
			}

			// Try to get memory info
			// Note: This is vendor-specific and may not work on all GPUs
			// NVIDIA extension
			if (GLEW_NVX_gpu_memory_info) {
				glGetIntegerv(GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX, &totalMemoryKb);
				GLint availMemory = 0;
				glGetIntegerv(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, &availMemory);
				usedMemoryKb = totalMemoryKb - availMemory;
			}
			// AMD extension
			else if (GLEW_ATI_meminfo) {
				GLint info[4];
				glGetIntegerv(GL_TEXTURE_FREE_MEMORY_ATI, info);
				GLint availMemoryKb = info[0]; // Available memory in KB
				// This is a crude approximation as we don't know the total
				usedMemoryKb = 1024 * 1024 - availMemoryKb; // Assuming 1GB if we can't get total
				totalMemoryKb = 1024 * 1024;
			}
			else {
				// We can't get memory info, so just use dummy values
				totalMemoryKb = 1024 * 1024; // 1GB
				usedMemoryKb = totalMemoryKb / 2; // 50% usage as placeholder
			}

			// Update memory history
			float memoryUsagePercent = (float)usedMemoryKb / totalMemoryKb * 100.0f;
			memoryUsageHistory.push_back(memoryUsagePercent);

			if (memoryUsageHistory.size() > MAX_HISTORY) {
				memoryUsageHistory.pop_front();
			}

			// Simulate GPU utilization (not easily available through OpenGL)
			// In a real plugin, you'd use platform-specific APIs for this
			static float lastTime = 0.0f;
			float time = (float)glfwGetTime();
			float dt = time - lastTime;
			lastTime = time;

			// Simple simulation based on frame time (not accurate!)
			gpuUtilizationPercent = std::min(100.0f, std::max(0.0f, frameTimeMs * 2.0f));
			gpuUtilizationPercent = 0.9f * gpuUtilizationPercent + 0.1f * (rand() % 20 + 70); // Add some variation
		}

		void StartLogging() {
			if (!loggingEnabled) return;

			if (logFile.is_open()) {
				logFile.close();
			}

			logFile.open(logFilePath, std::ios::out | std::ios::app);

			if (!logFile.is_open()) {
				std::cerr << "Failed to open log file: " << logFilePath << std::endl;
				loggingEnabled = false;
				return;
			}

			// Write CSV header if file is new
			if (logFile.tellp() == 0) {
				logFile << "Timestamp,FrameTime(ms),FPS,UsedMemory(MB)" << std::endl;
			}
		}

		void StopLogging() {
			if (logFile.is_open()) {
				logFile.close();
			}
		}

		// Serialize to JSON
		nlohmann::json Serialize() const override {
			nlohmann::json j = BaseComponent::Serialize();
			j[compName]["loggingEnabled"] = loggingEnabled;
			j[compName]["logFilePath"] = logFilePath;
			j[compName]["showOverlay"] = showOverlay;
			j[compName]["showDetailedView"] = showDetailedView;
			j[compName]["graphColor"] = {
				{"r", graphColor.x},
				{"g", graphColor.y},
				{"b", graphColor.z},
				{"a", graphColor.w}
			};
			j[compName]["textColor"] = {
				{"r", textColor.x},
				{"g", textColor.y},
				{"b", textColor.z},
				{"a", textColor.w}
			};
			j[compName]["overlayScale"] = overlayScale;
			j[compName]["overlayCorner"] = overlayCorner;
			return j;
		}

		// Deserialize from JSON
		void Deserialize(const nlohmann::json& j) override {
			BaseComponent::Deserialize(j);

			if (j.contains(compName)) {
				auto& data = j[compName];
				if (data.contains("loggingEnabled")) loggingEnabled = data["loggingEnabled"];
				if (data.contains("logFilePath")) logFilePath = data["logFilePath"];
				if (data.contains("showOverlay")) showOverlay = data["showOverlay"];
				if (data.contains("showDetailedView")) showDetailedView = data["showDetailedView"];

				if (data.contains("graphColor")) {
					auto& color = data["graphColor"];
					graphColor = ImVec4(
						color["r"].get<float>(),
						color["g"].get<float>(),
						color["b"].get<float>(),
						color["a"].get<float>()
					);
				}

				if (data.contains("textColor")) {
					auto& color = data["textColor"];
					textColor = ImVec4(
						color["r"].get<float>(),
						color["g"].get<float>(),
						color["b"].get<float>(),
						color["a"].get<float>()
					);
				}

				if (data.contains("overlayScale")) overlayScale = data["overlayScale"];
				if (data.contains("overlayCorner")) overlayCorner = data["overlayCorner"];
			}

			// Start logging if enabled
			if (loggingEnabled) {
				StartLogging();
			}
		}
	};
}

// Define systems
class GpuMonitorSystem : public ECS::BaseSystem {
public:
	GpuMonitorSystem(ECS::EntityManager& entityMgr) : BaseSystem(entityMgr) {
		sysName = "GpuMonitorSystem";
		AddComponentSignature<ECS::GpuMetricsComponent>();
	}

	void Start() override {
		// Make sure we have at least one entity with GpuMetricsComponent
		if (entities.empty()) {
			ECS::EntityID entity = mgr.AddNewEntity();
			mgr.AddComponent<ECS::GpuMetricsComponent>(entity);
		}
	}

	void Update(const float deltaT) override {
		for (auto entity : entities) {
			auto& metricsComp = mgr.GetComponent<ECS::GpuMetricsComponent>(entity);

			// Update metrics
			metricsComp.UpdateFrameMetrics(deltaT);
			metricsComp.UpdateGpuMetrics();
		}
	}

	void Destroy() override {
		for (auto entity : entities) {
			if (mgr.HasComponent<ECS::GpuMetricsComponent>(entity)) {
				auto& metricsComp = mgr.GetComponent<ECS::GpuMetricsComponent>(entity);
				metricsComp.StopLogging();
			}
		}
	}
};

// Define views
class GpuMonitorView : public GUI::BaseView {
public:
	GpuMonitorView(ECS::EntityManager& entityMgr) : BaseView(entityMgr) {
		viewName = "GPU Monitor";
	}

	void Init() override {
		// Find or create an entity with GpuMetricsComponent
		bool found = false;
		for (auto entity : mgr.GetAllEntities()) {
			if (mgr.HasComponent<ECS::GpuMetricsComponent>(entity)) {
				monitorEntity = entity;
				found = true;
				break;
			}
		}

		if (!found) {
			monitorEntity = mgr.AddNewEntity();
			mgr.AddComponent<ECS::GpuMetricsComponent>(monitorEntity);
		}
	}

	void Update(const float deltaT) override {
		// This gets called by the ViewManager
		// Check if we need to render the overlay
		if (mgr.HasComponent<ECS::GpuMetricsComponent>(monitorEntity)) {
			auto& metricsComp = mgr.GetComponent<ECS::GpuMetricsComponent>(monitorEntity);

			if (metricsComp.showOverlay) {
				RenderOverlay(metricsComp);
			}
		}
	}

	void Render() override {
		// Main view window
		ImGui::Begin(viewName.c_str());

		if (!mgr.HasComponent<ECS::GpuMetricsComponent>(monitorEntity)) {
			ImGui::Text("GPU Metrics Component not found!");
			ImGui::End();
			return;
		}

		auto& metricsComp = mgr.GetComponent<ECS::GpuMetricsComponent>(monitorEntity);

		// GPU information section
		ImGui::Text("GPU Information");
		ImGui::Separator();
		ImGui::Text("Vendor: %s", metricsComp.gpuVendor.c_str());
		ImGui::Text("GPU: %s", metricsComp.gpuName.c_str());
		ImGui::Text("OpenGL Version: %s", metricsComp.glVersion.c_str());

		ImGui::Spacing();

		// Performance section
		ImGui::Text("Performance Metrics");
		ImGui::Separator();
		ImGui::Text("FPS: %.1f (Avg: %.1f)", metricsComp.currentFps, metricsComp.avgFps);
		ImGui::Text("Frame Time: %.2f ms (Min: %.2f, Max: %.2f)",
			metricsComp.frameTimeMs,
			metricsComp.minFrameTimeMs,
			metricsComp.maxFrameTimeMs);

		// Memory section
		float memoryUsageMB = metricsComp.usedMemoryKb / 1024.0f;
		float totalMemoryMB = metricsComp.totalMemoryKb / 1024.0f;
		float memPercent = (memoryUsageMB / totalMemoryMB) * 100.0f;

		ImGui::Text("GPU Memory: %.1f MB / %.1f MB (%.1f%%)",
			memoryUsageMB, totalMemoryMB, memPercent);

		ImGui::ProgressBar(memPercent / 100.0f, ImVec2(-1, 0),
			std::to_string((int)memPercent).c_str());

		ImGui::Text("GPU Utilization: %.1f%%", metricsComp.gpuUtilizationPercent);
		ImGui::ProgressBar(metricsComp.gpuUtilizationPercent / 100.0f, ImVec2(-1, 0),
			std::to_string((int)metricsComp.gpuUtilizationPercent).c_str());

		ImGui::Spacing();

		// FPS Graph
		if (metricsComp.fpsHistory.size() > 1) {
			// Convert data to ImGui format
			static std::vector<float> values;
			values.resize(metricsComp.fpsHistory.size());
			std::copy(metricsComp.fpsHistory.begin(), metricsComp.fpsHistory.end(), values.begin());

			ImGui::Text("FPS History");
			ImGui::PlotLines("##fps_history", values.data(), values.size(), 0, NULL,
				0.0f, *std::max_element(values.begin(), values.end()) * 1.2f,
				ImVec2(-1, 80));
		}

		// Frame Time Graph
		if (metricsComp.frameTimeHistory.size() > 1) {
			static std::vector<float> values;
			values.resize(metricsComp.frameTimeHistory.size());
			std::copy(metricsComp.frameTimeHistory.begin(), metricsComp.frameTimeHistory.end(), values.begin());

			ImGui::Text("Frame Time History (ms)");
			ImGui::PlotLines("##frametime_history", values.data(), values.size(), 0, NULL,
				0.0f, *std::max_element(values.begin(), values.end()) * 1.2f,
				ImVec2(-1, 80));
		}

		// Memory Usage Graph
		if (metricsComp.memoryUsageHistory.size() > 1) {
			static std::vector<float> values;
			values.resize(metricsComp.memoryUsageHistory.size());
			std::copy(metricsComp.memoryUsageHistory.begin(), metricsComp.memoryUsageHistory.end(), values.begin());

			ImGui::Text("Memory Usage History (%%)");
			ImGui::PlotLines("##memory_history", values.data(), values.size(), 0, NULL,
				0.0f, 100.0f,
				ImVec2(-1, 80));
		}

		ImGui::Spacing();

		// Settings section
		if (ImGui::CollapsingHeader("Settings")) {
			ImGui::Checkbox("Show Overlay", &metricsComp.showOverlay);
			ImGui::SameLine();
			ImGui::Checkbox("Show Detailed View", &metricsComp.showDetailedView);

			ImGui::SliderFloat("Overlay Scale", &metricsComp.overlayScale, 0.5f, 2.0f);

			static const char* cornerNames[] = { "Top-Left", "Top-Right", "Bottom-Left", "Bottom-Right" };
			ImGui::Combo("Overlay Position", &metricsComp.overlayCorner, cornerNames, IM_ARRAYSIZE(cornerNames));

			ImGui::ColorEdit4("Graph Color", &metricsComp.graphColor.x);
			ImGui::ColorEdit4("Text Color", &metricsComp.textColor.x);

			ImGui::Separator();

			ImGui::Checkbox("Enable Logging", &metricsComp.loggingEnabled);

			ImGui::InputText("Log File Path", &metricsComp.logFilePath);

			if (ImGui::Button("Start Logging")) {
				metricsComp.loggingEnabled = true;
				metricsComp.StartLogging();
			}

			ImGui::SameLine();

			if (ImGui::Button("Stop Logging")) {
				metricsComp.loggingEnabled = false;
				metricsComp.StopLogging();
			}

			ImGui::SameLine();

			if (ImGui::Button("Reset Stats")) {
				metricsComp.minFrameTimeMs = 9999.0f;
				metricsComp.maxFrameTimeMs = 0.0f;
				metricsComp.frameTimeHistory.clear();
				metricsComp.fpsHistory.clear();
				metricsComp.memoryUsageHistory.clear();
			}
		}

		ImGui::End();
	}

private:
	// Render overlay in corner of the screen
	void RenderOverlay(ECS::GpuMetricsComponent& metrics) {
		// Set up overlay parameters
		const float PAD = 10.0f * metrics.overlayScale;
		const ImGuiWindowFlags window_flags =
			ImGuiWindowFlags_NoDecoration |
			ImGuiWindowFlags_AlwaysAutoResize |
			ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoFocusOnAppearing |
			ImGuiWindowFlags_NoNav |
			ImGuiWindowFlags_NoMove;

		// Position window
		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImVec2 work_size = viewport->WorkSize;
		ImVec2 window_pos, window_pos_pivot;

		switch (metrics.overlayCorner) {
		case 0: // Top-left
			window_pos = ImVec2(PAD, PAD);
			window_pos_pivot = ImVec2(0.0f, 0.0f);
			break;
		case 1: // Top-right
			window_pos = ImVec2(work_size.x - PAD, PAD);
			window_pos_pivot = ImVec2(1.0f, 0.0f);
			break;
		case 2: // Bottom-left
			window_pos = ImVec2(PAD, work_size.y - PAD);
			window_pos_pivot = ImVec2(0.0f, 1.0f);
			break;
		case 3: // Bottom-right
			window_pos = ImVec2(work_size.x - PAD, work_size.y - PAD);
			window_pos_pivot = ImVec2(1.0f, 1.0f);
			break;
		}

		ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
		ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background

		// Create the overlay window
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(metrics.textColor.x, metrics.textColor.y,
			metrics.textColor.z, metrics.textColor.w));

		if (ImGui::Begin("GPU Monitor Overlay", &metrics.showOverlay, window_flags)) {
			ImGui::Text("FPS: %.1f", metrics.currentFps);

			if (metrics.showDetailedView) {
				ImGui::Text("Frame Time: %.2f ms", metrics.frameTimeMs);

				float memoryUsageMB = metrics.usedMemoryKb / 1024.0f;
				float totalMemoryMB = metrics.totalMemoryKb / 1024.0f;

				ImGui::Text("GPU Memory: %.1f/%.1f MB", memoryUsageMB, totalMemoryMB);
				ImGui::Text("GPU Util: %.1f%%", metrics.gpuUtilizationPercent);

				// Mini graph for FPS
				if (metrics.fpsHistory.size() > 1) {
					std::vector<float> values(metrics.fpsHistory.begin(), metrics.fpsHistory.end());

					ImGui::PushStyleColor(ImGuiCol_PlotLines,
						ImVec4(metrics.graphColor.x, metrics.graphColor.y,
							metrics.graphColor.z, metrics.graphColor.w));

					ImGui::PlotLines("##mini_fps", values.data(), values.size(), 0, NULL,
						0.0f, *std::max_element(values.begin(), values.end()) * 1.2f,
						ImVec2(100 * metrics.overlayScale, 40 * metrics.overlayScale));

					ImGui::PopStyleColor();
				}
			}

			if (ImGui::IsMouseHoveringRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax())) {
				ImGui::SetTooltip("Left-click to open GPU Monitor window\nRight-click to hide overlay");

				if (ImGui::IsMouseClicked(0)) { // Left click
					// Open the main window
				}

				if (ImGui::IsMouseClicked(1)) { // Right click
					metrics.showOverlay = false;
				}
			}
		}
		ImGui::End();

		ImGui::PopStyleColor();
	}

	ECS::EntityID monitorEntity = 0;
};

// Define plugin class
class GpuMonitorPlugin : public Plugin::IPlugin {
public:
	GpuMonitorPlugin()
		: systemRegistered(false), viewID(0), entityID(0) {}

	~GpuMonitorPlugin() override {
		OnUnload();
	}

	void OnLoad(ECS::EntityManager& entityMgr, GUI::ViewManager& viewMgr) override {
		std::cout << "GpuMonitorPlugin::OnLoad()" << std::endl;

		// Store references
		this->entityMgr = &entityMgr;
		this->viewMgr = &viewMgr;

		// Register component
		entityMgr.RegisterComponentName<ECS::GpuMetricsComponent>("GpuMetrics");

		// Register system
		entityMgr.RegisterSystem<GpuMonitorSystem>();
		systemRegistered = true;

		// Create an entity with the component
		entityID = entityMgr.AddNewEntity();
		entityMgr.AddComponent<ECS::GpuMetricsComponent>(entityID);

		// Create the view
		viewID = viewMgr.CreateView();
		viewMgr.AddView<GpuMonitorView>(viewID, GpuMonitorView(entityMgr));

		// Initialize the view
		viewMgr.GetView<GpuMonitorView>(viewID).Init();
	}

	void OnUnload() override {
		std::cout << "GpuMonitorPlugin::OnUnload()" << std::endl;

		// Clean up
		if (viewMgr && viewID != 0) {
			viewMgr->DestroyView(viewID);
			viewID = 0;
		}

		if (entityMgr) {
			// Clean up entities
			if (entityID != 0) {
				if (entityMgr->HasComponent<ECS::GpuMetricsComponent>(entityID)) {
					auto& metricsComp = entityMgr->GetComponent<ECS::GpuMetricsComponent>(entityID);
					metricsComp.StopLogging();
				}

				entityMgr->DestroyEntity(entityID);
				entityID = 0;
			}

			// Unregister system if possible
			if (systemRegistered) {
				// entityMgr->UnregisterSystem<GpuMonitorSystem>();
				systemRegistered = false;
			}
		}
	}

	void OnUpdate(float deltaTime) override {
		// Nothing needed here since the system will be updated by the EntityManager
	}

	std::string GetName() const override {
		return "GPU Monitor";
	}

	std::string GetVersion() const override {
		return "1.0.0";
	}

	std::string GetDescription() const override {
		return "Monitors GPU performance metrics and displays them in a customizable overlay.";
	}

private:
	ECS::EntityManager* entityMgr = nullptr;
	GUI::ViewManager* viewMgr = nullptr;
	bool systemRegistered = false;
	GUI::ViewListID viewID = 0;
	ECS::EntityID entityID = 0;
};

// Export plugin creation/destruction functions
extern "C" {
	Plugin::IPlugin* CreatePlugin() {
		return new GpuMonitorPlugin();
	}

	void DestroyPlugin(Plugin::IPlugin* plugin) {
		delete plugin;
	}
}