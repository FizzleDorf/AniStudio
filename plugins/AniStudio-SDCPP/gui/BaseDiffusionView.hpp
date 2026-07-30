// BaseDiffusionView.hpp
#pragma once

#include "BaseView.hpp"
#include "EntityManager.hpp"
#include "SDCPPComponents.h"
#include "Components.h"
#include "ContextMenuUtils.hpp"
#include <memory>
#include <unordered_map>
#include <string>
#include <vector>

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

        void ToggleComponent(const std::string& name);
        bool IsComponentActive(const std::string& name) const;
        void SetComponentActive(const std::string& name, bool active);

        virtual bool UseStateActiveSeparation() const { return true; }

        // Quick save/load
        void QuickSave();
        void QuickLoad();
        void SaveMetadataToJson(const std::string& filepath);
        void LoadMetadataFromJson(const std::string& filepath);
        void LoadMetadataFromPNG(const std::string& pngPath);

    protected:
        ECS::EntityID stateEntity = 0;
        ECS::EntityID activeEntity = 0;
        std::unordered_map<std::string, bool> componentVisibility;
        std::unique_ptr<Utils::ContextMenuUtils> contextMenuUtils;

        virtual std::vector<std::string> GetDefaultComponents() const = 0;
        virtual std::vector<std::string> GetDefaultVisibleComponents() const;

        void InitializeBase();
        void CopyComponentToActive(const std::string& name);
        void RemoveComponentFromActive(const std::string& name);
        void SyncComponentToState(ECS::ComponentTypeID compId);
        void RenderComponent(ECS::ComponentTypeID compId, const std::string& name);
        void RenderComponentsUI();

        void RenderMainContextMenu();
        void RenderMenuBar();

    private:
        std::unordered_map<std::string, std::function<void(ECS::EntityID)>> m_componentAdders;
        void RegisterAllComponentAdders();
    };

} // namespace GUI