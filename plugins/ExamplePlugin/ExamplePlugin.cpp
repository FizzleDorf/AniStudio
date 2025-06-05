/*
 * Example Plugin Implementation for AniStudio
 * This file contains the actual implementation of the example plugin.
 */

#include "ExamplePlugin.hpp"
#include <iostream>

 // Implement any complex methods here if needed
namespace {
	// Anonymous namespace for plugin-local utilities
	void LogPluginMessage(const std::string& level, const std::string& message) {
		std::cout << "[ExamplePlugin " << level << "] " << message << std::endl;
	}
}

// Move the main plugin macro HERE - this must be in a .cpp file to be compiled and exported
ANISTUDIO_PLUGIN_MAIN(
	ExamplePlugin,
	"Example Plugin",
	"1.0.0",
	"A simple example plugin demonstrating the AniStudio plugin system"
)