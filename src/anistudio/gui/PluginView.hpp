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