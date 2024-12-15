#pragma once

#include "Types.hpp"

namespace ECS {

	class BaseSystem {
	public:
		BaseSystem() = default;
		virtual ~BaseSystem() = default;
        
		void SetEntityManager(ECS::EntityManager &entityManager) { this->mgr = &entityManager; }

		void RemoveEntity(const EntityID entity) { entities.erase(entity); }

		void AddEntity(const EntityID entity) { entities.insert(entity); }

		const EntitySignature GetSignature() const {
			return signature;
		}

		template<typename T>
		void AddComponentSignature() {
			signature.insert(CompType<T>());
		}

		virtual void Start(){}
		virtual void Update(){}
		virtual void Render(){}
		virtual void Destroy(){}

		std::string GetSystemName() { return sysName; }

	protected:
		friend class EntityManager;
		EntitySignature signature;
		std::set<EntityID> entities;
        ECS::EntityManager *mgr = nullptr;
        std::string sysName = "System_Component";
	};
}