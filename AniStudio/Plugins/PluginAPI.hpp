#pragma once

// Plugin API macro definitions
// This file ensures consistent symbol export/import across the plugin system

#ifndef PLUGIN_API
#ifdef _WIN32
	// Windows DLL export/import
#ifdef PLUGIN_EXPORTS
	// When building a plugin DLL
#define PLUGIN_API __declspec(dllexport)
#else
	// When using plugin functions from main application
#define PLUGIN_API __declspec(dllimport)
#endif
#else
	// Unix-like systems (Linux, macOS)
#define PLUGIN_API __attribute__((visibility("default")))
#endif
#endif

// Core API macro for main application exports
#ifndef ANI_CORE_API
#ifdef _WIN32
#ifdef ANI_CORE_EXPORTS
#define ANI_CORE_API __declspec(dllexport)
#else
#define ANI_CORE_API
#endif
#else
#define ANI_CORE_API
#endif
#endif