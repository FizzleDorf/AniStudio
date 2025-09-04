#pragma once
#include "PluginManager.hpp"
#include <unordered_map>
#include <vector>
#include <string>
#include <functional>

// Forward declaration for ImGui
struct ImGuiContext;

namespace GUI {
	class ViewManager;
}

namespace Plugins {

	// Forward declaration
	class StudioPluginManager;

	// Studio-specific registry that also handles views - NOW WITH PROPER IMGUI CONTEXT
	class StudioPluginRegistry : public PluginRegistry {
	public:
		StudioPluginRegistry(const std::string& pluginName,
			StudioPluginManager* manager,
			GUI::ViewManager& viewMgr,
			ImGuiContext* mainContext = nullptr);

		// Override to handle view registration
		GUI::ViewTypeID RegisterView(const ViewDescriptor& desc) override;

	private:
		GUI::ViewManager& viewManager;
		StudioPluginManager* studioManager;
		ImGuiContext* mainImGuiContext;
	};

	class StudioPluginManager : public PluginManager {
	public:
		StudioPluginManager(ECS::EntityManager& entityMgr, GUI::ViewManager& viewMgr, ImGuiContext* mainContext = nullptr);
		~StudioPluginManager() = default;

		// Override to call both engine and studio init
		bool enablePlugin(const std::string& pluginName) override;

		// Override to handle view registration - NOW PROPERLY PASSES IMGUI CONTEXT
		GUI::ViewTypeID registerView(const std::string& pluginName, const ViewDescriptor& desc) override;

	protected:
		// Override to clean up views - NOW ACTUALLY REMOVES THEM
		void cleanupPluginViews(const std::string& pluginName) override;

	private:
		GUI::ViewManager& viewManager;
		ImGuiContext* mainImGuiContext = nullptr;

		// Track views registered by plugins - both IDs and names for proper cleanup
		std::unordered_map<std::string, std::vector<GUI::ViewTypeID>> pluginViews;
		std::unordered_map<std::string, std::vector<std::string>> pluginViewNames;

		// Make StudioPluginRegistry a friend so it can access our registerView method
		friend class StudioPluginRegistry;
	};

} // namespace Plugins