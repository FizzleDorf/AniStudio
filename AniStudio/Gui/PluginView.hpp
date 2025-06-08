/*
	Fixed PluginView.hpp - Properly handles plugin management UI
*/

#pragma once

#include "Base/BaseView.hpp"
#include "PluginManager.hpp"
#include <imgui.h>
#include <vector>
#include <string>

namespace GUI {

	class PluginView : public BaseView {
	public:
		struct Notification {
			std::string message;
			ImVec4 color;
			float timeRemaining;
		};

		PluginView(ECS::EntityManager& entityMgr, Plugin::PluginManager& pluginMgr);
		~PluginView() override;

		// BaseView overrides
		void Init() override;
		void Render() override;
		void Update(const float deltaT) override;

		// Serialization
		nlohmann::json Serialize() const override;
		void Deserialize(const nlohmann::json& j) override;

	private:
		// Reference to the plugin manager
		Plugin::PluginManager& pluginManager;

		// UI state
		std::string selectedPlugin;
		std::vector<std::string> loadedPlugins;
		std::vector<Plugin::PluginManager::PluginInfo> cachedPluginInfo;
		std::vector<Notification> notifications;
		float refreshTimer = 0.0f;

		// Event handlers
		void OnPluginLoaded(const std::string& name, bool isReload);
		void OnPluginUnloaded(const std::string& name);

		// UI rendering methods
		void RenderDirectoryControls();
		void RenderHotReloadControls();
		void RenderPluginStats();
		void RenderPluginList();
		void RenderPluginDetails();
		void RenderNotifications();

		// Utility methods
		void RefreshPluginList();
	};

} // namespace GUI