/*
		d8888          d8b  .d8888b.  888                  888 d8b
	   d88888          Y8P d88P  Y88b 888                  888 Y8P
	  d88P888              Y88b.      888                  888
	 d88P 888 88888b.  888  "Y888b.   888888 888  888  .d88888 888  .d88b.
	d88P  888 888 "88b 888     "Y8888 888    888  888 d88" 888 888 d88""88b
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

#include "Plugin.hpp"
#include "cr.h"
#include <iostream>

 // ============================================================================
 // CR PLUGIN IMPLEMENTATION MACROS - For easy CR plugin creation
 // ============================================================================

 // Global plugin instance for CR callbacks
namespace Plugin {
	extern Plugin* g_currentPlugin;
}

// Macro to implement the CR entry point for a plugin class
#define IMPLEMENT_CR_PLUGIN(PluginClass) \
    namespace Plugin { \
        Plugin* g_currentPlugin = nullptr; \
    } \
    \
    static PluginClass* s_pluginInstance = nullptr; \
    \
    extern "C" int cr_main(struct cr_plugin* ctx, enum cr_op operation) { \
        auto* hostData = static_cast<Plugin::HostData*>(ctx->userdata); \
        \
        switch (operation) { \
            case CR_LOAD: { \
                if (!s_pluginInstance) { \
                    s_pluginInstance = new PluginClass(); \
                    Plugin::g_currentPlugin = s_pluginInstance; \
                } \
                return s_pluginInstance->OnLoad(hostData) ? 0 : -1; \
            } \
            \
            case CR_STEP: { \
                if (s_pluginInstance) { \
                    float deltaTime = 1.0f/60.0f; /* default fallback */ \
                    return s_pluginInstance->OnUpdate(hostData, deltaTime) ? 0 : -1; \
                } \
                return -1; \
            } \
            \
            case CR_UNLOAD: { \
                if (s_pluginInstance) { \
                    s_pluginInstance->OnUnload(hostData); \
                } \
                return 0; \
            } \
            \
            case CR_CLOSE: { \
                if (s_pluginInstance) { \
                    s_pluginInstance->OnClose(hostData); \
                    delete s_pluginInstance; \
                    s_pluginInstance = nullptr; \
                    Plugin::g_currentPlugin = nullptr; \
                } \
                return 0; \
            } \
        } \
        return 0; \
    }

// Macro for component registration with error checking
#define REGISTER_COMPONENT_SAFE(ComponentType, Name, hostData) \
    do { \
        try { \
            RegisterComponent<ComponentType>(hostData, Name); \
            std::cout << "[Plugin] Registered component: " << Name << std::endl; \
        } catch (const std::exception& e) { \
            std::cerr << "[Plugin] Failed to register component " << Name << ": " << e.what() << std::endl; \
        } \
    } while(0)

// Macro for system registration with error checking
#define REGISTER_SYSTEM_SAFE(SystemType, hostData) \
    do { \
        try { \
            RegisterSystem<SystemType>(hostData); \
            std::cout << "[Plugin] Registered system: " << #SystemType << std::endl; \
        } catch (const std::exception& e) { \
            std::cerr << "[Plugin] Failed to register system " << #SystemType << ": " << e.what() << std::endl; \
        } \
    } while(0)

// Macro for view registration with error checking
#define REGISTER_VIEW_SAFE(ViewType, Name, hostData) \
    do { \
        if (HasGUI(hostData)) { \
            try { \
                RegisterView<ViewType>(hostData, Name); \
                std::cout << "[Plugin] Registered view: " << Name << std::endl; \
            } catch (const std::exception& e) { \
                std::cerr << "[Plugin] Failed to register view " << Name << ": " << e.what() << std::endl; \
            } \
        } else { \
            std::cout << "[Plugin] Skipping view registration (no GUI): " << Name << std::endl; \
        } \
    } while(0)

// Helper macro for CR state variables
#define PLUGIN_STATE static CR_STATE