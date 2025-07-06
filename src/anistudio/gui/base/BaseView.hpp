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
 * For commercial license iformation, please contact legal@kframe.ai.
 */

#pragma once
#include "ECS.h"
#include "ViewTypes.hpp"
#include <imgui.h>
#include <nlohmann/json.hpp>
#include <string>

namespace GUI {
	class BaseView {
	public:
		std::string viewName = "Base_View";
		std::string category = "";

		BaseView(ECS::EntityManager &entityMgr) : mgr(entityMgr) {}
		virtual ~BaseView() {}

		inline const ViewListID GetID() const { return viewID; }

		virtual void Init() {}
		virtual void Update(const float deltaT) {}

		// CRITICAL FIX: Remove the "= 0" - make it virtual, not pure virtual
		// Your derived classes DO implement this method
		virtual void Render() {
			// Default implementation for safety
			if (ImGui::Begin(viewName.c_str())) {
				ImGui::Text("Default BaseView Render");
				ImGui::Text("ViewID: %zu", viewID);
				ImGui::Text("This view should override Render()");
			}
			ImGui::End();
		}

		virtual void HandleInput(int key, int action) {}

		virtual nlohmann::json Serialize() const {
			nlohmann::json j;
			j["viewName"] = viewName;
			return j;
		}

		virtual void Deserialize(const nlohmann::json &j) {
			if (j.contains("viewName"))
				viewName = j["viewName"];
		}

	protected:
		ECS::EntityManager &mgr;

	private:
		friend class ViewManager;
		ViewListID viewID;
	};
} // namespace GUI