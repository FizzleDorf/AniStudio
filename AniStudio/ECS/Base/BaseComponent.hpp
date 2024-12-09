#pragma once

#include "Types.hpp"
#include "nlohmann/json.hpp"

namespace ECS {
	struct BaseComponent {
		std::string name = "Base_Component";
		BaseComponent() : entityID() {}
		virtual ~BaseComponent() {}
		inline const EntityID GetID() const { return entityID;}
        virtual std::string Serialize() { return NULL; }
	private:
		friend class EntityManager; //fren class!
		EntityID entityID;
	};
}