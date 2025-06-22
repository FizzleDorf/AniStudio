//============================================================================
// PluginInterface.cpp - Plugin Interface Implementation
//============================================================================

#include "PluginInterface.hpp"
#include <iostream>

namespace Plugin {

	void SetManagerGetters(
		GetEntityManagerFunc entityGetter,
		GetViewManagerFunc viewGetter,
		GetImGuiContextFunc contextGetter,
		GetImGuiAllocFunc allocGetter,
		GetImGuiFreeFunc freeGetter,
		GetImGuiUserDataFunc userDataGetter
	) {
		// This is implemented by individual plugins
		std::cout << "PluginInterface::SetManagerGetters called" << std::endl;
	}

	ECS::EntityManager* GetHostEntityManagerViaPointer() {
		// This will be implemented by plugins that need it
		return nullptr;
	}

	GUI::ViewManager* GetHostViewManagerViaPointer() {
		// This will be implemented by plugins that need it
		return nullptr;
	}

	ImGuiContext* GetHostImGuiContextViaPointer() {
		// This will be implemented by plugins that need it
		return nullptr;
	}

} // namespace Plugin