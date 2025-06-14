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
#include <unordered_set>
#include <memory>
#include <iostream>
#include "UISchemaUtils.hpp"
#include "ZepUtils.hpp"

namespace UISchema {

	static const std::string DEFAULT_STRING_WIDGET = "input_text";

	// Enhanced config struct with vim mode support
	struct ZepEditorConfig {
		bool showLineNumbers = true;
		bool wordWrap = true;
		bool readOnly = false;
		bool showWhitespace = false;
		bool enableSyntaxHighlighting = true;
		bool autoIndent = true;
		bool showSearch = false;
		bool showMenuBar = true;
		std::string theme = "dark";
		std::string mode = "standard";  // "standard" or "vim"
		float fontSize = 14.0f;
	};

	class StringWidgets {
	public:
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

		// Create editor
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

		// Apply config - SIMPLIFIED
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
			editor->SetFontSize(config.fontSize);
			editor->SetMode(config.mode);
			editor->SetShowMenuBar(config.showMenuBar);
		}

		// Parse schema options into config
		static ZepEditorConfig ParseSchemaOptions(const nlohmann::json& options) {
			ZepEditorConfig config;

			config.showLineNumbers = GetSchemaValue<bool>(options, "showLineNumbers", config.showLineNumbers);
			config.wordWrap = GetSchemaValue<bool>(options, "wordWrap", config.wordWrap);
			config.readOnly = GetSchemaValue<bool>(options, "readOnly", config.readOnly);
			config.showWhitespace = GetSchemaValue<bool>(options, "showWhitespace", config.showWhitespace);
			config.enableSyntaxHighlighting = GetSchemaValue<bool>(options, "enableSyntaxHighlighting", config.enableSyntaxHighlighting);
			config.autoIndent = GetSchemaValue<bool>(options, "autoIndent", config.autoIndent);
			config.showSearch = GetSchemaValue<bool>(options, "showSearch", config.showSearch);
			config.showMenuBar = GetSchemaValue<bool>(options, "showMenuBar", config.showMenuBar);
			config.theme = GetSchemaValue<std::string>(options, "theme", config.theme);
			config.mode = GetSchemaValue<std::string>(options, "mode", config.mode);
			config.fontSize = GetSchemaValue<float>(options, "fontSize", config.fontSize);

			return config;
		}

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

		// Track which editors have been configured from schema
		static std::unordered_set<uintptr_t>& GetConfiguredEditors() {
			static std::unordered_set<uintptr_t> configuredEditors;
			return configuredEditors;
		}

		// ZepUtils handles child window creation
		static bool RenderZepEditor(const std::string& label, std::string* value, const nlohmann::json& options = {}) {
			auto editor = GetOrCreateEditor(value);
			if (!editor) {
				ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Failed to create Zep editor");
				return false;
			}

			auto& configMap = GetConfigMap();
			auto& configuredEditors = GetConfiguredEditors();
			uintptr_t uniqueKey = reinterpret_cast<uintptr_t>(value);

			// ONLY apply schema config ONCE when editor is first created
			if (configuredEditors.find(uniqueKey) == configuredEditors.end()) {
				ZepEditorConfig schemaConfig = ParseSchemaOptions(options);
				configMap[uniqueKey] = schemaConfig;  // Set initial config from schema
				ApplyConfig(editor, schemaConfig);
				configuredEditors.insert(uniqueKey);  // Mark as configured
			}

			bool modified = false;

			// Get available content size
			ImVec2 contentSize = ImGui::GetContentRegionAvail();
			if (contentSize.x < 100) contentSize.x = 100;
			if (contentSize.y < 100) contentSize.y = 100;

			// Let ZepUtils handle child window creation, menu bar, focus, etc.
			editor->Render(ImVec2(0, 0), contentSize, true);  // true = create child window

			// Check if content changed
			std::string currentText = editor->GetText();
			if (currentText != *value) {
				*value = currentText;
				modified = true;
			}

			return modified;
		}

		// Separate widget types for different modes
		static bool RenderZepEditorVim(const std::string& label, std::string* value, const nlohmann::json& options = {}) {
			nlohmann::json vimOptions = options;
			vimOptions["mode"] = "vim";
			return RenderZepEditor(label, value, vimOptions);
		}

		static bool RenderZepEditorStandard(const std::string& label, std::string* value, const nlohmann::json& options = {}) {
			nlohmann::json standardOptions = options;
			standardOptions["mode"] = "standard";
			return RenderZepEditor(label, value, standardOptions);
		}

		static bool Render(const std::string& label, std::string* value, const std::string& widgetType, const nlohmann::json& schema) {
			if (widgetType == "input_text") {
				return RenderInputText(label, value, schema);
			}
			else if (widgetType == "zep_editor" || widgetType == "dynamic_textarea") {
				return RenderZepEditor(label, value, schema);
			}
			else if (widgetType == "zep_editor_vim") {
				return RenderZepEditorVim(label, value, schema);
			}
			else if (widgetType == "zep_editor_standard") {
				return RenderZepEditorStandard(label, value, schema);
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