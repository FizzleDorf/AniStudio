#pragma once

#include "Types.hpp"
#include <deque>
#include <cassert>
#include <algorithm>

namespace ECS {

	class ICompList {
	public:
		ICompList() = default;
		virtual ~ICompList() = default;
		virtual void Erase(const EntityID entity) {}
		virtual void Reserve(size_t capacity) {}
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
			}
			else {
				std::cout << "Component already exists for entity: " << component.GetID() << std::endl;
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
			}
			else {
				std::cout << "No component found for entity: " << entity << std::endl;
			}
		}

		size_t Size() const {
			return data.size();
		}

		void Clear() {
			data.clear();
		}

		std::deque<T> data;
	};
} // namespace ECS