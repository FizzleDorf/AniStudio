#pragma once

#include "BaseView.hpp"
#include "StudioPluginManager.hpp"
#include <imgui.h>
#include <string>
#include <vector>
#include <set>
#include <filesystem>

namespace GUI {

	struct PluginDirectoryInfo {
		std::filesystem::path path;
		bool isDefault;
		std::string displayName;

		PluginDirectoryInfo(const std::filesystem::path& p, bool def = false)
			: path(p), isDefault(def), displayName(p.filename().string()) {}
	};

	struct AvailablePluginInfo {
		std::string name;
		std::filesystem::path directory;
		bool isLoaded;
		bool isEnabled;
		uint32_t currentVersion;
		std::string sourceDirectory; // Which search directory it came from
		bool isProjectScope; // true = project-specific, false = global

		AvailablePluginInfo(const std::string& n, const std::filesystem::path& dir, const std::string& src)
			: name(n), directory(dir), isLoaded(false), isEnabled(false), currentVersion(0), sourceDirectory(src), isProjectScope(false) {}
	};

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
		std::string m_statusMessage;
		float m_statusTimer = 0.0f;
		float m_filterListWidth = 250.0f;

		// Hot reload settings
		bool m_hotReloadEnabled = true;
		bool m_hotReloadWasEnabled = false;

		// Plugin search directories
		std::vector<PluginDirectoryInfo> m_searchDirectories;

		// Available plugins (discovered from all search directories)
		std::vector<AvailablePluginInfo> m_availablePlugins;

		// Selected items in filter list
		std::set<std::string> m_selectedDirectories;
		std::string m_lastSelectedDirectory;

		// Selected plugin for details
		std::string m_selectedPluginForDetails;

		// Plugin discovery and management
		void RefreshAvailablePlugins();
		void DiscoverPluginsInDirectory(const std::filesystem::path& searchDir);
		void AddSearchDirectory(const std::filesystem::path& newDir);
		void RemoveSearchDirectory(const std::filesystem::path& dir);

		// Plugin actions
		void LoadPlugin(const std::string& pluginName);
		void EnablePlugin(const std::string& pluginName);
		void DisablePlugin(const std::string& pluginName);
		void ReloadPlugin(const std::string& pluginName);
		void UnloadPlugin(const std::string& pluginName);
		void SetPluginScope(const std::string& pluginName, bool isProjectScope);

		// UI rendering
		void RenderMainContent();
		void RenderFilterList();
		void RenderPluginLists();
		void RenderAvailablePluginsList();
		void RenderActivePluginsList();
		void RenderPluginDetails();
		void RenderStatusBar();

		// Selection handling
		void HandleDirectorySelection(const std::string& dirName, bool ctrlHeld, bool shiftHeld);
		void SelectAllDirectories();
		void DeselectAllDirectories();

		// Utility
		void ShowStatus(const std::string& message, float duration = 3.0f);
		const char* GetPluginStateText(const Plugins::PluginInfo& plugin) const;
		ImVec4 GetPluginStateColor(const Plugins::PluginInfo& plugin) const;
		bool IsPluginInSelectedDirectories(const AvailablePluginInfo& plugin) const;
		AvailablePluginInfo* FindAvailablePlugin(const std::string& pluginName);
	};

} // namespace GUI