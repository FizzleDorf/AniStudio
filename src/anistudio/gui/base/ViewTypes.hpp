#pragma once
#include <set>
#include <typeindex>
#include <unordered_map>
#include <string>
#include <iostream>
#include <mutex>

#ifdef _WIN32
#ifdef BUILDING_ANISTUDIO
#define REGISTRY_API __declspec(dllexport)
#else
#define REGISTRY_API __declspec(dllimport)
#endif
#else
#define REGISTRY_API
#endif

namespace GUI {
	class BaseView;

	const size_t MAX_VIEW_COUNT = 100;

	using WorkspaceID = size_t;
	using ViewTypeID = size_t;
	using ViewSignature = std::set<ViewTypeID>;

	// Dummy function - never called
	template <typename T>
	inline static const ViewTypeID ViewType() noexcept {
		return SIZE_MAX;
	}

	template <>
	inline const ViewTypeID ViewType<BaseView>() noexcept {
		return SIZE_MAX;
	}

	inline bool IsValidViewTypeID(ViewTypeID typeID) {
		return typeID != SIZE_MAX && typeID < MAX_VIEW_COUNT;
	}

} // namespace GUI