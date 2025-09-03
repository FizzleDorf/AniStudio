/*
		d8888          d8b  .d8888b.  888                  888 d8b
	   d88888          Y8P d88P  Y88b 888                  888 Y8P
	  d88P888              Y88b.      888                  888
	 d88P 888 88888b.  888  "Y888b.   888888 888  888  .d88888 888  .d88b.
	d88P  888 888 "88b 888     "Y88b. 888    888  888 d88" 888 888 d88""88b
   d88P   888 888  888 888       "888 888    888  888 888  888 888 888  888
  d8888888888 888  888 888 Y88b  d88P Y88b.  Y88b 888 Y88b 888 888 Y88..88P
 d88P     888 888  888 888  "Y8888P"   "Y888  "Y88888  "Y88888 888  "Y88P"

 * This file is part of AniStudio.
 * Copyright (C) 2025 FizzleDorf (AnimAnon)
 *
 * This software is dual-licensed under the GNU Lesser General Public License v3.0 (LGPL-3.0)
 * and a commercial license. You may choose to use it under either license.
 *
 * For the LGPL-3.0, see the LICENSE-LGPL-3.0.txt file in the repository.
 * For commercial license iformation, please contact legal@kframe.ai.
 */

#pragma once
#include <set>
#include <typeindex>
#include <unordered_map>
#include <string>
#include <iostream>
#include <atomic>

namespace GUI {
	class BaseView;

	const size_t MAX_VIEW_COUNT = 100;

	using WorkspaceID = size_t;
	using ViewTypeID = size_t;
	using ViewSignature = std::set<ViewTypeID>;

	// External counter - defined in ViewManager.cpp
	extern std::atomic<ViewTypeID> g_nextViewTypeId;

	// View Type Registry for consistent view IDs
	class ViewTypeRegistry {
	private:
		static std::unordered_map<std::type_index, ViewTypeID> typeToID;
		static std::unordered_map<ViewTypeID, std::string> idToName;

	public:
		// Register a view type with a name - EACH TYPE GETS UNIQUE ID
		template <typename T>
		static ViewTypeID RegisterType(const std::string& name) {
			std::type_index typeIdx = std::type_index(typeid(T));
			auto it = typeToID.find(typeIdx);

			if (it != typeToID.end()) {
				// Already registered, return existing ID
				ViewTypeID existingId = it->second;

				// Update name if different
				auto currentNameIt = idToName.find(existingId);
				if (currentNameIt != idToName.end() && currentNameIt->second != name) {
					idToName[existingId] = name;
					std::cout << "Updated view name: ID " << existingId
						<< " to '" << name << "'" << std::endl;
				}
				return existingId;
			}

			// Check if name is already used - ERROR
			for (const auto& pair : idToName) {
				if (pair.second == name) {
					std::cerr << "ERROR: View name '" << name << "' already used for ID: "
						<< pair.first << ". View names must be unique!" << std::endl;
					// Use fallback name
					std::string fallbackName = name + "_" + std::to_string(typeid(T).hash_code());
					return RegisterType<T>(fallbackName);
				}
			}

			// Register new view type with unique ID using external counter
			ViewTypeID newId = g_nextViewTypeId++;
			typeToID[typeIdx] = newId;
			idToName[newId] = name;

			std::cout << "Registered view: " << name << " with ID: " << newId << std::endl;
			return newId;
		}

		// Get view type ID
		template <typename T>
		static ViewTypeID GetIDByType() {
			std::type_index typeIdx = std::type_index(typeid(T));
			auto it = typeToID.find(typeIdx);
			if (it != typeToID.end()) {
				return it->second;
			}
			return MAX_VIEW_COUNT; // Invalid ID
		}

		// Get view name by ID
		static std::string GetNameByID(ViewTypeID id) {
			auto it = idToName.find(id);
			if (it != idToName.end()) {
				return it->second;
			}
			return "Unknown";
		}

		// Reset registry
		static void Reset() {
			typeToID.clear();
			idToName.clear();
		}
	};

	// Initialize static members
	inline std::unordered_map<std::type_index, ViewTypeID> ViewTypeRegistry::typeToID;
	inline std::unordered_map<ViewTypeID, std::string> ViewTypeRegistry::idToName;

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

	template <>
	inline static const ViewTypeID ViewType<BaseView>() noexcept {
		static const ViewTypeID INVALID_VIEW_TYPE_ID = SIZE_MAX;
		return INVALID_VIEW_TYPE_ID;
	}

	inline bool IsValidViewTypeID(ViewTypeID typeID) {
		return typeID != SIZE_MAX && typeID < MAX_VIEW_COUNT;
	}

} // namespace GUI