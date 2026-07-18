#pragma once
#include "BaseComponent.hpp"
#include <string>
#include <imgui.h>

namespace ECS {

    class BaseSettingsComponent : public BaseComponent {
    public:
        virtual ~BaseSettingsComponent() = default;

        virtual std::string GetTabName() const = 0;
        virtual std::string GetTabCategory() const = 0;
        virtual int GetDisplayOrder() const { return 100; }

        virtual void RenderUI() = 0;
        virtual void RenderFilteredUI(const std::string& filter) {
            RenderUI();
        }

        virtual bool SaveSettings() = 0;
        virtual bool LoadSettings() = 0;
        virtual void ResetToDefaults() = 0;
        virtual void CreateBackup() = 0;
        virtual void RestoreFromBackup() = 0;
        virtual bool HasUnsavedChanges() const = 0;

        virtual void SetImGuiContext(ImGuiContext* context) {}

        nlohmann::json Serialize() const override {
            nlohmann::json j;
            j["compName"] = compName;
            return j;
        }
        void Deserialize(const nlohmann::json& j) override {}

    protected:
        static std::string GetSettingsDirectory() { return "../data/settings"; }
        static std::string GetDefaultsDirectory() { return "../data/defaults"; }
    };

} // namespace ECS