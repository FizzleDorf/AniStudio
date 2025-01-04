#include "ECS.h"
#include <fstream>
#include <nlohmann/json.hpp>
#include <png.h>
#include <string>
using namespace ECS;
// Function to write JSON metadata as a text chunk in PNG
void WriteMetadataToPNG(const EntityID, const nlohmann::json &metadata) {
    std::string metadataStr = metadata.dump(); // Serialize the JSON to string

    // Open the PNG file
    FILE *fp = fopen(filePath.c_str(), "wb");
    if (!fp) {
        std::cerr << "Error: Unable to open file for writing: " << filePath << std::endl;
        return;
    }

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!png) {
        std::cerr << "Error: Unable to create PNG write structure!" << std::endl;
        fclose(fp);
        return;
    }

    png_infop info = png_create_info_struct(png);
    if (!info) {
        std::cerr << "Error: Unable to create PNG info structure!" << std::endl;
        png_destroy_write_struct(&png, nullptr);
        fclose(fp);
        return;
    }

    // Set up error handling
    if (setjmp(png_jmpbuf(png))) {
        std::cerr << "Error: An error occurred during the writing process!" << std::endl;
        png_destroy_write_struct(&png, &info);
        fclose(fp);
        return;
    }

    // Set up the output file
    png_init_io(png, fp);

    // Create and add the text chunk (for metadata)
    png_text text;
    text.key = "metadata";           // Key name for the text chunk
    text.text = metadataStr.c_str(); // Value of the text chunk
    text.compression = PNG_TEXT_COMPRESSION_TYPE_DEFAULT;

    // Write the metadata text chunk
    png_set_text(png, info, &text, 1);

    // Now write the PNG image data (You should write your image data here as well)
    // Example image data, you would replace this with your actual image data
    png_bytepp rows = (png_bytepp)malloc(sizeof(png_bytep) * height);
    for (int i = 0; i < height; ++i) {
        rows[i] = (png_bytep)(imageData + i * width * channels);
    }

    png_set_IHDR(png, info, width, height, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_TYPE_DEFAULT,
                 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_write_info(png, info);
    png_write_image(png, rows);
    png_write_end(png, info);

    // Clean up
    free(rows);
    png_destroy_write_struct(&png, &info);
    fclose(fp);
}

// Your existing code with metadata serialization
void SaveImageWithMetadata(const EntityID entityID) {
    // Serialize the metadata for the entity
    nlohmann::json metadata = SerializeEntityComponents(entityID);

    // Save the image data as a PNG and embed the metadata
    std::string imagePath = "output.png"; // The path where the image will be saved
    WriteMetadataToPNG(imagePath, metadata);

    // Clean up context (if needed)
    free_sd_ctx(sd_context);
}
