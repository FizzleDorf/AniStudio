#pragma once

#include "BaseView.hpp"
#include "StudioPluginManager.hpp"
#include <imgui.h>
#include <string>
#include <vector>
#include <filesystem>
#include <chrono>

namespace GUI {

	class PluginView : public BaseView {
	public:
		static constexpr const char* GetMetadataJSON() {
			return R"({
                "displayName": "Plugin Manager",
                "category": "Tools",
                "description": "Manage and configure plugins with versioned hot reload support"
            })";
		}

		PluginView(ECS::EntityManager& entityMgr, Plugins::StudioPluginManager& pluginMgr);
		~PluginView() = default;

		void Init() override;
		void Update(float deltaT) override;
		void Render() override;

	private:
		Plugins::StudioPluginManager& m_pluginManager;

		// UI state
		std::string m_pluginDirectory;
		std::string m_statusMessage;
		float m_statusTimer = 0.0f;

		// Hot reload settings - manages enabling/disabling based on view visibility
		bool m_hotReloadEnabled = true;
		bool m_hotReloadWasEnabled = false;

		// Plugin directories found
		std::vector<std::filesystem::path> m_pluginDirectories;

		// Selected plugin for details
		std::string m_selectedPlugin;

		// File dialog state
		bool m_showLoadDialog = false;
		char m_loadDialogPath[512] = "";

		// Plugin actions
		void RefreshPluginDirectories();
		void LoadPlugin(const std::filesystem::path& pluginDir);
		void LoadPluginFromFile();
		void EnablePlugin(const std::string& pluginName);
		void DisablePlugin(const std::string& pluginName);
		void ReloadPlugin(const std::string& pluginName);
		void UnloadPlugin(const std::string& pluginName);

		// Plugin state management actions
		void SaveGlobalPluginState();
		void SaveProjectPluginState();
		void LoadGlobalPluginState();
		void LoadProjectPluginState();

		// Versioned DLL helper functions
		uint32_t GetHighestVersionInDirectory(const std::filesystem::path& pluginDir, const std::string& pluginName) const;
		size_t CountVersionedDlls(const std::filesystem::path& pluginDir, const std::string& pluginName) const;

		// UI rendering
		void RenderToolbar();
		void RenderPluginDirectoryList();
		void RenderLoadedPluginsList();
		void RenderPluginDetails();
		void RenderStatusBar();

		// Utility
		void ShowStatus(const std::string& message, float duration = 3.0f);
		const char* GetPluginStateText(const Plugins::PluginInfo& plugin) const;
		ImVec4 GetPluginStateColor(const Plugins::PluginInfo& plugin) const;
	};

} // namespace GUI