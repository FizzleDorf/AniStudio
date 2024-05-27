#pragma once

#include "Types.hpp"
#include "CompList.hpp"
#include "BaseSystem.hpp"
#include "BaseComponent.hpp"

namespace ECS {
	class EntityManager {

	public:
		
		EntityManager() : entityCount(0) {
			for (EntityID entity = 0u; entity < MAX_ENTITY_COUNT; entity++) {
				availableEntities.push(entity);
			}
		}

		~EntityManager() {

		}

		void Update() {
			for (auto& system : registeredSystems) {
				system.second->Update();
			}
		}

		void Render() {
			for (auto& system : registeredSystems) {
				system.second->Update();
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
			entitiesSignatures.erase(entity);

			for (auto& array : componentsArrays) {
				array.second->Erase(entity);
			}

			for (auto& system : registeredSystems) {
				system.second->RemoveEntity(entity);
			}

			entityCount--;
			availableEntities.push(entity);
		}

		template<typename T, typename... Args>
		void AddComponent(const EntityID entity, Args&&... args) {
			assert(entity < MAX_ENTITY_COUNT && "EntityID out of range!");
			assert(GetEnitiySignature(entity)->size() < MAX_COMPONENT_COUNT && "Component count limit reached!");

			T component(std::forward<Args>(args)...);
			component.entityID = entity;
			GetEnitiySignature(entity)->insert(CompType<T>());
			GetCompList<T>()->Insert(component);
			UpdateEntityTargetSystem(entity);
		}

		template<typename T>
		void RemoveComponent(const EntityID entity) {
			assert(entity < MAX_ENTITY_COUNT && "EntityID out of range!");
			const ComponentTypeID compType = CompType<T>();
			entitiesSignatures.at(entity).erase(compType);
			GetCompList<T>()->Erase(entity);
			UpdateEntityTargetSystem(entity);
		}

		template<typename T>
		void GetComponent(const EntityID entity) {
			assert(entity < MAX_ENTITY_COUNT && "EntityID out of range!");
			const ComponentTypeID compType = CompType<T>();
			entitiesSignatures.at(entity).erase(compType);
			return GetCompList<T>()->Get(entity);
		}

		template<typename T>
		const bool HasComponent(const EntityID entity) {
			assert(entity < MAX_ENTITY_COUNT && "EntityID out of range!");
			const EntitySignature signature = entitiesSignatures.at(entity);
			const ComponentTypeID compType = CompType<T>();
			return (signature.count(compType) > 0);
		}

		template<typename T>
		void RegisterSystem() {
			const SystemTypeID systemType = SystemType<T>();
			assert(registeredSystems.count(systemType) == 0 && "System already registered!");
			auto system = std::make_shared<T>();

			for (EntityID entity = 0; entity < entityCount; entity++) {
				AddEntityToSystem(entity, system.get());
			}

			system->Start();
			registeredSystems[systemType] = std::move(system);
		}

		template<typename T>
		void UnregisterSystem() {
			const SystemTypeID systemType = SystemType<T>();
			assert(registeredSystems.count(systemType) == 0 && "System already registered!");
			registeredSystems.erase(systemType);
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
			assert(entitiesSignatures.find(entity) != entitiesSignatures.end()&& "Signature not found");
			entitiesSignatures[entity] = std::move(std::make_shared<EntitySignature>());
		}

		std::shared_ptr<EntitySignature> GetEnitiySignature(const EntityID entity) {
			assert(entitiesSignatures.find(entity) != entitiesSignatures.end() && "Signature Not Found");
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
			for (const auto compType : system_signature) {
				if (GetEnitiySignature(entity)->count(compType) == 0) {
					return false;
				}
			}
			return true;
		}

	private:
		EntityID entityCount;
		std::queue<EntityID> availableEntities;
		std::map<EntityID, std::shared_ptr<EntitySignature>> entitiesSignatures;
		std::map<SystemTypeID,std::shared_ptr<BaseSystem>> registeredSystems;
		std::map<ComponentTypeID, std::shared_ptr<ICompList>> componentsArrays;

	};
}