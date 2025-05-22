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
#include "pch.h"

namespace ECS {

class ICompList {
public:
    ICompList() = default;
    virtual ~ICompList() = default;
    virtual void Erase(const EntityID entity) {}
};

template <typename T>
class CompList : public ICompList {
public:
    CompList() = default;
    ~CompList() = default;

    void Insert(const T &component) {
        auto comp = std::find_if(data.begin(), data.end(), [&](const T &c) { return c.GetID() == component.GetID(); });
        if (comp == data.end()) {
            data.push_back(component);
            std::cout << "Component added! ID: " << component.GetID() 
                      << ", Type ID: " << CompType<T>() << std::endl;
        } else {
            std::cout << "Component already Exists! ID: " << component.GetID() << std::endl;
        }
    }

    T &Get(const EntityID entity) {
        auto comp = std::find_if(data.begin(), data.end(), [&](const T &c) { return c.GetID() == entity; });
        assert(comp != data.end() && "Component doesn't exist!");
        return *comp;
    }

    void Erase(const EntityID entity) override final {
        auto comp = std::find_if(data.begin(), data.end(), [&](const T &c) { return c.GetID() == entity; });
        if (comp != data.end()) {
            data.erase(comp);
            std::cout << "Component Erased! ID: " << entity << ", Type ID: " << CompType<T>() << std::endl;
        } else {
            std::cout << "No Entity Found with ID: " << entity << ", Type ID: " << CompType<T>() << std::endl;
        }
    }

    std::vector<T> data;
};
} // namespace ECS
