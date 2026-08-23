#ifndef ASSETSVIEW_HPP
#define ASSETSVIEW_HPP

#include "BaseView.hpp"
#include "ContextMenuUtils.hpp"
#include "ThumbnailUtils.hpp"
#include <vector>
#include <string>
#include <filesystem>
#include <unordered_map>
#include <set>

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

    private:
        std::vector<std::filesystem::path> assetFiles;
        std::string assetsPath;
        bool needsRefresh;
        std::vector<ECS::EntityID> loadedEntities;
        std::unordered_map<std::string, ECS::EntityID> pathToEntity;
        std::set<std::string> loadedPaths;
        std::unique_ptr<Utils::ContextMenuUtils> contextMenuUtils;

        Thumbnail::DisplayMode currentDisplayMode = Thumbnail::DisplayMode::Detailed;
        enum class FileTypeFilter { All, Image, Video, Audio };
        FileTypeFilter currentTypeFilter = FileTypeFilter::All;
        enum class SortMode { Name, Size, Date };
        SortMode currentSort = SortMode::Name;
        bool sortAscending = true;
        ECS::EntityID selectedEntityID = 0;

        void RefreshAssets();
        void LoadNewAssets();
        void RenderAssetGrid();
        void LoadAsset(const std::filesystem::path& path);
        void ClearLoadedAssets();
        void ApplyFiltersAndSort();
        void RenderMenuBar();
        bool CopyFileToAssets(const std::string& sourcePath);
        Thumbnail::ThumbnailData BuildThumbnailData(const std::filesystem::path& path, ECS::EntityID entityID);
        void SelectAssetEntity(ECS::EntityID entityID);
        void CopyMetadataFromFile(const std::string& filePath);
    };

} // namespace GUI

#endif