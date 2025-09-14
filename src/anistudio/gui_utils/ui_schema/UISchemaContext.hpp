#pragma once

#include <string>
#include <sstream>

namespace UISchema {

	struct UIRenderContext {
		std::string componentName;
		int entityNumber;

		UIRenderContext(const std::string& compName = "", int entity = 0)
			: componentName(compName), entityNumber(entity) {}

		std::string GenerateWidgetId(const std::string& propertyName, const std::string& suffix = "") const {
			std::stringstream ss;
			ss << componentName << "_" << propertyName << "_" << entityNumber;
			if (!suffix.empty()) {
				ss << "_" << suffix;
			}
			return ss.str();
		}

		std::string GenerateUniqueLabel(const std::string& propertyName, const std::string& suffix = "") const {
			return propertyName + "##" + GenerateWidgetId(propertyName, suffix);
		}
	};

} // namespace UISchema