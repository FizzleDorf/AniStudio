#include "HelpView.hpp"
#include "FilePaths.hpp"
#include <iostream>
#include <algorithm>
#include <sstream>

namespace GUI {

	HelpView::HelpView(ECS::EntityManager& entityMgr) : BaseView(entityMgr) {
		viewName = "Help";
	}

	void HelpView::Init() {
		SetupMarkdownConfig();
		LoadDocuments();
	}

	void HelpView::SetupMarkdownConfig() {
		// Configure markdown rendering
		markdownConfig.linkCallback = LinkCallback;
		markdownConfig.imageCallback = ImageCallback;
		markdownConfig.formatCallback = FormatCallback;
		markdownConfig.tooltipCallback = ImGui::defaultMarkdownTooltipCallback;
		markdownConfig.linkIcon = "[LINK]"; // You can use font icons here if available
		markdownConfig.userData = this;

		// Set up heading formats - these will use default fonts if no custom fonts are loaded
		markdownConfig.headingFormats[0] = { headerFont1, true };  // H1
		markdownConfig.headingFormats[1] = { headerFont2, true };  // H2
		markdownConfig.headingFormats[2] = { headerFont3, false }; // H3
	}

	void HelpView::LoadDocuments() {
		documents.clear();
		documentIndex.clear();

		// Load README from project root
		std::filesystem::path readmePath = "../README.md";
		if (std::filesystem::exists(readmePath)) {
			LoadMarkdownFile(readmePath);
		}

		// Load all markdown files from docs directory
		std::filesystem::path docsPath = "../docs";
		if (std::filesystem::exists(docsPath) && std::filesystem::is_directory(docsPath)) {
			for (const auto& entry : std::filesystem::recursive_directory_iterator(docsPath)) {
				if (entry.is_regular_file()) {
					std::string extension = entry.path().extension().string();
					std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

					if (extension == ".md" || extension == ".markdown") {
						LoadMarkdownFile(entry.path());
					}
				}
			}
		}

		// Sort documents by title for better navigation
		std::sort(documents.begin(), documents.end(),
			[](const MarkdownDocument& a, const MarkdownDocument& b) {
			return a.title < b.title;
		});

		// Rebuild index after sorting
		documentIndex.clear();
		for (size_t i = 0; i < documents.size(); ++i) {
			std::string filename = std::filesystem::path(documents[i].filePath).filename().string();
			documentIndex[filename] = i;
		}

		std::cout << "Loaded " << documents.size() << " documentation files" << std::endl;
	}

	void HelpView::LoadMarkdownFile(const std::filesystem::path& filePath) {
		try {
			std::ifstream file(filePath);
			if (!file.is_open()) {
				std::cerr << "Failed to open file: " << filePath << std::endl;
				return;
			}

			MarkdownDocument doc;
			doc.filePath = filePath.string();

			// Extract title from filename or first header
			doc.title = filePath.stem().string();

			// Read file content
			std::stringstream buffer;
			buffer << file.rdbuf();
			doc.content = buffer.str();
			doc.isLoaded = true;

			// Try to extract a better title from the first H1 header
			std::string content = doc.content;
			size_t headerPos = content.find("# ");
			if (headerPos != std::string::npos) {
				size_t lineEnd = content.find('\n', headerPos);
				if (lineEnd != std::string::npos) {
					std::string headerTitle = content.substr(headerPos + 2, lineEnd - headerPos - 2);
					// Remove any extra whitespace
					headerTitle.erase(0, headerTitle.find_first_not_of(" \t"));
					headerTitle.erase(headerTitle.find_last_not_of(" \t") + 1);
					if (!headerTitle.empty()) {
						doc.title = headerTitle;
					}
				}
			}

			documents.push_back(std::move(doc));

			std::cout << "Loaded: " << doc.title << " from " << filePath << std::endl;
		}
		catch (const std::exception& e) {
			std::cerr << "Error loading markdown file " << filePath << ": " << e.what() << std::endl;
		}
	}

	void HelpView::Render() {
		ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);

		if (ImGui::Begin("Help & Documentation", nullptr, ImGuiWindowFlags_MenuBar)) {

			// Menu bar
			if (ImGui::BeginMenuBar()) {
				if (ImGui::BeginMenu("View")) {
					ImGui::MenuItem("Show Navigation", nullptr, &showNavigationPanel);
					ImGui::EndMenu();
				}

				if (ImGui::BeginMenu("Documents")) {
					if (ImGui::MenuItem("Reload All")) {
						LoadDocuments();
					}
					ImGui::EndMenu();
				}

				ImGui::EndMenuBar();
			}

			if (documents.empty()) {
				ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "No documentation files found.");
				ImGui::Text("Expected locations:");
				ImGui::BulletText("README.md in project root");
				ImGui::BulletText("*.md files in docs/ directory");
			}
			else {
				// Main content area
				if (showNavigationPanel) {
					// Split layout: navigation panel + content
					ImGui::BeginChild("Navigation", ImVec2(navigationWidth, 0), true);
					RenderNavigationPanel();
					ImGui::EndChild();

					ImGui::SameLine();

					// Splitter
					ImGui::Button("||", ImVec2(8, -1));
					if (ImGui::IsItemActive()) {
						navigationWidth += ImGui::GetIO().MouseDelta.x;
						navigationWidth = std::max(150.0f, std::min(400.0f, navigationWidth));
					}
					ImGui::SameLine();

					// Content area
					ImGui::BeginChild("Content");
					RenderMarkdownContent();
					ImGui::EndChild();
				}
				else {
					// Full width content
					RenderMarkdownContent();
				}
			}
		}
		ImGui::End();
	}

	void HelpView::RenderNavigationPanel() {
		// Search filter
		ImGui::Text("Search:");
		if (ImGui::InputText("##search", searchBuffer, sizeof(searchBuffer))) {
			searchFilter = searchBuffer;
		}

		ImGui::Separator();

		// Document list
		for (size_t i = 0; i < documents.size(); ++i) {
			const auto& doc = documents[i];

			// Apply search filter
			if (!MatchesSearchFilter(doc)) {
				continue;
			}

			bool isSelected = (selectedDocumentIndex == static_cast<int>(i));

			if (ImGui::Selectable(doc.title.c_str(), isSelected)) {
				selectedDocumentIndex = static_cast<int>(i);
			}

			// Show tooltip with file path
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("%s", doc.filePath.c_str());
			}
		}
	}

	void HelpView::RenderMarkdownContent() {
		if (selectedDocumentIndex >= 0 && selectedDocumentIndex < static_cast<int>(documents.size())) {
			const auto& doc = documents[selectedDocumentIndex];

			// Header with document title
			ImGui::Text("Document: %s", doc.title.c_str());
			ImGui::Separator();

			// Render markdown content
			if (doc.isLoaded && !doc.content.empty()) {
				ImGui::Markdown(doc.content.c_str(), doc.content.length(), markdownConfig);
			}
			else {
				ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Failed to load document content.");
			}
		}
		else {
			ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Select a document from the navigation panel to view its content.");
		}
	}

	bool HelpView::MatchesSearchFilter(const MarkdownDocument& doc) const {
		if (searchFilter.empty()) {
			return true;
		}

		// Case-insensitive search in title and content
		std::string lowerFilter = searchFilter;
		std::transform(lowerFilter.begin(), lowerFilter.end(), lowerFilter.begin(), ::tolower);

		std::string lowerTitle = doc.title;
		std::transform(lowerTitle.begin(), lowerTitle.end(), lowerTitle.begin(), ::tolower);

		std::string lowerContent = doc.content;
		std::transform(lowerContent.begin(), lowerContent.end(), lowerContent.begin(), ::tolower);

		return lowerTitle.find(lowerFilter) != std::string::npos ||
			lowerContent.find(lowerFilter) != std::string::npos;
	}

	nlohmann::json HelpView::Serialize() const {
		nlohmann::json j;
		j["viewName"] = viewName;
		j["showNavigationPanel"] = showNavigationPanel;
		j["navigationWidth"] = navigationWidth;
		j["selectedDocumentIndex"] = selectedDocumentIndex;
		j["searchFilter"] = searchFilter;
		return j;
	}

	void HelpView::Deserialize(const nlohmann::json& j) {
		BaseView::Deserialize(j);

		if (j.contains("showNavigationPanel"))
			showNavigationPanel = j["showNavigationPanel"];
		if (j.contains("navigationWidth"))
			navigationWidth = j["navigationWidth"];
		if (j.contains("selectedDocumentIndex"))
			selectedDocumentIndex = j["selectedDocumentIndex"];
		if (j.contains("searchFilter")) {
			searchFilter = j["searchFilter"];
			strncpy(searchBuffer, searchFilter.c_str(), sizeof(searchBuffer) - 1);
			searchBuffer[sizeof(searchBuffer) - 1] = '\0';
		}
	}

	// Static callback implementations
	void HelpView::LinkCallback(ImGui::MarkdownLinkCallbackData data) {
		std::string url(data.link, data.linkLength);

		if (!data.isImage) {
			std::cout << "Link clicked: " << url << std::endl;

			// Handle different types of links
			if (url.find("http://") == 0 || url.find("https://") == 0) {
				// External web link - you might want to open in browser
				std::cout << "External link: " << url << std::endl;
				// On Windows: system(("start " + url).c_str());
				// On Linux: system(("xdg-open " + url).c_str());
				// On macOS: system(("open " + url).c_str());
			}
			else if (url.find(".md") != std::string::npos) {
				// Internal markdown link - switch to that document
				HelpView* helpView = static_cast<HelpView*>(data.userData);
				if (helpView) {
					auto it = helpView->documentIndex.find(url);
					if (it != helpView->documentIndex.end()) {
						helpView->selectedDocumentIndex = static_cast<int>(it->second);
					}
				}
			}
		}
	}

	ImGui::MarkdownImageData HelpView::ImageCallback(ImGui::MarkdownLinkCallbackData data) {
		// Basic image handling - you might want to extend this to load actual images
		ImGui::MarkdownImageData imageData;
		imageData.isValid = false; // Set to true when you have actual image loading
		imageData.useLinkCallback = false;
		imageData.user_texture_id = 0; // Set to actual texture ID when available
		imageData.size = ImVec2(100.0f, 100.0f);

		// For now, just show placeholder
		std::string imagePath(data.link, data.linkLength);
		std::cout << "Image referenced: " << imagePath << std::endl;

		return imageData;
	}

	void HelpView::FormatCallback(const ImGui::MarkdownFormatInfo& markdownFormatInfo, bool start) {
		// Use the default formatting
		ImGui::defaultMarkdownFormatCallback(markdownFormatInfo, start);

		// You can add custom formatting here
		switch (markdownFormatInfo.type) {
		case ImGui::MarkdownFormatType::HEADING:
			if (markdownFormatInfo.level == 1 && start) {
				// Custom styling for H1 headers
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 0.8f, 1.0f, 1.0f));
			}
			else if (markdownFormatInfo.level == 1 && !start) {
				ImGui::PopStyleColor();
			}
			break;

		case ImGui::MarkdownFormatType::EMPHASIS:
			// Custom emphasis styling could go here
			break;

		default:
			break;
		}
	}

} // namespace GUI