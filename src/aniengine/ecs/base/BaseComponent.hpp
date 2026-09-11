#pragma once

#include "Types.hpp"
#include "nlohmann/json.hpp"
#include <unordered_map>
#include <variant>
#include <string>
#include "PropertyTypes.hpp"

namespace ECS {
	// Parent for all components
	struct BaseComponent {

		// Names and Categories are defined in the compiler
		virtual const char* GetCompName() const { return "Base_Component";}
		virtual const char* GetCompCategory() const { return "";}
		
		// Schema for rendering the UI. This is static so it only
		//	Allocated after the first use and stays cached in the binary.
		//	Components that don't use the schema don't allocate.
		// TODO: at some point the schema rendering through json should
		//	be replaced
 		virtual const nlohmann::json& GetSchema() const {
			const static nlohmann::json j = "uiSchema";
			return j;
		}

		// Get property map for UI rendering
		virtual std::unordered_map<std::string, Engine::PropertyVariant> GetPropertyMap() {
			return {}; // Empty map by default
		}

		// Simple getter for the EntityID that owns this component.
		inline const EntityID GetID() const { return entityID; }

		BaseComponent() : entityID() {}
		virtual ~BaseComponent() {}

		// Serialize (write) to JSON
		virtual nlohmann::json Serialize() const {
			nlohmann::json j;
			j["compName"] = GetCompName();
			return j;
		}

		// Deserialize (read) from JSON
		virtual void Deserialize(const nlohmann::json& j) {
			if (j.contains(GetCompName())) {
				// parse params here
			}
		}

	private:
		friend class EntityManager; // Entity manager is a friend for protexted access.
		EntityID entityID;
	};
} // namespace ECS