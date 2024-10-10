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

	template<typename T>
	class CompList : public ICompList {
	public:
		CompList() = default;
		~CompList() = default;

		void Insert(const T& component) {
			auto comp = std::find_if(data.begin(), data.end(), [&](const T& c) { return c.GetID() == component.GetID(); });
			if (comp == data.end()) { 
				data.push_back(component);
			}
			else {
				std::printf("Component already Exists!");
			}
		}

		T& Get(const EntityID entity) {
			auto comp = std::find_if(data.begin(), data.end(), [&](const T& c) { return c.GetID() == entity; });
			assert(comp != data.end() && "Component doesn't exist!");
			return *comp;
		}

		void Erase(const EntityID entity) override final {
			auto comp = std::find_if(data.begin(), data.end(), [&](const T& c) { return c.GetID() == entity; });
			if (comp != data.end()) {
				data.erase(comp);
			}
			else {
				std::cout << "No Entity Found!";
			}
		}

		std::vector<T> data;
	};
}
