// BaseDiffusionView.hpp
#pragma once

#include "BaseView.hpp"
#include "EntityManager.hpp"
#include "SDCPPComponents.h"
#include "AniEngineComponents.hpp"
#include "AniStudioComponents.hpp"
#include "ContextMenuUtils.hpp"
#include "ClipboardUtilities.hpp"
#include "Log.hpp"
#include <memory>
#include <unordered_map>
#include <string>
#include <vector>
#include <array>
#include <functional>

namespace GUI {

    class BaseDiffusionView : public BaseView {
    public:
        BaseDiffusionView(ECS::EntityManager& mgr, ViewManager& vm);
        virtual ~BaseDiffusionView();

        virtual void Init() override;
        virtual void Render() override final;
        virtual nlohmann::json Serialize() const override;
        virtual void Deserialize(const nlohmann::json& j) override;

        ECS::EntityID GetActiveEntity() const { return activeEntity; }
        virtual std::string GetTaskType() const = 0;

        void QuickSave();
        void QuickLoad();
        void SaveMetadataToJson(const std::string& filepath);
        void LoadMetadataFromJson(const std::string& filepath);
        void LoadMetadataFromMedia(const std::string& filePath);

    protected:
        ECS::EntityID activeEntity = 0;
        std::unique_ptr<Utils::ContextMenuUtils> contextMenuUtils;
        bool m_quickLoaded = false;
        bool m_hasRestoredState = false;

        // Kept as a virtual for the subclasses; the render path uses the
        // compile-time table instead of this list.
        virtual std::vector<std::string> GetDefaultComponents() const = 0;
        virtual std::vector<std::string> GetFilteredComponents() const { return {}; }
        virtual std::string GetDefaultModelComponent() const { return "Checkpoint"; }

        void ResetToDefaults();
        void RenderComponentsUI();
        void RenderMainContextMenu();
        void RenderMenuBar();

    private:
        struct ComponentEntry {
            const char* name;
            std::string category;
            std::function<void(ECS::EntityID)> add;
            std::function<bool(ECS::EntityID)> present;
            std::function<void(ECS::EntityID)> remove;
            std::function<void()> render;
        };

        std::vector<ComponentEntry> m_components;
        void BuildComponentTable();

        bool m_expandAllRequested = false;
        bool m_collapseAllRequested = false;

        void AddComponentByName(ECS::EntityID entity, const std::string& name);
        bool IsComponentFiltered(const std::string& name) const;

        void RenderAddRemoveComponentsMenu();

        template<typename T>
        void RenderComponentByType(const char* name);
    };

} // namespace GUI