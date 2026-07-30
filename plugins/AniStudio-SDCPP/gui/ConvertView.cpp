// ConvertView.cpp
#include "ConvertView.hpp"

using namespace ECS;

namespace GUI {

    std::vector<std::string> ConvertView::GetDefaultComponents() const {
        return {
            "CheckpointComponent",
            "VaeComponent",
            "ConversionComponent"
        };
    }

} // namespace GUI