#pragma once

#include "BaseView.hpp"
#include "ContextMenuUtils.hpp"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <memory>

namespace GUI {

    class MetadataView : public BaseView {
    public:
        static constexpr const char* GetMetadataJSON() {
            return R"({
                "displayName": "Metadata Viewer",
                "category": "Tools",
                "description": "View and copy metadata from media, entities, and JSON files."
            })";
        }

        MetadataView(ECS::EntityManager& mgr, ViewManager& vm);
        ~MetadataView() = default;

        void Init() override;
        void Update(float deltaT) override;
        void Render() override;

        bool LoadFromFile(const std::string& filePath);
        bool LoadFromEntity(ECS::EntityID entityID);
        bool LoadFromJSON(const nlohmann::json& jsonData, const std::string& sourceName = "");
        void ClearMetadata();

        nlohmann::json Serialize() const override;
        void Deserialize(const nlohmann::json& data) override;

    private:
        enum class DisplayMode {
            Simplified,
            RawJSON
        };
        DisplayMode currentMode = DisplayMode::Simplified;

        nlohmann::json metadata;
        nlohmann::json displayMetadata;
        std::string sourcePath;
        std::string sourceDisplayName;
        std::string filterText;
        char filterBuffer[256];

        void RenderMenuBar();
        void RenderFileMenu();
        void RenderViewMenu();
        void RenderEditMenu();
        void RenderFilterBar();

        void RenderSimplifiedView();
        void RenderRawJSONView();

        void RenderJSONNode(const nlohmann::json& value, const std::string& key, const std::string& path);
        void RenderValue(const nlohmann::json& value, const std::string& key, const std::string& path);
        void RenderNodeContextMenu(const std::string& path);

        void CopyValue(const std::string& path);
        void CopyPath(const std::string& path);
        void CopyMetadataAsJSON();

        std::string ReadRawTextChunkFromPNG(const std::string& filePath, const std::string& key);
        std::vector<std::pair<std::string, std::string>> ReadAllTextChunksFromPNG(const std::string& filePath);
        nlohmann::json ParseRawMetadata(const std::string& rawText);
        nlohmann::json FlattenEntityMetadata(const nlohmann::json& input);
        nlohmann::json FilterJSONNode(const nlohmann::json& node, const std::string& filterLower);

        bool PasteFromClipboard();
        void HandleDropTarget();
        void UpdateDisplayMetadata();

        std::unique_ptr<Utils::ContextMenuUtils> contextMenuUtils;
    };

}