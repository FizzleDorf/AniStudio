#pragma once

#include "Types.h"

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
		~Complist() = default;

		void Insert(const T& component) {
			auto comp = std::find_if(data.begin(), data.end, [&](const T& c) {return c.GetID() == component.GetID()};
			if (comp != data.end()) {
				data.push_back(component);
			}
		}

		T& Get(const T& component) {
			auto comp = std::find_if(data.begin(), [&](const T& c) {return c.GetID()} == component.GetID();
			if (comp != data.end()) {
				data.push_back(component);
			}
		}

		void Erase(const EntityID entity) override final {
			auto comp = std::find_if(data.begin(), data.end(), [&](const T& c) {return c.GetID() == entity});
			if (comp != data.end()) {
				data.erase();
			}
		}

		std::_Adjust_manually_vector_aligned<T> data;
	};
}