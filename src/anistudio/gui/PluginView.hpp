// PluginView.hpp
#pragma once

#include "BaseView.hpp"
#include "PluginManager.hpp"
#include <imgui.h>
#include <string>
#include <vector>
#include <chrono>

namespace GUI {

	struct PluginNotification {
		std::string message;
		bool isError;
		float timeLeft;
		ImVec4 color;
	};

	class PluginView : public BaseView {
	public:
		explicit PluginView(ECS::EntityManager& entityMgr, Plugin::PluginManager& pluginMgr);
		~PluginView() override = default;

		void Init() override;
		void Update(float deltaTime) override;
		void Render() override;

	private:
		// Reference to plugin manager
		Plugin::PluginManager& pluginManager;

		// UI State members
		char directoryInputBuffer[512];
		std::vector<PluginNotification> notifications;
		bool showAdvancedControls = false;
		bool showStats = false;
		bool autoRefresh = true;
		bool showDirectoryInput = false;
		bool pluginListOpen = true;
		float refreshTimer = 0.0f;
		float refreshInterval = 2.0f;
		const float notificationDuration = 3.0f;
		std::string selectedPluginName;

		// Event handlers for CR plugin system
		void OnPluginLoaded(const std::string& name, bool isReload);
		void OnPluginUnloaded(const std::string& name);
		void OnPluginError(const std::string& name, const std::string& error);

		// UI rendering methods
		void RenderPluginControls();
		void RenderPluginList();
		void RenderHotReloadControls();
		void RenderPluginInfo();
		void RenderPluginStats();
		void RenderNotifications();
		void RenderDirectoryControls();

		// Helper methods
		void UpdateNotifications(float deltaT);
		void AddNotification(const std::string& message, bool isError);
		void HandleKeyboardShortcuts();
		ImVec4 GetStatusColor(bool isLoaded, bool hasError) const;
		const char* GetStatusText(bool isLoaded, bool hasError) const;
	};
}