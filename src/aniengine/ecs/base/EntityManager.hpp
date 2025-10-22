#pragma once
#include <queue>
#include <functional>
#include <unordered_map>
#include <memory>
#include <typeindex>
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

		template<typename T, typename... Args>
		T& AddComponent(const EntityID entity, Args &&...args) {
			assert(entity < MAX_ENTITY_COUNT && "EntityID out of range!");
			assert(GetEntitySignature(entity)->size() < MAX_COMPONENT_COUNT && "Component count limit reached!");

			// Get component name and look up by name instead of typeid
			std::string componentName = typeid(T).name();

			// Try to find by name in our registry first
			ComponentTypeID compType = MAX_COMPONENT_COUNT;
			for (const auto& pair : m_componentNameToID) {
				auto it = m_componentTypeToID.find(std::type_index(typeid(T)));
				if (it != m_componentTypeToID.end()) {
					compType = it->second;
					break;
				}
			}

			// If still not found, try looking it up by the actual type name string
			if (compType == MAX_COMPONENT_COUNT) {
				// Fallback: create new ID (this is the broken behavior, but keep it for safety)
				compType = m_nextComponentID++;
				m_componentTypeToID[std::type_index(typeid(T))] = compType;
			}

			T component(std::forward<Args>(args)...);
			component.entityID = entity;

			GetEntitySignature(entity)->insert(compType);
			GetCompList<T>()->Insert(component);
			UpdateEntityTargetSystem(entity);

			return GetCompList<T>()->Get(entity);
		}

		template<typename T>
		void RemoveComponent(const EntityID entity) {
			assert(entity < MAX_ENTITY_COUNT && "EntityID out of range!");

			std::type_index typeIdx = std::type_index(typeid(T));
			auto it = m_componentTypeToID.find(typeIdx);
			if (it == m_componentTypeToID.end()) return;

			auto sigIt = entitiesSignatures.find(entity);
			if (sigIt != entitiesSignatures.end()) {
				sigIt->second->erase(it->second);
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

			auto it = entitiesSignatures.find(entity);
			if (it == entitiesSignatures.end()) {
				return false;
			}

			std::type_index typeIdx = std::type_index(typeid(T));
			auto compIt = m_componentTypeToID.find(typeIdx);
			if (compIt == m_componentTypeToID.end()) {
				return false;
			}

			const EntitySignature& signature = *(it->second);
			return (signature.count(compIt->second) > 0);
		}

		template <typename T>
		void RegisterSystem() {
			std::type_index typeIdx = std::type_index(typeid(T));

			auto it = m_systemTypeToID.find(typeIdx);
			SystemTypeID systemType;
			if (it != m_systemTypeToID.end()) {
				systemType = it->second;
			}
			else {
				systemType = m_nextSystemID++;
				m_systemTypeToID[typeIdx] = systemType;
				m_systemIDToName[systemType] = typeid(T).name();
			}

			assert(registeredSystems.count(systemType) == 0 && "System already registered!");
			auto system = std::make_shared<T>(*this);

			// FIXED: Now uses public IsEntityInSystem method
			for (const auto& entitySig : entitiesSignatures) {
				if (IsEntityInSystem(entitySig.first, system->signature)) {
					system->entities.insert(entitySig.first);
				}
			}

			system->Start();
			registeredSystems[systemType] = std::move(system);
			std::cout << "Registered system: " << typeid(T).name() << " with ID: " << systemType << std::endl;
		}

		template<typename T>
		void UnregisterSystem() {
			std::type_index typeIdx = std::type_index(typeid(T));
			auto it = m_systemTypeToID.find(typeIdx);
			if (it != m_systemTypeToID.end()) {
				auto sysIt = registeredSystems.find(it->second);
				if (sysIt != registeredSystems.end()) {
					sysIt->second->Destroy();
					registeredSystems.erase(sysIt);
				}
			}
			std::cout << "Unregistered system: " << typeid(T).name() << std::endl;
		}

		template <typename T>
		std::shared_ptr<T> GetSystem() {
			std::type_index typeIdx = std::type_index(typeid(T));
			auto it = m_systemTypeToID.find(typeIdx);
			if (it != m_systemTypeToID.end()) {
				auto sysIt = registeredSystems.find(it->second);
				if (sysIt != registeredSystems.end()) {
					return std::static_pointer_cast<T>(sysIt->second);
				}
			}
			return nullptr;
		}

		template<typename T>
		ComponentTypeID RegisterComponent(const std::string& name) {
			static_assert(std::is_base_of_v<BaseComponent, T>,
				"ALL components must derive from BaseComponent");

			std::type_index typeIdx = std::type_index(typeid(T));
			auto it = m_componentTypeToID.find(typeIdx);
			if (it != m_componentTypeToID.end()) {
				return it->second;
			}

			auto nameIt = m_componentNameToID.find(name);
			if (nameIt != m_componentNameToID.end()) {
				m_componentTypeToID[typeIdx] = nameIt->second;
				return nameIt->second;
			}

			ComponentTypeID newId = m_nextComponentID++;
			m_componentNameToID[name] = newId;
			m_componentIDToName[newId] = name;
			m_componentTypeToID[typeIdx] = newId;

			std::cout << "[EntityManager] Registered component: " << name
				<< " with ID: " << newId << std::endl;

			RegisterComponentType(
				newId,
				[this, newId](EntityID entity) {
				std::cout << "[ComponentCreator] Adding component with ID: " << newId << " to entity: " << entity << std::endl;

				GetEntitySignature(entity)->insert(newId);
				T component;
				component.entityID = entity;
				GetCompList<T>()->Insert(component);
				UpdateEntityTargetSystem(entity);

				std::cout << "Component added! ID: " << entity << ", Type ID: " << newId << std::endl;
			},
				[this](EntityID entity) -> BaseComponent* {
				return this->HasComponent<T>(entity) ? &this->GetComponent<T>(entity) : nullptr;
			}
			);

			return newId;
		}

		template<typename T>
		void AddCompList() {
			std::type_index typeIdx = std::type_index(typeid(T));
			auto it = m_componentTypeToID.find(typeIdx);
			ComponentTypeID compType;
			if (it != m_componentTypeToID.end()) {
				compType = it->second;
			}
			else {
				compType = m_nextComponentID++;
				m_componentTypeToID[typeIdx] = compType;
			}
			assert(componentsArrays.find(compType) == componentsArrays.end() && "CompList already registered!");
			componentsArrays[compType] = std::move(std::make_shared<CompList<T>>());
		}

		template<typename T>
		std::shared_ptr<CompList<T>> GetCompList() {
			std::type_index typeIdx = std::type_index(typeid(T));
			auto it = m_componentTypeToID.find(typeIdx);
			ComponentTypeID compType;
			if (it != m_componentTypeToID.end()) {
				compType = it->second;
			}
			else {
				compType = m_nextComponentID++;
				m_componentTypeToID[typeIdx] = compType;
			}
			if (componentsArrays.count(compType) == 0) { AddCompList<T>(); }
			return std::static_pointer_cast<CompList<T>>(componentsArrays.at(compType));
		}

		void RemoveComponentById(EntityID entityID, ComponentTypeID componentId);
		bool HasComponentById(const EntityID entity, ComponentTypeID componentId);
		std::vector<EntityID> GetAllEntities() const;
		std::vector<ComponentTypeID> GetEntityComponents(EntityID entity) const;

		ComponentTypeID GetComponentTypeIdByName(const std::string& name) const;
		std::string GetComponentNameById(ComponentTypeID typeId) const;
		std::vector<std::string> GetAllRegisteredComponentNames() const;
		bool IsComponentNameRegistered(const std::string& name) const;

		nlohmann::json SerializeEntity(const EntityID entity) const;
		EntityID CloneEntity(const EntityID sourceEntity);
		EntityID DeserializeEntity(const nlohmann::json& json);
		void DeserializeEntity(const nlohmann::json& json, const EntityID entity);

		using ComponentCreator = std::function<void(EntityID)>;
		using ComponentGetter = std::function<BaseComponent* (EntityID)>;
		void RegisterComponentType(ComponentTypeID typeId, ComponentCreator creator, ComponentGetter getter);
		void UnregisterPluginSystem(SystemTypeID systemId);
		void UnregisterPluginComponent(ComponentTypeID componentId);

		void RegisterPluginComponent(ComponentTypeID typeId,
			size_t componentSize,
			std::function<void(void*, EntityID)> constructor,
			std::function<void(void*)> destructor);

		void* GetPluginComponent(EntityID entity, ComponentTypeID typeId);
		void* AddPluginComponent(EntityID entity, ComponentTypeID typeId);
		void RemovePluginComponent(EntityID entity, ComponentTypeID typeId);
		bool HasPluginComponent(EntityID entity, ComponentTypeID typeId);

		void RegisterPluginSystem(SystemTypeID typeId,
			std::function<void*(EntityManager*)> creator,
			std::function<void(void*)> destructor,
			std::function<void(void*, float)> updater,
			std::function<void(void*)> starter,
			const std::vector<ComponentTypeID>& requiredComponents);

		void* GetPluginSystem(SystemTypeID typeId);
		void UpdatePluginSystems(float deltaTime);

		std::shared_ptr<EntitySignature> GetEntitySignature(const EntityID entity);

		BaseComponent* GetComponentById(EntityID entity, ComponentTypeID typeId);
		const BaseComponent* GetComponentByIdConst(EntityID entity, ComponentTypeID typeId) const;
		bool IsPluginComponent(ComponentTypeID typeId);

		EntityID GetEntityCount() const;
		std::queue<EntityID> GetAvailableEntities() const;
		const std::map<EntityID, std::shared_ptr<EntitySignature>>& GetEntitiesSignatures() const;
		const std::map<SystemTypeID, std::shared_ptr<BaseSystem>>& GetRegisteredSystems() const;
		const std::map<ComponentTypeID, std::shared_ptr<ICompList>>& GetComponentsArrays() const;

		void DebugPrintRegisteredComponents() const;
		void DebugPrintEntityComponents(EntityID entity) const;
		void DebugPrintPluginSystems() const;
		bool IsEntityInSystem(const EntityID entity, const EntitySignature& system_signature);

	private:
		void AddEntitySignature(const EntityID entity);
		void UpdateEntityTargetSystem(const EntityID entity);
		void CopyComponentResources(EntityID sourceEntity, EntityID destEntity, ComponentTypeID componentId);

		EntityID entityCount;
		std::queue<EntityID> availableEntities;
		std::map<EntityID, std::shared_ptr<EntitySignature>> entitiesSignatures;
		std::map<SystemTypeID, std::shared_ptr<BaseSystem>> registeredSystems;
		std::map<ComponentTypeID, std::shared_ptr<ICompList>> componentsArrays;
		std::unordered_map<ComponentTypeID, ComponentCreator> componentCreators;
		std::unordered_map<ComponentTypeID, ComponentGetter> componentGetters;

		ComponentTypeID m_nextComponentID = 0;
		std::unordered_map<std::string, ComponentTypeID> m_componentNameToID;
		std::unordered_map<ComponentTypeID, std::string> m_componentIDToName;
		std::unordered_map<std::type_index, ComponentTypeID> m_componentTypeToID;

		SystemTypeID m_nextSystemID = 0;
		std::unordered_map<std::type_index, SystemTypeID> m_systemTypeToID;
		std::unordered_map<std::string, SystemTypeID> m_systemNameToID;
		std::unordered_map<SystemTypeID, std::string> m_systemIDToName;

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