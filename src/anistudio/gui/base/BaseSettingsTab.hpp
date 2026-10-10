#pragma once
#include <string>

namespace ECS {

    class BaseSettingsTab {
    public:
        virtual ~BaseSettingsTab() = default;

        virtual std::string GetTabName() const = 0;
        virtual std::string GetTabCategory() const = 0;
        virtual void Render() = 0;
        virtual void SetFilter(const std::string& filter) = 0;
        virtual void SetImGuiContext(void* ctx) { (void)ctx; }
    };

}