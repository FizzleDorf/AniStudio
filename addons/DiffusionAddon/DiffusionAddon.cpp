#include "BasePlugin.hpp"
#include "EntityManager.hpp"
#include "ViewManager.hpp"
#include "Events.hpp"
#include "DiffusionCallbackUtils.hpp"

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
	DiffusionAddon() : BasePlugin("DiffusionAddon", "1.0.0"), m_imguiContext(nullptr) {}

	bool OnEngineInit(ECS::EntityManager& entityMgr) override {
		LogInfo("Initializing Stable Diffusion Addon...");

		// Register components directly with EntityManager using templates
		entityMgr.RegisterComponent<ECS::PromptComponent>("Prompt");
		entityMgr.RegisterComponent<ECS::SamplerComponent>("Sampler");
		entityMgr.RegisterComponent<ECS::GuidanceComponent>("Guidance");
		entityMgr.RegisterComponent<ECS::ClipSkipComponent>("ClipSkip");
		entityMgr.RegisterComponent<ECS::SLGComponent>("SLG");
		entityMgr.RegisterComponent<ECS::PhotoMakerComponent>("PhotoMaker");

		// Models
		entityMgr.RegisterComponent<ECS::ModelComponent>("Model");
		entityMgr.RegisterComponent<ECS::ClipLComponent>("ClipL");
		entityMgr.RegisterComponent<ECS::ClipGComponent>("ClipG");
		entityMgr.RegisterComponent<ECS::T5XXLComponent>("T5XXL");
		entityMgr.RegisterComponent<ECS::DiffusionModelComponent>("DiffusionModel");
		entityMgr.RegisterComponent<ECS::VaeComponent>("Vae");
		entityMgr.RegisterComponent<ECS::TaesdComponent>("Taesd");
		entityMgr.RegisterComponent<ECS::ControlNetComponent>("ControlNet");
		entityMgr.RegisterComponent<ECS::LoraComponent>("Lora");
		entityMgr.RegisterComponent<ECS::EmbeddingComponent>("Embedding");

		// Video models
		entityMgr.RegisterComponent<ECS::HighNoiseDiffusionModelComponent>("HighNoiseDiffusionModel");
		entityMgr.RegisterComponent<ECS::ClipVisionComponent>("ClipVision");
		entityMgr.RegisterComponent<ECS::HighNoiseSamplerComponent>("HighNoiseSampler");
		entityMgr.RegisterComponent<ECS::VideoParamsComponent>("VideoParams");
		entityMgr.RegisterComponent<ECS::StackedIdEmbedComponent>("StackedIdEmbed");

		// Latents
		entityMgr.RegisterComponent<ECS::LatentComponent>("Latent");
		entityMgr.RegisterComponent<ECS::LatentTransformComponent>("LatentTransform");

		// Other
		entityMgr.RegisterComponent<ECS::LayerSkipComponent>("LayerSkip");
		entityMgr.RegisterComponent<ECS::ChromaComponent>("Chroma");
		entityMgr.RegisterComponent<ECS::EsrganComponent>("Esrgan");

		entityMgr.RegisterSystem<ECS::SDCPPSystem>();

		// Store the entity manager reference for event handling
		m_entityMgr = &entityMgr;

		// Register event handlers for diffusion tasks
		RegisterEventHandlers();

		LogInfo("SDCPPSystem registered with ID: " + std::to_string(m_systemId));
		LogInfo("Stable Diffusion Addon initialized");
		return true;
	}

	bool OnStudioInit(ECS::EntityManager& entityMgr, GUI::ViewManager& viewMgr) override {
		LogInfo("Registering Stable Diffusion views...");

		std::cout << "[DiffusionAddon] Using stored ImGui context: " << m_imguiContext << std::endl;
		if (m_imguiContext) {
			ImGui::SetCurrentContext(m_imguiContext);
		}

		m_viewMgr = &viewMgr;

		viewMgr.RegisterView<GUI::DiffusionView>("DiffusionView", "Diffusion");
		viewMgr.RegisterView<GUI::ConvertView>("ConvertView", "Diffusion");
		viewMgr.RegisterView<GUI::UpscaleView>("UpscaleView", "Diffusion");
		viewMgr.RegisterView<GUI::VideoDiffusionView>("VideoDiffusionView", "Diffusion");

		LogInfo("Views registered via direct ViewManager");
		return true;
	}

	void SetImGuiContext(ImGuiContext* context) override {
		m_imguiContext = context;
		LogInfo("ImGui context set for DiffusionAddon: " + std::to_string(reinterpret_cast<uintptr_t>(m_imguiContext)));
	}

	bool HasValidImGuiContext() const override {
		return m_imguiContext != nullptr;
	}

	void DiffusionAddon::OnShutdown() {
		LogInfo("SHUTTING DOWN DIFFUSION ADDON IMMEDIATELY");

		if (m_entityMgr) {
			auto system = m_entityMgr->GetSystem<ECS::SDCPPSystem>();
			if (system) {
				system->TerminateImmediately();
				LogInfo("SDCPPSystem terminated immediately");
			}
		}

		UnregisterEventHandlers();

		if (m_entityMgr) {
			auto allEntities = m_entityMgr->GetAllEntities();
			std::vector<EntityID> entitiesToDestroy;

			for (EntityID entity : allEntities) {
				bool hasDiffusionComponent = false;

				// Try multiple ways to detect diffusion components
				try {
					hasDiffusionComponent =
						m_entityMgr->HasComponent<ECS::PromptComponent>(entity) ||
						m_entityMgr->HasComponent<ECS::SamplerComponent>(entity) ||
						m_entityMgr->HasComponent<ECS::GuidanceComponent>(entity) ||
						m_entityMgr->HasComponent<ECS::ClipSkipComponent>(entity) ||
						m_entityMgr->HasComponent<ECS::SLGComponent>(entity) ||
						m_entityMgr->HasComponent<ECS::PhotoMakerComponent>(entity) ||
						m_entityMgr->HasComponent<ECS::ModelComponent>(entity) ||
						m_entityMgr->HasComponent<ECS::ClipLComponent>(entity) ||
						m_entityMgr->HasComponent<ECS::ClipGComponent>(entity) ||
						m_entityMgr->HasComponent<ECS::T5XXLComponent>(entity) ||
						m_entityMgr->HasComponent<ECS::DiffusionModelComponent>(entity) ||
						m_entityMgr->HasComponent<ECS::VaeComponent>(entity) ||
						m_entityMgr->HasComponent<ECS::TaesdComponent>(entity) ||
						m_entityMgr->HasComponent<ECS::ControlNetComponent>(entity) ||
						m_entityMgr->HasComponent<ECS::LoraComponent>(entity) ||
						m_entityMgr->HasComponent<ECS::EmbeddingComponent>(entity) ||
						m_entityMgr->HasComponent<ECS::HighNoiseDiffusionModelComponent>(entity) ||
						m_entityMgr->HasComponent<ECS::ClipVisionComponent>(entity) ||
						m_entityMgr->HasComponent<ECS::HighNoiseSamplerComponent>(entity) ||
						m_entityMgr->HasComponent<ECS::VideoParamsComponent>(entity) ||
						m_entityMgr->HasComponent<ECS::StackedIdEmbedComponent>(entity) ||
						m_entityMgr->HasComponent<ECS::LatentComponent>(entity) ||
						m_entityMgr->HasComponent<ECS::LatentTransformComponent>(entity) ||
						m_entityMgr->HasComponent<ECS::LayerSkipComponent>(entity) ||
						m_entityMgr->HasComponent<ECS::ChromaComponent>(entity) ||
						m_entityMgr->HasComponent<ECS::EsrganComponent>(entity);
				}
				catch (const std::exception& e) {
					LogError("Error checking components for entity " + std::to_string(entity) + ": " + e.what());
				}

				if (hasDiffusionComponent) {
					entitiesToDestroy.push_back(entity);
				}
			}

			LogInfo("Found " + std::to_string(entitiesToDestroy.size()) + " entities to destroy");

			for (EntityID entity : entitiesToDestroy) {
				try {
					m_entityMgr->DestroyEntity(entity);
					LogInfo("Destroyed entity: " + std::to_string(entity));
				}
				catch (const std::exception& e) {
					LogError("Error destroying entity " + std::to_string(entity) + ": " + e.what());
				}
			}
		}

		// STEP 4: Unregister components and system FIRST (before views to avoid deadlock)
		if (m_entityMgr) {
			try {
				// Unregister all diffusion components by name
				const char* componentNames[] = {
					"Prompt", "Sampler", "Guidance", "ClipSkip", "SLG", "PhotoMaker",
					"Model", "ClipL", "ClipG", "T5XXL", "DiffusionModel", "Vae",
					"Taesd", "ControlNet", "Lora", "Embedding", "HighNoiseDiffusionModel",
					"ClipVision", "HighNoiseSampler", "VideoParams", "StackedIdEmbed",
					"Latent", "LatentTransform", "LayerSkip", "Chroma", "Esrgan"
				};

				for (const char* name : componentNames) {
					try {
						m_entityMgr->UnregisterComponentByName(name);
						LogInfo("Unregistered component: " + std::string(name));
					}
					catch (const std::exception& e) {
						LogError("Error unregistering component " + std::string(name) + ": " + e.what());
					}
					catch (...) {
						// Component might not be registered, continue
						LogInfo("Component not registered (skipping): " + std::string(name));
					}
				}

				LogInfo("Unregistered all diffusion components");

				// Unregister system
				try {
					m_entityMgr->UnregisterSystem<ECS::SDCPPSystem>();
					LogInfo("Unregistered SDCPPSystem");
				}
				catch (const std::exception& e) {
					LogError("Error unregistering SDCPPSystem: " + std::string(e.what()));
				}

			}
			catch (const std::exception& e) {
				LogError("Error during component/system cleanup: " + std::string(e.what()));
			}
		}

		if (m_viewMgr) {
			const char* viewNames[] = {
				"DiffusionView", "ConvertView", "UpscaleView", "VideoDiffusionView"
			};

			for (const char* viewName : viewNames) {
				try {
					std::this_thread::sleep_for(std::chrono::milliseconds(50));
					LogInfo("Attempting to unregister view: " + std::string(viewName));
					m_viewMgr->UnregisterView(viewName);
					LogInfo("Successfully unregistered view: " + std::string(viewName));
				}
				catch (const std::exception& e) {
					LogError("Error unregistering view " + std::string(viewName) + ": " + e.what());
					// Continue with other views even if one fails
				}
				catch (...) {
					LogError("Unknown error unregistering view: " + std::string(viewName));
				}
			}

			LogInfo("View unregistration complete");
		}

		// STEP 6: Clear all references
		m_entityMgr = nullptr;
		m_viewMgr = nullptr;
		m_imguiContext = nullptr;

		LogInfo("DiffusionAddon shutdown complete");
	}

	void OnUpdate(float deltaTime) override {}

private:
	std::vector<std::string> m_registeredEvents;

	void RegisterEventHandlers() {
		auto& events = ANI::Events::Ref();

		// Store event names for cleanup
		m_registeredEvents = {
			"QueueDiffusionTask",
			"RemoveFromDiffusionQueue",
			"MoveInDiffusionQueue",
			"StopCurrentDiffusionTask",
			"ClearDiffusionQueue",
			"PauseDiffusionWorker",
			"ResumeDiffusionWorker"
		};

		// Register all events with their handlers
		events.RegisterEventWithData("QueueDiffusionTask",
			[this](const std::any& data) {
			this->OnQueueDiffusionTask(data);
		});

		events.RegisterEventWithData("RemoveFromDiffusionQueue",
			[this](const std::any& data) {
			this->OnRemoveFromDiffusionQueue(data);
		});

		events.RegisterEventWithData("MoveInDiffusionQueue",
			[this](const std::any& data) {
			this->OnMoveInDiffusionQueue(data);
		});

		events.RegisterEvent("StopCurrentDiffusionTask",
			[this]() {
			this->OnStopCurrentDiffusionTask();
		});

		events.RegisterEvent("ClearDiffusionQueue",
			[this]() {
			this->OnClearDiffusionQueue();
		});

		events.RegisterEvent("PauseDiffusionWorker",
			[this]() {
			this->OnPauseDiffusionWorker();
		});

		events.RegisterEvent("ResumeDiffusionWorker",
			[this]() {
			this->OnResumeDiffusionWorker();
		});

		LogInfo("Diffusion event handlers registered");
	}

	void UnregisterEventHandlers() {
		LogInfo("Unregistering diffusion event handlers");
		auto& events = ANI::Events::Ref();

		for (const auto& eventName : m_registeredEvents) {
			try {
				events.UnregisterEvent(eventName);
				LogInfo("Unregistered event: " + eventName);
			}
			catch (const std::exception& e) {
				LogError("Failed to unregister event " + eventName + ": " + e.what());
			}
		}
		m_registeredEvents.clear();
	}

	void OnQueueDiffusionTask(const std::any& data) {
		try {
			auto taskData = std::any_cast<std::pair<ECS::EntityID, ECS::SDCPPSystem::TaskType>>(data);
			auto entityID = taskData.first;
			auto taskType = taskData.second;

			auto system = m_entityMgr->GetSystem<ECS::SDCPPSystem>();
			if (system) {
				system->QueueTask(entityID, taskType);
				LogInfo("Queued diffusion task for entity " + std::to_string(entityID));
			}
			else {
				LogError("SDCPPSystem not found when queuing task");
			}
		}
		catch (const std::bad_any_cast& e) {
			LogError("Invalid data type for QueueDiffusionTask event: " + std::string(e.what()));
		}
	}

	void OnRemoveFromDiffusionQueue(const std::any& data) {
		try {
			auto index = std::any_cast<size_t>(data);

			auto system = m_entityMgr->GetSystem<ECS::SDCPPSystem>();
			if (system) {
				system->RemoveFromQueue(index);
				LogInfo("Removed item from diffusion queue at index " + std::to_string(index));
			}
			else {
				LogError("SDCPPSystem not found when removing from queue");
			}
		}
		catch (const std::bad_any_cast& e) {
			LogError("Invalid data type for RemoveFromDiffusionQueue event: " + std::string(e.what()));
		}
	}

	void OnMoveInDiffusionQueue(const std::any& data) {
		try {
			auto indices = std::any_cast<std::pair<size_t, size_t>>(data);
			auto fromIndex = indices.first;
			auto toIndex = indices.second;

			auto system = m_entityMgr->GetSystem<ECS::SDCPPSystem>();
			if (system) {
				system->MoveInQueue(fromIndex, toIndex);
				LogInfo("Moved item in diffusion queue from " + std::to_string(fromIndex) + " to " + std::to_string(toIndex));
			}
			else {
				LogError("SDCPPSystem not found when moving in queue");
			}
		}
		catch (const std::bad_any_cast& e) {
			LogError("Invalid data type for MoveInDiffusionQueue event: " + std::string(e.what()));
		}
	}

	void OnStopCurrentDiffusionTask() {
		auto system = m_entityMgr->GetSystem<ECS::SDCPPSystem>();
		if (system) {
			system->StopCurrentTask();
			LogInfo("Stopped current diffusion task");
		}
		else {
			LogError("SDCPPSystem not found when stopping task");
		}
	}

	void OnClearDiffusionQueue() {
		auto system = m_entityMgr->GetSystem<ECS::SDCPPSystem>();
		if (system) {
			system->ClearQueue();
			LogInfo("Cleared diffusion queue");
		}
		else {
			LogError("SDCPPSystem not found when clearing queue");
		}
	}

	void OnPauseDiffusionWorker() {
		auto system = m_entityMgr->GetSystem<ECS::SDCPPSystem>();
		if (system) {
			system->PauseWorker();
			LogInfo("Paused diffusion worker");
		}
		else {
			LogError("SDCPPSystem not found when pausing worker");
		}
	}

	void OnResumeDiffusionWorker() {
		auto system = m_entityMgr->GetSystem<ECS::SDCPPSystem>();
		if (system) {
			system->ResumeWorker();
			LogInfo("Resumed diffusion worker");
		}
		else {
			LogError("SDCPPSystem not found when resuming worker");
		}
	}

private:
	ECS::SystemTypeID m_systemId = 0;
	ECS::EntityManager* m_entityMgr = nullptr;
	GUI::ViewManager* m_viewMgr = nullptr;
	ImGuiContext* m_imguiContext = nullptr;
};

extern "C" {
	// Remove the PLUGIN_EXPORT macro definition since it's already defined in BasePlugin.hpp
	__declspec(dllexport) Plugins::BasePlugin* CreatePlugin() {
		return new DiffusionAddon();
	}

	__declspec(dllexport) void DestroyPlugin(Plugins::BasePlugin* plugin) {
		delete plugin;
	}
}