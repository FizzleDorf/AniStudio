#include "EntityManager.hpp"
#include <iostream>
#include <cassert>

namespace ECS {

	EntityManager::EntityManager() : entityCount(0) {
		Reset();
	}

	EntityManager::~EntityManager() {}

	void EntityManager::Update(const float deltaT) {
		for (auto& system : registeredSystems) {
			system.second->Update(deltaT);
		}
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
		for (auto& system : registeredSystems) {
			AddEntityToSystem(entity, system.second.get());
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