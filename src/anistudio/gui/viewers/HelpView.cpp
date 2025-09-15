#include "HelpView.hpp"
#include "FilePaths.hpp"
#include <iostream>
#include <algorithm>
#include <sstream>
#include "Events.hpp"

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#elif __linux__
#include <cstdlib>
#elif __APPLE__
#include <cstdlib>
#endif

namespace GUI {

	HelpView::HelpView(ECS::EntityManager& entityMgr) : BaseView(entityMgr) {
		viewName = "HelpView";
	}

	HelpView::~HelpView() {
		CleanupImageCache();
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

		// Load README from project root (executable is in build/bin, so go up two levels)
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
			doc.directory = filePath.parent_path().string();

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

		if (ImGui::Begin(GetWindowTitle().c_str(), &windowOpen, ImGuiWindowFlags_MenuBar)) {
			if (!windowOpen) {
				ANI::Events::Ref().RequestRemoveView(GetID(), viewName);
			}

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

				// Debug info
				ImGui::Separator();
				ImGui::Text("Debug Information:");
				std::string currentPath = std::filesystem::current_path().string();
				ImGui::Text("Current working directory: %s", currentPath.c_str());

				std::filesystem::path readmePath = "../README.md";
				ImGui::Text("Looking for README at: %s", std::filesystem::absolute(readmePath).string().c_str());
				ImGui::Text("README exists: %s", std::filesystem::exists(readmePath) ? "YES" : "NO");

				std::filesystem::path docsPath = "../docs";
				ImGui::Text("Looking for docs at: %s", std::filesystem::absolute(docsPath).string().c_str());
				ImGui::Text("Docs directory exists: %s", std::filesystem::exists(docsPath) ? "YES" : "NO");
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

			// Process markdown content for tables
			if (doc.isLoaded && !doc.content.empty()) {
				std::string processedContent = doc.content;

				// Simple table detection and conversion
				// This is a basic implementation - you might want to use a more sophisticated markdown parser
				std::istringstream stream(processedContent);
				std::string line;
				std::string finalContent;
				bool inTable = false;

				while (std::getline(stream, line)) {
					// Check if line contains table separators
					if (line.find('|') != std::string::npos) {
						if (!inTable) {
							// Start of table - add some spacing
							finalContent += "\n";
							inTable = true;
						}

						// For now, just render table rows as regular text
						// You could enhance this to create actual ImGui tables
						finalContent += line + "\n";
					}
					else {
						if (inTable) {
							// End of table - add spacing
							finalContent += "\n";
							inTable = false;
						}
						finalContent += line + "\n";
					}
				}

				// Render markdown content
				ImGui::Markdown(finalContent.c_str(), finalContent.length(), markdownConfig);
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

	void HelpView::CleanupImageCache() {
		for (auto& pair : imageCache) {
			if (pair.second.textureID != 0) {
				Utils::OpenGLUtils::DeleteTexture(pair.second.textureID);
			}
		}
		imageCache.clear();
	}

	GLuint HelpView::LoadImageForMarkdown(const std::string& imagePath, const std::string& documentDir) {
		// Check cache first
		auto it = imageCache.find(imagePath);
		if (it != imageCache.end()) {
			return it->second.loaded ? it->second.textureID : 0;
		}

		// Try to resolve the image path
		std::filesystem::path fullPath;

		// Check if it's an absolute path
		if (std::filesystem::path(imagePath).is_absolute()) {
			fullPath = imagePath;
		}
		else {
			// Try relative to document directory
			fullPath = std::filesystem::path(documentDir) / imagePath;
			if (!std::filesystem::exists(fullPath)) {
				// Try relative to current working directory
				fullPath = std::filesystem::current_path() / imagePath;
				if (!std::filesystem::exists(fullPath)) {
					// Try relative to project root
					fullPath = std::filesystem::path("..") / imagePath;
				}
			}
		}

		ImageCache cache;
		cache.loaded = false;
		cache.textureID = 0;

		if (std::filesystem::exists(fullPath)) {
			// Load image using your existing ImageUtils
			int width, height, channels;
			unsigned char* data = Utils::ImageUtils::LoadImageData(fullPath.string(), width, height, channels);

			if (data) {
				cache.textureID = Utils::OpenGLUtils::GenerateTexture(width, height, channels, data);
				cache.width = width;
				cache.height = height;
				cache.loaded = (cache.textureID != 0);

				Utils::ImageUtils::FreeImageData(data);

				std::cout << "Loaded image: " << fullPath << " (texture ID: " << cache.textureID << ")" << std::endl;
			}
			else {
				std::cerr << "Failed to load image data: " << fullPath << std::endl;
			}
		}
		else {
			std::cerr << "Image not found: " << imagePath << " (tried: " << fullPath << ")" << std::endl;
		}

		// Cache the result (even if failed)
		imageCache[imagePath] = cache;
		return cache.loaded ? cache.textureID : 0;
	}

	void HelpView::OpenExternalLink(const std::string& url) {
		std::cout << "Opening external link: " << url << std::endl;

#ifdef _WIN32
		ShellExecuteA(NULL, "open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);
#elif __linux__
		std::string command = "xdg-open " + url;
		system(command.c_str());
#elif __APPLE__
		std::string command = "open " + url;
		system(command.c_str());
#else
		std::cout << "Platform not supported for opening links: " << url << std::endl;
#endif
	}

	void HelpView::HandleInternalLink(const std::string& link) {
		std::cout << "Handling internal link: " << link << std::endl;

		// Try to find document by filename
		auto it = documentIndex.find(link);
		if (it != documentIndex.end()) {
			selectedDocumentIndex = static_cast<int>(it->second);
			std::cout << "Switched to document: " << documents[selectedDocumentIndex].title << std::endl;
		}
		else {
			// Try to find by title or partial match
			for (size_t i = 0; i < documents.size(); ++i) {
				if (documents[i].title.find(link) != std::string::npos ||
					documents[i].filePath.find(link) != std::string::npos) {
					selectedDocumentIndex = static_cast<int>(i);
					std::cout << "Switched to document (partial match): " << documents[selectedDocumentIndex].title << std::endl;
					return;
				}
			}
			std::cout << "Could not find document for link: " << link << std::endl;
		}
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
		HelpView* helpView = static_cast<HelpView*>(data.userData);

		if (!data.isImage && helpView) {
			std::cout << "Link clicked: " << url << std::endl;

			// Handle different types of links
			if (url.find("http://") == 0 || url.find("https://") == 0 || url.find("www.") == 0) {
				// External web link
				helpView->OpenExternalLink(url);
			}
			else if (url.find(".md") != std::string::npos || url.find("#") != std::string::npos) {
				// Internal markdown link
				// Remove anchors for now (could be enhanced to support them)
				size_t anchorPos = url.find('#');
				if (anchorPos != std::string::npos) {
					url = url.substr(0, anchorPos);
				}
				if (!url.empty()) {
					helpView->HandleInternalLink(url);
				}
			}
			else {
				// Try as internal link anyway
				helpView->HandleInternalLink(url);
			}
		}
	}

	ImGui::MarkdownImageData HelpView::ImageCallback(ImGui::MarkdownLinkCallbackData data) {
		ImGui::MarkdownImageData imageData;
		imageData.isValid = false;
		imageData.useLinkCallback = false;
		imageData.user_texture_id = 0;
		imageData.size = ImVec2(100.0f, 100.0f);

		HelpView* helpView = static_cast<HelpView*>(data.userData);
		if (helpView && helpView->selectedDocumentIndex >= 0 &&
			helpView->selectedDocumentIndex < static_cast<int>(helpView->documents.size())) {

			std::string imagePath(data.link, data.linkLength);
			const auto& currentDoc = helpView->documents[helpView->selectedDocumentIndex];

			GLuint textureID = helpView->LoadImageForMarkdown(imagePath, currentDoc.directory);

			if (textureID != 0) {
				// Get cached image info
				auto it = helpView->imageCache.find(imagePath);
				if (it != helpView->imageCache.end() && it->second.loaded) {
					imageData.isValid = true;
					imageData.user_texture_id = (ImTextureID)(intptr_t)textureID;

					// Set size based on actual image dimensions, but limit max size
					float maxWidth = 400.0f;
					float maxHeight = 300.0f;
					float width = static_cast<float>(it->second.width);
					float height = static_cast<float>(it->second.height);

					// Scale down if too large
					if (width > maxWidth || height > maxHeight) {
						float scaleX = maxWidth / width;
						float scaleY = maxHeight / height;
						float scale = std::min(scaleX, scaleY);
						width *= scale;
						height *= scale;
					}

					imageData.size = ImVec2(width, height);

					// Adjust size if it's larger than available content region
					ImVec2 const contentSize = ImGui::GetContentRegionAvail();
					if (imageData.size.x > contentSize.x) {
						float const ratio = imageData.size.y / imageData.size.x;
						imageData.size.x = contentSize.x;
						imageData.size.y = contentSize.x * ratio;
					}
				}
			}
			else {
				std::cout << "Failed to load image: " << imagePath << std::endl;
			}
		}

		return imageData;
	}

	void HelpView::FormatCallback(const ImGui::MarkdownFormatInfo& markdownFormatInfo, bool start) {
		// Use the default formatting first
		ImGui::defaultMarkdownFormatCallback(markdownFormatInfo, start);

		// Add custom formatting
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
			// Custom emphasis styling
			if (markdownFormatInfo.level == 2) { // Strong emphasis (**)
				if (start) {
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.8f, 1.0f));
				}
				else {
					ImGui::PopStyleColor();
				}
			}
			break;

		default:
			break;
		}
	}

} // namespace GUI