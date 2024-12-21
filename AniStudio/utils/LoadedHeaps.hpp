#pragma once

#include "EntityManager.hpp"
#include "ImageComponent.hpp"
#include "VideoComponent.hpp"
#include <stdexcept>
#include <string>
#include <vector>

using namespace ECS;

class LoadedHeaps {
public:
    void AddImage(const ImageComponent &image) {
        std::lock_guard<std::mutex> lock(mutex);
        loadedImages.push_back(image);
    }

    void AddVideo(const VideoComponent &video) {
        std::lock_guard<std::mutex> lock(mutex);
        loadedVideos.push_back(video);
    }

    void RemoveImage(const size_t index) {
        std::lock_guard<std::mutex> lock(mutex);
        if (index >= loadedImages.size()) {
            throw std::out_of_range("Image index out of range");
        }
        loadedImages.erase(loadedImages.begin() + index);
    }

    void RemoveVideo(const size_t index) {
        std::lock_guard<std::mutex> lock(mutex);
        if (index >= loadedVideos.size()) {
            throw std::out_of_range("Video index out of range");
        }
        loadedVideos.erase(loadedVideos.begin() + index);
    }

    ImageComponent &GetImage(const size_t index) {
        std::lock_guard<std::mutex> lock(mutex);
        if (index >= loadedImages.size()) {
            throw std::out_of_range("Image index out of range");
        }
        return loadedImages[index];
    }

    VideoComponent &GetVideo(const size_t index) {
        std::lock_guard<std::mutex> lock(mutex);
        if (index >= loadedVideos.size()) {
            throw std::out_of_range("Video index out of range");
        }
        return loadedVideos[index];
    }

    std::vector<ImageComponent> &GetImages() { return loadedImages; }
    std::vector<VideoComponent> &GetVideos() { return loadedVideos; }

private:
    std::vector<ImageComponent> loadedImages;
    std::vector<VideoComponent> loadedVideos;
    std::mutex mutex;
};

extern LoadedHeaps loadedMedia;
