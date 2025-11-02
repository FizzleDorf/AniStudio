#include "BasePlugin.hpp"
#include "EntityManager.hpp"
#include "ViewManager.hpp"
#include "Events.hpp"

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

		// Use the stored ImGui context like ExamplePlugin does
		std::cout << "[DiffusionAddon] Using stored ImGui context: " << m_imguiContext << std::endl;
		if (m_imguiContext) {
			ImGui::SetCurrentContext(m_imguiContext);
		}

		// Store view manager reference
		m_viewMgr = &viewMgr;

		// Register views directly like ExamplePlugin does
		viewMgr.RegisterView<GUI::DiffusionView>("DiffusionView", "Diffusion");
		viewMgr.RegisterView<GUI::ConvertView>("ConvertView", "Tools");
		viewMgr.RegisterView<GUI::UpscaleView>("UpscaleView", "Tools");
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
		LogInfo("Shutting down DiffusionAddon - cleaning up all resources");

		// STEP 1: Stop all active diffusion tasks
		if (m_entityMgr) {
			auto system = m_entityMgr->GetSystem<ECS::SDCPPSystem>();
			if (system) {
				system->StopCurrentTask();
				system->ClearQueue();
				LogInfo("Stopped all diffusion tasks");
			}
		}

		// STEP 2: Unregister event handlers
		UnregisterEventHandlers();

		// STEP 3: DESTROY ALL ENTITIES WITH DIFFUSION COMPONENTS FIRST
		if (m_entityMgr) {
			// Get all entities and check if they have ANY diffusion component
			auto allEntities = m_entityMgr->GetAllEntities();
			std::vector<EntityID> entitiesToDestroy;

			for (EntityID entity : allEntities) {
				// Check for ANY diffusion component
				if (m_entityMgr->HasComponent<ECS::PromptComponent>(entity) ||
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
					m_entityMgr->HasComponent<ECS::EsrganComponent>(entity)) {
					entitiesToDestroy.push_back(entity);
				}
			}

			// Destroy the fucking entities
			for (EntityID entity : entitiesToDestroy) {
				m_entityMgr->DestroyEntity(entity);
			}
			LogInfo("Destroyed " + std::to_string(entitiesToDestroy.size()) + " entities with diffusion components");
		}

		// STEP 4: Unregister system
		if (m_entityMgr) {
			m_entityMgr->UnregisterSystem<ECS::SDCPPSystem>();
			LogInfo("Unregistered SDCPPSystem");
		}

		// STEP 5: Unregister ALL diffusion views
		if (m_viewMgr) {
			m_viewMgr->UnregisterView("DiffusionView");
			m_viewMgr->UnregisterView("ConvertView");
			m_viewMgr->UnregisterView("UpscaleView");
			m_viewMgr->UnregisterView("VideoDiffusionView");
			LogInfo("Unregistered all diffusion views");
		}

		// STEP 6: UNREGISTER ALL FUCKING COMPONENTS
		if (m_entityMgr) {
			m_entityMgr->UnregisterComponentByName("Prompt");
			m_entityMgr->UnregisterComponentByName("Sampler");
			m_entityMgr->UnregisterComponentByName("Guidance");
			m_entityMgr->UnregisterComponentByName("ClipSkip");
			m_entityMgr->UnregisterComponentByName("SLG");
			m_entityMgr->UnregisterComponentByName("PhotoMaker");
			m_entityMgr->UnregisterComponentByName("Model");
			m_entityMgr->UnregisterComponentByName("ClipL");
			m_entityMgr->UnregisterComponentByName("ClipG");
			m_entityMgr->UnregisterComponentByName("T5XXL");
			m_entityMgr->UnregisterComponentByName("DiffusionModel");
			m_entityMgr->UnregisterComponentByName("Vae");
			m_entityMgr->UnregisterComponentByName("Taesd");
			m_entityMgr->UnregisterComponentByName("ControlNet");
			m_entityMgr->UnregisterComponentByName("Lora");
			m_entityMgr->UnregisterComponentByName("Embedding");
			m_entityMgr->UnregisterComponentByName("HighNoiseDiffusionModel");
			m_entityMgr->UnregisterComponentByName("ClipVision");
			m_entityMgr->UnregisterComponentByName("HighNoiseSampler");
			m_entityMgr->UnregisterComponentByName("VideoParams");
			m_entityMgr->UnregisterComponentByName("StackedIdEmbed");
			m_entityMgr->UnregisterComponentByName("Latent");
			m_entityMgr->UnregisterComponentByName("LatentTransform");
			m_entityMgr->UnregisterComponentByName("LayerSkip");
			m_entityMgr->UnregisterComponentByName("Chroma");
			m_entityMgr->UnregisterComponentByName("Esrgan");
			LogInfo("Unregistered all diffusion components");
		}

		// STEP 7: Clear all references
		m_entityMgr = nullptr;
		m_viewMgr = nullptr;
		m_imguiContext = nullptr;

		LogInfo("DiffusionAddon shutdown complete");
	}

	void OnUpdate(float deltaTime) override {
		// System updates itself through ECS
	}

private:
	void RegisterEventHandlers() {
		// Register event handlers that call into the SDCPPSystem
		ANI::Events::Ref().RegisterEventWithData("QueueDiffusionTask",
			[this](const std::any& data) {
			this->OnQueueDiffusionTask(data);
		});

		ANI::Events::Ref().RegisterEventWithData("RemoveFromDiffusionQueue",
			[this](const std::any& data) {
			this->OnRemoveFromDiffusionQueue(data);
		});

		ANI::Events::Ref().RegisterEventWithData("MoveInDiffusionQueue",
			[this](const std::any& data) {
			this->OnMoveInDiffusionQueue(data);
		});

		ANI::Events::Ref().RegisterEvent("StopCurrentDiffusionTask",
			[this]() {
			this->OnStopCurrentDiffusionTask();
		});

		ANI::Events::Ref().RegisterEvent("ClearDiffusionQueue",
			[this]() {
			this->OnClearDiffusionQueue();
		});

		ANI::Events::Ref().RegisterEvent("PauseDiffusionWorker",
			[this]() {
			this->OnPauseDiffusionWorker();
		});

		ANI::Events::Ref().RegisterEvent("ResumeDiffusionWorker",
			[this]() {
			this->OnResumeDiffusionWorker();
		});

		LogInfo("Diffusion event handlers registered");
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

	void UnregisterEventHandlers() {
		LogInfo("Unregistering diffusion event handlers");

		// Unregister all event handlers that this plugin registered
		auto& events = ANI::Events::Ref();
		events.UnregisterEvent("QueueDiffusionTask");
		events.UnregisterEvent("RemoveFromDiffusionQueue");
		events.UnregisterEvent("MoveInDiffusionQueue");
		events.UnregisterEvent("StopCurrentDiffusionTask");
		events.UnregisterEvent("ClearDiffusionQueue");
		events.UnregisterEvent("PauseDiffusionWorker");
		events.UnregisterEvent("ResumeDiffusionWorker");
	}
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