/*
		d8888          d8b  .d8888b.  888                  888 d8b
	   d88888          Y8P d88P  Y88b 888                  888 Y8P
	  d88P888              Y88b.      888                  888
	 d88P 888 88888b.  888  "Y888b.   888888 888  888  .d88888 888  .d88b.
	d88P  888 888 "88b 888     "Y88b. 888    888  888 d88" 888 888 d88""88b
   d88P   888 888  888 888       "888 888    888  888 888  888 888 888  888
  d8888888888 888  888 888 Y88b  d88P Y88b.  Y88b 888 Y88b 888 888 Y88..88P
 d88P     888 888  888 888  "Y8888P"   "Y888  "Y88888  "Y88888 888  "Y88P"

 * This file is part of AniStudio.
 * Copyright (C) 2025 FizzleDorf (AnimAnon)
 *
 * This software is dual-licensed under the GNU Lesser General Public License v3.0 (LGPL-3.0)
 * and a commercial license. You may choose to use it under either license.
 *
 * For the LGPL-3.0, see the LICENSE-LGPL-3.0.txt file in the repository.
 * For commercial license information, please contact legal@kframe.ai.
 */

#pragma once

#include "BaseView.hpp"
#include <imgui.h>
#include <string>
#include <vector>
#include <filesystem>

 // Forward declarations for the plugin managers
namespace Plugin {
	class EnginePluginManager;
	class StudioPluginManager;
}

/*
Plugin management GUI for AniStudio. This view provides a comprehensive interface
for loading, unloading, and managing plugins at runtime. Supports both Engine and
Studio plugin managers through a unified wrapper interface.

Features:
- Automatic plugin discovery from standard directories
- Load/unload/reload individual plugins
- Hot reload support with file watching
- Real-time notifications for plugin operations
- Plugin type indication (Engine vs Studio)
- Error handling and reporting
- Keyboard shortcuts (Ctrl+R to reload, Ctrl+U to unload)
- Context menus for plugin management
*/

namespace GUI {

	// Notification structure for the plugin view
	struct PluginNotification {
		std::string message;
		float timeLeft = 0.0f;
		bool isError = false;
		ImVec4 color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
	};

	// Plugin manager interface for the view
	class IPluginManagerWrapper {
	public:
		virtual ~IPluginManagerWrapper() = default;
		virtual bool LoadPlugin(const std::string& path) = 0;
		virtual bool UnloadPlugin(const std::string& name) = 0;
		virtual bool ReloadPlugin(const std::string& name) = 0;
		virtual void UnloadAllPlugins() = 0;
		virtual bool IsPluginLoaded(const std::string& name) const = 0;
		virtual std::vector<std::string> GetLoadedPluginNames() const = 0;
		virtual void StartHotReload(const std::string& watchDir) = 0;
		virtual void StopHotReload() = 0;
		virtual bool IsHotReloadActive() const = 0;
		virtual const std::string& GetWatchDirectory() const = 0;
		virtual std::string GetPluginType() const = 0; // "Engine" or "Studio"
	};

	// Wrapper for EnginePluginManager
	class EnginePluginWrapper : public IPluginManagerWrapper {
	private:
		Plugin::EnginePluginManager& manager;
	public:
		EnginePluginWrapper(Plugin::EnginePluginManager& mgr) : manager(mgr) {}

		bool LoadPlugin(const std::string& path) override;
		bool UnloadPlugin(const std::string& name) override;
		bool ReloadPlugin(const std::string& name) override;
		void UnloadAllPlugins() override;
		bool IsPluginLoaded(const std::string& name) const override;
		std::vector<std::string> GetLoadedPluginNames() const override;
		void StartHotReload(const std::string& watchDir) override;
		void StopHotReload() override;
		bool IsHotReloadActive() const override;
		const std::string& GetWatchDirectory() const override;
		std::string GetPluginType() const override { return "Engine"; }
	};

	// Wrapper for StudioPluginManager
	class StudioPluginWrapper : public IPluginManagerWrapper {
	private:
		Plugin::StudioPluginManager& manager;
	public:
		StudioPluginWrapper(Plugin::StudioPluginManager& mgr) : manager(mgr) {}

		bool LoadPlugin(const std::string& path) override;
		bool UnloadPlugin(const std::string& name) override;
		bool ReloadPlugin(const std::string& name) override;
		void UnloadAllPlugins() override;
		bool IsPluginLoaded(const std::string& name) const override;
		std::vector<std::string> GetLoadedPluginNames() const override;
		void StartHotReload(const std::string& watchDir) override;
		void StopHotReload() override;
		bool IsHotReloadActive() const override;
		const std::string& GetWatchDirectory() const override;
		std::string GetPluginType() const override { return "Studio"; }
	};

	class PluginView : public BaseView {
	private:
		std::unique_ptr<IPluginManagerWrapper> pluginManager;

		// UI state
		std::string selectedPluginName;
		char directoryInputBuffer[512] = "";
		char pluginPathBuffer[512] = "";
		bool showDirectoryInput = false;
		bool showAdvancedControls = false;
		bool showStats = false;
		bool autoRefresh = false;
		bool pluginListOpen = true;

		// Timing
		float refreshTimer = 0.0f;
		float refreshInterval = 2.0f;

		// Notifications
		std::vector<PluginNotification> notifications;
		float notificationDuration = 3.0f;

		// Available plugins cache
		std::vector<std::string> availablePlugins;
		float lastScanTime = 0.0f;
		float scanInterval = 2.0f;

	public:
		// Constructor for Engine plugin manager
		PluginView(ECS::EntityManager& entityMgr, Plugin::EnginePluginManager& engineMgr);

		// Constructor for Studio plugin manager  
		PluginView(ECS::EntityManager& entityMgr, Plugin::StudioPluginManager& studioMgr);

		static constexpr const char* GetMetadataJSON() {
			return R"({
                "displayName": "Plugin Manager", 
                "category": "Development",
                "description": "Comprehensive plugin management interface with hot reload support"
            })";
		}

		static ViewMetadata GetMetadata() {
			return GetMetadataFor<PluginView>();
		}

		void Init() override;
		void Update(const float deltaT) override;
		void Render() override;

	private:
		// Main rendering functions
		void RenderPluginControls();
		void RenderPluginList();
		void RenderAvailablePlugins();
		void RenderHotReloadControls();
		void RenderPluginInfo();
		void RenderPluginStats();
		void RenderNotifications();
		void RenderDirectoryControls();

		// Utility functions
		void UpdateNotifications(float deltaT);
		void AddNotification(const std::string& message, bool isError);
		void HandleKeyboardShortcuts();
		void ScanForAvailablePlugins();
		void LoadAllAvailablePlugins();

		// Helper functions
		ImVec4 GetStatusColor(bool isLoaded, bool hasError) const;
		const char* GetStatusText(bool isLoaded, bool hasError) const;
		std::string GetPluginNameFromPath(const std::string& path) const;
	};

} // namespace GUI