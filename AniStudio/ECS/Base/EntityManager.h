#pragma once

#include "Types.h"
#include "CompList.h"
#include "BaseSystem.h"
#include "BaseComponent.h"

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

		const EntityID AddNewEntity() {
			const EntityID entityId = availableEntities.front();
			availableEntities.pop();
			entityCount++;
			return entityId;
		}

		void DestroyEntity(const EntityID entity) {
			assert(entity < MAX_ENTITY_COUNT && "EntityID out of range!");
			entitiesSignatures.erase(entity);

			for (auto& array : componentArrays) {
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
			assert(entities_signatures[entity].size() < MAX_COMPONENT_COUNT && "Component count limit reached!");

			T component(std::forward<Args>(args)...);
			component.entityID = entity;
			GetCompList<T>()->Insert(component);

			const ComponentTypeID compType = CompType<T>();
			entitiesSignatures.at(entity).insert(compType);
			AttachEntityToSystem(entity);
		}

		template<typename T>
		void RemoveComponent(const EntityID entity) {
			assert(entity < MAX_ENTITY_COUNT && "EntityID out of range!");
			const ComponentTypeID compType = CompType<T>();
			entitiesSignatures.at(entity).erase(compType);
			GetCompList<T>()->Erase(entity);
			AttachEntityToSystem(entity);
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
		std::map<SystemTypeID,std::unique_ptr<BaseSystem>> registeredSystems;
		std::map<ComponentTypeID, std::shared_ptr<ICompList>> componentArrays;

	};
}