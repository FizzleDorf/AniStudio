#pragma once

#include <nlohmann/json.hpp>
#include <vector>
#include <string>

namespace GUI {

	struct ProjectTemplate {
		std::string name;
		std::string description;
		std::string category;
		std::vector<std::string> defaultOpenViews;
		nlohmann::json settings;
	};

} // namespace GUI