/*
		d8888          d8b  .d8888b.  888                  888 d8b
	   d88888          Y8P d88P  Y88b 888                  888 Y8P
	  d88P888              Y88b.      888                  888
	 d88P 888 88888b.  888  "Y888b.   888888 888  888  .d88888 888  .d88b.
	d88P  888 888 "88b 888     "Y88b. 888    888  888 d88" 888 888 d88""88b
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
#include <queue>
#include <functional>
#include <unordered_map>
#include <memory>
#include "Types.hpp"
#include "CompList.hpp"
#include "BaseSystem.hpp"
#include "BaseComponent.hpp"

namespace ECS {
	class EntityManager {

	public:
		EntityManager();
		~EntityManager();

		void Update(const float deltaT);
		void Reset();
		const EntityID AddNewEntity();
		void DestroyEntity(const EntityID entity);
		bool IsEntityValid(EntityID entity) const;

		// Template methods that need to be in header
		template<typename T, typename... Args>
		T& AddComponent(const EntityID entity, Args &&...args) {
			assert(entity < MAX_ENTITY_COUNT && "EntityID out of range!");
			assert(GetEntitySignature(entity)->size() < MAX_COMPONENT_COUNT && "Component count limit reached!");

			// Use the component type ID from registry - it must be registered
			const ComponentTypeID compType = CompType<T>();

			// Create the component with forwarded arguments
			T component(std::forward<Args>(args)...);
			component.entityID = entity;

			// Add the component type to the entity's signature
			GetEntitySignature(entity)->insert(compType);

			// Add the component to the component list
			GetCompList<T>()->Insert(component);

			// Update entity in systems
			UpdateEntityTargetSystem(entity);

			return GetCompList<T>()->Get(entity);
		}

		template<typename T>
		void RemoveComponent(const EntityID entity) {
			assert(entity < MAX_ENTITY_COUNT && "EntityID out of range!");
			const ComponentTypeID compType = CompType<T>();

			auto it = entitiesSignatures.find(entity);
			if (it != entitiesSignatures.end()) {
				it->second->erase(compType);
				GetCompList<T>()->Erase(entity);
				UpdateEntityTargetSystem(entity);
			}
		}

		template<typename T>
		T& GetComponent(const EntityID entity) {
			assert(entity < MAX_ENTITY_COUNT && "EntityID out of range!");
			return GetCompList<T>()->Get(entity);
		}

		template<typename T>
		const bool HasComponent(const EntityID entity) {
			assert(entity < MAX_ENTITY_COUNT && "EntityID out of range!");
			// Check if the entity exists in the map
			auto it = entitiesSignatures.find(entity);
			if (it == entitiesSignatures.end()) {
				return false; // Entity signature not found
			}
			const EntitySignature& signature = *(it->second);
			const ComponentTypeID compType = CompType<T>();
			return (signature.count(compType) > 0);
		}

		template <typename T>
		void RegisterSystem() {
			const SystemTypeID systemType = SystemType<T>();
			assert(registeredSystems.count(systemType) == 0 && "System already registered!");
			auto system = std::make_shared<T>(*this); // Pass EntityManager reference

			// Loop through existing entities to add them to the system if needed
			for (const auto& entitySig : entitiesSignatures) {
				AddEntityToSystem(entitySig.first, system.get());
			}

			system->Start();
			registeredSystems[systemType] = std::move(system);
			std::cout << "Registered system: " << typeid(T).name() << " with ID: " << systemType << std::endl;
		}

		template<typename T>
		void UnregisterSystem() {
			const SystemTypeID systemType = SystemType<T>();
			auto it = registeredSystems.find(systemType);
			if (it != registeredSystems.end()) {
				it->second->Destroy();
				registeredSystems.erase(it);
			}
			std::cout << "Unregistered system: " << typeid(T).name() << " with ID: " << systemType << std::endl;
		}

		template <typename T>
		std::shared_ptr<T> GetSystem() {
			const SystemTypeID systemType = SystemType<T>();
			auto it = registeredSystems.find(systemType);
			if (it != registeredSystems.end()) {
				return std::static_pointer_cast<T>(it->second);
			}
			return nullptr;
		}

		template <typename T>
		void RegisterComponentName(const std::string& name) {
			// Register with the component registry
			ComponentTypeID typeId = ComponentTypeRegistry::RegisterType<T>(name);

			// Register the functionality for this component type
			RegisterComponentType(
				typeId,
				[this](EntityID entity) { this->AddComponent<T>(entity); },
				[this](EntityID entity) -> BaseComponent* {
				if (this->HasComponent<T>(entity)) {
					return &this->GetComponent<T>(entity);
				}
				return nullptr;
			}
			);

			std::cout << "Registered component: " << name << " with ID: " << typeId << std::endl;
		}

		template<typename T>
		void AddCompList() {
			const ComponentTypeID compType = CompType<T>();
			assert(componentsArrays.find(compType) == componentsArrays.end() && "CompList already registered!");
			componentsArrays[compType] = std::move(std::make_shared<CompList<T>>());
		}

		template<typename T>
		std::shared_ptr<CompList<T>> GetCompList() {
			const ComponentTypeID compType = CompType<T>();
			if (componentsArrays.count(compType) == 0) { AddCompList<T>(); }
			return std::static_pointer_cast<CompList<T>>(componentsArrays.at(compType));
		}

		// Non-template public methods
		void RemoveComponentById(EntityID entityID, ComponentTypeID componentId);
		bool HasComponentById(const EntityID entity, ComponentTypeID componentId);
		std::vector<EntityID> GetAllEntities() const;
		std::vector<ComponentTypeID> GetEntityComponents(EntityID entity) const;

		// Component registration by name
		ComponentTypeID GetComponentTypeIdByName(const std::string& name) const;
		std::string GetComponentNameById(ComponentTypeID typeId) const;
		std::vector<std::string> GetAllRegisteredComponentNames() const;
		bool IsComponentNameRegistered(const std::string& name) const;

		// Serialization
		nlohmann::json SerializeEntity(const EntityID entity) const;
		EntityID CloneEntity(const EntityID sourceEntity);
		EntityID DeserializeEntity(const nlohmann::json& json);
		void DeserializeEntity(const nlohmann::json& json, const EntityID entity);

		// Plugin support for component registration
		using ComponentCreator = std::function<void(EntityID)>;
		using ComponentGetter = std::function<BaseComponent* (EntityID)>;
		void RegisterComponentType(ComponentTypeID typeId, ComponentCreator creator, ComponentGetter getter);
		void UnregisterPluginSystem(SystemTypeID systemId);
		void UnregisterPluginComponent(ComponentTypeID componentId);

		// NEW: Plugin component support - hash-based registration
		void RegisterPluginComponent(ComponentTypeID typeId,
			size_t componentSize,
			std::function<void(void*, EntityID)> constructor,
			std::function<void(void*)> destructor);

		void* GetPluginComponent(EntityID entity, ComponentTypeID typeId);
		void* AddPluginComponent(EntityID entity, ComponentTypeID typeId);
		void RemovePluginComponent(EntityID entity, ComponentTypeID typeId);
		bool HasPluginComponent(EntityID entity, ComponentTypeID typeId);

		// NEW: Plugin system support
		void RegisterPluginSystem(SystemTypeID typeId,
			std::function<void*(EntityManager*)> creator,
			std::function<void(void*)> destructor,
			std::function<void(void*, float)> updater,
			std::function<void(void*)> starter,
			const std::vector<ComponentTypeID>& requiredComponents);

		void* GetPluginSystem(SystemTypeID typeId);
		void UpdatePluginSystems(float deltaTime);

		// Make entity signature access public for plugins
		std::shared_ptr<EntitySignature> GetEntitySignature(const EntityID entity);

		// Required for PluginInterface.cpp to access these methods
		BaseComponent* GetComponentById(EntityID entity, ComponentTypeID typeId);
		const BaseComponent* GetComponentByIdConst(EntityID entity, ComponentTypeID typeId) const;
		bool IsPluginComponent(ComponentTypeID typeId);

		// Getters for private variables
		EntityID GetEntityCount() const;
		std::queue<EntityID> GetAvailableEntities() const;
		const std::map<EntityID, std::shared_ptr<EntitySignature>>& GetEntitiesSignatures() const;
		const std::map<SystemTypeID, std::shared_ptr<BaseSystem>>& GetRegisteredSystems() const;
		const std::map<ComponentTypeID, std::shared_ptr<ICompList>>& GetComponentsArrays() const;

		// Debug functions
		void DebugPrintRegisteredComponents() const;
		void DebugPrintEntityComponents(EntityID entity) const;
		void DebugPrintPluginSystems() const;

	private:
		// Private helper methods
		void AddEntitySignature(const EntityID entity);
		void UpdateEntityTargetSystem(const EntityID entity);
		void AddEntityToSystem(const EntityID entity, BaseSystem* system);
		bool IsEntityInSystem(const EntityID entity, const EntitySignature& system_signature);
		void CopyComponentResources(EntityID sourceEntity, EntityID destEntity, ComponentTypeID componentId);

		// Private member variables
		EntityID entityCount;
		std::queue<EntityID> availableEntities;
		std::map<EntityID, std::shared_ptr<EntitySignature>> entitiesSignatures;
		std::map<SystemTypeID, std::shared_ptr<BaseSystem>> registeredSystems;
		std::map<ComponentTypeID, std::shared_ptr<ICompList>> componentsArrays;
		std::unordered_map<ComponentTypeID, ComponentCreator> componentCreators;
		std::unordered_map<ComponentTypeID, ComponentGetter> componentGetters;

		// NEW: Plugin system storage
		struct PluginSystemInfo {
			void* instance;
			std::function<void(void*)> destructor;
			std::function<void(void*, float)> updater;
			std::vector<ComponentTypeID> requiredComponents;
			std::set<EntityID> entities;
		};
		std::map<SystemTypeID, PluginSystemInfo> pluginSystems;
	};
}