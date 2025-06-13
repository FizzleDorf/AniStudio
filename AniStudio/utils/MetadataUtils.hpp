/*
		d8888          d8b  .d8888b.  888                  888 d8b
	   d88888          Y8P d88P  Y88b 888                  888 Y8P
	  d88P888              Y88b.      888                  888
	 d88P 888 88888b.  888  "Y888b.   888888 888  888  .d88888 888  .d88b.
	d88P  888 888 "88b 888     "Y88b. 888    888  888 d88" 888 888 d88""88b
   d88P   888 888  888 888       "888 888    888  888 888  888 888 888  888
  d8888888888 888  888 888 Y88b  d88P Y88b.  Y88b 888 Y88b 888 888 Y88..88P
 d88P     888 888  888 888  "Y8888P"   "Y888  "Y88888  "Y88888 888  "Y88P"

 * This file is part of AniStudio.
 * Copyright (C) 2025 FizzleDorf (AnimAnon)
 *
 * This software is dual-licensed under the GNU Lesser General Public License v3.0 (LGPL-3.0)
 * and a commercial license. You may choose to use it under either license.
 *
 * For the LGPL-3.0, see the LICENSE-LGPL-3.0.txt file in the repository.
 * For commercial license information, please contact legal@kframe.ai.
 */

#pragma once

#include <string>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <png.h>

namespace Utils {

    class MetadataUtils {
    public:

        //Save metadata to a JSON file
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

        // Load metadata from a JSON file
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

        // Load metadata from a PNG file's text chunks
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

            // Get text chunks
            png_textp text_ptr;
            int num_text;
            if (png_get_text(png, info, &text_ptr, &num_text) > 0) {
                for (int i = 0; i < num_text; i++) {
                    if (strcmp(text_ptr[i].key, "parameters") == 0) {
                        try {
                            // Parse metadata from PNG
                            result = nlohmann::json::parse(text_ptr[i].text);
                            std::cout << "Successfully loaded metadata" << std::endl;
                        }
                        catch (const std::exception& e) {
                            std::cerr << "Error parsing JSON metadata: " << e.what() << std::endl;
                        }
                        break;
                    }
                }
            }

            png_destroy_read_struct(&png, &info, nullptr);
            fclose(fp);

            return result;
        }

        //Convert metadata from nested object format to array format
        static nlohmann::json ConvertMetadataFormat(const nlohmann::json& metadata) {
            nlohmann::json convertedJson;

            // Keep the app info
            convertedJson["ID"] = metadata.value("ID", 0);
            if (metadata.contains("software"))
                convertedJson["software"] = metadata["software"];
            if (metadata.contains("timestamp"))
                convertedJson["timestamp"] = metadata["timestamp"];
            if (metadata.contains("version"))
                convertedJson["version"] = metadata["version"];

            convertedJson["components"] = nlohmann::json::array();

            // If metadata has the nested components object format, convert it
            if (metadata.contains("components") && metadata["components"].is_object()) {
                auto& componentsObj = metadata["components"];

                // Process each component type
                for (auto it = componentsObj.begin(); it != componentsObj.end(); ++it) {
                    std::string componentName = it.key();
                    nlohmann::json componentData = it.value();

                    // Handle base component as a special case
                    if (componentName == "Base_Component") {
                        nlohmann::json baseComp;
                        baseComp["compName"] = "Base_Component";
                        convertedJson["components"].push_back(baseComp);
                        continue;
                    }

                    // For nested objects, create an array element for each component
                    if (componentData.is_object()) {
                        // Remove the double nesting if present
                        if (componentData.contains(componentName) && componentData[componentName].is_object()) {
                            nlohmann::json arrayElement;
                            arrayElement[componentName] = componentData[componentName];
                            convertedJson["components"].push_back(arrayElement);
                        }
                        else {
                            // Just add it as is
                            nlohmann::json arrayElement;
                            arrayElement[componentName] = componentData;
                            convertedJson["components"].push_back(arrayElement);
                        }
                    }
                }
            }
            else if (metadata.contains("components") && metadata["components"].is_array()) {
                // If it's already in the array format, use it directly
                convertedJson["components"] = metadata["components"];
            }

            return convertedJson;
        }

        //Create standard metadata structure for generation parameters
        static nlohmann::json CreateGenerationMetadata(const nlohmann::json& entityData,
            const nlohmann::json& additionalInfo = {}) {
            nlohmann::json metadata = entityData;

            // Add standard generation info
            metadata["software"] = "AniStudio";
            metadata["timestamp"] = std::time(nullptr);
            metadata["version"] = "1.0";

            // Add any additional info
            if (!additionalInfo.is_null()) {
                for (auto it = additionalInfo.begin(); it != additionalInfo.end(); ++it) {
                    metadata[it.key()] = it.value();
                }
            }

            return metadata;
        }
    };

} // namespace Utils