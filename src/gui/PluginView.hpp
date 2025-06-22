//============================================================================
// PluginView.hpp - FIXED Plugin Management UI View
//============================================================================

#pragma once

#include "GUI.h"
#include "PluginManager.hpp"
#include <string>

namespace Plugin {

	class PluginView : public GUI::BaseView {
	public:
		explicit PluginView(ECS::EntityManager& entityMgr, PluginManager& pluginMgr);
		~PluginView() override;

		void Init() override;
		void Render() override;
		void Update(float deltaTime) override;

	private:
		PluginManager& pluginManager;

		// UI State
		std::string selectedPlugin;
		bool showOnlyLoaded = false;
		std::string searchFilter;

		// UI sections
		void RenderToolbar();
		void RenderPluginList();
		void RenderPluginDetails();

		// Plugin operations
		void LoadSelectedPlugin();
		void UnloadSelectedPlugin();
		void ReloadSelectedPlugin();
		void RemoveSelectedPlugin();

		// UI helpers
		void RenderStatusBadge(const std::string& status, const ImVec4& color);

		// Error handling - FIXED: Change to take no parameters to match usage
		void ShowErrorModal();
		bool showErrorModal = false;
		std::string errorTitle;
		std::string errorMessage;
	};

} // namespace Plugin