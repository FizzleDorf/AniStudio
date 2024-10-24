#pragma once

#include <iostream>

void LogCallback(sd_log_level_t level, const char *text, void *data) {
    switch (level) {
    case SD_LOG_DEBUG:
        std::cout << "[DEBUG]: " << text << std::endl;
        break;
    case SD_LOG_INFO:
        std::cout << "[INFO]: " << text << std::endl;
        break;
    case SD_LOG_WARN:
        std::cout << "[WARNING]: " << text << std::endl;
        break;
    case SD_LOG_ERROR:
        std::cerr << "[ERROR]: " << text << std::endl;
        break;
    default:
        std::cerr << "[UNKNOWN LOG LEVEL]: " << text << std::endl;
        break;
    }
}

void ProgressCallback(int step, int steps, float time, void *data) {
    std::cout << "Progress: Step " << step << " of " << steps << " | Time: " << time << "s" << std::endl;
}
