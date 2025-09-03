#pragma once
#include "PluginManager.hpp"
#include <unordered_map>
#include <vector>

namespace GUI {
	class ViewManager;
}

namespace Plugins {

	// Forward declaration
	class StudioPluginManager;

	// Studio-specific registry that also handles views
	class StudioPluginRegistry : public PluginRegistry {
	public:
		StudioPluginRegistry(const std::string& pluginName, StudioPluginManager* manager, GUI::ViewManager& viewMgr);

		// Override to handle view registration
		GUI::ViewTypeID RegisterView(const ViewDescriptor& desc) override;

	private:
		GUI::ViewManager& viewManager;
		StudioPluginManager* studioManager;
	};

	class StudioPluginManager : public PluginManager {
	public:
		StudioPluginManager(ECS::EntityManager& entityMgr, GUI::ViewManager& viewMgr);
		~StudioPluginManager() = default;

		// Override to call both engine and studio init
		bool enablePlugin(const std::string& pluginName) override;

		// Override to handle view registration
		GUI::ViewTypeID registerView(const std::string& pluginName, const ViewDescriptor& desc) override;

	protected:
		// Override to clean up views
		void cleanupPluginViews(const std::string& pluginName) override;

	private:
		GUI::ViewManager& viewManager;

		// Track views registered by plugins
		std::unordered_map<std::string, std::vector<GUI::ViewTypeID>> pluginViews;

		// Make StudioPluginRegistry a friend so it can access our registerView method
		friend class StudioPluginRegistry;
	};

} // namespace Plugins