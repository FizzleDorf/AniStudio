// AssetsView.hpp
#ifndef ASSETSVIEW_HPP
#define ASSETSVIEW_HPP

#include "BaseView.hpp"
#include "ContextMenuUtils.hpp"
#include "ThumbnailUtils.hpp"
#include "ThumbnailFilters.hpp"
#include <vector>
#include <string>
#include <filesystem>
#include <unordered_map>
#include <set>
#include <limits>

namespace GUI {

    class AssetsView : public BaseView {
    public:
        static constexpr const char* GetMetadataJSON() {
            return R"({
            "displayName": "Assets",
            "category": "Viewers",
            "description": "Displays assets from the project's assets directory"
        })";
        }

        AssetsView(ECS::EntityManager& mgr, ViewManager& vm);
        ~AssetsView() = default;

        void Init() override;
        void Update(float deltaT) override;
        void Render() override;
        nlohmann::json Serialize() const override;
        void Deserialize(const nlohmann::json& j) override;

    private:
        std::vector<std::filesystem::path> assetFiles;
        std::string assetsPath;
        bool needsRefresh;
        std::vector<ECS::EntityID> loadedEntities;
        std::unordered_map<std::string, ECS::EntityID> pathToEntity;
        std::set<std::string> loadedPaths;
        std::unique_ptr<Utils::ContextMenuUtils> contextMenuUtils;

        ThumbnailFilters::Settings filterSettings;

        ECS::EntityID selectedEntityID = 0;
        bool needsSort = true;

        void RefreshAssets();
        void LoadNewAssets();
        void RenderAssetGrid();
        void LoadAsset(const std::filesystem::path& path);
        void ClearLoadedAssets();
        void RenderMenuBar();
        bool CopyFileToAssets(const std::string& sourcePath);
        void SelectAssetEntity(ECS::EntityID entityID);
        void CopyMetadataFromFile(const std::string& filePath);
        void ApplyFiltersAndSort();
    };

}

#endif