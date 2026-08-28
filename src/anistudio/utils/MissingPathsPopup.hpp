#pragma once
#include <memory>
#include <string>

namespace ECS { class FilePathSystem; }

namespace Utils {
    void SetDefaultPath(const std::string& key, const std::string& defaultPath);
    std::string GetDefaultPath(const std::string& key);
    void CheckMissingPaths(ECS::FilePathSystem* fileSys);
    void RenderMissingPathsPopup();
}