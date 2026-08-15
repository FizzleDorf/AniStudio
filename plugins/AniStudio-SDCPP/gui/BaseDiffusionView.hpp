// BaseDiffusionView.hpp
#pragma once

#include "BaseView.hpp"
#include "EntityManager.hpp"
#include "SDCPPComponents.h"
#include "Components.h"
#include "ContextMenuUtils.hpp"
#include "ClipboardUtilities.hpp"
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

        virtual bool UseStateActiveSeparation() const { return true; }

        void QuickSave();
        void QuickLoad();
        void SaveMetadataToJson(const std::string& filepath);
        void LoadMetadataFromJson(const std::string& filepath);
        void LoadMetadataFromMedia(const std::string& filePath);

    protected:
        ECS::EntityID stateEntity = 0;
        ECS::EntityID activeEntity = 0;
        std::unique_ptr<Utils::ContextMenuUtils> contextMenuUtils;
        bool m_quickLoaded = false;

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

        std::vector<std::string> GetAllComponentNames() const;
        void AddComponentByName(ECS::EntityID entity, const std::string& name);
    };

} // namespace GUI