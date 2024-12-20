#pragma once

#include "ECS.h"
#include "ImageComponent.hpp"
#include "VideoComponent.hpp"
#include <stdexcept>
#include <string>
#include <vector>

using namespace ECS;

class LoadedHeaps {
public:
    // Adds an image to the loaded images vector
    void AddImage(const ImageComponent &image) { loadedImages.push_back(image); }

    // Adds a video to the loaded videos vector
    void AddVideo(const VideoComponent &video) { loadedVideos.push_back(video); }

    // Removes an image by index
    void RemoveImage(size_t index) {
        if (index >= loadedImages.size()) {
            throw std::out_of_range("Image index out of range");
        }
        loadedImages.erase(loadedImages.begin() + index);
    }

    // Removes a video by index
    void RemoveVideo(size_t index) {
        if (index >= loadedVideos.size()) {
            throw std::out_of_range("Video index out of range");
        }
        loadedVideos.erase(loadedVideos.begin() + index);
    }

    // Retrieves a const reference to an image by index
    ImageComponent &GetImage(size_t index) {
        if (index >= loadedImages.size()) {
            throw std::out_of_range("Image index out of range");
        }
        return loadedImages[index];
    }
    VideoComponent &GetVideo(size_t index) {
        if (index >= loadedVideos.size()) {
            throw std::out_of_range("Video index out of range");
        }
        return loadedVideos[index];
    }

    // Retrieves a mutable reference to the vector of images
    std::vector<ImageComponent> &GetImages() { return loadedImages; }
    std::vector<VideoComponent> &GetVideos() { return loadedVideos; }

    // Retrieves a const reference to the vector of images
    const std::vector<ImageComponent> &GetImages() const { return loadedImages; }
    const std::vector<VideoComponent> &GetVideos() const { return loadedVideos; }

private:
    std::vector<ImageComponent> loadedImages; // Vector to hold loaded images
    std::vector<VideoComponent> loadedVideos; // Vector to hold loaded videos
};

extern LoadedHeaps loadedMedia;
