#pragma once
#include <memory>
#include <string>

namespace ECS { class FilePathSystem; }

namespace Utils {
    void SetDefaultPath(const std::string& key, const std::string& defaultPath);
    void CheckMissingPaths(std::shared_ptr<ECS::FilePathSystem> fileSys);
    void RenderMissingPathsPopup();
}