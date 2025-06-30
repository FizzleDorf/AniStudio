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

 // MenuBar.hpp
#pragma once
#include "GUI.h"
#include "FilePaths.hpp"
#include "pch.h"

namespace GUI {

	// View State Management Structure with ViewListIDs
	struct ViewStates {
		// Core Views
		bool settingsWindowOpen = false;
		bool debugWindowOpen = false;
		bool convertWindowOpen = false;
		bool viewsWindowOpen = false;
		bool pluginsWindowOpen = false;
		bool helpWindowOpen = false;

		// Tool Views
		bool diffusionViewOpen = false;
		bool upscaleViewOpen = false;
		bool imageViewOpen = false;
		bool nodeGraphViewOpen = false;
		bool sequencerViewOpen = false;
		bool nodeViewOpen = false;
		bool videoViewOpen = false;
		bool videoSequencerViewOpen = false;
		bool zepViewOpen = false;

		// ViewListIDs for ECS views (0 means not created)
		GUI::ViewListID settingsViewID = 0;
		GUI::ViewListID debugViewID = 0;
		GUI::ViewListID convertViewID = 0;
		GUI::ViewListID viewListManagerViewID = 0;
		GUI::ViewListID pluginViewID = 0;
		GUI::ViewListID helpViewID = 0;

		// Tool ViewListIDs
		GUI::ViewListID diffusionViewID = 0;  // Will contain both DiffusionView and ImageView
		GUI::ViewListID upscaleViewID = 0;
		GUI::ViewListID imageViewID = 0;
		GUI::ViewListID nodeGraphViewID = 0;
		GUI::ViewListID sequencerViewID = 0;
		GUI::ViewListID nodeViewID = 0;
		GUI::ViewListID videoViewID = 0;
		GUI::ViewListID videoSequencerViewID = 0;
		GUI::ViewListID zepViewID = 0;
	};

	// Global view states instance
	extern ViewStates g_viewStates;

	// Main menu bar function - now takes the actual managers being used
	void ShowMenuBar(GLFWwindow* window, GUI::ViewManager& viewManager, ECS::EntityManager& entityManager);

	// Individual view management functions that handle ECS views
	void ShowOrCreateSettingsView();
	void ShowOrCreateDebugView();
	void ShowOrCreateConvertView();
	void ShowOrCreateViewsView();
	void ShowOrCreatePluginsView();
	void ShowOrCreateHelpView();
	void ShowOrCreateDiffusionView();  // Contains both DiffusionView and ImageView
	void ShowOrCreateUpscaleView();
	void ShowOrCreateImageView();
	void ShowOrCreateNodeGraphView();
	void ShowOrCreateSequencerView();
	void ShowOrCreateNodeView();
	void ShowOrCreateVideoView();
	void ShowOrCreateVideoSequencerView();
	void ShowOrCreateZepView();

	// Helper functions for view lifecycle
	void DestroyViewIfClosed(GUI::ViewListID& viewID, bool& isOpen);
	void DestroyAllClosedViews();

	// Helper function to render all active views
	void RenderAllViews();

} // namespace GUI