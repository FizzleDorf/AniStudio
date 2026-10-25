#pragma once

#include "Types.hpp"
#include <unordered_map>
#include <cassert>

namespace ECS {

	class ICompList {
	public:
		ICompList() = default;
		virtual ~ICompList() = default;
		virtual void Erase(const EntityID entity) = 0;
		virtual void Reserve(size_t capacity) {}
	};

	template <typename T>
	class CompList : public ICompList {
	public:
		CompList() = default;
		~CompList() = default;

		void Insert(const EntityID entity, const T& component) {
			data[entity] = component;
		}

		T& Get(const EntityID entity) {
			auto it = data.find(entity);
			assert(it != data.end() && "Component doesn't exist!");
			return it->second;
		}

		void Erase(const EntityID entity) override final {
			data.erase(entity);
		}

		size_t Size() const { return data.size(); }
		void Clear() { data.clear(); }

		std::unordered_map<EntityID, T> data;
	};
}