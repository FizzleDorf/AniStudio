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

		const EntityID AddNewEntity() {
			const EntityID entityId = availableEntities.front();
			availableEntities.pop();
			entityCount++;
			return entityId;
		}

		void DestroyEntity(const EntityID entity) {
			assert(entity < MAX_ENTITY_COUNT && "EntityID out of range!");
			entitySignatures.erase(entity);

			for (auto& array : componentArrays) {
				array.second->Erase(entity);
			}

			for (auto& system : registeredSystems) {
				system.second->RemoveEntity(entity);
			}

			entityCount--;
			availableEntities.push(entity);
		}

	private:
		EntityID entityCount;
		std::queue<EntityID> availableEntities;
		std::map<EntityID, EntitySignature> entitySignatures;
		std::map<SystemTypeID,std::unique_ptr<BaseSystem>> registeredSystems;
		std::map<ComponentTypeID, std::shared_ptr<ICompList>> componentArrays;

	};
}