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

// Core views
#include "DebugView.hpp"
#include "SettingsView.hpp"
#include "ViewListManagerView.hpp"
#include "PluginView.hpp"
#include "HelpView.hpp"
#include "MenuBar.hpp"

// Project management views (from gui/projects/)
#include "project/ProjectManagerView.hpp"
#include "project/NewProjectView.hpp"
#include "project/LoadProjectView.hpp"

// AI Generation views
#include "DiffusionView.hpp"
#include "UpscaleView.hpp"
#include "ConvertView.hpp"

// Media views
#include "ImageView.hpp"
#include "VideoView.hpp"

// Node Editor views
#include "NodeGraphView.hpp"
#include "NodeView.hpp"

// Animation views
#include "SequencerView.hpp"

// Utility views
#include "ConvertView.hpp"
#include "PluginView.hpp"

// Optional views (if available)
#ifdef ZEP_AVAILABLE
#include "ZepView.hpp"
#endif