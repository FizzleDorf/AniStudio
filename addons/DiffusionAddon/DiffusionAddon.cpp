#include "BasePlugin.hpp"
#include "PluginRegistry.hpp"
#include "EntityManager.hpp"
#include "ViewManager.hpp"

// Components and Systems
#include "SDCPPComponents.h"
#include "SDcppSystem.hpp"

// Views
#include "DiffusionView.hpp"
#include "ConvertView.hpp"
#include "UpscaleView.hpp"
#include "VideoDiffusionView.hpp"

#include <iostream>

class DiffusionAddon : public Plugins::BasePlugin {
public:
	DiffusionAddon() : BasePlugin("DiffusionAddon", "1.0.0") {}

	bool OnEngineInit(ECS::EntityManager& entityMgr, Plugins::IPluginRegistry& registry) override {
		LogInfo("Initializing Stable Diffusion Addon...");

		// Register components directly with EntityManager using templates
		entityMgr.RegisterComponentName<ECS::PromptComponent>("Prompt");
		entityMgr.RegisterComponentName<ECS::SamplerComponent>("Sampler");
		entityMgr.RegisterComponentName<ECS::GuidanceComponent>("Guidance");
		entityMgr.RegisterComponentName<ECS::ClipSkipComponent>("ClipSkip");
		entityMgr.RegisterComponentName<ECS::SLGComponent>("SLG");
		entityMgr.RegisterComponentName<ECS::PhotoMakerComponent>("PhotoMaker");

		// Models
		entityMgr.RegisterComponentName<ECS::ModelComponent>("Model");
		entityMgr.RegisterComponentName<ECS::ClipLComponent>("ClipL");
		entityMgr.RegisterComponentName<ECS::ClipGComponent>("ClipG");
		entityMgr.RegisterComponentName<ECS::T5XXLComponent>("T5XXL");
		entityMgr.RegisterComponentName<ECS::DiffusionModelComponent>("DiffusionModel");
		entityMgr.RegisterComponentName<ECS::VaeComponent>("Vae");
		entityMgr.RegisterComponentName<ECS::TaesdComponent>("Taesd");
		entityMgr.RegisterComponentName<ECS::ControlNetComponent>("ControlNet");
		entityMgr.RegisterComponentName<ECS::LoraComponent>("Lora");
		entityMgr.RegisterComponentName<ECS::EmbeddingComponent>("Embedding");

		// Video models
		entityMgr.RegisterComponentName<ECS::HighNoiseDiffusionModelComponent>("HighNoiseDiffusionModel");
		entityMgr.RegisterComponentName<ECS::ClipVisionComponent>("ClipVision");
		entityMgr.RegisterComponentName<ECS::HighNoiseSamplerComponent>("HighNoiseSampler");
		entityMgr.RegisterComponentName<ECS::VideoParamsComponent>("VideoParams");
		entityMgr.RegisterComponentName<ECS::StackedIdEmbedComponent>("StackedIdEmbed");

		// Latents
		entityMgr.RegisterComponentName<ECS::LatentComponent>("Latent");
		entityMgr.RegisterComponentName<ECS::LatentTransformComponent>("LatentTransform");

		// Other
		entityMgr.RegisterComponentName<ECS::LayerSkipComponent>("LayerSkip");
		entityMgr.RegisterComponentName<ECS::ChromaComponent>("Chroma");
		entityMgr.RegisterComponentName<ECS::EsrganComponent>("Esrgan");

		// FIXED: Register system using PluginRegistry (like ExamplePlugin) instead of direct EntityManager
		Plugins::SystemDescriptor systemDesc;
		systemDesc.name = "SDCPPSystem";
		systemDesc.creator = [](ECS::EntityManager* mgr) -> void* {
			return new ECS::SDCPPSystem(*mgr);
		};
		systemDesc.destructor = [](void* system) {
			delete static_cast<ECS::SDCPPSystem*>(system);
		};
		systemDesc.updater = [](void* system, float deltaTime) {
			static_cast<ECS::SDCPPSystem*>(system)->Update(deltaTime);
		};
		systemDesc.requiredComponents = {
			entityMgr.GetComponentTypeIdByName("Prompt"),
			entityMgr.GetComponentTypeIdByName("Sampler"),
			entityMgr.GetComponentTypeIdByName("Guidance"),
			entityMgr.GetComponentTypeIdByName("Model"),
			entityMgr.GetComponentTypeIdByName("DiffusionModel"),
			entityMgr.GetComponentTypeIdByName("Vae"),
			entityMgr.GetComponentTypeIdByName("Latent"),
			entityMgr.GetComponentTypeIdByName("OutputImage")
		};

		m_systemId = registry.RegisterSystem(systemDesc);

		if (m_systemId == 0) {
			LogError("Failed to register SDCPPSystem");
			return false;
		}

		LogInfo("SDCPPSystem registered with ID: " + std::to_string(m_systemId));
		LogInfo("Stable Diffusion Addon initialized");
		return true;
	}

	bool OnStudioInit(ECS::EntityManager& entityMgr, GUI::ViewManager& viewMgr, Plugins::IPluginRegistry& registry) override {
		LogInfo("Registering Stable Diffusion views...");

		// Get the ImGui context from the registry (like ExamplePlugin does)
		ImGuiContext* mainContext = registry.GetImGuiContext();
		std::cout << "[DiffusionAddon] Got ImGui context from registry: " << mainContext << std::endl;
		if (mainContext) {
			ImGui::SetCurrentContext(mainContext);
		}

		RegisterDiffusionViews(registry, mainContext);

		LogInfo("Views registered via PluginRegistry only");
		return true;
	}

	void OnShutdown() override {
		LogInfo("Shutting down");
	}

	void OnUpdate(float deltaTime) override {
		// System updates itself through ECS
	}

private:
	void RegisterDiffusionViews(Plugins::IPluginRegistry& registry, ImGuiContext* mainContext) {
		// Register DiffusionView - SELF-CONTAINED like ExamplePlugin
		Plugins::ViewDescriptor diffusionViewDesc;
		diffusionViewDesc.name = "DiffusionView";
		diffusionViewDesc.category = "Diffusion";
		diffusionViewDesc.factory = [mainContext](ECS::EntityManager& mgr, ImGuiContext* factoryContext) -> std::unique_ptr<GUI::BaseView> {
			try {
				std::cout << "[DiffusionAddon] Creating SELF-CONTAINED DiffusionView" << std::endl;
				// Create view with the main context - it will manage itself
				return std::make_unique<GUI::DiffusionView>(mgr, mainContext);
			}
			catch (const std::exception& e) {
				std::cerr << "[DiffusionAddon] Error creating self-contained DiffusionView: " << e.what() << std::endl;
				return nullptr;
			}
		};

		m_diffusionViewId = registry.RegisterView(diffusionViewDesc);
		if (m_diffusionViewId == GUI::MAX_VIEW_COUNT) {
			LogError("Failed to register DiffusionView");
		}
		else {
			LogInfo("DiffusionView registered with ID: " + std::to_string(m_diffusionViewId));
		}

		// Register ConvertView - SELF-CONTAINED
		Plugins::ViewDescriptor convertViewDesc;
		convertViewDesc.name = "ConvertView";
		convertViewDesc.category = "Tools";
		convertViewDesc.factory = [mainContext](ECS::EntityManager& mgr, ImGuiContext* factoryContext) -> std::unique_ptr<GUI::BaseView> {
			try {
				std::cout << "[DiffusionAddon] Creating SELF-CONTAINED ConvertView" << std::endl;
				return std::make_unique<GUI::ConvertView>(mgr, mainContext);
			}
			catch (const std::exception& e) {
				std::cerr << "[DiffusionAddon] Error creating self-contained ConvertView: " << e.what() << std::endl;
				return nullptr;
			}
		};

		m_convertViewId = registry.RegisterView(convertViewDesc);
		if (m_convertViewId == GUI::MAX_VIEW_COUNT) {
			LogError("Failed to register ConvertView");
		}
		else {
			LogInfo("ConvertView registered with ID: " + std::to_string(m_convertViewId));
		}

		// Register UpscaleView - SELF-CONTAINED
		Plugins::ViewDescriptor upscaleViewDesc;
		upscaleViewDesc.name = "UpscaleView";
		upscaleViewDesc.category = "Tools";
		upscaleViewDesc.factory = [mainContext](ECS::EntityManager& mgr, ImGuiContext* factoryContext) -> std::unique_ptr<GUI::BaseView> {
			try {
				std::cout << "[DiffusionAddon] Creating SELF-CONTAINED UpscaleView" << std::endl;
				return std::make_unique<GUI::UpscaleView>(mgr, mainContext);
			}
			catch (const std::exception& e) {
				std::cerr << "[DiffusionAddon] Error creating self-contained UpscaleView: " << e.what() << std::endl;
				return nullptr;
			}
		};

		m_upscaleViewId = registry.RegisterView(upscaleViewDesc);
		if (m_upscaleViewId == GUI::MAX_VIEW_COUNT) {
			LogError("Failed to register UpscaleView");
		}
		else {
			LogInfo("UpscaleView registered with ID: " + std::to_string(m_upscaleViewId));
		}

		// Register VideoDiffusionView - SELF-CONTAINED
		Plugins::ViewDescriptor videoDiffusionViewDesc;
		videoDiffusionViewDesc.name = "VideoDiffusionView";
		videoDiffusionViewDesc.category = "Diffusion";
		videoDiffusionViewDesc.factory = [mainContext](ECS::EntityManager& mgr, ImGuiContext* factoryContext) -> std::unique_ptr<GUI::BaseView> {
			try {
				std::cout << "[DiffusionAddon] Creating SELF-CONTAINED VideoDiffusionView" << std::endl;
				return std::make_unique<GUI::VideoDiffusionView>(mgr, mainContext);
			}
			catch (const std::exception& e) {
				std::cerr << "[DiffusionAddon] Error creating self-contained VideoDiffusionView: " << e.what() << std::endl;
				return nullptr;
			}
		};

		m_videoDiffusionViewId = registry.RegisterView(videoDiffusionViewDesc);
		if (m_videoDiffusionViewId == GUI::MAX_VIEW_COUNT) {
			LogError("Failed to register VideoDiffusionView");
		}
		else {
			LogInfo("VideoDiffusionView registered with ID: " + std::to_string(m_videoDiffusionViewId));
		}
	}

private:
	ECS::SystemTypeID m_systemId = 0;  // ADDED: Store system ID
	GUI::ViewTypeID m_diffusionViewId = GUI::MAX_VIEW_COUNT;
	GUI::ViewTypeID m_convertViewId = GUI::MAX_VIEW_COUNT;
	GUI::ViewTypeID m_upscaleViewId = GUI::MAX_VIEW_COUNT;
	GUI::ViewTypeID m_videoDiffusionViewId = GUI::MAX_VIEW_COUNT;
};

extern "C" {
#ifdef _WIN32
#define PLUGIN_EXPORT __declspec(dllexport)
#else
#define PLUGIN_EXPORT __attribute__((visibility("default")))
#endif

	PLUGIN_EXPORT Plugins::BasePlugin* CreatePlugin() {
		return new DiffusionAddon();
	}

	PLUGIN_EXPORT void DestroyPlugin(Plugins::BasePlugin* plugin) {
		delete plugin;
	}
}