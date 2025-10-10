#include "EntityManager.hpp"
#include <iostream>
#include <cassert>

namespace ECS {

	// STATIC MEMBER DEFINITIONS - These must be here to ensure single instance
	ComponentTypeID ComponentTypeRegistry::nextTypeID = 0;
	std::unordered_map<std::string, ComponentTypeID> ComponentTypeRegistry::nameToID;
	std::unordered_map<ComponentTypeID, std::string> ComponentTypeRegistry::idToName;
	std::unordered_map<std::type_index, ComponentTypeID> ComponentTypeRegistry::typeToID;

	SystemTypeID SystemTypeRegistry::nextTypeID = 0;
	std::unordered_map<std::type_index, SystemTypeID> SystemTypeRegistry::typeToID;
	std::unordered_map<std::string, SystemTypeID> SystemTypeRegistry::nameToID;
	std::unordered_map<SystemTypeID, std::string> SystemTypeRegistry::idToName;

	// Plugin Component Array - Manages raw memory components for plugins
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
			// Clean up all components
			for (auto&[entityId, ptr] : m_components) {
				if (ptr) {
					m_destructor(ptr);
					std::free(ptr);
				}
			}
		}

		void* Insert(EntityID entity) {
			// Remove existing component if present
			Erase(entity);

			// Allocate memory for component
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

	// NEW: Helper method to detect plugin components
	bool EntityManager::IsPluginComponent(ComponentTypeID typeId) {
		auto arrayIt = componentsArrays.find(typeId);
		if (arrayIt != componentsArrays.end()) {
			// If it's stored as PluginComponentArray, it's a plugin component
			return std::dynamic_pointer_cast<PluginComponentArray>(arrayIt->second) != nullptr;
		}
		return false;
	}

	EntityManager::EntityManager() : entityCount(0) {
		Reset();
	}

	EntityManager::~EntityManager() {
		// Destroy all plugin systems first
		for (auto&[systemId, systemInfo] : pluginSystems) {
			if (systemInfo.instance && systemInfo.destructor) {
				std::cout << "[EntityManager] Destroying plugin system ID: " << systemId << std::endl;
				systemInfo.destructor(systemInfo.instance);
			}
		}
		pluginSystems.clear();

		// Destroy all regular systems
		for (auto& system : registeredSystems) {
			if (system.second) {
				system.second->Destroy();
			}
		}
		registeredSystems.clear();
	}

	void EntityManager::Update(const float deltaT) {
		// Update regular systems
		for (auto& system : registeredSystems) {
			system.second->Update(deltaT);
		}

		// Update plugin systems
		UpdatePluginSystems(deltaT);
	}

	void EntityManager::Reset() {
		std::cout << "Resetting EntityManager..." << std::endl;

		// Call Destroy() on all systems before clearing them
		for (auto& system : registeredSystems) {
			if (system.second) {
				system.second->Destroy();
			}
		}

		// Clear all entity signatures
		entitiesSignatures.clear();

		// Clear all registered systems
		registeredSystems.clear();

		// Clear plugin systems
		for (auto&[systemId, systemInfo] : pluginSystems) {
			if (systemInfo.instance && systemInfo.destructor) {
				systemInfo.destructor(systemInfo.instance);
			}
		}
		pluginSystems.clear();

		// Clear all component arrays
		componentsArrays.clear();

		// Keep componentCreators and componentGetters to preserve registered component types
		// componentCreators.clear();
		// componentGetters.clear();

		// Reset entity queue
		while (!availableEntities.empty()) {
			availableEntities.pop();
		}
		for (EntityID entity = 0u; entity < MAX_ENTITY_COUNT; ++entity) {
			availableEntities.push(entity);
		}

		// Reset entity count
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

		// Remove from plugin systems
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

		// Destroy system instance
		if (systemInfo.instance && systemInfo.destructor) {
			std::cout << "[EntityManager] Destroying plugin system ID: " << systemId << std::endl;
			systemInfo.destructor(systemInfo.instance);
		}

		pluginSystems.erase(it);
		std::cout << "[EntityManager] Unregistered plugin system ID: " << systemId << std::endl;
	}

	void EntityManager::UnregisterPluginComponent(ComponentTypeID componentId) {
		// Remove from component arrays
		auto arrayIt = componentsArrays.find(componentId);
		if (arrayIt != componentsArrays.end()) {
			componentsArrays.erase(arrayIt);
		}

		// Remove from creators and getters
		componentCreators.erase(componentId);
		componentGetters.erase(componentId);

		std::cout << "[EntityManager] Unregistered plugin component ID: " << componentId << std::endl;
	}

	bool EntityManager::HasComponentById(const EntityID entity, ComponentTypeID componentId) {
		assert(entity < MAX_ENTITY_COUNT && "EntityID out of range!");
		// Check if the entity exists in the map
		auto it = entitiesSignatures.find(entity);
		if (it == entitiesSignatures.end()) {
			return false; // Entity signature not found
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
		return ComponentTypeRegistry::GetIDByName(name);
	}

	std::string EntityManager::GetComponentNameById(ComponentTypeID typeId) const {
		return ComponentTypeRegistry::GetNameByID(typeId);
	}

	std::vector<std::string> EntityManager::GetAllRegisteredComponentNames() const {
		return ComponentTypeRegistry::GetAllNames();
	}

	bool EntityManager::IsComponentNameRegistered(const std::string& name) const {
		return ComponentTypeRegistry::IsNameRegistered(name);
	}

	nlohmann::json EntityManager::SerializeEntity(const EntityID entity) const {
		nlohmann::json entityJson;

		entityJson["ID"] = entity;
		entityJson["components"] = nlohmann::json::array();

		auto componentTypes = GetEntityComponents(entity);
		for (const auto& componentId : componentTypes) {
			// Use const version to avoid modifying original
			if (auto* baseComponent = GetComponentByIdConst(entity, componentId)) {
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

	EntityID EntityManager::CloneEntity(const EntityID sourceEntity) {
		if (!IsEntityValid(sourceEntity)) {
			std::cerr << "Error: Cannot clone invalid entity " << sourceEntity << std::endl;
			return 0;
		}

		// Create new entity
		EntityID newEntity = AddNewEntity();

		// Get all components from source entity
		auto componentTypes = GetEntityComponents(sourceEntity);

		try {
			for (const auto& componentId : componentTypes) {
				// Create the component on new entity first
				auto creator = componentCreators.find(componentId);
				if (creator != componentCreators.end()) {
					creator->second(newEntity);

					// Now safely copy data from source to destination
					if (auto* sourceComponent = GetComponentByIdConst(sourceEntity, componentId)) {
						if (auto* destComponent = GetComponentById(newEntity, componentId)) {
							// Use JSON serialization for safe copying
							nlohmann::json componentData = sourceComponent->Serialize();
							destComponent->Deserialize(componentData);

							// Handle special cases for components with pointers/resources
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
			// Clean up failed entity
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
							// Create component first
							creator->second(entity);

							// Then deserialize data safely
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
						// Only create component if it doesn't exist
						if (!HasComponentById(entity, typeId)) {
							auto creator = componentCreators.find(typeId);
							if (creator != componentCreators.end()) {
								creator->second(entity);
							}
						}

						// Deserialize data to existing or new component
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

	// NEW: Plugin component registration
	void EntityManager::RegisterPluginComponent(ComponentTypeID typeId,
		size_t componentSize,
		std::function<void(void*, EntityID)> constructor,
		std::function<void(void*)> destructor) {

		// Create plugin component array
		auto pluginArray = std::make_shared<PluginComponentArray>(componentSize, constructor, destructor);
		componentsArrays[typeId] = pluginArray;

		// Register component creator and getter
		RegisterComponentType(
			typeId,
			[this, typeId, pluginArray](EntityID entity) {
			// Add component to entity signature
			GetEntitySignature(entity)->insert(typeId);
			// Insert into plugin array
			pluginArray->Insert(entity);
			// Update systems
			UpdateEntityTargetSystem(entity);
		},
			[pluginArray](EntityID entity) -> BaseComponent* {
			// Plugin components don't derive from BaseComponent, so return nullptr
			// The plugin system will use direct void* access
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
				// Add component to entity signature
				GetEntitySignature(entity)->insert(typeId);
				// Insert into plugin array
				void* component = pluginArray->Insert(entity);
				// Update systems (including plugin systems)
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

	// NEW: Plugin system registration
	void EntityManager::RegisterPluginSystem(SystemTypeID typeId,
		std::function<void*(EntityManager*)> creator,
		std::function<void(void*)> destructor,
		std::function<void(void*, float)> updater,
		std::function<void(void*)> starter,
		const std::vector<ComponentTypeID>& requiredComponents) {

		std::cout << "[EntityManager] Registering plugin system ID: " << typeId
			<< " with " << requiredComponents.size() << " required components" << std::endl;

		// Create system instance
		void* systemInstance = creator(this);
		if (!systemInstance) {
			std::cerr << "[EntityManager] Failed to create plugin system with ID: " << typeId << std::endl;
			return;
		}

		std::cout << "[EntityManager] Plugin system instance created successfully" << std::endl;

		// Store system info
		PluginSystemInfo systemInfo;
		systemInfo.instance = systemInstance;
		systemInfo.destructor = destructor;
		systemInfo.updater = updater;
		systemInfo.requiredComponents = requiredComponents;

		// Add existing entities that match system signature
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

		// Store the system
		pluginSystems[typeId] = std::move(systemInfo);

		std::cout << "[EntityManager] Plugin system stored with ID: " << typeId << std::endl;

		// Start the system
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

		// Additional registry debug info
		ComponentTypeRegistry::DebugPrint();
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
			std::string systemName = SystemTypeRegistry::GetNameByID(systemId);
			std::cout << "\nPlugin System ID: " << systemId << " (" << systemName << ")" << std::endl;
			std::cout << "  Instance: " << systemInfo.instance << std::endl;
			std::cout << "  Required components: " << systemInfo.requiredComponents.size() << std::endl;
			for (ComponentTypeID compId : systemInfo.requiredComponents) {
				std::cout << "    - " << ComponentTypeRegistry::GetNameByID(compId)
					<< " (ID: " << compId << ")" << std::endl;
			}
			std::cout << "  Entities: " << systemInfo.entities.size() << std::endl;
		}
		std::cout << "=========================" << std::endl;
	}

	// Private helper methods implementation

	void EntityManager::AddEntitySignature(const EntityID entity) {
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

	std::shared_ptr<EntitySignature> EntityManager::GetEntitySignature(const EntityID entity) {
		auto it = entitiesSignatures.find(entity);
		if (it == entitiesSignatures.end()) {
			// If no signature exists, create one
			AddEntitySignature(entity);
		}
		return entitiesSignatures.at(entity);
	}

	void EntityManager::UpdateEntityTargetSystem(const EntityID entity) {
		// Update regular systems
		for (auto& system : registeredSystems) {
			AddEntityToSystem(entity, system.second.get());
		}

		// Update plugin systems
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

	void EntityManager::AddEntityToSystem(const EntityID entity, BaseSystem* system) {
		if (IsEntityInSystem(entity, system->signature)) {
			system->entities.insert(entity);
		}
		else {
			system->entities.erase(entity);
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

		// Handle InputImageComponent and ImageComponent special cases
		if (componentName == "InputImage" || componentName == "Image") {
			// For image components, we need to handle the texture and image data
			auto sourceComp = GetComponentById(sourceEntity, componentId);
			auto destComp = GetComponentById(destEntity, componentId);

			if (sourceComp && destComp) {
				// Cast to appropriate types and handle resource copying
				// This is where you'd implement specific logic for each component type
				// For now, the JSON serialization should handle most cases
			}
		}
		// Add more special cases as needed for other component types
	}

} // namespace ECS