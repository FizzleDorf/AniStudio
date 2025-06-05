/*
 * ExamplePlugin.hpp - Simple example plugin
 */

#pragma once

#include "BasePlugin.hpp"
// Remove this include to avoid circular dependency
// #include "PluginManager.hpp"

class ExamplePlugin : public Plugin::BasePlugin {
public:
	ExamplePlugin() = default;
	~ExamplePlugin() override = default;

	// Implement BasePlugin interface
	bool Initialize(ECS::EntityManager& entityManager, GUI::ViewManager& viewManager) override;
	void Shutdown() override;
	void Update(float deltaTime) override;

	// Implement pure virtual functions from BasePlugin
	const std::string& GetName() const override;
	const std::string& GetVersion() const override;
	
	// Optional: Override GetDescription if you want a custom description
	// const std::string& GetDescription() const override;
};