#include "EntityManager.hpp"
#include "Log.hpp"

#include <cassert>

namespace ECS {

	class PluginComponentArray : public ICompList {
	private:
		std::unordered_map<EntityID, void*> m_components;
		size_t m_componentSize;
		std::function<void(void*, EntityID)> m_constructor;
		std::function<void(void*)> m_destructor;

	public:
		PluginComponentArray(size_t componentSize,
			std::function<void(void*, EntityID)> constructor,
			std::function<void(void*)> destructor)
			: m_componentSize(componentSize), m_constructor(constructor), m_destructor(destructor) {
		}

		~PluginComponentArray() {
			for (auto& [entityId, ptr] : m_components) {
				if (ptr) {
					m_destructor(ptr);
					std::free(ptr);
				}
			}
		}

		void* Insert(EntityID entity) {
			Erase(entity);
			void* ptr = std::malloc(m_componentSize);
			if (ptr) {
				m_constructor(ptr, entity);
				m_components[entity] = ptr;
			}
			return ptr;
		}

		void* Get(EntityID entity) {
			auto it = m_components.find(entity);
			return (it != m_components.end()) ? it->second : nullptr;
		}

		void Erase(EntityID entity) override {
			auto it = m_components.find(entity);
			if (it != m_components.end()) {
				m_destructor(it->second);
				std::free(it->second);
				m_components.erase(it);
			}
		}

		size_t Size() const {
			return m_components.size();
		}
	};

	bool EntityManager::IsPluginComponent(ComponentTypeID typeId) {
		auto arrayIt = componentsArrays.find(typeId);
		if (arrayIt != componentsArrays.end()) {
			return std::dynamic_pointer_cast<PluginComponentArray>(arrayIt->second) != nullptr;
		}
		return false;
	}

	EntityManager::EntityManager() : entityCount(0) {
		Reset();
	}

	EntityManager::~EntityManager() {
		for (auto& [systemId, systemInfo] : pluginSystems) {
			if (systemInfo.instance && systemInfo.destructor) {
				ANI_LOG_DEBUG("Destroying plugin system ID: %u", systemId);
				systemInfo.destructor(systemInfo.instance);
			}
		}
		pluginSystems.clear();

		for (auto& system : registeredSystems) {
			if (system.second) {
				system.second->Destroy();
			}
		}
		registeredSystems.clear();
	}

	void EntityManager::Update(const float deltaT) {
		for (auto& system : registeredSystems) {
			system.second->Update(deltaT);
		}
		UpdatePluginSystems(deltaT);
	}

	void EntityManager::Reset() {
		ANI_LOG_INFO("Resetting EntityManager...");

		for (auto& system : registeredSystems) {
			if (system.second) {
				system.second->Destroy();
			}
		}

		entitiesSignatures.clear();
		registeredSystems.clear();

		for (auto& [systemId, systemInfo] : pluginSystems) {
			if (systemInfo.instance && systemInfo.destructor) {
				systemInfo.destructor(systemInfo.instance);
			}
		}
		pluginSystems.clear();

		componentsArrays.clear();

		while (!availableEntities.empty()) {
			availableEntities.pop();
		}
		for (EntityID entity = 0u; entity < MAX_ENTITY_COUNT; ++entity) {
			availableEntities.push(entity);
		}

		entityCount = 0;

		ANI_LOG_INFO("EntityManager reset complete. Registered components preserved.");
	}

	const EntityID EntityManager::AddNewEntity() {
		const EntityID entity = availableEntities.front();
		AddEntitySignature(entity);
		availableEntities.pop();
		entityCount++;
		return entity;
	}

	void EntityManager::DestroyEntity(const EntityID entity) {
		assert(entity < MAX_ENTITY_COUNT && "EntityID out of range!");

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

		for (auto& [systemId, systemInfo] : pluginSystems) {
			systemInfo.entities.erase(entity);
		}

		entityCount--;
		availableEntities.push(entity);
		ANI_LOG_TRACE("Removed Entity: %u", entity);
	}

	bool EntityManager::IsEntityValid(EntityID entity) const {
		return entity < MAX_ENTITY_COUNT &&
			entitiesSignatures.find(entity) != entitiesSignatures.end();
	}

	void EntityManager::RemoveComponentById(EntityID entityID, ComponentTypeID componentId) {
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

	void EntityManager::UnregisterPluginSystem(SystemTypeID systemId) {
		auto it = pluginSystems.find(systemId);
		if (it == pluginSystems.end()) {
			return;
		}

		PluginSystemInfo& systemInfo = it->second;

		if (systemInfo.instance && systemInfo.destructor) {
			ANI_LOG_DEBUG("Destroying plugin system ID: %u", systemId);
			systemInfo.destructor(systemInfo.instance);
		}

		pluginSystems.erase(it);
		ANI_LOG_INFO("Unregistered plugin system ID: %u", systemId);
	}

	void EntityManager::UnregisterPluginComponent(ComponentTypeID componentId) {
		auto arrayIt = componentsArrays.find(componentId);
		if (arrayIt != componentsArrays.end()) {
			componentsArrays.erase(arrayIt);
		}

		componentCreators.erase(componentId);
		componentGetters.erase(componentId);

		ANI_LOG_INFO("Unregistered plugin component ID: %u", componentId);
	}

	bool EntityManager::HasComponentById(const EntityID entity, ComponentTypeID componentId) {
		assert(entity < MAX_ENTITY_COUNT && "EntityID out of range!");
		auto it = entitiesSignatures.find(entity);
		if (it == entitiesSignatures.end()) {
			return false;
		}
		const EntitySignature& signature = *(it->second);
		return (signature.count(componentId) > 0);
	}

	std::vector<EntityID> EntityManager::GetAllEntities() const {
		std::vector<EntityID> entities;
		for (const auto& pair : entitiesSignatures) {
			entities.push_back(pair.first);
		}
		return entities;
	}

	std::vector<ComponentTypeID> EntityManager::GetEntityComponents(EntityID entity) const {
		assert(entity < MAX_ENTITY_COUNT && "EntityID out of range!");
		auto it = entitiesSignatures.find(entity);
		if (it != entitiesSignatures.end()) {
			const EntitySignature& signature = *(it->second);
			return { signature.begin(), signature.end() };
		}
		return {};
	}

	ComponentTypeID EntityManager::GetComponentTypeIdByName(const std::string& name) const {
		auto it = m_componentNameToID.find(name);
		return (it != m_componentNameToID.end()) ? it->second : MAX_COMPONENT_COUNT;
	}

	std::string EntityManager::GetComponentNameById(ComponentTypeID typeId) const {
		auto it = m_componentIDToName.find(typeId);
		return (it != m_componentIDToName.end()) ? it->second : "Unknown";
	}

	std::vector<std::string> EntityManager::GetAllRegisteredComponentNames() const {
		std::vector<std::string> names;
		for (const auto& pair : m_componentNameToID) {
			names.push_back(pair.first);
		}
		return names;
	}

	bool EntityManager::IsComponentNameRegistered(const std::string& name) const {
		return m_componentNameToID.find(name) != m_componentNameToID.end();
	}

	nlohmann::json EntityManager::SerializeEntity(const EntityID entity) const {
		nlohmann::json entityJson;

		entityJson["ID"] = entity;
		entityJson["components"] = nlohmann::json::array();

		auto componentTypes = GetEntityComponents(entity);

		for (const auto& componentId : componentTypes) {
			BaseComponent* baseComponent = const_cast<EntityManager*>(this)->GetComponentById(entity, componentId);

			if (baseComponent) {
				nlohmann::json componentJson;
				std::string componentName = GetComponentNameById(componentId);

				if (componentName != "Unknown") {
					nlohmann::json serialized = baseComponent->Serialize();

					if (serialized.contains(componentName)) {
						componentJson[componentName] = serialized[componentName];
					}
					else {
						componentJson[componentName] = serialized;
					}

					entityJson["components"].push_back(componentJson);
				}
			}
		}

		return entityJson;
	}

	EntityID EntityManager::CloneEntity(const EntityID sourceEntity) {
		if (!IsEntityValid(sourceEntity)) {
			ANI_LOG_ERROR("Error: Cannot clone invalid entity %u", sourceEntity);
			return 0;
		}

		EntityID newEntity = AddNewEntity();

		auto componentTypes = GetEntityComponents(sourceEntity);

		try {
			for (const auto& componentId : componentTypes) {
				auto creator = componentCreators.find(componentId);
				if (creator != componentCreators.end()) {
					creator->second(newEntity);

					if (auto* sourceComponent = GetComponentById(sourceEntity, componentId)) {
						if (auto* destComponent = GetComponentById(newEntity, componentId)) {
							nlohmann::json componentData = sourceComponent->Serialize();
							destComponent->Deserialize(componentData);
							CopyComponentResources(sourceEntity, newEntity, componentId);
						}
					}
				}
			}

			ANI_LOG_INFO("Successfully cloned entity %u to %u", sourceEntity, newEntity);
			return newEntity;
		}
		catch (const std::exception& e) {
			ANI_LOG_ERROR("Error cloning entity %u: %s", sourceEntity, e.what());
			DestroyEntity(newEntity);
			return 0;
		}
	}

	EntityID EntityManager::DeserializeEntity(const nlohmann::json& json) {
		if (!json.contains("components") || !json["components"].is_array()) {
			ANI_LOG_ERROR("Error: Invalid entity data format in JSON");
			return 0;
		}

		EntityID entity = AddNewEntity();

		try {
			for (const auto& componentJson : json["components"]) {
				for (auto it = componentJson.begin(); it != componentJson.end(); ++it) {
					std::string componentName = it.key();
					ComponentTypeID typeId = GetComponentTypeIdByName(componentName);

					ANI_LOG_TRACE("[Deserialize] key='%s' -> typeId=%u -> registeredName='%s'",
						componentName.c_str(), typeId, GetComponentNameById(typeId).c_str());

					if (typeId != MAX_COMPONENT_COUNT) {
						auto creator = componentCreators.find(typeId);
						if (creator != componentCreators.end()) {
							creator->second(entity);

							BaseComponent* component = GetComponentById(entity, typeId);

							if (component) {
								ANI_LOG_TRACE("[Deserialize] constructed component for key='%s', GetCompName()='%s', schema empty=%s",
									componentName.c_str(),
									component->GetCompName(),
									component->GetSchema().empty() ? "yes" : "no");

								component->Deserialize(componentJson[componentName]);

								ANI_LOG_TRACE("[Deserialize] after Deserialize, GetCompName()='%s', schema empty=%s",
									component->GetCompName(),
									component->GetSchema().empty() ? "yes" : "no");
							}
							else {
								ANI_LOG_ERROR("[Deserialize] ERROR: Component %s was not created!", componentName.c_str());
							}
						}
						else {
							ANI_LOG_WARN("[Deserialize] No creator for component: %s", componentName.c_str());
						}
					}
					else {
						ANI_LOG_WARN("[Deserialize] Component not found: %s", componentName.c_str());
					}
				}
			}
			return entity;
		}
		catch (const std::exception& e) {
			ANI_LOG_ERROR("Error deserializing entity: %s", e.what());
			DestroyEntity(entity);
			return 0;
		}
	}

	void EntityManager::DeserializeEntity(const nlohmann::json& json, const EntityID entity) {
		if (!IsEntityValid(entity)) {
			ANI_LOG_ERROR("Error: Cannot deserialize to invalid entity %u", entity);
			return;
		}

		if (!json.contains("components") || !json["components"].is_array()) {
			ANI_LOG_ERROR("Error: Invalid entity data format in JSON");
			return;
		}

		try {
			for (const auto& componentJson : json["components"]) {
				for (auto it = componentJson.begin(); it != componentJson.end(); ++it) {
					std::string componentName = it.key();
					ComponentTypeID typeId = GetComponentTypeIdByName(componentName);
					if (typeId != MAX_COMPONENT_COUNT) {
						if (!HasComponentById(entity, typeId)) {
							auto creator = componentCreators.find(typeId);
							if (creator != componentCreators.end()) {
								creator->second(entity);
							}
						}

						if (auto* component = GetComponentById(entity, typeId)) {
							component->Deserialize(componentJson[componentName]);
						}
					}
				}
			}
		}
		catch (const std::exception& e) {
			ANI_LOG_ERROR("Error deserializing to entity %u: %s", entity, e.what());
		}
	}

	void EntityManager::RegisterComponentType(ComponentTypeID typeId, ComponentCreator creator, ComponentGetter getter) {
		componentCreators[typeId] = creator;
		componentGetters[typeId] = getter;
	}

	void EntityManager::RegisterPluginComponent(ComponentTypeID typeId,
		size_t componentSize,
		std::function<void(void*, EntityID)> constructor,
		std::function<void(void*)> destructor) {

		auto pluginArray = std::make_shared<PluginComponentArray>(componentSize, constructor, destructor);
		componentsArrays[typeId] = pluginArray;

		RegisterComponentType(
			typeId,
			[this, typeId, pluginArray](EntityID entity) {
				GetEntitySignature(entity)->insert(typeId);
				pluginArray->Insert(entity);
				UpdateEntityTargetSystem(entity);
			},
			[pluginArray](EntityID entity) -> BaseComponent* {
				return nullptr;
			}
		);

		ANI_LOG_INFO("Registered plugin component with ID: %u (size: %zu bytes)",
			typeId, componentSize);
	}

	template<typename T>
	void EntityManager::UnregisterComponent() {
		std::type_index typeIdx = std::type_index(typeid(T));
		auto it = m_componentTypeToID.find(typeIdx);
		if (it == m_componentTypeToID.end()) {
			ANI_LOG_WARN("Component type not registered: %s", typeid(T).name());
			return;
		}

		ComponentTypeID typeId = it->second;
		UnregisterComponentById(typeId);
	}

	void EntityManager::UnregisterComponentByName(const std::string& name) {
		ComponentTypeID typeId = GetComponentTypeIdByName(name);
		if (typeId != MAX_COMPONENT_COUNT) {
			UnregisterComponentById(typeId);
		}
		else {
			ANI_LOG_WARN("Component name not found: %s", name.c_str());
		}
	}

	void EntityManager::UnregisterComponentById(ComponentTypeID typeId) {
		ANI_LOG_INFO("Unregistering component ID: %u", typeId);

		RemoveComponentFromAllEntities(typeId);

		auto arrayIt = componentsArrays.find(typeId);
		if (arrayIt != componentsArrays.end()) {
			componentsArrays.erase(arrayIt);
		}

		componentCreators.erase(typeId);
		componentGetters.erase(typeId);

		std::string componentName;

		auto idToNameIt = m_componentIDToName.find(typeId);
		if (idToNameIt != m_componentIDToName.end()) {
			componentName = idToNameIt->second;
			m_componentIDToName.erase(idToNameIt);
		}

		if (!componentName.empty()) {
			m_componentNameToID.erase(componentName);
		}

		for (auto it = m_componentTypeToID.begin(); it != m_componentTypeToID.end(); ) {
			if (it->second == typeId) {
				it = m_componentTypeToID.erase(it);
			}
			else {
				++it;
			}
		}

		ANI_LOG_INFO("Successfully unregistered component ID: %u", typeId);
	}

	void EntityManager::RemoveComponentFromAllEntities(ComponentTypeID typeId) {
		for (auto& [entityId, signature] : entitiesSignatures) {
			if (signature->count(typeId) > 0) {
				signature->erase(typeId);

				auto arrayIt = componentsArrays.find(typeId);
				if (arrayIt != componentsArrays.end()) {
					arrayIt->second->Erase(entityId);
				}
			}
		}
	}

	void* EntityManager::GetPluginComponent(EntityID entity, ComponentTypeID typeId) {
		auto arrayIt = componentsArrays.find(typeId);
		if (arrayIt != componentsArrays.end()) {
			auto pluginArray = std::dynamic_pointer_cast<PluginComponentArray>(arrayIt->second);
			if (pluginArray) {
				return pluginArray->Get(entity);
			}
		}
		return nullptr;
	}

	void* EntityManager::AddPluginComponent(EntityID entity, ComponentTypeID typeId) {
		auto arrayIt = componentsArrays.find(typeId);
		if (arrayIt != componentsArrays.end()) {
			auto pluginArray = std::dynamic_pointer_cast<PluginComponentArray>(arrayIt->second);
			if (pluginArray) {
				GetEntitySignature(entity)->insert(typeId);
				void* component = pluginArray->Insert(entity);
				UpdateEntityTargetSystem(entity);

				ANI_LOG_TRACE("Added plugin component to entity %u with type ID: %u",
					entity, typeId);
				return component;
			}
		}
		return nullptr;
	}

	void EntityManager::RemovePluginComponent(EntityID entity, ComponentTypeID typeId) {
		auto it = entitiesSignatures.find(entity);
		if (it != entitiesSignatures.end()) {
			it->second->erase(typeId);

			auto arrayIt = componentsArrays.find(typeId);
			if (arrayIt != componentsArrays.end()) {
				arrayIt->second->Erase(entity);
			}

			UpdateEntityTargetSystem(entity);

			ANI_LOG_TRACE("Removed plugin component from entity %u with type ID: %u",
				entity, typeId);
		}
	}

	bool EntityManager::HasPluginComponent(EntityID entity, ComponentTypeID typeId) {
		return GetPluginComponent(entity, typeId) != nullptr;
	}

	void EntityManager::RegisterPluginSystem(SystemTypeID typeId,
		std::function<void* (EntityManager*)> creator,
		std::function<void(void*)> destructor,
		std::function<void(void*, float)> updater,
		std::function<void(void*)> starter,
		const std::vector<ComponentTypeID>& requiredComponents) {

		ANI_LOG_INFO("Registering plugin system ID: %u with %zu required components",
			typeId, requiredComponents.size());

		void* systemInstance = creator(this);
		if (!systemInstance) {
			ANI_LOG_ERROR("Failed to create plugin system with ID: %u", typeId);
			return;
		}

		ANI_LOG_DEBUG("Plugin system instance created successfully");

		PluginSystemInfo systemInfo;
		systemInfo.instance = systemInstance;
		systemInfo.destructor = destructor;
		systemInfo.updater = updater;
		systemInfo.requiredComponents = requiredComponents;

		int matchingEntities = 0;
		for (const auto& [entityId, entitySignature] : entitiesSignatures) {
			bool matches = true;
			for (ComponentTypeID compType : requiredComponents) {
				if (entitySignature->count(compType) == 0) {
					matches = false;
					break;
				}
			}
			if (matches) {
				systemInfo.entities.insert(entityId);
				matchingEntities++;
			}
		}

		pluginSystems[typeId] = std::move(systemInfo);

		ANI_LOG_DEBUG("Plugin system stored with ID: %u", typeId);

		if (starter) {
			ANI_LOG_DEBUG("Starting plugin system...");
			starter(systemInstance);
		}

		ANI_LOG_INFO("Registered plugin system with ID: %u requiring %zu components with %d initial entities",
			typeId, requiredComponents.size(), matchingEntities);
	}

	void* EntityManager::GetPluginSystem(SystemTypeID typeId) {
		auto it = pluginSystems.find(typeId);
		return (it != pluginSystems.end()) ? it->second.instance : nullptr;
	}

	void EntityManager::UpdatePluginSystems(float deltaTime) {
		for (auto& [systemId, systemInfo] : pluginSystems) {
			if (systemInfo.instance && systemInfo.updater) {
				systemInfo.updater(systemInfo.instance, deltaTime);
			}
		}
	}

	EntityID EntityManager::GetEntityCount() const {
		return entityCount;
	}

	std::queue<EntityID> EntityManager::GetAvailableEntities() const {
		return availableEntities;
	}

	const std::map<EntityID, std::shared_ptr<EntitySignature>>& EntityManager::GetEntitiesSignatures() const {
		return entitiesSignatures;
	}

	const std::map<SystemTypeID, std::shared_ptr<BaseSystem>>& EntityManager::GetRegisteredSystems() const {
		return registeredSystems;
	}

	const std::map<ComponentTypeID, std::shared_ptr<ICompList>>& EntityManager::GetComponentsArrays() const {
		return componentsArrays;
	}

	void EntityManager::DebugPrintRegisteredComponents() const {
		ANI_LOG_DEBUG("Registered Component Types:");
		auto names = GetAllRegisteredComponentNames();
		for (const auto& name : names) {
			ComponentTypeID id = GetComponentTypeIdByName(name);
			ANI_LOG_DEBUG("  - %s (ID: %u)", name.c_str(), id);
		}
	}

	void EntityManager::DebugPrintEntityComponents(EntityID entity) const {
		ANI_LOG_DEBUG("Entity %u raw components (%zu):", entity, GetEntityComponents(entity).size());
		for (const auto& compId : GetEntityComponents(entity)) {
			std::string name = GetComponentNameById(compId);
			ANI_LOG_DEBUG("  - ID: %u (%s)", compId, name.c_str());
		}
	}

	void EntityManager::DebugPrintPluginSystems() const {
		ANI_LOG_DEBUG("=== PLUGIN SYSTEMS DEBUG ===");
		ANI_LOG_DEBUG("Total plugin systems registered: %zu", pluginSystems.size());

		for (const auto& [systemId, systemInfo] : pluginSystems) {
			auto nameIt = m_systemIDToName.find(systemId);
			std::string systemName = (nameIt != m_systemIDToName.end()) ? nameIt->second : "Unknown";
			ANI_LOG_DEBUG("\nPlugin System ID: %u (%s)", systemId, systemName.c_str());
			ANI_LOG_DEBUG("  Instance: %p", systemInfo.instance);
			ANI_LOG_DEBUG("  Required components: %zu", systemInfo.requiredComponents.size());
			for (ComponentTypeID compId : systemInfo.requiredComponents) {
				ANI_LOG_DEBUG("    - %s (ID: %u)",
					GetComponentNameById(compId).c_str(), compId);
			}
			ANI_LOG_DEBUG("  Entities: %zu", systemInfo.entities.size());
		}
		ANI_LOG_DEBUG("=========================");
	}

	void EntityManager::AddEntitySignature(const EntityID entity) {
		auto it = entitiesSignatures.find(entity);
		if (it != entitiesSignatures.end()) {
			it->second->clear();
		}
		else {
			entitiesSignatures[entity] = std::make_shared<EntitySignature>();
		}
	}

	std::shared_ptr<EntitySignature> EntityManager::GetEntitySignature(const EntityID entity) {
		auto it = entitiesSignatures.find(entity);
		if (it == entitiesSignatures.end()) {
			AddEntitySignature(entity);
		}
		return entitiesSignatures.at(entity);
	}

	void EntityManager::UpdateEntityTargetSystem(const EntityID entity) {
		for (auto& system : registeredSystems) {
			if (IsEntityInSystem(entity, system.second->signature)) {
				system.second->entities.insert(entity);
			}
			else {
				system.second->entities.erase(entity);
			}
		}

		auto entitySignature = GetEntitySignature(entity);
		for (auto& [systemId, systemInfo] : pluginSystems) {
			bool matches = true;
			for (ComponentTypeID compType : systemInfo.requiredComponents) {
				if (entitySignature->count(compType) == 0) {
					matches = false;
					break;
				}
			}

			if (matches) {
				if (systemInfo.entities.insert(entity).second) {
					ANI_LOG_TRACE("Added entity %u to plugin system %u",
						entity, systemId);
				}
			}
			else {
				systemInfo.entities.erase(entity);
			}
		}
	}

	bool EntityManager::IsEntityInSystem(const EntityID entity, const EntitySignature& system_signature) {
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

	const BaseComponent* EntityManager::GetComponentByIdConst(EntityID entity, ComponentTypeID typeId) const {
		auto getter = componentGetters.find(typeId);
		if (getter != componentGetters.end()) {
			return const_cast<const BaseComponent*>(getter->second(entity));
		}
		return nullptr;
	}

	BaseComponent* EntityManager::GetComponentById(EntityID entity, ComponentTypeID typeId) {
		auto getter = componentGetters.find(typeId);
		if (getter != componentGetters.end()) {
			return getter->second(entity);
		}
		return nullptr;
	}

	void EntityManager::CopyComponentResources(EntityID sourceEntity, EntityID destEntity, ComponentTypeID componentId) {
		std::string componentName = GetComponentNameById(componentId);

		if (componentName == "InputImage" || componentName == "Image") {
			auto sourceComp = GetComponentById(sourceEntity, componentId);
			auto destComp = GetComponentById(destEntity, componentId);
		}
	}

} // namespace ECS