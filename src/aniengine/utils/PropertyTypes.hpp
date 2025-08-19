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

#include <variant>
#include <vector>
#include <string>
#include <unordered_map>

 // Forward declarations for ImGui types (to avoid including imgui.h in engine)
struct ImVec2;
struct ImVec4;

namespace Engine {

	// Centralized property variant types used by both ECS and UI systems
	// These types bridge the gap between engine data and UI representation

	using PropertyVariant = std::variant<
		bool*,
		int*,
		float*,
		double*,
		std::string*,
		ImVec2*,
		ImVec4*,
		std::vector<std::string>*,
		void*  // For component pointers and custom widget data
	>;

	using PropertyMap = std::unordered_map<std::string, PropertyVariant>;

} // namespace Engine

// Alias for backward compatibility with existing UISchema code
namespace UISchema {
	using PropertyVariant = Engine::PropertyVariant;
	using PropertyMap = Engine::PropertyMap;
} // namespace UISchema