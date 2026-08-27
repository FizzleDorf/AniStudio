#pragma once

#include <cstdint> 
#include <variant>
#include <vector>
#include <string>
#include <unordered_map>

struct ImVec2;
struct ImVec4;

namespace Engine {

	// Centralized property variant types used by both ECS and UI systems
	using PropertyVariant = std::variant<
		bool*,
		int*,
		int64_t*,
		float*,
		double*,
		std::string*,
		ImVec2*,
		ImVec4*,
		std::vector<std::string>*,
		void*  // For component pointers and custom widget data
	>;

	using PropertyMap = std::unordered_map<std::string, PropertyVariant>;

} // namespace Engine

// Alias for backward compatibility with existing UISchema code
namespace UISchema {
	using PropertyVariant = Engine::PropertyVariant;
	using PropertyMap = Engine::PropertyMap;
} // namespace UISchema