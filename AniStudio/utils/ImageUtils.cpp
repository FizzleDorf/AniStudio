#include "ImageUtils.hpp"

namespace Util {

unsigned char *LoadImageWithSTB(const std::string &filePath, int &width, int &height, int &channels) {
    unsigned char *imageData = stbi_load(filePath.c_str(), &width, &height, &channels, 0);
    if (!imageData) {
        throw std::runtime_error("Failed to load image with stb_image: " + filePath);
    }
    return imageData;
}

void SaveImageWithSTB(const std::string &filePath, unsigned char *imageData, int width, int height, int channels,
                      bool flipVertically) {
    unsigned char *dataToWrite = imageData;

    if (flipVertically) {
        int stride = width * channels;
        unsigned char *flippedData = new unsigned char[width * height * channels];
        for (int y = 0; y < height; ++y) {
            memcpy(flippedData + (height - 1 - y) * stride, imageData + y * stride, stride);
        }
        dataToWrite = flippedData;
    }

    std::string ext = filePath.substr(filePath.find_last_of('.') + 1);
    int success = 0;

    if (ext == "png") {
        success = stbi_write_png(filePath.c_str(), width, height, channels, dataToWrite, width * channels);
    } else if (ext == "jpg" || ext == "jpeg") {
        success = stbi_write_jpg(filePath.c_str(), width, height, channels, dataToWrite, 100); // Quality: 100
    } else if (ext == "bmp") {
        success = stbi_write_bmp(filePath.c_str(), width, height, channels, dataToWrite);
    } else if (ext == "tga") {
        success = stbi_write_tga(filePath.c_str(), width, height, channels, dataToWrite);
    } else if (ext == "hdr") {
        if (channels != 3 && channels != 4) {
            throw std::runtime_error("HDR format requires 3 or 4 channels (RGB or RGBA)");
        }
        std::vector<float> hdrData(width * height * channels);
        for (int i = 0; i < width * height * channels; ++i) {
            hdrData[i] = dataToWrite[i] / 255.0f;
        }
        success = stbi_write_hdr(filePath.c_str(), width, height, channels, hdrData.data());
    } else {
        throw std::runtime_error("Unsupported file format: " + ext);
    }

    if (!success) {
        throw std::runtime_error("Failed to save image with stb_image_write: " + filePath);
    }

    if (flipVertically) {
        delete[] dataToWrite;
    }
}


cv::Mat LoadImageAsMat(const std::string &filePath, int flags) {
    cv::Mat mat = cv::imread(filePath, flags);
    if (mat.empty()) {
        throw std::runtime_error("Failed to load image with OpenCV: " + filePath);
    }
    return mat;
}

void SaveMatAsImage(const std::string &filePath, const cv::Mat &mat, bool flipVertically) {
    if (mat.empty()) {
        throw std::runtime_error("Cannot save an empty Mat as an image.");
    }

    cv::Mat image = mat;

    if (flipVertically) {
        cv::flip(mat, image, 0);
    }

    if (!cv::imwrite(filePath, image)) {
        throw std::runtime_error("Failed to save image with OpenCV: " + filePath);
    }
}

cv::Mat STBToMat(unsigned char *imageData, int width, int height, int channels) {
    int type = (channels == 1) ? CV_8UC1 : (channels == 3) ? CV_8UC3 : CV_8UC4;
    cv::Mat mat(height, width, type, imageData);
    if (channels == 3) {
        cv::cvtColor(mat, mat, cv::COLOR_RGB2BGR);
    }
    return mat.clone();
}

unsigned char *MatToSTB(const cv::Mat &mat, int &channels) {
    if (mat.empty()) {
        throw std::runtime_error("Cannot convert an empty Mat to stb_image format.");
    }

    channels = mat.channels();
    cv::Mat convertedMat = mat;

    if (channels == 3) {
        cv::cvtColor(mat, convertedMat, cv::COLOR_BGR2RGB);
    }

    unsigned char *imageData = new unsigned char[convertedMat.total() * convertedMat.elemSize()];
    memcpy(imageData, convertedMat.data, convertedMat.total() * convertedMat.elemSize());
    return imageData;
}

} // namespace Util
