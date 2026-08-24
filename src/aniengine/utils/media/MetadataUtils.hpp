#pragma once

#include <string>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <png.h>

#ifdef USE_EXIV2
#include <exiv2/exiv2.hpp>
#endif

namespace Utils {

    class MetadataUtils {
    public:

        static bool SaveMetadataToJson(const std::string& filepath, const nlohmann::json& metadata) {
            try {
                std::ofstream file(filepath);
                if (file.is_open()) {
                    file << metadata.dump(4);
                    file.close();
                    std::cout << "Metadata saved to: " << filepath << std::endl;
                    return true;
                }
                else {
                    std::cerr << "Failed to open file for writing: " << filepath << std::endl;
                    return false;
                }
            }
            catch (const std::exception& e) {
                std::cerr << "Error saving metadata: " << e.what() << std::endl;
                return false;
            }
        }

        static nlohmann::json LoadMetadataFromJson(const std::string& filepath) {
            try {
                std::ifstream file(filepath);
                if (file.is_open()) {
                    nlohmann::json metadata;
                    file >> metadata;
                    file.close();
                    std::cout << "Metadata loaded from: " << filepath << std::endl;
                    return metadata;
                }
                else {
                    std::cerr << "Failed to open file for reading: " << filepath << std::endl;
                    return nlohmann::json();
                }
            }
            catch (const std::exception& e) {
                std::cerr << "Error loading metadata: " << e.what() << std::endl;
                return nlohmann::json();
            }
        }

        static nlohmann::json LoadMetadataFromPNG(const std::string& imagePath) {
            std::cout << "Attempting to load metadata from: " << imagePath << std::endl;
            nlohmann::json result;

            FILE* fp = fopen(imagePath.c_str(), "rb");
            if (!fp) {
                std::cerr << "Failed to open PNG file: " << imagePath << std::endl;
                return result;
            }

            png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
            if (!png) {
                fclose(fp);
                return result;
            }

            png_infop info = png_create_info_struct(png);
            if (!info) {
                png_destroy_read_struct(&png, nullptr, nullptr);
                fclose(fp);
                return result;
            }

            if (setjmp(png_jmpbuf(png))) {
                png_destroy_read_struct(&png, &info, nullptr);
                fclose(fp);
                return result;
            }

            png_init_io(png, fp);
            png_read_info(png, info);

            png_textp text_ptr;
            int num_text;
            if (png_get_text(png, info, &text_ptr, &num_text) > 0) {
                for (int i = 0; i < num_text; i++) {
                    std::string text = text_ptr[i].text;
                    try {
                        nlohmann::json parsed = nlohmann::json::parse(text);
                        if (parsed.is_object() || parsed.is_array()) {
                            result[text_ptr[i].key] = parsed;
                            std::cout << "Loaded JSON from PNG chunk: " << text_ptr[i].key << std::endl;
                        }
                    }
                    catch (...) {
                        result[text_ptr[i].key] = text;
                    }
                }
            }

            png_destroy_read_struct(&png, &info, nullptr);
            fclose(fp);

            return result;
        }

#ifdef USE_EXIV2
        static bool WriteMetadataToTIFF(const std::string& imagePath, const nlohmann::json& metadata) {
            try {
                auto image = Exiv2::ImageFactory::open(imagePath);
                if (!image) {
                    std::cerr << "Failed to open TIFF with Exiv2: " << imagePath << std::endl;
                    return false;
                }

                image->readMetadata();
                Exiv2::ExifData& exifData = image->exifData();
                std::string jsonStr = metadata.dump();

                auto value = Exiv2::Value::create(Exiv2::asciiString);
                value->read(jsonStr);
                exifData["Exif.Image.ImageDescription"] = *value;

                image->writeMetadata();
                std::cout << "Successfully wrote EXIF metadata to TIFF: " << imagePath << std::endl;
                return true;
            }
            catch (const Exiv2::Error& e) {
                std::cerr << "Exiv2 error: " << e.what() << std::endl;
                return false;
            }
        }

        static nlohmann::json ReadMetadataFromTIFF(const std::string& imagePath) {
            nlohmann::json result;
            try {
                auto image = Exiv2::ImageFactory::open(imagePath);
                if (!image) {
                    std::cerr << "Failed to open TIFF with Exiv2: " << imagePath << std::endl;
                    return result;
                }

                image->readMetadata();
                Exiv2::ExifData& exifData = image->exifData();
                for (const auto& exif : exifData) {
                    std::string key = exif.key();
                    std::string value = exif.toString();
                    try {
                        nlohmann::json parsed = nlohmann::json::parse(value);
                        if (parsed.is_object() || parsed.is_array()) {
                            result[key] = parsed;
                        }
                        else {
                            result[key] = value;
                        }
                    }
                    catch (...) {
                        result[key] = value;
                    }
                }
            }
            catch (const Exiv2::Error& e) {
                std::cerr << "Exiv2 error: " << e.what() << std::endl;
            }
            return result;
        }
#endif

        static nlohmann::json NormalizeAniStudioMetadata(const nlohmann::json& meta) {
            if (meta.is_null() || !meta.is_object()) return meta;

            nlohmann::json result = meta;

            if (meta.contains("dataType") && meta["dataType"] == "entity" && meta.contains("data")) {
                const auto& data = meta["data"];
                if (data.is_object()) {
                    if (data.contains("parameters") && data["parameters"].is_object()) {
                        const auto& params = data["parameters"];
                        if (params.contains("components") && params["components"].is_array()) {
                            return params;
                        }
                    }
                    if (data.contains("components") && data["components"].is_array()) {
                        return data;
                    }
                }
                return result;
            }

            if (meta.is_object()) {
                for (auto it = meta.begin(); it != meta.end(); ++it) {
                    const auto& value = it.value();
                    if (value.is_object()) {
                        if (value.contains("components") && value["components"].is_array()) {
                            return value;
                        }
                        if (value.contains("parameters") && value["parameters"].is_object()) {
                            const auto& params = value["parameters"];
                            if (params.contains("components") && params["components"].is_array()) {
                                return params;
                            }
                        }
                    }
                }
            }

            if (meta.contains("components") && meta["components"].is_array()) {
                return meta;
            }

            return meta;
        }

        static nlohmann::json ConvertMetadataFormat(const nlohmann::json& metadata) {
            nlohmann::json convertedJson;

            convertedJson["ID"] = metadata.value("ID", 0);
            if (metadata.contains("software"))
                convertedJson["software"] = metadata["software"];
            if (metadata.contains("timestamp"))
                convertedJson["timestamp"] = metadata["timestamp"];
            if (metadata.contains("version"))
                convertedJson["version"] = metadata["version"];

            convertedJson["components"] = nlohmann::json::array();

            if (metadata.contains("components") && metadata["components"].is_object()) {
                auto& componentsObj = metadata["components"];

                for (auto it = componentsObj.begin(); it != componentsObj.end(); ++it) {
                    std::string componentName = it.key();
                    nlohmann::json componentData = it.value();

                    if (componentName == "Base_Component") {
                        nlohmann::json baseComp;
                        baseComp["compName"] = "Base_Component";
                        convertedJson["components"].push_back(baseComp);
                        continue;
                    }

                    if (componentData.is_object()) {
                        if (componentData.contains(componentName) && componentData[componentName].is_object()) {
                            nlohmann::json arrayElement;
                            arrayElement[componentName] = componentData[componentName];
                            convertedJson["components"].push_back(arrayElement);
                        }
                        else {
                            nlohmann::json arrayElement;
                            arrayElement[componentName] = componentData;
                            convertedJson["components"].push_back(arrayElement);
                        }
                    }
                }
            }
            else if (metadata.contains("components") && metadata["components"].is_array()) {
                convertedJson["components"] = metadata["components"];
            }

            return convertedJson;
        }

        static nlohmann::json CreateGenerationMetadata(const nlohmann::json& entityData,
            const nlohmann::json& additionalInfo = {}) {
            nlohmann::json metadata = entityData;

            metadata["software"] = "AniStudio";
            metadata["timestamp"] = std::time(nullptr);
            metadata["version"] = "1.0";

            if (!additionalInfo.is_null()) {
                for (auto it = additionalInfo.begin(); it != additionalInfo.end(); ++it) {
                    metadata[it.key()] = it.value();
                }
            }

            return metadata;
        }
    };

}