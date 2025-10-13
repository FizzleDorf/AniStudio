#include "EntityManager.hpp"
#include <iostream>
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
			: m_componentSize(componentSize), m_constructor(constructor), m_destructor(destructor) {}

		~PluginComponentArray() {
			for (auto&[entityId, ptr] : m_components) {
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
		for (auto&[systemId, systemInfo] : pluginSystems) {
			if (systemInfo.instance && systemInfo.destructor) {
				std::cout << "[EntityManager] Destroying plugin system ID: " << systemId << std::endl;
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
		std::cout << "Resetting EntityManager..." << std::endl;

		for (auto& system : registeredSystems) {
			if (system.second) {
				system.second->Destroy();
			}
		}

		entitiesSignatures.clear();
		registeredSystems.clear();

		for (auto&[systemId, systemInfo] : pluginSystems) {
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

		std::cout << "EntityManager reset complete. Registered components preserved." << std::endl;
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

		for (auto&[systemId, systemInfo] : pluginSystems) {
			systemInfo.entities.erase(entity);
		}

		entityCount--;
		availableEntities.push(entity);
		std::cout << "Removed Entity: " << entity << "\n";
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
			std::cout << "[EntityManager] Destroying plugin system ID: " << systemId << std::endl;
			systemInfo.destructor(systemInfo.instance);
		}

		pluginSystems.erase(it);
		std::cout << "[EntityManager] Unregistered plugin system ID: " << systemId << std::endl;
	}

	void EntityManager::UnregisterPluginComponent(ComponentTypeID componentId) {
		auto arrayIt = componentsArrays.find(componentId);
		if (arrayIt != componentsArrays.end()) {
			componentsArrays.erase(arrayIt);
		}

		componentCreators.erase(componentId);
		componentGetters.erase(componentId);

		std::cout << "[EntityManager] Unregistered plugin component ID: " << componentId << std::endl;
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
			if (auto* baseComponent = GetComponentByIdConst(entity, componentId)) {
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
			std::cerr << "Error: Cannot clone invalid entity " << sourceEntity << std::endl;
			return 0;
		}

		EntityID newEntity = AddNewEntity();

		auto componentTypes = GetEntityComponents(sourceEntity);

		try {
			for (const auto& componentId : componentTypes) {
				auto creator = componentCreators.find(componentId);
				if (creator != componentCreators.end()) {
					creator->second(newEntity);

					if (auto* sourceComponent = GetComponentByIdConst(sourceEntity, componentId)) {
						if (auto* destComponent = GetComponentById(newEntity, componentId)) {
							nlohmann::json componentData = sourceComponent->Serialize();
							destComponent->Deserialize(componentData);

							CopyComponentResources(sourceEntity, newEntity, componentId);
						}
					}
				}
			}

			std::cout << "Successfully cloned entity " << sourceEntity << " to " << newEntity << std::endl;
			return newEntity;
		}
		catch (const std::exception& e) {
			std::cerr << "Error cloning entity " << sourceEntity << ": " << e.what() << std::endl;
			DestroyEntity(newEntity);
			return 0;
		}
	}

	EntityID EntityManager::DeserializeEntity(const nlohmann::json& json) {
		if (!json.contains("components") || !json["components"].is_array()) {
			std::cerr << "Error: Invalid entity data format in JSON" << std::endl;
			return 0;
		}

		EntityID entity = AddNewEntity();

		try {
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
		catch (const std::exception& e) {
			std::cerr << "Error deserializing entity: " << e.what() << std::endl;
			DestroyEntity(entity);
			return 0;
		}
	}

	void EntityManager::DeserializeEntity(const nlohmann::json& json, const EntityID entity) {
		if (!IsEntityValid(entity)) {
			std::cerr << "Error: Cannot deserialize to invalid entity " << entity << std::endl;
			return;
		}

		if (!json.contains("components") || !json["components"].is_array()) {
			std::cerr << "Error: Invalid entity data format in JSON" << std::endl;
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
			std::cerr << "Error deserializing to entity " << entity << ": " << e.what() << std::endl;
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

		std::cout << "[EntityManager] Registered plugin component with ID: " << typeId
			<< " (size: " << componentSize << " bytes)" << std::endl;
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

				std::cout << "[EntityManager] Added plugin component to entity " << entity
					<< " with type ID: " << typeId << std::endl;
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

			std::cout << "[EntityManager] Removed plugin component from entity " << entity
				<< " with type ID: " << typeId << std::endl;
		}
	}

	bool EntityManager::HasPluginComponent(EntityID entity, ComponentTypeID typeId) {
		return GetPluginComponent(entity, typeId) != nullptr;
	}

	void EntityManager::RegisterPluginSystem(SystemTypeID typeId,
		std::function<void*(EntityManager*)> creator,
		std::function<void(void*)> destructor,
		std::function<void(void*, float)> updater,
		std::function<void(void*)> starter,
		const std::vector<ComponentTypeID>& requiredComponents) {

		std::cout << "[EntityManager] Registering plugin system ID: " << typeId
			<< " with " << requiredComponents.size() << " required components" << std::endl;

		void* systemInstance = creator(this);
		if (!systemInstance) {
			std::cerr << "[EntityManager] Failed to create plugin system with ID: " << typeId << std::endl;
			return;
		}

		std::cout << "[EntityManager] Plugin system instance created successfully" << std::endl;

		PluginSystemInfo systemInfo;
		systemInfo.instance = systemInstance;
		systemInfo.destructor = destructor;
		systemInfo.updater = updater;
		systemInfo.requiredComponents = requiredComponents;

		int matchingEntities = 0;
		for (const auto&[entityId, entitySignature] : entitiesSignatures) {
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

		std::cout << "[EntityManager] Plugin system stored with ID: " << typeId << std::endl;

		if (starter) {
			std::cout << "[EntityManager] Starting plugin system..." << std::endl;
			starter(systemInstance);
		}

		std::cout << "[EntityManager] Registered plugin system with ID: " << typeId
			<< " requiring " << requiredComponents.size() << " components"
			<< " with " << matchingEntities << " initial entities" << std::endl;
	}

	void* EntityManager::GetPluginSystem(SystemTypeID typeId) {
		auto it = pluginSystems.find(typeId);
		return (it != pluginSystems.end()) ? it->second.instance : nullptr;
	}

	void EntityManager::UpdatePluginSystems(float deltaTime) {
		for (auto&[systemId, systemInfo] : pluginSystems) {
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
		std::cout << "Registered Component Types:" << std::endl;
		auto names = GetAllRegisteredComponentNames();
		for (const auto& name : names) {
			ComponentTypeID id = GetComponentTypeIdByName(name);
			std::cout << "  - " << name << " (ID: " << id << ")" << std::endl;
		}
	}

	void EntityManager::DebugPrintEntityComponents(EntityID entity) const {
		std::cout << "Entity " << entity << " raw components (" << GetEntityComponents(entity).size() << "):" << std::endl;
		for (const auto& compId : GetEntityComponents(entity)) {
			std::string name = GetComponentNameById(compId);
			std::cout << "  - ID: " << compId << " (" << name << ")" << std::endl;
		}
	}

	void EntityManager::DebugPrintPluginSystems() const {
		std::cout << "=== PLUGIN SYSTEMS DEBUG ===" << std::endl;
		std::cout << "Total plugin systems registered: " << pluginSystems.size() << std::endl;

		for (const auto&[systemId, systemInfo] : pluginSystems) {
			auto nameIt = m_systemIDToName.find(systemId);
			std::string systemName = (nameIt != m_systemIDToName.end()) ? nameIt->second : "Unknown";
			std::cout << "\nPlugin System ID: " << systemId << " (" << systemName << ")" << std::endl;
			std::cout << "  Instance: " << systemInfo.instance << std::endl;
			std::cout << "  Required components: " << systemInfo.requiredComponents.size() << std::endl;
			for (ComponentTypeID compId : systemInfo.requiredComponents) {
				std::cout << "    - " << GetComponentNameById(compId)
					<< " (ID: " << compId << ")" << std::endl;
			}
			std::cout << "  Entities: " << systemInfo.entities.size() << std::endl;
		}
		std::cout << "=========================" << std::endl;
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
		for (auto&[systemId, systemInfo] : pluginSystems) {
			bool matches = true;
			for (ComponentTypeID compType : systemInfo.requiredComponents) {
				if (entitySignature->count(compType) == 0) {
					matches = false;
					break;
				}
			}

			if (matches) {
				if (systemInfo.entities.insert(entity).second) {
					std::cout << "[EntityManager] Added entity " << entity
						<< " to plugin system " << systemId << std::endl;
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

			if (sourceComp && destComp) {
				// Component-specific resource copying logic here
			}
		}
	}

} // namespace ECS