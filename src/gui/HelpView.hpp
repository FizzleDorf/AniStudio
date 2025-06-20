// HelpView.hpp
#pragma once

#include "Base/BaseView.hpp"
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
		std::string directory; // Directory containing the document for relative image paths
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
		HelpView(ECS::EntityManager& entityMgr);
		~HelpView();

		void Init() override;
		void Render() override;
		nlohmann::json Serialize() const override;
		void Deserialize(const nlohmann::json& j) override;

	private:
		// Documentation management
		std::vector<MarkdownDocument> documents;
		std::map<std::string, size_t> documentIndex; // filename -> index mapping
		int selectedDocumentIndex = 0;

		// Image cache for markdown images
		std::unordered_map<std::string, ImageCache> imageCache;

		// UI state
		bool showNavigationPanel = true;
		float navigationWidth = 200.0f;
		std::string searchFilter;
		char searchBuffer[256] = { 0 };

		// Markdown configuration
		ImGui::MarkdownConfig markdownConfig;

		// Font management for markdown headers
		ImFont* headerFont1 = nullptr;
		ImFont* headerFont2 = nullptr;
		ImFont* headerFont3 = nullptr;

		// Methods
		void LoadDocuments();
		void LoadMarkdownFile(const std::filesystem::path& filePath);
		void RenderNavigationPanel();
		void RenderMarkdownContent();
		void SetupMarkdownConfig();
		bool MatchesSearchFilter(const MarkdownDocument& doc) const;
		void CleanupImageCache();

		// Image handling
		GLuint LoadImageForMarkdown(const std::string& imagePath, const std::string& documentDir);

		// Link handling
		void OpenExternalLink(const std::string& url);
		void HandleInternalLink(const std::string& link);

		// Static callbacks for imgui_markdown
		static void LinkCallback(ImGui::MarkdownLinkCallbackData data);
		static ImGui::MarkdownImageData ImageCallback(ImGui::MarkdownLinkCallbackData data);
		static void FormatCallback(const ImGui::MarkdownFormatInfo& markdownFormatInfo, bool start);
	};

} // namespace GUI