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
#include "Types.hpp"
#include <set>
#include <string>

namespace ECS {

// Forward declaration
class EntityManager;

class BaseSystem {
public:
    BaseSystem(EntityManager &entityMgr) : mgr(entityMgr), sysName("System_Component") {}

    virtual ~BaseSystem() = default;

    void RemoveEntity(const EntityID entity) { entities.erase(entity); }
    void AddEntity(const EntityID entity) { entities.insert(entity); }

    const EntitySignature GetSignature() const { return signature; }

    template <typename T>
    void AddComponentSignature() {
        signature.insert(CompType<T>());
    }

    virtual void Start() {}
    virtual void Update(const float deltaT) {}
    virtual void Destroy() {}

    std::string GetSystemName() { return sysName; }

protected:
    friend class EntityManager;
    EntitySignature signature;
    std::set<EntityID> entities;
    std::string sysName;
    EntityManager &mgr;
};

} // namespace ECS