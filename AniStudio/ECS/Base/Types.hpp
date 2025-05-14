#pragma once
#include <set>
#include <typeindex>
#include <unordered_map>
#include <string>
#include <vector>
#include <iostream>

namespace ECS {
	class BaseSystem;
	struct BaseComponent;

	// Constants
	const size_t MAX_ENTITY_COUNT = 5000;
	const size_t MAX_COMPONENT_COUNT = 32;

	// Custom Types
	using EntityID = size_t;
	using SystemTypeID = size_t;
	using ComponentTypeID = size_t;
	using EntitySignature = std::set<ComponentTypeID>;

	// Component Type Registry for consistent IDs
	class ComponentTypeRegistry {
	private:
		static ComponentTypeID nextTypeID;
		static std::unordered_map<std::string, ComponentTypeID> nameToID;
		static std::unordered_map<ComponentTypeID, std::string> idToName;
		static std::unordered_map<std::type_index, ComponentTypeID> typeToID;

	public:
		// Register a component type with a name
		template <typename T>
		static ComponentTypeID RegisterType(const std::string& name) {
			// Check if this type is already registered
			std::type_index typeIdx = std::type_index(typeid(T));
			auto typeIt = typeToID.find(typeIdx);

			if (typeIt != typeToID.end()) {
				// Type already registered, return existing ID
				ComponentTypeID existingId = typeIt->second;

				// Add name as an alias if not already registered
				if (nameToID.find(name) == nameToID.end()) {
					nameToID[name] = existingId;
					std::cout << "Added alias '" << name << "' for existing component type ID: " << existingId << std::endl;
				}

				return existingId;
			}

			// Check if name is already used for a different type
			auto nameIt = nameToID.find(name);
			if (nameIt != nameToID.end()) {
				std::cerr << "Warning: Name '" << name << "' already used for component ID: "
					<< nameIt->second << ". Using existing ID." << std::endl;
				typeToID[typeIdx] = nameIt->second;
				return nameIt->second;
			}

			// Register new type
			ComponentTypeID newId = nextTypeID++;
			nameToID[name] = newId;
			idToName[newId] = name;
			typeToID[typeIdx] = newId;

			return newId;
		}

		// Get component ID by name
		static ComponentTypeID GetIDByName(const std::string& name) {
			auto it = nameToID.find(name);
			if (it != nameToID.end()) {
				return it->second;
			}
			return MAX_COMPONENT_COUNT; // Invalid ID
		}

		// Get component ID by type
		template <typename T>
		static ComponentTypeID GetIDByType() {
			std::type_index typeIdx = std::type_index(typeid(T));
			auto it = typeToID.find(typeIdx);
			if (it != typeToID.end()) {
				return it->second;
			}
			return MAX_COMPONENT_COUNT; // Invalid ID
		}

		// Get component name by ID
		static std::string GetNameByID(ComponentTypeID id) {
			auto it = idToName.find(id);
			if (it != idToName.end()) {
				return it->second;
			}
			return "Unknown";
		}

		// Check if a type is registered
		template <typename T>
		static bool IsTypeRegistered() {
			std::type_index typeIdx = std::type_index(typeid(T));
			return typeToID.find(typeIdx) != typeToID.end();
		}

		// Check if a name is registered
		static bool IsNameRegistered(const std::string& name) {
			return nameToID.find(name) != nameToID.end();
		}

		// Get all registered names
		static std::vector<std::string> GetAllNames() {
			std::vector<std::string> names;
			for (const auto& pair : nameToID) {
				names.push_back(pair.first);
			}
			return names;
		}

		// Reset registry
		static void Reset() {
			nextTypeID = 0;
			nameToID.clear();
			idToName.clear();
			typeToID.clear();
		}

		// Print registry state (for debugging)
		static void DebugPrint() {
			std::cout << "Component Type Registry State:" << std::endl;
			std::cout << "Total registered types: " << typeToID.size() << std::endl;

			for (const auto& pair : idToName) {
				std::cout << "ID: " << pair.first << " -> Name: " << pair.second << std::endl;
			}
		}
	};

	// Initialize static members
	inline ComponentTypeID ComponentTypeRegistry::nextTypeID = 0;
	inline std::unordered_map<std::string, ComponentTypeID> ComponentTypeRegistry::nameToID;
	inline std::unordered_map<ComponentTypeID, std::string> ComponentTypeRegistry::idToName;
	inline std::unordered_map<std::type_index, ComponentTypeID> ComponentTypeRegistry::typeToID;

	// Non-template function for system IDs
	static SystemTypeID GetRuntimeSystemTypeID() {
		static SystemTypeID typeID = 0u;
		return typeID++;
	}

	// Get component type ID - always use the registry
	template <typename T>
	inline static const ComponentTypeID CompType() noexcept {
		static_assert((std::is_base_of<BaseComponent, T>::value && !std::is_same<BaseComponent, T>::value),
			"INVALID COMPONENT TYPE");

		// Check if type is registered
		ComponentTypeID id = ComponentTypeRegistry::GetIDByType<T>();
		if (id != MAX_COMPONENT_COUNT) {
			return id;
		}

		// If not registered yet, register with C++ type name (should generally not happen)
		std::string typeName = typeid(T).name();
		return ComponentTypeRegistry::RegisterType<T>(typeName);
	}

	template <typename T>
	inline static const SystemTypeID SystemType() noexcept {
		static_assert((std::is_base_of<BaseSystem, T>::value && !std::is_same<BaseSystem, T>::value),
			"INVALID SYSTEM TYPE");
		static const SystemTypeID typeID = GetRuntimeSystemTypeID();
		return typeID;
	}
} // namespace ECS