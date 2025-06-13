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

	// Simple config struct
	struct ZepEditorConfig {
		bool showLineNumbers = true;
		bool wordWrap = false;
		bool readOnly = false;
		bool showWhitespace = false;
		bool enableSyntaxHighlighting = true;
		bool autoIndent = true;
		bool showSearch = false;
		bool showMenuBar = true;
		std::string theme = "dark";
	};

	class StringWidgets {
	private:
		// Simple editor map
		static std::unordered_map<uintptr_t, std::shared_ptr<Utils::ZepTextEditor>>& GetEditorMap() {
			static std::unordered_map<uintptr_t, std::shared_ptr<Utils::ZepTextEditor>> editorMap;
			return editorMap;
		}

		// Simple config map
		static std::unordered_map<uintptr_t, ZepEditorConfig>& GetConfigMap() {
			static std::unordered_map<uintptr_t, ZepEditorConfig> configMap;
			return configMap;
		}

		// Create editor - SIMPLE
		static std::shared_ptr<Utils::ZepTextEditor> GetOrCreateEditor(std::string* value) {
			auto& editorMap = GetEditorMap();
			auto& configMap = GetConfigMap();
			uintptr_t uniqueKey = reinterpret_cast<uintptr_t>(value);

			auto it = editorMap.find(uniqueKey);
			if (it == editorMap.end()) {
				auto editor = std::make_shared<Utils::ZepTextEditor>();
				if (!editor->Initialize()) {
					std::cerr << "Failed to initialize Zep editor" << std::endl;
					return nullptr;
				}

				editor->SetText(*value);
				editorMap[uniqueKey] = editor;
				configMap[uniqueKey] = ZepEditorConfig{};

				return editor;
			}

			return it->second;
		}

		// Apply config - SIMPLE
		static void ApplyConfig(std::shared_ptr<Utils::ZepTextEditor> editor, const ZepEditorConfig& config) {
			if (!editor) return;

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

		// Zep editor
		static bool RenderZepEditor(const std::string& label, std::string* value, const nlohmann::json& options = {}) {
			auto editor = GetOrCreateEditor(value);
			if (!editor) {
				ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Failed to create Zep editor");
				return false;
			}

			auto& configMap = GetConfigMap();
			uintptr_t uniqueKey = reinterpret_cast<uintptr_t>(value);
			ZepEditorConfig& config = configMap[uniqueKey];

			// Apply schema options
			config.showMenuBar = GetSchemaValue<bool>(options, "showMenuBar", config.showMenuBar);
			ApplyConfig(editor, config);

			bool modified = false;

			// Get the available content size
			ImVec2 contentSize = ImGui::GetContentRegionAvail();
			if (contentSize.x < 100) contentSize.x = 100;
			if (contentSize.y < 100) contentSize.y = 100;

			std::string childId = "ZepEditor##" + std::to_string(uniqueKey);

			// Create child window with proper flags for menu bar
			ImGuiWindowFlags childFlags = ImGuiWindowFlags_None;
			if (config.showMenuBar) {
				childFlags |= ImGuiWindowFlags_MenuBar;
			}

			if (ImGui::BeginChild(childId.c_str(), contentSize, true, childFlags)) {

				// Render menu bar if enabled - this must be first
				if (config.showMenuBar) {
					if (ImGui::BeginMenuBar()) {
						if (ImGui::BeginMenu("File")) {
							if (ImGui::MenuItem("New", "Ctrl+N")) {
								editor->SetText("");
								*value = "";
								modified = true;
							}
							if (ImGui::MenuItem("Save", "Ctrl+S")) {
								// TODO: Save functionality
							}
							ImGui::EndMenu();
						}

						if (ImGui::BeginMenu("Options")) {
							bool lineNumbers = config.showLineNumbers;
							if (ImGui::MenuItem("Line Numbers", nullptr, &lineNumbers)) {
								config.showLineNumbers = lineNumbers;
								ApplyConfig(editor, config);
							}

							bool wordWrap = config.wordWrap;
							if (ImGui::MenuItem("Word Wrap", nullptr, &wordWrap)) {
								config.wordWrap = wordWrap;
								ApplyConfig(editor, config);
							}

							bool showWhitespace = config.showWhitespace;
							if (ImGui::MenuItem("Show Whitespace", nullptr, &showWhitespace)) {
								config.showWhitespace = showWhitespace;
								ApplyConfig(editor, config);
							}

							bool syntaxHighlighting = config.enableSyntaxHighlighting;
							if (ImGui::MenuItem("Syntax Highlighting", nullptr, &syntaxHighlighting)) {
								config.enableSyntaxHighlighting = syntaxHighlighting;
								ApplyConfig(editor, config);
							}
							ImGui::EndMenu();
						}

						ImGui::EndMenuBar();
					}
				}

				// Get the current cursor position and available size
				ImVec2 cursorPos = ImGui::GetCursorScreenPos();
				ImVec2 availableSize = ImGui::GetContentRegionAvail();

				availableSize.x = std::max(50.0f, availableSize.x);
				availableSize.y = std::max(50.0f, availableSize.y);

				editor->Render(cursorPos, availableSize);

				// Advance the cursor to consume the space
				ImGui::Dummy(availableSize);
			}
			ImGui::EndChild();

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
			else {
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

		// Cleanup editor
		static void CleanupEditor(std::string* value) {
			auto& editorMap = GetEditorMap();
			auto& configMap = GetConfigMap();
			uintptr_t uniqueKey = reinterpret_cast<uintptr_t>(value);

			editorMap.erase(uniqueKey);
			configMap.erase(uniqueKey);
		}

		// Clear focus
		static void ClearFocus() {
			Utils::ZepFocusTracker::ClearFocus();
		}

		// Cleanup all
		static void Cleanup() {
			GetEditorMap().clear();
			GetConfigMap().clear();
			Utils::ZepFocusTracker::ClearFocus();
		}
	};

} // namespace UISchema