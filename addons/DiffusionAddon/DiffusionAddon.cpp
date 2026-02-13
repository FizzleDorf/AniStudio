#include "BasePlugin.hpp"
#include "EntityManager.hpp"
#include "ViewManager.hpp"
#include "Events.hpp"
#include "DiffusionCallbackUtils.hpp"
#include "FilePathService.hpp"
#include "EngineContext.hpp"
#include "StudioContext.hpp"
#include "ProjectManager.hpp"
#include "WindowState.hpp"

#include "SDCPPComponents.h"
#include "SDcppSystem.hpp"

#include "DiffusionView.hpp"
#include "ConvertView.hpp"
#include "UpscaleView.hpp"
#include "VideoDiffusionView.hpp"
#include "ModelCacheView.hpp"

#include <iostream>

// Platform-specific export macro for plugins
#if defined(_WIN32) || defined(_WIN64)
    #define PLUGIN_EXPORT __declspec(dllexport)
#else
    #define PLUGIN_EXPORT __attribute__((visibility("default")))
#endif

class DiffusionAddon : public Plugins::BasePlugin {
public:
	DiffusionAddon() : BasePlugin("DiffusionAddon", "1.0.0"), m_imguiContext(nullptr) {}

	bool OnEngineInit(ECS::EntityManager& entityMgr) override {
		LogInfo("Initializing Stable Diffusion Addon...");

		auto context = GetEngineContext();

		if (!context) {
			auto studioCtx = GetStudioContext();
			if (studioCtx) {
				context = std::static_pointer_cast<ANI::EngineContext>(studioCtx);
				LogInfo("Got EngineContext from StudioContext fallback");
			}
		}

		if (!context) {
			LogError("Engine context is null! Plugin cannot initialize.");
			return false;
		}

		LogInfo("Engine context obtained successfully");

		if (Utils::FilePathService::IsInitialized()) {
			LogInfo("FilePathService already initialized");
		}
		else {
			if (context->filePaths) {
				Utils::FilePathService::SetInstance(context->filePaths);
				LogInfo("FilePathService set with context FilePaths");
			}
			else {
				LogError("No FilePaths in context to set in FilePathService!");
				return false;
			}
		}

		std::string modelRoot = Utils::FilePathService::GetPath("ModelRoot");
		if (!modelRoot.empty()) {
			LogInfo("Model root path: " + modelRoot);
		}
		else {
			LogError("Model root path is empty!");
			return false;
		}

		// Base SDCPP Components
		entityMgr.RegisterComponent<ECS::PromptComponent>("Prompt");
		entityMgr.RegisterComponent<ECS::SamplerComponent>("Sampler");
		entityMgr.RegisterComponent<ECS::GuidanceComponent>("Guidance");
		entityMgr.RegisterComponent<ECS::LatentComponent>("Latent");
		entityMgr.RegisterComponent<ECS::CheckpointComponent>("Checkpoint");
		entityMgr.RegisterComponent<ECS::ClipLComponent>("ClipL");
		entityMgr.RegisterComponent<ECS::ClipGComponent>("ClipG");
		entityMgr.RegisterComponent<ECS::T5XXLComponent>("T5XXL");
		entityMgr.RegisterComponent<ECS::ClipVisionComponent>("ClipVision");
		entityMgr.RegisterComponent<ECS::LlmEncoderComponent>("LlmEncoder");
		entityMgr.RegisterComponent<ECS::LlmVisionComponent>("LlmVision");
		entityMgr.RegisterComponent<ECS::DiffusionModelComponent>("DiffusionModel");
		entityMgr.RegisterComponent<ECS::VaeComponent>("Vae");
		entityMgr.RegisterComponent<ECS::TaesdComponent>("Taesd");

		// Other settings
		entityMgr.RegisterComponent<ECS::ClipSkipComponent>("ClipSkip");
		entityMgr.RegisterComponent<ECS::SLGComponent>("SLG");
		entityMgr.RegisterComponent<ECS::PhotoMakerComponent>("PhotoMaker");
		entityMgr.RegisterComponent<ECS::HighNoiseDiffusionModelComponent>("HighNoiseDiffusionModel");
		entityMgr.RegisterComponent<ECS::HighNoiseSamplerComponent>("HighNoiseSampler");
		entityMgr.RegisterComponent<ECS::VideoParamsComponent>("VideoParams");

		// Advanced
		entityMgr.RegisterComponent<ECS::ControlNetComponent>("ControlNet");
		entityMgr.RegisterComponent<ECS::LoraComponent>("Lora");
		entityMgr.RegisterComponent<ECS::EmbeddingComponent>("Embedding");
		entityMgr.RegisterComponent<ECS::StackedIdEmbedComponent>("StackedIdEmbed");
		entityMgr.RegisterComponent<ECS::LatentTransformComponent>("LatentTransform");
		entityMgr.RegisterComponent<ECS::LayerSkipComponent>("LayerSkip");
		entityMgr.RegisterComponent<ECS::ChromaComponent>("Chroma");
		entityMgr.RegisterComponent<ECS::EsrganComponent>("Esrgan");
		entityMgr.RegisterComponent<ECS::ConversionComponent>("Conversion");

		entityMgr.RegisterSystem<ECS::SDCPPSystem>();

		m_entityMgr = &entityMgr;

		RegisterEventHandlers();

		LogInfo("SDCPPSystem registered");
		LogInfo("Stable Diffusion Addon initialized");
		return true;
	}

	bool OnStudioInit(ECS::EntityManager& entityMgr, GUI::ViewManager& viewMgr) override {
		LogInfo("Registering Stable Diffusion views...");

		if (!m_imguiContext) {
			LogError("ImGui context not set! Make sure SetImGuiContext was called.");
			return false;
		}

		ImGui::SetCurrentContext(m_imguiContext);
		LogInfo("ImGui context set for DiffusionAddon");

		std::cout << "[DiffusionAddon] Using stored ImGui context: " << m_imguiContext << std::endl;

		m_viewMgr = &viewMgr;

		viewMgr.RegisterView<GUI::DiffusionView>("DiffusionView", "Diffusion");
		viewMgr.RegisterView<GUI::ConvertView>("ConvertView", "Diffusion");
		viewMgr.RegisterView<GUI::UpscaleView>("UpscaleView", "Diffusion");
		viewMgr.RegisterView<GUI::VideoDiffusionView>("VideoDiffusionView", "Diffusion");
		viewMgr.RegisterView<GUI::ModelCacheView>("ModelCacheView", "Diffusion");

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

	void OnShutdown() {
		LogInfo("SHUTTING DOWN DIFFUSION ADDON");

		UnregisterEventHandlers();

		if (m_entityMgr) {
			auto system = m_entityMgr->GetSystem<ECS::SDCPPSystem>();
			if (system) {
				system->TerminateImmediately();
				LogInfo("SDCPPSystem terminated");

				std::this_thread::sleep_for(std::chrono::milliseconds(200));

				try {
					m_entityMgr->UnregisterSystem<ECS::SDCPPSystem>();
					LogInfo("Unregistered SDCPPSystem");
				}
				catch (const std::exception& e) {
					LogError("Error unregistering SDCPPSystem: " + std::string(e.what()));
				}
			}
		}

		if (m_viewMgr) {
			const char* viewNames[] = {
				"DiffusionView", "ConvertView", "UpscaleView", "VideoDiffusionView"
			};

			for (const char* viewName : viewNames) {
				try {
					m_viewMgr->CloseAllViewsOfType(viewName);
					LogInfo("Closed all views of type: " + std::string(viewName));
					std::this_thread::sleep_for(std::chrono::milliseconds(50));
				}
				catch (...) {
				}
			}
		}

		if (m_entityMgr) {
			auto allEntities = m_entityMgr->GetAllEntities();
			std::vector<EntityID> entitiesToDestroy;

			for (EntityID entity : allEntities) {
				if (CheckEntityForDiffusionComponents(entity)) {
					entitiesToDestroy.push_back(entity);
				}
			}

			LogInfo("Found " + std::to_string(entitiesToDestroy.size()) +
				" entities to destroy out of " + std::to_string(allEntities.size()));

			for (auto it = entitiesToDestroy.rbegin(); it != entitiesToDestroy.rend(); ++it) {
				try {
					m_entityMgr->DestroyEntity(*it);
					LogInfo("Destroyed entity: " + std::to_string(*it));
				}
				catch (const std::exception& e) {
					LogError("Error destroying entity " + std::to_string(*it) + ": " + std::string(e.what()));
				}
			}
		}

		if (m_viewMgr) {
			const char* viewNames[] = {
				"DiffusionView", "ConvertView", "UpscaleView", "VideoDiffusionView"
			};

			for (const char* viewName : viewNames) {
				try {
					m_viewMgr->UnregisterView(viewName);
					LogInfo("Unregistered view: " + std::string(viewName));
					std::this_thread::sleep_for(std::chrono::milliseconds(50));
				}
				catch (const std::exception& e) {
					LogError("Error unregistering view " + std::string(viewName) + ": " + std::string(e.what()));
				}
			}
		}

		m_entityMgr = nullptr;
		m_viewMgr = nullptr;
		m_imguiContext = nullptr;

		LogInfo("DiffusionAddon shutdown complete");
	}

	bool CheckEntityForDiffusionComponents(EntityID entity) {
		if (!m_entityMgr) return false;

		try {
			if (m_entityMgr->HasComponent<ECS::PromptComponent>(entity)) return true;
			if (m_entityMgr->HasComponent<ECS::CheckpointComponent>(entity)) return true;
			if (m_entityMgr->HasComponent<ECS::DiffusionModelComponent>(entity)) return true;
			if (m_entityMgr->HasComponent<ECS::LatentComponent>(entity)) return true;
			return false;
		}
		catch (...) {
			return false;
		}
	}

	void OnUpdate(float deltaTime) override {}

private:
	std::vector<std::string> m_registeredEvents;

	void RegisterEventHandlers() {
		auto& events = ANI::Events::Ref();

		m_registeredEvents = {
			"QueueDiffusionTask",
			"RemoveFromDiffusionQueue",
			"MoveInDiffusionQueue",
			"StopCurrentDiffusionTask",
			"ClearDiffusionQueue",
			"PauseDiffusionWorker",
			"ResumeDiffusionWorker"
		};

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
	PLUGIN_EXPORT Plugins::BasePlugin* CreatePlugin() {
		return new DiffusionAddon();
	}

	PLUGIN_EXPORT void DestroyPlugin(Plugins::BasePlugin* plugin) {
		delete plugin;
	}
}