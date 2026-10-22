#pragma once

#include "BaseView.hpp"
#include <nlohmann/json.hpp>
#include <string>

namespace GUI {

    class MetadataView : public BaseView {
    public:
        static constexpr const char* GetMetadataJSON() {
            return R"({
            "displayName": "Metadata Viewer",
            "category": "Viewers",
            "description": "View and inspect metadata from images and videos"
        })";
        }

        MetadataView(ECS::EntityManager& mgr, ViewManager& vm);
        ~MetadataView() = default;

        void Init() override;
        void Update(float deltaT) override;
        void Render() override;

        void LoadFromFile(const std::string& filePath);
        void LoadFromClipboard();
        void SaveMetadataToFile();
        void ClearMetadata();
        void PasteFromClipboard();

        void SetMetadata(const nlohmann::json& metadata, const std::string& source = "");

    private:
        enum class DisplayMode {
            Tree,
            Text
        };

        enum class MetadataSource {
            File,
            LSB
        };

        nlohmann::json metadata;
        std::string currentFile;
        DisplayMode displayMode = DisplayMode::Tree;
        std::string filterText;

        MetadataSource metadataSource = MetadataSource::File;
        nlohmann::json fileMetadata;
        nlohmann::json lsbMetadata;
        int nodeCounter = 0;

        int treeGeneration = 0;
        int treeForceOpen = 1;
        bool treeForceOpenPending = false;

        void RenderMenuBar();
        void RenderMetadataDisplay();
        void RenderJsonTree(const nlohmann::json& j, int depth = 0, const std::string& path = "");
        void RenderJsonValue(const nlohmann::json& value);
        void RenderCopyButton(const nlohmann::json& value);
        void RenderContextMenu();
        bool HasMatchingChild(const nlohmann::json& j, const std::string& lowerFilter);
        ImVec4 GetColorForType(const nlohmann::json& value);
        bool MatchesFilter(const std::string& text);
        void CopyValueToClipboard(const nlohmann::json& value);
        void ReadAllMetadataFromFile(const std::string& filePath);
        void ApplyMetadataSource();
        void ExpandEmbeddedJson(nlohmann::json& j);
        void CopyEntireEntityToClipboard();
    };

}