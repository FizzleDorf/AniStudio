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

#include "Base/BaseView.hpp"
#include "EntityManager.hpp"
#include "ZepUtils.hpp"
#include <memory>
#include <string>

namespace GUI {

	class ZepView : public BaseView {
	public:
		ZepView(ECS::EntityManager& entityMgr) : BaseView(entityMgr) {
			viewName = "Text Editor";
			textEditor = std::make_unique<Utils::ZepTextEditor>();
		}

		void Init() override {
			if (!textEditor->Initialize()) {
				std::cerr << "Failed to initialize ZepTextEditor" << std::endl;
			}
		}

		void Update(const float deltaT) override {
			// Update method should only handle logic updates, not rendering
			// Zep handles its own updates during the Display call
		}

		void Render() override {
			if (ImGui::Begin(viewName.c_str())) {

				// Get the available content region for the editor
				ImVec2 contentSize = ImGui::GetContentRegionAvail();
				ImVec2 editorPos = ImGui::GetCursorScreenPos();

				// Ensure we have a minimum size
				if (contentSize.x < 100) contentSize.x = 100;
				if (contentSize.y < 100) contentSize.y = 100;

				textEditor->Render(editorPos, contentSize, true);  // true = create child window with menu bar
			}
			ImGui::End();
		}

		// Helper methods for text operations
		std::string GetText() const {
			return textEditor->GetText();
		}

		void SetText(const std::string& text) {
			textEditor->SetText(text);
		}

		bool LoadFile(const std::string& filePath) {
			return textEditor->LoadFile(filePath);
		}

		bool SaveFile(const std::string& filePath) {
			return textEditor->SaveFile(filePath);
		}

	private:
		std::unique_ptr<Utils::ZepTextEditor> textEditor;
	};

} // namespace GUI