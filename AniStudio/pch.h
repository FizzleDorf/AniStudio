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

// Standard libs
#include <set>
#include <map>
#include <queue>
#include <vector>
#include <string>
#include <cstring>
#include <memory>
#include <cassert>
#include <sstream>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <functional>
#include <stdio.h>         
#include <stdlib.h> 
#include <cstdlib>
#include <future>
#include <mutex>
#include <atomic>
#include <thread>
#include <filesystem>
#include <condition_variable>
#include <unordered_set>

// JSON
#include <nlohmann/json.hpp>

// Vulkan
// #include <vulkan/vulkan.h> (

// GLEW first to avoid conflicts with OpenGL macros
#include <GL/glew.h>

// GLFW
#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// GLM
#include <glm/glm.hpp>


// Volk headers
// #ifdef IMGUI_IMPL_VULKAN_USE_VOLK
// #define VOLK_IMPLEMENTATION
// #include <volk.h>
// #endif

// ImGui
#include "imgui.h"
 
//http request headers
// #include "cpr/cpr.h"
// #include "curl/curl.h"

// AniStudio Constants
#include "Constants.hpp"