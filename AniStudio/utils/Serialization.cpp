#include "Serialization.hpp"
#include <filesystem>
#include <iostream>

namespace ECS {

nlohmann::json SerializeEntityComponents(EntityID entity, EntityManager &mgr) {
    nlohmann::json componentData;

    // Helper lambda to check and serialize a component if it exists
    auto serializeComponent = [&mgr](EntityID entity, auto componentType) {
        using T = decltype(componentType);
        if (mgr.HasComponent<T>(entity)) {
            const auto &comp = mgr.GetComponent<T>(entity);
            return comp.Serialize();
        }
        return nlohmann::json{};
    };

    // Serialize each component type
    componentData.merge_patch(serializeComponent(entity, ModelComponent{}));
    componentData.merge_patch(serializeComponent(entity, CLipLComponent{}));
    componentData.merge_patch(serializeComponent(entity, CLipGComponent{}));
    componentData.merge_patch(serializeComponent(entity, T5XXLComponent{}));
    componentData.merge_patch(serializeComponent(entity, DiffusionModelComponent{}));
    componentData.merge_patch(serializeComponent(entity, VaeComponent{}));
    componentData.merge_patch(serializeComponent(entity, TaesdComponent{}));
    componentData.merge_patch(serializeComponent(entity, ControlnetComponent{}));
    componentData.merge_patch(serializeComponent(entity, LoraComponent{}));
    componentData.merge_patch(serializeComponent(entity, LatentComponent{}));
    componentData.merge_patch(serializeComponent(entity, SamplerComponent{}));
    componentData.merge_patch(serializeComponent(entity, CFGComponent{}));
    componentData.merge_patch(serializeComponent(entity, PromptComponent{}));
    componentData.merge_patch(serializeComponent(entity, EmbeddingComponent{}));
    componentData.merge_patch(serializeComponent(entity, LayerSkipComponent{}));
    componentData.merge_patch(serializeComponent(entity, ImageComponent{}));

    return componentData;
}

bool WriteMetadataToPNG(EntityID entity, const nlohmann::json &metadata, EntityManager &mgr) {
    const auto &imageComp = mgr.GetComponent<ImageComponent>(entity);

    FILE *fp = fopen(imageComp.filePath.c_str(), "rb");
    if (!fp) {
        std::cerr << "Failed to open PNG for reading: " << imageComp.filePath << std::endl;
        return false;
    }

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!png) {
        fclose(fp);
        return false;
    }

    png_infop info = png_create_info_struct(png);
    if (!info) {
        png_destroy_read_struct(&png, nullptr, nullptr);
        fclose(fp);
        return false;
    }

    png_init_io(png, fp);
    png_read_info(png, info);

    // Create temporary file
    std::string tempFile = imageComp.filePath + ".tmp";
    FILE *out = fopen(tempFile.c_str(), "wb");
    if (!out) {
        png_destroy_read_struct(&png, &info, nullptr);
        fclose(fp);
        return false;
    }

    png_structp pngWrite = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    png_infop infoWrite = png_create_info_struct(pngWrite);
    png_init_io(pngWrite, out);

    // Copy original PNG header
    png_uint_32 width, height;
    int bit_depth, color_type;
    png_get_IHDR(png, info, &width, &height, &bit_depth, &color_type, nullptr, nullptr, nullptr);
    png_set_IHDR(pngWrite, infoWrite, width, height, bit_depth, color_type, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    // Set up metadata text
    std::string metadataStr = metadata.dump();
    png_text texts[2];

    // Main parameters chunk
    texts[0].compression = PNG_TEXT_COMPRESSION_NONE;
    texts[0].key = const_cast<char *>("parameters");
    texts[0].text = const_cast<char *>(metadataStr.c_str());
    texts[0].text_length = metadataStr.length();
    texts[0].itxt_length = 0;
    texts[0].lang = nullptr;
    texts[0].lang_key = nullptr;

    // Software identifier
    texts[1].compression = PNG_TEXT_COMPRESSION_NONE;
    texts[1].key = const_cast<char *>("Software");
    texts[1].text = const_cast<char *>("AniStudio");
    texts[1].text_length = 9;
    texts[1].itxt_length = 0;
    texts[1].lang = nullptr;
    texts[1].lang_key = nullptr;

    png_set_text(pngWrite, infoWrite, texts, 2);
    png_write_info(pngWrite, infoWrite);

    // Copy image data
    png_bytep row = new png_byte[png_get_rowbytes(png, info)];
    for (png_uint_32 y = 0; y < height; y++) {
        png_read_row(png, row, nullptr);
        png_write_row(pngWrite, row);
    }
    delete[] row;

    png_write_end(pngWrite, infoWrite);

    // Clean up
    png_destroy_write_struct(&pngWrite, &infoWrite);
    png_destroy_read_struct(&png, &info, nullptr);
    fclose(out);
    fclose(fp);

    // Replace original with new file using proper filesystem calls
    std::filesystem::path originalPath(imageComp.filePath);
    std::filesystem::path tempPath(tempFile);
    std::filesystem::remove(originalPath);
    std::filesystem::rename(tempPath, originalPath);

    return true;
}

} // namespace ECS