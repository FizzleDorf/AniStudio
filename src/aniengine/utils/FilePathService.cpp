#include "FilePathService.hpp"

namespace Utils {

	std::unique_ptr<FilePaths> FilePathService::filePathsInstance = nullptr;
	std::mutex FilePathService::instanceMutex;

} // namespace Utils