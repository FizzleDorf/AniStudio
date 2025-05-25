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
#include "pch.h"
#include "Types.hpp"
#include "CompList.hpp"
#include "BaseSystem.hpp"
#include "BaseComponent.hpp"

namespace ECS {
	class EntityManager {

	public:
		EntityManager() : entityCount(0) {
			Reset();
		}

		~EntityManager() {}

		void Update(const float deltaT) {
			for (auto& system : registeredSystems) {
				system.second->Update(deltaT);
			}
		}

		const EntityID AddNewEntity() {
			const EntityID entity = availableEntities.front();
			AddEntitySignature(entity);
			availableEntities.pop();
			entityCount++;
			return entity;
		}

		void DestroyEntity(const EntityID entity) {
			assert(entity < MAX_ENTITY_COUNT && "EntityID out of range!");

			// If entity doesn't exist in signatures, just return
			if (entitiesSignatures.find(entity) == entitiesSignatures.end()) {
				return;
			}

			entitiesSignatures.erase(entity);

			for (auto& array : componentsArrays) {
				array.second->Erase(entity);
			}

			for (auto& system : registeredSystems) {
				system.second->RemoveEntity(entity);
			}

			entityCount--;
			availableEntities.push(entity);
			std::cout << "Removed Entity: " << entity << "\n";
		}

		bool IsEntityValid(EntityID entity) const {
			return entity < MAX_ENTITY_COUNT &&
				entitiesSignatures.find(entity) != entitiesSignatures.end();
		}

		// Add component by type - use existing ID if registered
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

		void RemoveComponentById(EntityID entityID, ComponentTypeID componentId) {
			auto it = entitiesSignatures.find(entityID);
			if (it != entitiesSignatures.end()) {
				it->second->erase(componentId);

				auto arrayIt = componentsArrays.find(componentId);
				if (arrayIt != componentsArrays.end()) {
					arrayIt->second->Erase(entityID);
				}

				UpdateEntityTargetSystem(entityID);
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

		bool HasComponentById(const EntityID entity, ComponentTypeID componentId) {
			assert(entity < MAX_ENTITY_COUNT && "EntityID out of range!");
			// Check if the entity exists in the map
			auto it = entitiesSignatures.find(entity);
			if (it == entitiesSignatures.end()) {
				return false; // Entity signature not found
			}
			const EntitySignature& signature = *(it->second);
			return (signature.count(componentId) > 0);
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
		}

		template<typename T>
		void UnregisterSystem() {
			const SystemTypeID systemType = SystemType<T>();
			auto it = registeredSystems.find(systemType);
			if (it != registeredSystems.end()) {
				registeredSystems.erase(it);
			}
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

		void Reset() {
			// Reset the component type registry first
			ComponentTypeRegistry::Reset();

			// Clear all entity signatures
			entitiesSignatures.clear();

			// Clear all registered systems
			registeredSystems.clear();

			// Clear all component arrays
			componentsArrays.clear();

			// Clear component mappings
			componentCreators.clear();
			componentGetters.clear();

			// Reset entity queue
			while (!availableEntities.empty()) {
				availableEntities.pop();
			}
			for (EntityID entity = 0u; entity < MAX_ENTITY_COUNT; ++entity) {
				availableEntities.push(entity);
			}

			// Reset entity count
			entityCount = 0;
		}

		std::vector<EntityID> GetAllEntities() const {
			std::vector<EntityID> entities;
			for (const auto& pair : entitiesSignatures) {
				entities.push_back(pair.first);
			}
			return entities;
		}

		std::vector<ComponentTypeID> GetEntityComponents(EntityID entity) const {
			assert(entity < MAX_ENTITY_COUNT && "EntityID out of range!");
			auto it = entitiesSignatures.find(entity);
			if (it != entitiesSignatures.end()) {
				const EntitySignature& signature = *(it->second);
				return { signature.begin(), signature.end() };
			}
			return {};
		}

		// Register a component by name - THE PRIMARY WAY TO REGISTER COMPONENTS
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

		// Get component type ID by name
		ComponentTypeID GetComponentTypeIdByName(const std::string& name) const {
			return ComponentTypeRegistry::GetIDByName(name);
		}

		// Get component name by type ID
		std::string GetComponentNameById(ComponentTypeID typeId) const {
			return ComponentTypeRegistry::GetNameByID(typeId);
		}

		// Get all registered component names
		std::vector<std::string> GetAllRegisteredComponentNames() const {
			return ComponentTypeRegistry::GetAllNames();
		}

		// Check if a component name is registered
		bool IsComponentNameRegistered(const std::string& name) const {
			return ComponentTypeRegistry::IsNameRegistered(name);
		}

		nlohmann::json EntityManager::SerializeEntity(const EntityID entity) {
			nlohmann::json entityJson;

			entityJson["ID"] = entity;
			entityJson["components"] = nlohmann::json::array();

			auto componentTypes = GetEntityComponents(entity);
			for (const auto& componentId : componentTypes) {
				if (auto* baseComponent = GetComponentById(entity, componentId)) {
					// Create a json object with the component name as key
					nlohmann::json componentJson;
					std::string componentName = GetComponentNameById(componentId);
					if (componentName != "Unknown") {
						// Extract the inner content from the component's serialization
						nlohmann::json serialized = baseComponent->Serialize();

						// Check if the serialized data has the component name as a key
						if (serialized.contains(componentName)) {
							// Use the inner content to avoid double nesting
							componentJson[componentName] = serialized[componentName];
						}
						else {
							// Use as-is if not nested
							componentJson[componentName] = serialized;
						}

						entityJson["components"].push_back(componentJson);
					}
				}
			}

			return entityJson;
		}

		EntityID DeserializeEntity(const nlohmann::json& json) {
			if (!json.contains("components") || !json["components"].is_array()) {
				std::cerr << "Error: Invalid entity data format in JSON" << std::endl;
				return 0;
			}

			EntityID entity = AddNewEntity();

			for (const auto& componentJson : json["components"]) {
				for (auto it = componentJson.begin(); it != componentJson.end(); ++it) {
					std::string componentName = it.key();
					ComponentTypeID typeId = GetComponentTypeIdByName(componentName);
					if (typeId != MAX_COMPONENT_COUNT) {
						auto creator = componentCreators.find(typeId);
						if (creator != componentCreators.end()) {
							creator->second(entity);
							if (auto* component = GetComponentById(entity, typeId)) {
								component->Deserialize(componentJson[componentName]);
							}
						}
					}
				}
			}

			return entity;
		}

		void DeserializeEntity(const nlohmann::json& json, const EntityID entity) {
			if (!json.contains("components") || !json["components"].is_array()) {
				std::cerr << "Error: Invalid entity data format in JSON" << std::endl;
				return;
			}

			for (const auto& componentJson : json["components"]) {
				for (auto it = componentJson.begin(); it != componentJson.end(); ++it) {
					std::string componentName = it.key();
					ComponentTypeID typeId = GetComponentTypeIdByName(componentName);
					if (typeId != MAX_COMPONENT_COUNT) {
						auto creator = componentCreators.find(typeId);
						if (creator != componentCreators.end()) {
							creator->second(entity);
							if (auto* component = GetComponentById(entity, typeId)) {
								component->Deserialize(componentJson[componentName]);
							}
						}
					}
				}
			}
		}

		// Plugin support for component registration
		using ComponentCreator = std::function<void(EntityID)>;
		using ComponentGetter = std::function<BaseComponent* (EntityID)>;

		void RegisterComponentType(ComponentTypeID typeId, ComponentCreator creator, ComponentGetter getter) {
			componentCreators[typeId] = creator;
			componentGetters[typeId] = getter;
		}

		// Getters for private variables
		EntityID GetEntityCount() const { return entityCount; }
		std::queue<EntityID> GetAvailableEntities() const { return availableEntities; }
		const std::map<EntityID, std::shared_ptr<EntitySignature>>& GetEntitiesSignatures() const { return entitiesSignatures; }
		const std::map<SystemTypeID, std::shared_ptr<BaseSystem>>& GetRegisteredSystems() const { return registeredSystems; }
		const std::map<ComponentTypeID, std::shared_ptr<ICompList>>& GetComponentsArrays() const { return componentsArrays; }

		// Debug function to print registered components
		void DebugPrintRegisteredComponents() const {
			std::cout << "Registered Component Types:" << std::endl;
			auto names = GetAllRegisteredComponentNames();
			for (const auto& name : names) {
				ComponentTypeID id = GetComponentTypeIdByName(name);
				std::cout << "  - " << name << " (ID: " << id << ")" << std::endl;
			}

			// Additional registry debug info
			ComponentTypeRegistry::DebugPrint();
		}

		// Debug function to print entity components
		void DebugPrintEntityComponents(EntityID entity) const {
			std::cout << "Entity " << entity << " raw components (" << GetEntityComponents(entity).size() << "):" << std::endl;
			for (const auto& compId : GetEntityComponents(entity)) {
				std::string name = GetComponentNameById(compId);
				std::cout << "  - ID: " << compId << " (" << name << ")" << std::endl;
			}
		}

	private:
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

		void AddEntitySignature(const EntityID entity) {
			auto it = entitiesSignatures.find(entity);
			if (it != entitiesSignatures.end()) {
				// Signature already exists, clear it
				it->second->clear();
			}
			else {
				// Create new signature
				entitiesSignatures[entity] = std::make_shared<EntitySignature>();
			}
		}

		std::shared_ptr<EntitySignature> GetEntitySignature(const EntityID entity) {
			auto it = entitiesSignatures.find(entity);
			if (it == entitiesSignatures.end()) {
				// If no signature exists, create one
				AddEntitySignature(entity);
			}
			return entitiesSignatures.at(entity);
		}

		void UpdateEntityTargetSystem(const EntityID entity) {
			for (auto& system : registeredSystems) {
				AddEntityToSystem(entity, system.second.get());
			}
		}

		void AddEntityToSystem(const EntityID entity, BaseSystem* system) {
			if (IsEntityInSystem(entity, system->signature)) {
				system->entities.insert(entity);
			}
			else {
				system->entities.erase(entity);
			}
		}

		bool IsEntityInSystem(const EntityID entity, const EntitySignature& system_signature) {
			auto entitySigIt = entitiesSignatures.find(entity);
			if (entitySigIt == entitiesSignatures.end()) {
				return false;
			}

			for (const auto compType : system_signature) {
				if (entitySigIt->second->count(compType) == 0) {
					return false;
				}
			}
			return true;
		}

		BaseComponent* GetComponentById(EntityID entity, ComponentTypeID typeId) {
			auto getter = componentGetters.find(typeId);
			if (getter != componentGetters.end()) {
				return getter->second(entity);
			}
			return nullptr;
		}

		EntityID entityCount;
		std::queue<EntityID> availableEntities;
		std::map<EntityID, std::shared_ptr<EntitySignature>> entitiesSignatures;
		std::map<SystemTypeID, std::shared_ptr<BaseSystem>> registeredSystems;
		std::map<ComponentTypeID, std::shared_ptr<ICompList>> componentsArrays;
		std::unordered_map<ComponentTypeID, ComponentCreator> componentCreators;
		std::unordered_map<ComponentTypeID, ComponentGetter> componentGetters;
	};
}