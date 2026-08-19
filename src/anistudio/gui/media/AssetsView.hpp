#ifndef ASSETSVIEW_HPP
#define ASSETSVIEW_HPP

#include "BaseView.hpp"
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

        void RefreshAssets();
        void RenderAssetGrid();
        void LoadAsset(const std::filesystem::path& path);
        void ClearLoadedAssets();
    };

} // namespace GUI

#endif