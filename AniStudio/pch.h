#pragma once

// Standard libs
#include <set>
#include <map>
#include <queue>
#include <vector>
#include <string>
#include <memory>
#include <cassert>
#include <sstream>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <functional>
#include <stdio.h>         
#include <stdlib.h> 
#include <future>
#include <mutex>
#include <atomic>
#include <thread>

// JSON
#include <nlohmann/json.hpp>

// Vulkan
#include <vulkan/vulkan.h>

// GLEW first to avoid conflicts with OpenGL macros
#include <GL/glew.h>

// GLFW
#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// Volk headers
#ifdef IMGUI_IMPL_VULKAN_USE_VOLK
#define VOLK_IMPLEMENTATION
#include <volk.h>
#endif

// ImGui
#include "imgui.h"
#include "ImGuiFileDialog.h"
 
//http request headers
// #include "cpr/cpr.h"
// #include "curl/curl.h"