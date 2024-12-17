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
				system.second->Render();
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
            std::cout << "Removed Entity: " << entity << "\n";
		}

		template<typename T, typename... Args>
		void AddComponent(const EntityID entity, Args&&... args) {
			assert(entity < MAX_ENTITY_COUNT && "EntityID out of range!");
			assert(GetEntitiySignature(entity)->size() < MAX_COMPONENT_COUNT && "Component count limit reached!");

			T component(std::forward<Args>(args)...);
			component.entityID = entity;
			GetEntitiySignature(entity)->insert(CompType<T>());
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
		T& GetComponent(const EntityID entity) {
			assert(entity < MAX_ENTITY_COUNT && "EntityID out of range!");
			const ComponentTypeID compType = CompType<T>();
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
			assert(registeredSystems.count(systemType) == 0 && "System already unregistered!");
			registeredSystems.erase(systemType);
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
            // Destroy all entities by clearing their signatures and components
            for (auto &entitySignaturePair : entitiesSignatures) {
                DestroyEntity(entitySignaturePair.first);
            }
            entitiesSignatures.clear();

            // Clear all registered systems
            registeredSystems.clear();

            // Clear and reset the entity queue
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
            for (const auto &pair : entitiesSignatures) {
                entities.push_back(pair.first);
            }
            return entities;
        }

        std::vector<ComponentTypeID> GetEntityComponents(EntityID entity) const {
            assert(entity < MAX_ENTITY_COUNT && "EntityID out of range!");
            auto it = entitiesSignatures.find(entity);
            if (it != entitiesSignatures.end()) {
                const EntitySignature &signature = *(it->second);
                return {signature.begin(), signature.end()};
            }
            return {};
        }


		// Getters for private variables
		EntityID GetEntityCount() const { return entityCount; }
		std::queue<EntityID> GetAvailableEntities() const { return availableEntities; }
		const std::map<EntityID, std::shared_ptr<EntitySignature>>& GetEntitiesSignatures() const { return entitiesSignatures; }
		const std::map<SystemTypeID, std::shared_ptr<BaseSystem>>& GetRegisteredSystems() const { return registeredSystems; }
		const std::map<ComponentTypeID, std::shared_ptr<ICompList>>& GetComponentsArrays() const { return componentsArrays; }
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
			assert(entitiesSignatures.find(entity) == entitiesSignatures.end() && "Signature not found");
			entitiesSignatures[entity] = std::move(std::make_shared<EntitySignature>());
		}

		std::shared_ptr<EntitySignature> GetEntitiySignature(const EntityID entity) {
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
				if (GetEntitiySignature(entity)->count(compType) == 0) {
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
    extern EntityManager mgr;
    }