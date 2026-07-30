// UpscaleView.cpp
#include "UpscaleView.hpp"

using namespace ECS;

namespace GUI {

    std::vector<std::string> UpscaleView::GetDefaultComponents() const {
        return {
            "InputImageComponent",
            "OutputImageComponent",
            "EsrganComponent"
        };
    }

} // namespace GUI