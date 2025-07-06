/*
		d8888          d8b  .d8888b.  888                  888 d8b
	   d88888          Y8P d88P  Y88b 888                  888 Y8P
	  d88P888              Y88b.      888                  888
	 d88P 888 88888b.  888  "Y888b.   888888 888  888  .d88888 888  .d88b.
	d88P  888 888 "88b 888     "Y88b. 888    888  888 d88" 888 888 d88""88b
   d88P   888 888  888 888       "888 888    888  888 888  888 888 888  888
  d8888888888 888  888 888 Y88b  d88P Y88b.  Y88b 888 Y88b 888 888 Y88..88P
 d88P     888 888  888 888  "Y8888P"   "Y888  "Y88888  "Y88888 888  "Y88P"

 * This file is part of AniStudio.
 * Copyright (C) 2025 FizzleDorf (AnimAnon)
 *
 * This software is dual-licensed under the GNU Lesser General Public License v3.0 (LGPL-3.0)
 * and a commercial license. You may choose to use it under either license.
 *
 * For the LGPL-3.0, see the LICENSE-LGPL-3.0.txt file in the repository.
 * For commercial license information, please contact legal@kframe.ai.
 */

#pragma once

#include "GUI.h"
#include "pch.h"
#include "FilePaths.hpp"
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