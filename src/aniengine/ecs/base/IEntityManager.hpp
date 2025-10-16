#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <functional>
#include <nlohmann/json_fwd.hpp>
#include "Entity.hpp"
#include "Types.hpp"

namespace ECS {

	class IEntityManager {
	public:
		virtual ~IEntityManager() = default;

		// Entity management
		virtual EntityID CreateEntity() = 0;
		virtual void DestroyEntity(EntityID entity) = 0;
		virtual bool IsEntityValid(EntityID entity) const = 0;
		virtual size_t GetEntityCount() const = 0;
		virtual std::vector<EntityID> GetAllEntities() const = 0;

		// Component management
		virtual void* AddComponent(EntityID entity, const std::string& componentName) = 0;
		virtual void* GetComponent(EntityID entity, const std::string& componentName) = 0;
		virtual void RemoveComponent(EntityID entity, const std::string& componentName) = 0;
		virtual bool HasComponent(EntityID entity, const std::string& componentName) = 0;

		// Component information
		virtual ComponentTypeID GetComponentTypeIdByName(const std::string& name) const = 0;
		virtual std::string GetComponentNameById(ComponentTypeID typeId) const = 0;
		virtual std::vector<std::string> GetAllRegisteredComponentNames() const = 0;

		// System registration
		virtual bool RegisterSystem(const std::string& name,
			std::function<void*(IEntityManager*)> creator,
			std::function<void(void*)> destructor,
			std::function<void(void*, float)> updater,
			std::function<void(void*)> starter,
			const std::vector<std::string>& requiredComponents) = 0;

		// Debug
		virtual void DebugPrintRegisteredComponents() const = 0;
		virtual void DebugPrintEntityComponents(EntityID entity) const = 0;

		// Serialization
		virtual nlohmann::json SerializeEntity(EntityID entity) const = 0;
		virtual EntityID DeserializeEntity(const nlohmann::json& json) = 0;
		virtual EntityID CloneEntity(const EntityID sourceEntity) = 0;
	};
}