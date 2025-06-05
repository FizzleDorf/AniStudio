/*
 * ExamplePlugin.hpp - Simple example plugin
 */

#pragma once

#include "BasePlugin.hpp"

class ExamplePlugin : public Plugin::BasePlugin {
public:
	ExamplePlugin() = default;
	~ExamplePlugin() override = default;

	bool Initialize() override;
	void Shutdown() override;
	void Update(float deltaTime) override;
	Plugin::PluginInfo GetInfo() const override;
};