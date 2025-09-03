#pragma once

// Plugin-specific precompiled header substitute
// Includes only what plugins actually need

// Standard library includes
#include <memory>
#include <vector>
#include <string>
#include <iostream>
#include <algorithm>
#include <functional>
#include <stdexcept>
#include <exception>
#include <typeinfo>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>
#include <unordered_map>
#include <map>
#include <set>
#include <unordered_set>
#include <array>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <cstring>
#include <cassert>

// Platform-specific includes
#ifdef _WIN32
#include <windows.h>
#include <tchar.h>
#endif

// Third-party includes that plugins might need
#include <imgui.h>