#pragma once

#include "Types.hpp"
#include <deque>
#include <unordered_map>
#include <cassert>
#include <algorithm>
#include <iostream>

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

		void Insert(const T& component) {
			const EntityID id = component.GetID();

			auto it = index.find(id);
			if (it != index.end()) {
				data[it->second] = component;
				return;
			}

			index[id] = data.size();
			data.push_back(component);
		}

		T& Get(const EntityID entity) {
			auto it = index.find(entity);
			assert(it != index.end() && "Component doesn't exist!");
			return data[it->second];
		}

		void Erase(const EntityID entity) override final {
			auto it = index.find(entity);
			if (it == index.end()) {
				return;
			}

			const size_t slot = it->second;
			const size_t last = data.size() - 1;

			if (slot != last) {
				data[slot] = std::move(data[last]);
				index[data[slot].GetID()] = slot;
			}

			data.pop_back();
			index.erase(it);
		}

		size_t Size() const {
			return data.size();
		}

		void Clear() {
			data.clear();
			index.clear();
		}

		std::deque<T> data;
		std::unordered_map<EntityID, size_t> index;
	};
} // namespace ECS