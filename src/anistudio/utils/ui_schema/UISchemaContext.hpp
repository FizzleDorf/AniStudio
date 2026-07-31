#pragma once

#include <string>
#include <sstream>
#include <unordered_map>

namespace UISchema {

    struct UIRenderContext {
        std::string componentName;
        int entityNumber;
        const std::unordered_map<std::string, std::string>* pathMap = nullptr;

        UIRenderContext(const std::string& compName = "", int entity = 0,
            const std::unordered_map<std::string, std::string>* map = nullptr)
            : componentName(compName), entityNumber(entity), pathMap(map) {
        }

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