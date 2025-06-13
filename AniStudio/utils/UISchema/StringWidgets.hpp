/*
 * StringWidgets.hpp - Simple Zep editor integration
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

		// SIMPLE Zep editor - EXACT demo pattern
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

			// If menu bar enabled, create child window with menu bar
			if (config.showMenuBar) {
				ImVec2 availableSize = ImGui::GetContentRegionAvail();
				if (availableSize.x < 100) availableSize.x = 100;
				if (availableSize.y < 100) availableSize.y = 100;

				std::string childId = "ZepEditor##" + std::to_string(uniqueKey);

					editor->RenderMenuBar();

					// Render editor in remaining space - EXACT demo pattern
					ImVec2 cursorPos = ImGui::GetCursorScreenPos();
					ImVec2 contentSize = ImGui::GetContentRegionAvail();
					editor->Render(cursorPos, contentSize);
			}
			else {
				// No menu bar - render editor directly like demo
				ImVec2 cursorPos = ImGui::GetCursorScreenPos();
				ImVec2 availableSize = ImGui::GetContentRegionAvail();
				editor->Render(cursorPos, availableSize);
			}

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