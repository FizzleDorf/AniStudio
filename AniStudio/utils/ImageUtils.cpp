#include "ImageUtils.hpp"

namespace Utils {
	// This mutex is shared across ImageUtils and SDcppUtils for thread-safe stb_image operations
	std::mutex stbi_mutex;
}