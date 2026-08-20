#ifndef ASSETSVIEW_HPP
#define ASSETSVIEW_HPP

#include "BaseView.hpp"
#include "ContextMenuUtils.hpp"
#include <vector>
#include <string>
#include <filesystem>

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
        std::unique_ptr<Utils::ContextMenuUtils> contextMenuUtils;

        enum class FileTypeFilter { All, Image, Video, Audio };
        FileTypeFilter currentTypeFilter = FileTypeFilter::All;
        enum class SortMode { Name, Size, Date };
        SortMode currentSort = SortMode::Name;
        bool sortAscending = true;

        void RefreshAssets();
        void RenderAssetGrid();
        void LoadAsset(const std::filesystem::path& path);
        void ClearLoadedAssets();
        void ApplyFiltersAndSort();
        void RenderFilters();
        void RenderThumbnail(const std::filesystem::path& path, size_t index, float thumbnailSize);
    };

} // namespace GUI

#endif