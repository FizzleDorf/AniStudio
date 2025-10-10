/*
		d8888          d8b  .d8888b.  888                  888 d8b
	   d88888          Y8P d88P  Y88b 888                  888 Y8P
	  d88P888              Y88b.      888                  888
	 d88P 888 88888b.  888  "Y888b.   888888 888  888  .d88888 888  .d88b.
	d88P  888 888 "88b 888     "Y88b. 888    888  888 d88" 888 888 d88""88b
   d88P   888 888  888 888       "888 888    888  888 888  888 888 888  888
  d8888888888 888  888 888 Y88b  d88P Y88b.  Y88b 888 Y88b 888 888 Y88..88P
 d88P     888 888  888 888  "Y8888P"   "Y888  "Y88888  "Y88888 888  "Y88P"
 */

#pragma once
#include <cstdint>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <vector>
#include <iostream>

namespace ECS {
	// Forward declarations
	class BaseComponent;
	class BaseSystem;

	// Type aliases
	using EntityID = size_t;
	using ComponentTypeID = size_t;
	using SystemTypeID = size_t;

	// Maximum counts
	constexpr size_t MAX_ENTITY_COUNT = 10000;
	constexpr size_t MAX_COMPONENT_COUNT = 100;
	constexpr size_t MAX_SYSTEM_COUNT = 50;

	// Signature types
	using EntitySignature = std::unordered_set<ComponentTypeID>;

	// Component Type Registry for consistent component IDs across modules
	class ComponentTypeRegistry {
	private:
		static ComponentTypeID nextTypeID;
		static std::unordered_map<std::string, ComponentTypeID> nameToID;
		static std::unordered_map<ComponentTypeID, std::string> idToName;
		static std::unordered_map<std::type_index, ComponentTypeID> typeToID;

	public:
		// Register a component type with name (for template types)
		template <typename T>
		static ComponentTypeID RegisterType(const std::string& name) {
			std::type_index typeIdx = std::type_index(typeid(T));

			// Check if type already registered
			auto it = typeToID.find(typeIdx);
			if (it != typeToID.end()) {
				std::cout << "[ComponentRegistry] Type already registered: " << name
					<< " with ID: " << it->second << ". Using existing ID." << std::endl;
				return it->second;
			}

			// Check if name already registered
			auto nameIt = nameToID.find(name);
			if (nameIt != nameToID.end()) {
				std::cout << "[ComponentRegistry] Name already registered: " << name
					<< " with ID: " << nameIt->second << ". Using existing ID." << std::endl;
				typeToID[typeIdx] = nameIt->second;
				return nameIt->second;
			}

			// Register new type
			ComponentTypeID newId = nextTypeID++;
			nameToID[name] = newId;
			idToName[newId] = name;
			typeToID[typeIdx] = newId;

			std::cout << "[ComponentRegistry] Registered NEW component type: " << name
				<< " with ID: " << newId << std::endl;

			return newId;
		}

		// NEW: Register plugin component by name only (no C++ type)
		static ComponentTypeID RegisterPluginComponent(const std::string& name) {
			// Check if name already registered
			auto nameIt = nameToID.find(name);
			if (nameIt != nameToID.end()) {
				std::cout << "[ComponentRegistry] Plugin component already registered: "
					<< name << " with ID: " << nameIt->second << std::endl;
				return nameIt->second;
			}

			// Register new plugin component with unique ID
			ComponentTypeID newId = nextTypeID++;
			nameToID[name] = newId;
			idToName[newId] = name;
			// DO NOT add to typeToID since there's no C++ type

			std::cout << "[ComponentRegistry] Registered NEW plugin component: "
				<< name << " with ID: " << newId << std::endl;

			return newId;
		}

		// NEW: Register a template type with an EXISTING plugin component ID
		// This allows CompType<T>() to return the same ID as GetIDByName(name)
		template <typename T>
		static void RegisterTypeWithExistingID(const std::string& name, ComponentTypeID existingId) {
			std::type_index typeIdx = std::type_index(typeid(T));

			// Map the C++ type to the existing plugin component ID
			typeToID[typeIdx] = existingId;

			std::cout << "[ComponentRegistry] Dual-registered type " << typeid(T).name()
				<< " with existing ID: " << existingId << " (name: " << name << ")" << std::endl;
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

	// System Type Registry for consistent system IDs
	class SystemTypeRegistry {
	private:
		static SystemTypeID nextTypeID;
		static std::unordered_map<std::type_index, SystemTypeID> typeToID;
		static std::unordered_map<std::string, SystemTypeID> nameToID;
		static std::unordered_map<SystemTypeID, std::string> idToName;

	public:
		// Register a system type and get unique ID (for template types)
		template <typename T>
		static SystemTypeID RegisterType() {
			std::type_index typeIdx = std::type_index(typeid(T));
			auto it = typeToID.find(typeIdx);

			if (it != typeToID.end()) {
				// Already registered, return existing ID
				return it->second;
			}

			// Register new system type
			SystemTypeID newId = nextTypeID++;
			typeToID[typeIdx] = newId;

			std::cout << "[SystemRegistry] Registered system type: " << typeid(T).name()
				<< " with ID: " << newId << std::endl;

			return newId;
		}

		// NEW: Register plugin system by name only (no C++ type)
		static SystemTypeID RegisterPluginSystem(const std::string& name) {
			// Check if name already registered
			auto nameIt = nameToID.find(name);
			if (nameIt != nameToID.end()) {
				std::cout << "[SystemRegistry] Plugin system already registered: "
					<< name << " with ID: " << nameIt->second << std::endl;
				return nameIt->second;
			}

			// Register new plugin system with unique ID
			SystemTypeID newId = nextTypeID++;
			nameToID[name] = newId;
			idToName[newId] = name;
			// DO NOT add to typeToID since there's no C++ type

			std::cout << "[SystemRegistry] Registered NEW plugin system: "
				<< name << " with ID: " << newId << std::endl;

			return newId;
		}

		// Get system type ID
		template <typename T>
		static SystemTypeID GetIDByType() {
			std::type_index typeIdx = std::type_index(typeid(T));
			auto it = typeToID.find(typeIdx);
			if (it != typeToID.end()) {
				return it->second;
			}

			// Auto-register if not found
			return RegisterType<T>();
		}

		// Get system ID by name
		static SystemTypeID GetIDByName(const std::string& name) {
			auto it = nameToID.find(name);
			if (it != nameToID.end()) {
				return it->second;
			}
			return MAX_SYSTEM_COUNT; // Invalid ID
		}

		// Get system name by ID
		static std::string GetNameByID(SystemTypeID id) {
			auto it = idToName.find(id);
			if (it != idToName.end()) {
				return it->second;
			}
			return "Unknown";
		}

		// Reset registry
		static void Reset() {
			nextTypeID = 0;
			typeToID.clear();
			nameToID.clear();
			idToName.clear();
		}

		// Debug print
		static void DebugPrint() {
			std::cout << "System Type Registry State:" << std::endl;
			std::cout << "Total registered systems: " << typeToID.size() << std::endl;
			std::cout << "Total registered plugin systems: " << nameToID.size() << std::endl;
			for (const auto& pair : idToName) {
				std::cout << "ID: " << pair.first << " -> Name: " << pair.second << std::endl;
			}
		}
	};

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

		return SystemTypeRegistry::GetIDByType<T>();
	}

} // namespace ECS