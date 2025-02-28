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

        void Update(const float deltaT) {
            for (auto& system : registeredSystems) {
                system.second->Update(deltaT);
            }
        }

        // Delegated to Viewmanager for adding custom user views
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
        T& AddComponent(const EntityID entity, Args &&...args) {
            assert(entity < MAX_ENTITY_COUNT && "EntityID out of range!");
            assert(GetEntitiySignature(entity)->size() < MAX_COMPONENT_COUNT && "Component count limit reached!");

            T component(std::forward<Args>(args)...);
            component.entityID = entity;
            GetEntitiySignature(entity)->insert(CompType<T>());
            GetCompList<T>()->Insert(component);
            UpdateEntityTargetSystem(entity);
            return GetCompList<T>()->Get(entity);
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

        template <typename T>
        void RegisterSystem() {
            const SystemTypeID systemType = SystemType<T>();
            assert(registeredSystems.count(systemType) == 0 && "System already registered!");
            auto system = std::make_shared<T>(*this); // Pass EntityManager reference

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
            for (auto& entitySignaturePair : entitiesSignatures) {
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
            for (const auto& pair : entitiesSignatures) {
                entities.push_back(pair.first);
            }
            return entities;
        }

        std::vector<ComponentTypeID> GetEntityComponents(EntityID entity) const {
            assert(entity < MAX_ENTITY_COUNT && "EntityID out of range!");
            auto it = entitiesSignatures.find(entity);
            if (it != entitiesSignatures.end()) {
                const EntitySignature& signature = *(it->second);
                return { signature.begin(), signature.end() };
            }
            return {};
        }

        // Method to register a component type with its name
        template <typename T>
        void RegisterComponentName(const std::string& name) {
            ComponentTypeID typeId = CompType<T>();
            componentNameToId[name] = typeId;
            componentIdToName[typeId] = name;

            // Also register it as a built-in component
            RegisterComponent<T>();
        }

        // Method to get a component's type ID by name
        ComponentTypeID GetComponentTypeIdByName(const std::string& name) const {
            auto it = componentNameToId.find(name);
            if (it != componentNameToId.end()) {
                return it->second;
            }
            return MAX_COMPONENT_COUNT; // Use this as INVALID_COMPONENT_ID
        }

        // Method to get a component's name by type ID
        std::string GetComponentNameById(ComponentTypeID typeId) const {
            auto it = componentIdToName.find(typeId);
            if (it != componentIdToName.end()) {
                return it->second;
            }
            return "Unknown";
        }

        nlohmann::json SerializeEntity(const EntityID entity) {
            nlohmann::json entityJson;

            entityJson["ID"] = entity;
            entityJson["components"] = nlohmann::json::array();

            auto componentTypes = GetEntityComponents(entity);
            for (const auto& componentId : componentTypes) {
                if (auto* baseComponent = GetComponentById(entity, componentId)) {
                    entityJson["components"].push_back(baseComponent->Serialize());
                }
            }

            return entityJson; // Don't forget to return the JSON object
        }

        EntityID DeserializeEntity(const nlohmann::json& json) {
            if (!json.contains("components") || !json["components"].is_array()) {
                std::cerr << "Error: Invalid entity data format in JSON" << std::endl;
                return 0;
            }

            EntityID entity = AddNewEntity();

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

        // Plugin support for component registration
        using ComponentCreator = std::function<void(EntityID)>;
        using ComponentGetter = std::function<BaseComponent* (EntityID)>;

        void RegisterComponentType(ComponentTypeID typeId, ComponentCreator creator, ComponentGetter getter) {
            componentCreators[typeId] = creator;
            componentGetters[typeId] = getter;
        }

        template <typename T>
        void RegisterComponent() {
            ComponentTypeID typeId = CompType<T>();
            RegisterComponentType(
                typeId, [this](EntityID entity) { this->AddComponent<T>(entity); },
                [this](EntityID entity) -> BaseComponent* {
                    if (this->HasComponent<T>(entity)) {
                        return &this->GetComponent<T>(entity);
                    }
                    return nullptr;
                });
        }

        void SaveWorkflow(const std::string& filepath) {
            nlohmann::json workflowJson;
            workflowJson["entities"] = nlohmann::json::array();

            for (const auto& entity : GetAllEntities()) {
                workflowJson["entities"].push_back(SerializeEntity(entity));
            }

            std::ofstream file(filepath);
            file << workflowJson.dump(4);
        }

        void LoadWorkflow(const std::string& filepath) {
            std::ifstream file(filepath);
            if (!file.is_open()) {
                return;
            }

            try {
                nlohmann::json workflowJson;
                file >> workflowJson;

                Reset();

                if (workflowJson.contains("entities")) {
                    for (const auto& entityJson : workflowJson["entities"]) {
                        DeserializeEntity(entityJson);
                    }
                }
            }
            catch (const std::exception& e) {
                std::cerr << "Error loading workflow: " << e.what() << std::endl;
            }
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

        BaseComponent* GetComponentById(EntityID entity, ComponentTypeID typeId) {
            auto getter = componentGetters.find(typeId);
            if (getter != componentGetters.end()) {
                return getter->second(entity);
            }
            return nullptr;
        }

        EntityID entityCount;
        std::queue<EntityID> availableEntities;
        std::map<EntityID, std::shared_ptr<EntitySignature>> entitiesSignatures;
        std::map<SystemTypeID, std::shared_ptr<BaseSystem>> registeredSystems;
        std::map<ComponentTypeID, std::shared_ptr<ICompList>> componentsArrays;
        std::unordered_map<ComponentTypeID, ComponentCreator> componentCreators;
        std::unordered_map<ComponentTypeID, ComponentGetter> componentGetters;
        // Map component names to their type IDs
        std::unordered_map<std::string, ComponentTypeID> componentNameToId;
        // Map type IDs to component names
        std::unordered_map<ComponentTypeID, std::string> componentIdToName;
    };
}