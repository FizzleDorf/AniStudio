#pragma once

#include "GUI.h"
#include "pch.h"
#include "imgui_markdown.h"
#include "ImageUtils.hpp"
#include "OpenGLUtils.hpp"
#include <filesystem>
#include <fstream>
#include <vector>
#include <map>
#include <unordered_map>

namespace GUI {

    struct MarkdownDocument {
        std::string title;
        std::string content;
        std::string filePath;
        std::string directory;
        bool isLoaded = false;
    };

    struct ImageCache {
        GLuint textureID = 0;
        int width = 0;
        int height = 0;
        bool loaded = false;
    };

    class HelpView : public BaseView {
    public:
        static constexpr const char* GetMetadataJSON() {
            return R"({
            "displayName": "Documentation",
            "category": "Help",
            "description": "Documentation for development and usage."
        })";
        }

        HelpView(ECS::EntityManager& mgr, ViewManager& vm)
            : BaseView(mgr, vm) {
            viewName = "HelpView";
        }
        ~HelpView() {
            CleanupImageCache();
        }

        void Init() override;
        void Render() override;
        nlohmann::json Serialize() const override;
        void Deserialize(const nlohmann::json& j) override;

    private:
        std::vector<MarkdownDocument> documents;
        std::map<std::string, size_t> documentIndex;
        int selectedDocumentIndex = 0;

        std::unordered_map<std::string, ImageCache> imageCache;

        bool showNavigationPanel = true;
        float navigationWidth = 200.0f;
        std::string searchFilter;
        char searchBuffer[256] = { 0 };

        ImGui::MarkdownConfig markdownConfig;

        ImFont* headerFont1 = nullptr;
        ImFont* headerFont2 = nullptr;
        ImFont* headerFont3 = nullptr;

        void LoadDocuments();
        void LoadMarkdownFile(const std::filesystem::path& filePath);
        void RenderNavigationPanel();
        void RenderMarkdownContent();
        void SetupMarkdownConfig();
        bool MatchesSearchFilter(const MarkdownDocument& doc) const;
        void CleanupImageCache();

        GLuint LoadImageForMarkdown(const std::string& imagePath, const std::string& documentDir);

        void OpenExternalLink(const std::string& url);
        void HandleInternalLink(const std::string& link);

        static void LinkCallback(ImGui::MarkdownLinkCallbackData data);
        static ImGui::MarkdownImageData ImageCallback(ImGui::MarkdownLinkCallbackData data);
        static void FormatCallback(const ImGui::MarkdownFormatInfo& markdownFormatInfo, bool start);
    };

} // namespace GUI