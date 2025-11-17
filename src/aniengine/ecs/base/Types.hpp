#pragma once
#include <cstdint>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <vector>
#include <iostream>

#ifdef _WIN32
#ifdef BUILDING_ANISTUDIO
#define REGISTRY_API __declspec(dllexport)
#else
#define REGISTRY_API __declspec(dllimport)
#endif
#else
#define REGISTRY_API
#endif

namespace ECS {
	class BaseComponent;
	class BaseSystem;

	using EntityID = size_t;
	using ComponentTypeID = size_t;
	using SystemTypeID = size_t;

	constexpr size_t MAX_ENTITY_COUNT = 10000;
	constexpr size_t MAX_COMPONENT_COUNT = 100;
	constexpr size_t MAX_SYSTEM_COUNT = 50;

	using EntitySignature = std::unordered_set<ComponentTypeID>;

	// Simple dummy function - or remove CompType usage entirely
	template <typename T>
	inline static const ComponentTypeID CompType() noexcept {
		return 0;
	}
} // namespace ECS