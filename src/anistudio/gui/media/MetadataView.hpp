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

        nlohmann::json metadata;
        std::string currentFile;
        DisplayMode displayMode = DisplayMode::Tree;
        std::string filterText;

        void RenderMenuBar();
        void RenderMetadataDisplay();
        void RenderJsonTree(const nlohmann::json& j, int depth = 0);
        void RenderJsonValue(const nlohmann::json& value, const std::string& key = "", int depth = 0);
        void RenderCopyButton(const nlohmann::json& value, const std::string& label);
        void RenderContextMenu();
        bool HasMatchingChild(const nlohmann::json& j, const std::string& lowerFilter);
        ImVec4 GetColorForType(const nlohmann::json& value);
        bool MatchesFilter(const std::string& text);
        void CopyValueToClipboard(const nlohmann::json& value);
        nlohmann::json ReadMetadataFromFile(const std::string& filePath);
        void CopyEntireEntityToClipboard();
    };

}