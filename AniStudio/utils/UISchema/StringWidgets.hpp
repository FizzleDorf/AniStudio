/*
 * StringWidgets.hpp - Enhanced with checkbox controls for Zep editors
 */

#pragma once

#include <imgui.h>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <iostream>
#include "UISchemaUtils.hpp"
#include "ZepUtils.hpp"

namespace UISchema {

	static const std::string DEFAULT_STRING_WIDGET = "input_text";

	// Configuration structure for Zep editor options
	struct ZepEditorConfig {
		bool showLineNumbers = true;
		bool wordWrap = false;
		bool readOnly = false;
		bool showWhitespace = false;
		bool enableSyntaxHighlighting = true;
		bool autoIndent = true;
		bool showSearch = false;
		std::string theme = "dark";  // "dark", "light", etc.
	};

	class StringWidgets {
	private:
		// Use memory address of the string pointer for truly unique identification
		static std::unordered_map<uintptr_t, std::shared_ptr<Utils::ZepTextEditor>>& GetEditorMap() {
			static std::unordered_map<uintptr_t, std::shared_ptr<Utils::ZepTextEditor>> editorMap;
			return editorMap;
		}

		// Store editor configurations
		static std::unordered_map<uintptr_t, ZepEditorConfig>& GetConfigMap() {
			static std::unordered_map<uintptr_t, ZepEditorConfig> configMap;
			return configMap;
		}

		// Create or get a unique Zep editor instance for this string pointer
		static std::shared_ptr<Utils::ZepTextEditor> GetOrCreateEditor(std::string* value) {
			auto& editorMap = GetEditorMap();
			auto& configMap = GetConfigMap();
			uintptr_t uniqueKey = reinterpret_cast<uintptr_t>(value);

			auto it = editorMap.find(uniqueKey);
			if (it == editorMap.end()) {
				// Create new editor instance
				auto editor = std::make_shared<Utils::ZepTextEditor>();
				if (!editor->Initialize()) {
					std::cerr << "Failed to initialize Zep editor for string at " << value << std::endl;
					return nullptr;
				}

				// Set the initial text content
				editor->SetText(*value);

				// Ensure it's in standard mode (should already be set in Initialize)
				editor->SetMode("standard");

				editorMap[uniqueKey] = editor;

				// Create default config for this editor
				configMap[uniqueKey] = ZepEditorConfig{};

				std::cout << "Created new Zep editor instance for string at " << value
					<< " (key: " << uniqueKey << ")" << std::endl;

				return editor;
			}

			return it->second;
		}

		// Apply configuration to the editor
		static void ApplyConfig(std::shared_ptr<Utils::ZepTextEditor> editor, const ZepEditorConfig& config) {
			if (!editor) return;

			// Apply configuration using the actual available API methods
			editor->SetShowLineNumbers(config.showLineNumbers);
			editor->SetWordWrap(config.wordWrap);
			editor->SetReadOnly(config.readOnly);
			editor->SetShowWhitespace(config.showWhitespace);
			editor->SetSyntaxHighlighting(config.enableSyntaxHighlighting);
			editor->SetAutoIndent(config.autoIndent);
			editor->SetTheme(config.theme);
			editor->ShowSearchBox(config.showSearch);
		}

	public:
		static bool RenderInputText(const std::string& label, std::string* value, const nlohmann::json& options = {}) {
			ImGuiInputTextFlags flags = GetInputTextFlags(options);

			size_t bufferSize = GetSchemaValue<size_t>(options, "maxLength", 1024);
			if (bufferSize < 1) bufferSize = 1;

			std::vector<char> buffer(bufferSize);
			strncpy(buffer.data(), value->c_str(), bufferSize - 1);
			buffer[bufferSize - 1] = '\0';

			bool changed = ImGui::InputText(label.c_str(), buffer.data(), bufferSize, flags);

			if (changed) {
				*value = buffer.data();
			}

			return changed;
		}

		// Enhanced Zep editor with checkbox controls
		static bool RenderZepEditor(const std::string& label, std::string* value, const nlohmann::json& options = {}) {
			auto editor = GetOrCreateEditor(value);
			if (!editor) {
				ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Failed to create Zep editor");
				return false;
			}

			// Get or create configuration for this editor
			auto& configMap = GetConfigMap();
			uintptr_t uniqueKey = reinterpret_cast<uintptr_t>(value);
			ZepEditorConfig& config = configMap[uniqueKey];

			bool modified = false;

			// Render checkbox controls in a collapsible header
			bool showOptions = GetSchemaValue<bool>(options, "showOptions", true);
			if (showOptions && ImGui::CollapsingHeader("Editor Options##zep_options", ImGuiTreeNodeFlags_None)) {

				// Create a table for organized layout
				if (ImGui::BeginTable("ZepOptionsTable", 2, ImGuiTableFlags_SizingFixedFit)) {
					ImGui::TableSetupColumn("Option", ImGuiTableColumnFlags_WidthFixed, 150.0f);
					ImGui::TableSetupColumn("Control", ImGuiTableColumnFlags_WidthStretch);

					// Line Numbers checkbox
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::Text("Show Line Numbers");
					ImGui::TableNextColumn();
					if (ImGui::Checkbox("##lineNumbers", &config.showLineNumbers)) {
						ApplyConfig(editor, config);
					}

					// Word Wrap checkbox
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::Text("Word Wrap");
					ImGui::TableNextColumn();
					if (ImGui::Checkbox("##wordWrap", &config.wordWrap)) {
						ApplyConfig(editor, config);
					}

					// Read Only checkbox
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::Text("Read Only");
					ImGui::TableNextColumn();
					if (ImGui::Checkbox("##readOnly", &config.readOnly)) {
						ApplyConfig(editor, config);
					}

					// Show Whitespace checkbox
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::Text("Show Whitespace");
					ImGui::TableNextColumn();
					if (ImGui::Checkbox("##showWhitespace", &config.showWhitespace)) {
						ApplyConfig(editor, config);
					}

					// Syntax Highlighting checkbox
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::Text("Syntax Highlighting");
					ImGui::TableNextColumn();
					if (ImGui::Checkbox("##syntaxHighlighting", &config.enableSyntaxHighlighting)) {
						ApplyConfig(editor, config);
					}

					// Auto Indent checkbox
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::Text("Auto Indent");
					ImGui::TableNextColumn();
					if (ImGui::Checkbox("##autoIndent", &config.autoIndent)) {
						ApplyConfig(editor, config);
					}

					// Search Box checkbox
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::Text("Show Search");
					ImGui::TableNextColumn();
					if (ImGui::Checkbox("##showSearch", &config.showSearch)) {
						ApplyConfig(editor, config);
					}

					ImGui::EndTable();
				}

				ImGui::Separator();
			}

			// ALWAYS ensure we're in standard mode
			editor->SetMode("standard");

			// Use available space (leave some room for options if shown)
			ImVec2 availableSize = ImGui::GetContentRegionAvail();

			// Ensure minimum size
			if (availableSize.x < 100) availableSize.x = 100;
			if (availableSize.y < 100) availableSize.y = 100;

			// Get current cursor position
			ImVec2 cursorPos = ImGui::GetCursorScreenPos();

			// Let the editor handle its own focus management and rendering
			editor->Render(cursorPos, availableSize);

			// Check if content changed and update the string value
			std::string currentText = editor->GetText();
			if (currentText != *value) {
				*value = currentText;
				modified = true;
			}

			return modified;
		}

		// Compact version with inline checkboxes
		static bool RenderZepEditorCompact(const std::string& label, std::string* value, const nlohmann::json& options = {}) {
			auto editor = GetOrCreateEditor(value);
			if (!editor) {
				ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Failed to create Zep editor");
				return false;
			}

			// Get configuration for this editor
			auto& configMap = GetConfigMap();
			uintptr_t uniqueKey = reinterpret_cast<uintptr_t>(value);
			ZepEditorConfig& config = configMap[uniqueKey];

			bool modified = false;

			// Render inline checkboxes
			if (ImGui::Checkbox("Lines##ln", &config.showLineNumbers)) {
				ApplyConfig(editor, config);
			}
			ImGui::SameLine();
			if (ImGui::Checkbox("Wrap##wp", &config.wordWrap)) {
				ApplyConfig(editor, config);
			}
			ImGui::SameLine();
			if (ImGui::Checkbox("Read Only##ro", &config.readOnly)) {
				ApplyConfig(editor, config);
			}
			ImGui::SameLine();
			if (ImGui::Checkbox("Syntax##sx", &config.enableSyntaxHighlighting)) {
				ApplyConfig(editor, config);
			}
			ImGui::SameLine();
			if (ImGui::Checkbox("Search##sr", &config.showSearch)) {
				ApplyConfig(editor, config);
			}

			// ALWAYS ensure we're in standard mode
			editor->SetMode("standard");

			// Use available space
			ImVec2 availableSize = ImGui::GetContentRegionAvail();
			if (availableSize.x < 100) availableSize.x = 100;
			if (availableSize.y < 100) availableSize.y = 100;

			ImVec2 cursorPos = ImGui::GetCursorScreenPos();
			editor->Render(cursorPos, availableSize);

			// Check if content changed
			std::string currentText = editor->GetText();
			if (currentText != *value) {
				*value = currentText;
				modified = true;
			}

			return modified;
		}

		static bool Render(const std::string& label, std::string* value, const std::string& widgetType, const nlohmann::json& schema) {
			if (widgetType == "input_text") {
				return RenderInputText(label, value, schema);
			}
			else if (widgetType == "zep_editor" || widgetType == "dynamic_textarea") {
				return RenderZepEditor(label, value, schema);
			}
			else if (widgetType == "zep_editor_compact") {
				return RenderZepEditorCompact(label, value, schema);
			}
			else {
				std::cerr << "Unknown widget type '" << widgetType << "' for string property, defaulting to input_text" << std::endl;
				return RenderInputText(label, value, schema);
			}
		}

		// Update editor content when deserializing
		static void UpdateEditorContent(std::string* value) {
			auto& editorMap = GetEditorMap();
			uintptr_t uniqueKey = reinterpret_cast<uintptr_t>(value);

			auto it = editorMap.find(uniqueKey);
			if (it != editorMap.end()) {
				it->second->SetText(*value);
			}
		}

		// Method to manually cleanup specific editor instance
		static void CleanupEditor(std::string* value) {
			auto& editorMap = GetEditorMap();
			auto& configMap = GetConfigMap();
			uintptr_t uniqueKey = reinterpret_cast<uintptr_t>(value);

			auto it = editorMap.find(uniqueKey);
			if (it != editorMap.end()) {
				std::cout << "Cleaning up Zep editor for string at " << value << std::endl;
				editorMap.erase(it);
			}

			// Also cleanup config
			auto configIt = configMap.find(uniqueKey);
			if (configIt != configMap.end()) {
				configMap.erase(configIt);
			}
		}

		// Method to clear focus from all editors
		static void ClearFocus() {
			Utils::ZepFocusTracker::ClearFocus();
		}

		// Get editor configuration for external use
		static ZepEditorConfig* GetEditorConfig(std::string* value) {
			auto& configMap = GetConfigMap();
			uintptr_t uniqueKey = reinterpret_cast<uintptr_t>(value);

			auto it = configMap.find(uniqueKey);
			if (it != configMap.end()) {
				return &it->second;
			}
			return nullptr;
		}

		// Search functionality for all editors
		static void FindInEditor(std::string* value, const std::string& searchTerm) {
			auto& editorMap = GetEditorMap();
			uintptr_t uniqueKey = reinterpret_cast<uintptr_t>(value);

			auto it = editorMap.find(uniqueKey);
			if (it != editorMap.end()) {
				auto& editor = it->second;
				editor->SetSearchTerm(searchTerm);
				editor->FindNext();
			}
		}

		// Cleanup function to call when shutting down
		static void Cleanup() {
			std::cout << "Cleaning up all Zep editors..." << std::endl;
			GetEditorMap().clear();
			GetConfigMap().clear();
			Utils::ZepFocusTracker::ClearFocus();
		}
	};

} // namespace UISchema