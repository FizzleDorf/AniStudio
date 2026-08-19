#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <png.h>
#include <nlohmann/json.hpp>
#include "MetadataUtils.hpp"
#include <memory>
#include <zlib.h>
#include <stb_image.h>

namespace Utils
{
    class PngMetadata
    {
    public:
        static bool WriteMetadataToPNG(const std::string& imagePath, const nlohmann::json& metadata, bool useStealth = false)
        {
            if (useStealth)
                return WriteStealthPNG(imagePath, metadata);
            else
                return WriteStandardPNG(imagePath, metadata);
        }

        static nlohmann::json ReadMetadataFromPNG(const std::string& imagePath)
        {
            return MetadataUtils::LoadMetadataFromPNG(imagePath);
        }

        static std::string CreateUniqueFilename(const std::string& baseFilename, const std::string& directory)
        {
            if (directory.empty())
            {
                std::cerr << "[PngMetadata] Directory is empty, cannot create unique filename" << std::endl;
                return baseFilename;
            }

            try
            {
                std::string validBaseName = baseFilename.empty() ? "AniStudio_output.png" : baseFilename;
                std::filesystem::path directoryPath(directory);
                if (!directoryPath.is_absolute())
                    directoryPath = std::filesystem::current_path() / directoryPath;

                std::error_code ec;
                std::filesystem::create_directories(directoryPath, ec);
                if (ec)
                {
                    std::cerr << "Failed to create directory: " << directoryPath.string() << " - " << ec.message() << std::endl;
                    directoryPath = std::filesystem::current_path();
                    std::filesystem::create_directories(directoryPath, ec);
                    if (ec) throw std::runtime_error("Failed to create fallback directory: " + directoryPath.string());
                }

                std::filesystem::path originalFilePath(validBaseName);
                std::string baseName = originalFilePath.stem().string();
                std::string extension = originalFilePath.extension().string();
                if (!extension.empty() && extension[0] != '.') extension = "." + extension;
                if (extension.empty()) extension = ".png";

                if (!std::filesystem::exists(directoryPath))
                    throw std::runtime_error("Directory does not exist after creation: " + directoryPath.string());

                int highestIndex = 0;
                for (const auto& entry : std::filesystem::directory_iterator(directoryPath))
                {
                    if (entry.is_regular_file())
                    {
                        std::string entryName = entry.path().stem().string();
                        std::string entryExt = entry.path().extension().string();
                        if (entryExt == extension)
                        {
                            std::string prefix = baseName + "-";
                            if (entryName.find(prefix) == 0)
                            {
                                std::string numberPart = entryName.substr(prefix.length());
                                try
                                {
                                    int index = std::stoi(numberPart);
                                    if (index > highestIndex) highestIndex = index;
                                }
                                catch (...) {}
                            }
                        }
                    }
                }

                highestIndex++;
                std::ostringstream formattedIndex;
                formattedIndex << std::setw(5) << std::setfill('0') << highestIndex;
                std::filesystem::path newFilePath = directoryPath / (baseName + "-" + formattedIndex.str() + extension);
                return newFilePath.string();
            }
            catch (const std::exception& e)
            {
                std::cerr << "Error in CreateUniqueFilename: " << e.what() << std::endl;
                return "AniStudio_fallback.png";
            }
        }

        static nlohmann::json CreateGenerationMetadata(const nlohmann::json& entityData,
            const nlohmann::json& additionalInfo = {})
        {
            return MetadataUtils::CreateGenerationMetadata(entityData, additionalInfo);
        }

    private:
        static bool WriteStandardPNG(const std::string& imagePath, const nlohmann::json& metadata)
        {
            FILE* fp = fopen(imagePath.c_str(), "rb");
            if (!fp)
            {
                std::cerr << "Failed to open PNG for reading: " << imagePath << std::endl;
                return false;
            }

            unsigned char header[8];
            if (fread(header, 1, 8, fp) != 8 || png_sig_cmp(header, 0, 8))
            {
                std::cerr << "Not a valid PNG file" << std::endl;
                fclose(fp);
                return false;
            }
            fseek(fp, 0, SEEK_SET);

            png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
            if (!png)
            {
                std::cerr << "Failed to create PNG read struct" << std::endl;
                fclose(fp);
                return false;
            }

            png_infop info = png_create_info_struct(png);
            if (!info)
            {
                std::cerr << "Failed to create PNG info struct" << std::endl;
                png_destroy_read_struct(&png, nullptr, nullptr);
                fclose(fp);
                return false;
            }

            if (setjmp(png_jmpbuf(png)))
            {
                std::cerr << "Error during PNG read initialization" << std::endl;
                png_destroy_read_struct(&png, &info, nullptr);
                fclose(fp);
                return false;
            }

            png_init_io(png, fp);
            png_read_info(png, info);

            png_uint_32 width, height;
            int bit_depth, color_type;
            png_get_IHDR(png, info, &width, &height, &bit_depth, &color_type, nullptr, nullptr, nullptr);

            std::string tempFile = imagePath + ".tmp";
            FILE* out = fopen(tempFile.c_str(), "wb");
            if (!out)
            {
                std::cerr << "Failed to create temporary file" << std::endl;
                png_destroy_read_struct(&png, &info, nullptr);
                fclose(fp);
                return false;
            }

            png_structp pngWrite = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
            if (!pngWrite)
            {
                std::cerr << "Failed to create PNG write struct" << std::endl;
                fclose(out);
                png_destroy_read_struct(&png, &info, nullptr);
                fclose(fp);
                return false;
            }

            png_infop infoWrite = png_create_info_struct(pngWrite);
            if (!infoWrite)
            {
                std::cerr << "Failed to create PNG write info struct" << std::endl;
                png_destroy_write_struct(&pngWrite, nullptr);
                fclose(out);
                png_destroy_read_struct(&png, &info, nullptr);
                fclose(fp);
                return false;
            }

            if (setjmp(png_jmpbuf(pngWrite)))
            {
                std::cerr << "Error during PNG write initialization" << std::endl;
                png_destroy_write_struct(&pngWrite, &infoWrite);
                fclose(out);
                png_destroy_read_struct(&png, &info, nullptr);
                fclose(fp);
                return false;
            }

            png_init_io(pngWrite, out);
            png_set_IHDR(pngWrite, infoWrite, width, height, bit_depth, color_type, PNG_INTERLACE_NONE,
                PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

            std::string metadataStr = metadata.dump();
            std::vector<png_text> texts;

            png_text paramText;
            paramText.compression = PNG_TEXT_COMPRESSION_NONE;
            paramText.key = const_cast<char*>("parameters");
            paramText.text = const_cast<char*>(metadataStr.c_str());
            paramText.text_length = metadataStr.length();
            paramText.itxt_length = 0;
            paramText.lang = nullptr;
            paramText.lang_key = nullptr;
            texts.push_back(paramText);

            png_text softwareText;
            softwareText.compression = PNG_TEXT_COMPRESSION_NONE;
            softwareText.key = const_cast<char*>("Software");
            softwareText.text = const_cast<char*>("AniStudio");
            softwareText.text_length = 9;
            softwareText.itxt_length = 0;
            softwareText.lang = nullptr;
            softwareText.lang_key = nullptr;
            texts.push_back(softwareText);

            png_set_text(pngWrite, infoWrite, texts.data(), static_cast<int>(texts.size()));
            png_write_info(pngWrite, infoWrite);

            std::vector<png_byte> row(png_get_rowbytes(png, info));
            for (png_uint_32 y = 0; y < height; y++)
            {
                png_read_row(png, row.data(), nullptr);
                png_write_row(pngWrite, row.data());
            }

            png_write_end(pngWrite, infoWrite);
            png_destroy_write_struct(&pngWrite, &infoWrite);
            png_destroy_read_struct(&png, &info, nullptr);
            fclose(out);
            fclose(fp);

            try
            {
                std::filesystem::path originalPath(imagePath);
                std::filesystem::path tempPath(tempFile);
                if (std::filesystem::exists(originalPath)) std::filesystem::remove(originalPath);
                std::filesystem::rename(tempPath, originalPath);
                std::cout << "Successfully wrote metadata to PNG: " << imagePath << std::endl;
                return true;
            }
            catch (const std::filesystem::filesystem_error& e)
            {
                std::cerr << "Error replacing file: " << e.what() << std::endl;
                return false;
            }
        }

        static bool WriteStealthPNG(const std::string& imagePath, const nlohmann::json& metadata)
        {
            int width, height, channels;
            unsigned char* data = stbi_load(imagePath.c_str(), &width, &height, &channels, 0);
            if (!data) {
                std::cerr << "Failed to load image for stealth metadata: " << imagePath << std::endl;
                return false;
            }

            bool needConversion = (channels != 4);
            unsigned char* rgbaData = data;
            if (needConversion) {
                rgbaData = (unsigned char*)malloc(width * height * 4);
                if (!rgbaData) {
                    stbi_image_free(data);
                    return false;
                }
                for (int i = 0; i < width * height; i++) {
                    int idx = i * channels;
                    int ridx = i * 4;
                    rgbaData[ridx] = data[idx];
                    rgbaData[ridx + 1] = (channels > 1) ? data[idx + 1] : data[idx];
                    rgbaData[ridx + 2] = (channels > 2) ? data[idx + 2] : data[idx];
                    rgbaData[ridx + 3] = (channels > 3) ? data[idx + 3] : 255;
                }
                stbi_image_free(data);
            }

            std::string jsonStr = metadata.dump();

            std::vector<unsigned char> compressed;
            uLongf compressedLen = compressBound(static_cast<uLong>(jsonStr.size())) + 18; // Extra for gzip header/trailer
            compressed.resize(compressedLen);

            z_stream strm;
            strm.zalloc = Z_NULL;
            strm.zfree = Z_NULL;
            strm.opaque = Z_NULL;
            // windowBits = 16 + MAX_WBITS = 31 => gzip format
            if (deflateInit2(&strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 16 + 15, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
                std::cerr << "Failed to initialize gzip compression" << std::endl;
                if (needConversion) free(rgbaData);
                return false;
            }
            strm.avail_in = static_cast<uInt>(jsonStr.size());
            strm.next_in = (Bytef*)jsonStr.data();
            strm.avail_out = static_cast<uInt>(compressedLen);
            strm.next_out = compressed.data();
            int ret = deflate(&strm, Z_FINISH);
            if (ret != Z_STREAM_END) {
                std::cerr << "Gzip compression failed: " << ret << std::endl;
                deflateEnd(&strm);
                if (needConversion) free(rgbaData);
                return false;
            }
            compressedLen = strm.total_out;
            deflateEnd(&strm);
            compressed.resize(compressedLen);

            const std::string signature = "stealth_pngcomp";

            // Convert compressed data to binary string (8 bits per byte)
            std::string binaryParam;
            binaryParam.reserve(compressed.size() * 8);
            for (unsigned char byte : compressed) {
                for (int bit = 7; bit >= 0; --bit) {
                    binaryParam += ((byte >> bit) & 1) ? '1' : '0';
                }
            }
            // Length of the binary parameter string in bits
            const uint32_t payloadLenBits = static_cast<uint32_t>(binaryParam.size());

            // Build full payload: signature + length (32 bits) + data
            std::vector<unsigned char> fullPayload;
            // Reserve enough space for signature bits + 32 bits + data bits
            // Each bit will be stored as a byte (0 or 1) in the alpha channel
            fullPayload.reserve(signature.size() * 8 + 32 + payloadLenBits);

            // Add signature bits (each character as 0 or 1)
            for (char c : signature) {
                for (int bit = 7; bit >= 0; --bit) {
                    fullPayload.push_back((c >> bit) & 1);
                }
            }

            // Add length bits (32 bits, big-endian)
            for (int bit = 31; bit >= 0; --bit) {
                fullPayload.push_back((payloadLenBits >> bit) & 1);
            }

            // Add data bits
            for (char c : binaryParam) {
                fullPayload.push_back(c == '1' ? 1 : 0);
            }

            size_t totalBits = fullPayload.size();
            size_t maxBits = width * height * 8;
            if (totalBits > maxBits) {
                std::cerr << "Metadata too large to embed stealthily" << std::endl;
                if (needConversion) free(rgbaData);
                return false;
            }

            // DEBUG: verify first 16 bytes of payload (as bytes)
            std::cout << "[DEBUG] fullPayload first 16 bytes (hex): ";
            for (size_t i = 0; i < 16 && i < fullPayload.size(); ++i) {
                std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)fullPayload[i] << " ";
            }
            std::cout << std::dec << std::endl;

            const int row_stride = width * 4;
            size_t bitOffset = 0;

            for (int x = 0; x < width; ++x) {
                for (int y = 0; y < height; ++y) {
                    unsigned char* pixelAlpha = rgbaData + y * row_stride + x * 4 + 3;
                    // Write 8 bits per pixel (one per channel bit, but we only use alpha LSB)
                    if (bitOffset < totalBits) {
                        if (fullPayload[bitOffset]) {
                            *pixelAlpha |= 1;
                        }
                        else {
                            *pixelAlpha &= ~1;
                        }
                        bitOffset++;
                    }
                    if (bitOffset >= totalBits) break;
                }
                if (bitOffset >= totalBits) break;
            }

            // DEBUG: read back first 16 bytes to confirm embedding
            std::vector<unsigned char> readback(16, 0);
            size_t bitPos = 0;
            for (int x = 0; x < width && bitPos < 128; ++x) {
                for (int y = 0; y < height && bitPos < 128; ++y) {
                    unsigned char alpha = rgbaData[y * row_stride + x * 4 + 3];
                    if (alpha & 1) {
                        readback[bitPos / 8] |= (1 << (7 - (bitPos % 8)));
                    }
                    bitPos++;
                }
            }
            std::cout << "[DEBUG] First 16 bytes read from alpha after embedding: ";
            for (int i = 0; i < 16; ++i) {
                std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)readback[i] << " ";
            }
            std::cout << std::dec << std::endl;

            // --- Write the PNG file (unchanged) ---
            std::string tempFile = imagePath + ".tmp";
            FILE* out = fopen(tempFile.c_str(), "wb");
            if (!out) {
                std::cerr << "Failed to create temporary file" << std::endl;
                if (needConversion) free(rgbaData);
                return false;
            }

            png_structp pngWrite = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
            if (!pngWrite) {
                std::cerr << "Failed to create PNG write struct" << std::endl;
                fclose(out);
                if (needConversion) free(rgbaData);
                return false;
            }

            png_infop infoWrite = png_create_info_struct(pngWrite);
            if (!infoWrite) {
                std::cerr << "Failed to create PNG info struct" << std::endl;
                png_destroy_write_struct(&pngWrite, nullptr);
                fclose(out);
                if (needConversion) free(rgbaData);
                return false;
            }

            if (setjmp(png_jmpbuf(pngWrite))) {
                std::cerr << "Error during PNG write" << std::endl;
                png_destroy_write_struct(&pngWrite, &infoWrite);
                fclose(out);
                if (needConversion) free(rgbaData);
                return false;
            }

            png_init_io(pngWrite, out);
            png_set_IHDR(pngWrite, infoWrite, width, height, 8, PNG_COLOR_TYPE_RGBA,
                PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

            // Add standard text chunks (for compatibility)
            std::string metadataStr = metadata.dump();
            std::vector<png_text> texts;

            png_text paramText;
            paramText.compression = PNG_TEXT_COMPRESSION_NONE;
            paramText.key = const_cast<char*>("parameters");
            paramText.text = const_cast<char*>(metadataStr.c_str());
            paramText.text_length = metadataStr.length();
            paramText.itxt_length = 0;
            paramText.lang = nullptr;
            paramText.lang_key = nullptr;
            texts.push_back(paramText);

            png_text softwareText;
            softwareText.compression = PNG_TEXT_COMPRESSION_NONE;
            softwareText.key = const_cast<char*>("Software");
            softwareText.text = const_cast<char*>("AniStudio");
            softwareText.text_length = 9;
            softwareText.itxt_length = 0;
            softwareText.lang = nullptr;
            softwareText.lang_key = nullptr;
            texts.push_back(softwareText);

            png_set_text(pngWrite, infoWrite, texts.data(), static_cast<int>(texts.size()));
            png_write_info(pngWrite, infoWrite);

            std::vector<png_byte> row(width * 4);
            for (int y = 0; y < height; y++) {
                memcpy(row.data(), rgbaData + y * row_stride, width * 4);
                png_write_row(pngWrite, row.data());
            }

            png_write_end(pngWrite, infoWrite);
            png_destroy_write_struct(&pngWrite, &infoWrite);
            fclose(out);

            if (needConversion) free(rgbaData);

            try {
                std::filesystem::path originalPath(imagePath);
                std::filesystem::path tempPath(tempFile);
                if (std::filesystem::exists(originalPath)) std::filesystem::remove(originalPath);
                std::filesystem::rename(tempPath, originalPath);
                std::cout << "Successfully wrote stealth metadata (with signature) to PNG: " << imagePath << std::endl;
                return true;
            }
            catch (const std::filesystem::filesystem_error& e) {
                std::cerr << "Error replacing file: " << e.what() << std::endl;
                return false;
            }
        }
    };
} // namespace Utils