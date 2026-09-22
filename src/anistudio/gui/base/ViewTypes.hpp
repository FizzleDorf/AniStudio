#pragma once
#include "Log.hpp"
#include <set>
#include <typeindex>
#include <unordered_map>
#include <string>
#include <atomic>
#include <mutex>
#include <cstdint>

namespace GUI {
	class BaseView;

	const size_t MAX_VIEW_COUNT = 100;

	using WorkspaceID = size_t;
	using ViewTypeID = size_t;
	using ViewSignature = std::set<ViewTypeID>;

	// View Type Registry for consistent view IDs - SAME PATTERN AS COMPONENTS/SYSTEMS
	class ViewTypeRegistry {
	private:
		static ViewTypeID nextTypeID;
		static std::unordered_map<std::string, ViewTypeID> nameToID;
		static std::unordered_map<ViewTypeID, std::string> idToName;
		static std::unordered_map<std::type_index, ViewTypeID> typeToID;
		static std::mutex mutex;

	public:
		// Register a view type with a name - SAME AS ComponentTypeRegistry
		template <typename T>
		static ViewTypeID RegisterType(const std::string& name) {
			std::lock_guard<std::mutex> lock(mutex);

			// Check if this type is already registered
			std::type_index typeIdx = std::type_index(typeid(T));
			auto typeIt = typeToID.find(typeIdx);

			if (typeIt != typeToID.end()) {
				// Type already registered, return existing ID
				ViewTypeID existingId = typeIt->second;

				// Add name as an alias if not already registered
				if (nameToID.find(name) == nameToID.end()) {
					nameToID[name] = existingId;
					ANI_LOG_DEBUG("[ViewTypeRegistry] Added alias '%s' for existing view type ID: %u",
						name.c_str(), (unsigned)existingId);
				}

				return existingId;
			}

			// Check if name is already used for a different type
			auto nameIt = nameToID.find(name);
			if (nameIt != nameToID.end()) {
				ANI_LOG_WARN("[ViewTypeRegistry] Name '%s' already used for view ID %u. Using existing ID.",
					name.c_str(), (unsigned)nameIt->second);
				typeToID[typeIdx] = nameIt->second;
				return nameIt->second;
			}

			// Register new type - USE ATOMIC COUNTER LIKE COMPONENTS/SYSTEMS
			ViewTypeID newId = nextTypeID++;
			nameToID[name] = newId;
			idToName[newId] = name;
			typeToID[typeIdx] = newId;

			ANI_LOG_DEBUG("[ViewTypeRegistry] Registered view: %s with ID: %u",
				name.c_str(), (unsigned)newId);
			return newId;
		}

		// Register a view type by name only (for plugins)
		static ViewTypeID RegisterTypeByName(const std::string& name) {
			std::lock_guard<std::mutex> lock(mutex);

			// Check if name is already used
			auto nameIt = nameToID.find(name);
			if (nameIt != nameToID.end()) {
				return nameIt->second; // Return existing ID
			}

			// Register new type with a unique ID
			ViewTypeID newId = nextTypeID++;
			nameToID[name] = newId;
			idToName[newId] = name;

			ANI_LOG_DEBUG("[ViewTypeRegistry] Registered view by name: %s with ID: %u",
				name.c_str(), (unsigned)newId);
			return newId;
		}

		// Get view ID by name
		static ViewTypeID GetIDByName(const std::string& name) {
			std::lock_guard<std::mutex> lock(mutex);
			auto it = nameToID.find(name);
			if (it != nameToID.end()) {
				return it->second;
			}
			return MAX_VIEW_COUNT; // Invalid ID
		}

		// Get view ID by type
		template <typename T>
		static ViewTypeID GetIDByType() {
			std::lock_guard<std::mutex> lock(mutex);
			std::type_index typeIdx = std::type_index(typeid(T));
			auto it = typeToID.find(typeIdx);
			if (it != typeToID.end()) {
				return it->second;
			}
			return MAX_VIEW_COUNT; // Invalid ID
		}

		// Get view name by ID
		static std::string GetNameByID(ViewTypeID id) {
			std::lock_guard<std::mutex> lock(mutex);
			auto it = idToName.find(id);
			if (it != idToName.end()) {
				return it->second;
			}
			return "Unknown";
		}

		// Check if a name is registered
		static bool IsNameRegistered(const std::string& name) {
			std::lock_guard<std::mutex> lock(mutex);
			return nameToID.find(name) != nameToID.end();
		}

		// Unregister a view type (for plugin cleanup)
		static bool UnregisterType(const std::string& name) {
			std::lock_guard<std::mutex> lock(mutex);
			auto nameIt = nameToID.find(name);
			if (nameIt == nameToID.end()) {
				ANI_LOG_TRACE("[ViewTypeRegistry] UnregisterType: name not registered: %s", name.c_str());
				return false;
			}

			ViewTypeID id = nameIt->second;

			// Remove from all registries
			nameToID.erase(nameIt);
			idToName.erase(id);

			// Remove from typeToID (need to find by value)
			for (auto it = typeToID.begin(); it != typeToID.end(); ) {
				if (it->second == id) {
					it = typeToID.erase(it);
				}
				else {
					++it;
				}
			}

			ANI_LOG_DEBUG("[ViewTypeRegistry] Unregistered view: %s with ID: %u",
				name.c_str(), (unsigned)id);
			return true;
		}

		// Reset registry
		static void Reset() {
			std::lock_guard<std::mutex> lock(mutex);
			ANI_LOG_INFO("[ViewTypeRegistry] Resetting registry (%zu views registered)",
				typeToID.size());
			nextTypeID = 0;
			nameToID.clear();
			idToName.clear();
			typeToID.clear();
		}

		// Debug print
		static void DebugPrint() {
			std::lock_guard<std::mutex> lock(mutex);
			ANI_LOG_DEBUG("View Type Registry State:");
			ANI_LOG_DEBUG("Total registered views: %zu", typeToID.size());
			for (const auto& pair : idToName) {
				ANI_LOG_TRACE("ID: %u -> Name: %s",
					(unsigned)pair.first, pair.second.c_str());
			}
		}
	};

	// Static member definitions - SAME PATTERN AS ComponentTypeRegistry
	inline ViewTypeID ViewTypeRegistry::nextTypeID = 0;
	inline std::unordered_map<std::string, ViewTypeID> ViewTypeRegistry::nameToID;
	inline std::unordered_map<ViewTypeID, std::string> ViewTypeRegistry::idToName;
	inline std::unordered_map<std::type_index, ViewTypeID> ViewTypeRegistry::typeToID;
	inline std::mutex ViewTypeRegistry::mutex;

	// Get view type ID - always use the registry
	template <typename T>
	inline static const ViewTypeID ViewType() noexcept {
		static_assert((std::is_base_of<BaseView, T>::value && !std::is_same<BaseView, T>::value),
			"INVALID VIEW TYPE: T must inherit from BaseView but cannot be BaseView itself.");

		// Check if type is registered
		ViewTypeID id = ViewTypeRegistry::GetIDByType<T>();
		if (id != MAX_VIEW_COUNT) {
			return id;
		}

		// If not registered yet, register with C++ type name
		std::string typeName = typeid(T).name();
		return ViewTypeRegistry::RegisterType<T>(typeName);
	}

	// FIXED: Template specialization syntax - removed "inline static"
	template <>
	const ViewTypeID ViewType<BaseView>() noexcept {
		static const ViewTypeID INVALID_VIEW_TYPE_ID = SIZE_MAX;
		return INVALID_VIEW_TYPE_ID;
	}

	inline bool IsValidViewTypeID(ViewTypeID typeID) {
		return typeID != SIZE_MAX && typeID < MAX_VIEW_COUNT;
	}

} // namespace GUI