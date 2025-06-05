#pragma once

#include "Base/BaseView.hpp"
#include "PluginManager.hpp"
#include "ImGuiFileDialog.h"
#include <imgui.h>
#include <string>
#include <vector>

namespace GUI {

	class PluginView : public BaseView {
	public:
		PluginView(ECS::EntityManager& entityMgr, Plugin::PluginManager& pluginMgr);
		~PluginView() override = default;

		void Init() override;
		void Render() override;
		void Update(const float deltaT) override;

		// BaseView serialization
		nlohmann::json Serialize() const override;
		void Deserialize(const nlohmann::json& j) override;

	private:
		Plugin::PluginManager& pluginManager;

		// UI state
		bool showLoadDialog = false;
		bool showPluginDetails = false;
		std::string selectedPlugin;
		char pluginPathBuffer[512] = "";
		char watchDirectoryBuffer[512] = "./plugins";
		int hotReloadInterval = 500;
		bool autoLoadNewPlugins = false;

		// Plugin statistics
		Plugin::PluginManager::Stats lastStats;

		// UI rendering methods
		void RenderPluginList();
		void RenderPluginDetails();
		void RenderHotReloadControls();
		void RenderPluginStatistics();
		void RenderLoadControls();

		// Helper methods
		void LoadPluginFromDialog();
		void RefreshPluginList();
		const char* GetStatusText(const Plugin::PluginManager::PluginInfo& info);
		ImVec4 GetStatusColor(const Plugin::PluginManager::PluginInfo& info);
		std::string FormatFileSize(size_t bytes);
		std::string FormatDuration(std::chrono::milliseconds ms);

		// Callbacks
		void OnPluginLoaded(const std::string& name, bool success);
		void OnPluginUnloaded(const std::string& name);
	};

} // namespace GUI