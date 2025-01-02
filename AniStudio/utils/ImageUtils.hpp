#pragma once

#include <opencv2/opencv.hpp>
#include <stb_image.h>
#include <stb_image_write.h>
#include <string>

namespace Util {

unsigned char *LoadImageWithSTB(const std::string &filePath, int &width, int &height, int &channels);
void SaveImageWithSTB(const std::string &filePath, unsigned char *imageData, int width, int height, int channels,
                      bool flipVertically = false);
cv::Mat LoadImageAsMat(const std::string &filePath, int flags = cv::IMREAD_UNCHANGED);
void SaveMatAsImage(const std::string &filePath, const cv::Mat &mat, bool flipVertically = false);
cv::Mat STBToMat(unsigned char *imageData, int width, int height, int channels);
unsigned char *MatToSTB(const cv::Mat &mat, int &channels);

} // namespace Util
