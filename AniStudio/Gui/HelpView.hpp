#pragma once

#include "Base/BaseView.hpp"
#include "pch.h"
#include "imgui_markdown.h"
#include <filesystem>
#include <fstream>
#include <vector>
#include <map>

namespace GUI {

	struct MarkdownDocument {
		std::string title;
		std::string content;
		std::string filePath;
		bool isLoaded = false;
	};

	class HelpView : public BaseView {
	public:
		HelpView(ECS::EntityManager& entityMgr);
		~HelpView() = default;

		void Init() override;
		void Render() override;
		nlohmann::json Serialize() const override;
		void Deserialize(const nlohmann::json& j) override;

	private:
		// Documentation management
		std::vector<MarkdownDocument> documents;
		std::map<std::string, size_t> documentIndex; // filename -> index mapping
		int selectedDocumentIndex = 0;

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

		// Static callbacks for imgui_markdown
		static void LinkCallback(ImGui::MarkdownLinkCallbackData data);
		static ImGui::MarkdownImageData ImageCallback(ImGui::MarkdownLinkCallbackData data);
		static void FormatCallback(const ImGui::MarkdownFormatInfo& markdownFormatInfo, bool start);
	};

} // namespace GUI