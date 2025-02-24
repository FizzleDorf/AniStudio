#pragma once
#include "ECS.h"
#include "ImageUtils.hpp"
#include <exiv2/exiv2.hpp>
#include <pch.h>
#include <nlohmann/json.hpp>

namespace ECS {

class ImageSystem : public BaseSystem {
public:
    ImageSystem(EntityManager &entityMgr) : BaseSystem(entityMgr) {
        sysName = "Image_System";
        AddComponentSignature<ImageComponent>();
        currentImageEntity = 0;
        Exiv2::XmpParser::initialize();
    }

    ~ImageSystem() { Exiv2::XmpParser::terminate(); }

    void LoadImage(const std::string &path) {
        try {
            EntityID newEntity = mgr.AddNewEntity();
            auto &imageComp = mgr.GetComponent<ImageComponent>(newEntity);

            cv::Mat image = Util::LoadImageAsMat(path);
            if (image.empty()) {
                throw std::runtime_error("Failed to load image: " + path);
            }

            imageComp.width = image.cols;
            imageComp.height = image.rows;
            imageComp.channels = image.channels();
            imageComp.data = std::vector<unsigned char>(image.data, image.data + image.total() * image.channels());
            imageComp.filePath = path;
            imageComp.fileName = std::filesystem::path(path).filename().string();

            currentImageEntity = newEntity;
            LoadMetadata(newEntity);
        } catch (const std::exception &e) {
            std::cerr << "Error loading image: " << e.what() << std::endl;
        }
    }

    void SaveImage(EntityID entity) {
        try {
            auto &imageComp = mgr.GetComponent<ImageComponent>(entity);
            if (imageComp.data.empty()) {
                throw std::runtime_error("No image data to save");
            }

            std::filesystem::path savePath = std::filesystem::path(imageComp.filePath) / imageComp.fileName;
            cv::Mat image(imageComp.height, imageComp.width, CV_8UC(imageComp.channels), imageComp.data.data());

            Util::SaveMatAsImage(savePath.string(), image);
            SaveMetadata(entity);
        } catch (const std::exception &e) {
            std::cerr << "Error saving image: " << e.what() << std::endl;
        }
    }

    void SaveMetadata(EntityID entity) {
        try {
            auto &imageComp = mgr.GetComponent<ImageComponent>(entity);
            auto image = Exiv2::ImageFactory::open(imageComp.filePath);
            if (!image.get()) {
                throw std::runtime_error("Failed to open image for metadata");
            }

            image->readMetadata();
            Exiv2::XmpData &xmpData = image->xmpData();

            nlohmann::json metadata;
            metadata["version"] = 1;
            metadata["entityData"] = mgr.SerializeEntity(entity);

            std::string jsonStr = metadata.dump();
            xmpData["Xmp.AniStudio.Metadata"] = jsonStr;

            image->setXmpData(xmpData);
            image->writeMetadata();
        } catch (const std::exception &e) {
            std::cerr << "Error saving metadata: " << e.what() << std::endl;
        }
    }

    void LoadMetadata(EntityID entity) {
        try {
            auto &imageComp = mgr.GetComponent<ImageComponent>(entity);
            auto image = Exiv2::ImageFactory::open(imageComp.filePath);
            if (!image.get()) {
                return;
            }

            image->readMetadata();
            Exiv2::XmpData &xmpData = image->xmpData();

            auto it = xmpData.findKey(Exiv2::XmpKey("Xmp.AniStudio.Metadata"));
            if (it != xmpData.end()) {
                std::string jsonStr = it->toString();
                nlohmann::json metadata = nlohmann::json::parse(jsonStr);

                if (metadata.contains("entityData")) {
                    mgr.DeserializeEntity(metadata["entityData"]);
                }
            }
        } catch (const std::exception &e) {
            std::cerr << "Error loading metadata: " << e.what() << std::endl;
        }
    }

    void SetCurrentImage(EntityID entity) {
        if (mgr.HasComponent<ImageComponent>(entity)) {
            currentImageEntity = entity;
        }
    }

    EntityID GetCurrentImage() const { return currentImageEntity; }

    std::vector<EntityID> GetLoadedImages() const {
        std::vector<EntityID> loadedImages;
        for (const auto &entity : entities) {
            if (mgr.HasComponent<ImageComponent>(entity)) {
                loadedImages.push_back(entity);
            }
        }
        return loadedImages;
    }

private:
    EntityID currentImageEntity;
};

} // namespace ECS