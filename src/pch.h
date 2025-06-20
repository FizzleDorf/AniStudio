#pragma once

// Standard library
#include <iostream>
#include <memory>
#include <utility>
#include <algorithm>
#include <functional>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <queue>
#include <stack>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <future>
#include <chrono>
#include <random>
#include <array>
#include <cassert>
#include <filesystem>
#include <variant>

// Platform-specific includes
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <commdlg.h>
#include <shellapi.h>
#endif

// Essential third-party libraries
#include <nlohmann/json.hpp>

// OpenGL and graphics
#include <GL/glew.h>

// ImGui - Essential for plugin GUI development
#include <imgui.h>

// GLM for math
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// GLFW for window management (only if available)
#ifdef GLFW_VERSION
#include <GLFW/glfw3.h>
#else
	// Try to include it, but don't fail if not available
#pragma warning(push)
#pragma warning(disable: 4996)
#include <GLFW/glfw3.h>
#pragma warning(pop)
#endif

// Common utilities for plugins
using json = nlohmann::json;