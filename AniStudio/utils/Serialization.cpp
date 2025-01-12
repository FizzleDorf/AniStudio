#include "Serialization.hpp"
#include <filesystem>
#include <iostream>

namespace ECS {

nlohmann::json SerializeEntityComponents(const EntityID entity, EntityManager &mgr) {
    nlohmann::json componentData;

    // Helper lambda to check and serialize a component if it exists
    auto serializeComponent = [&](auto componentType) {
        using T = decltype(componentType);
        if (mgr.HasComponent<T>(entity)) {
            const auto &comp = mgr.GetComponent<T>(entity);
            componentData.merge_patch(comp.Serialize());
        }
    };

    // Serialize each component type
    serializeComponent(ModelComponent{});
    serializeComponent(CLipLComponent{});
    serializeComponent(CLipGComponent{});
    serializeComponent(T5XXLComponent{});
    serializeComponent(DiffusionModelComponent{});
    serializeComponent(VaeComponent{});
    serializeComponent(TaesdComponent{});
    serializeComponent(ControlnetComponent{});
    serializeComponent(LoraComponent{});
    serializeComponent(LatentComponent{});
    serializeComponent(SamplerComponent{});
    serializeComponent(CFGComponent{});
    serializeComponent(PromptComponent{});
    serializeComponent(EmbeddingComponent{});
    serializeComponent(LayerSkipComponent{});
    serializeComponent(ImageComponent{});

    return componentData;
}

bool WriteMetadataToPNG(const EntityID entity, EntityManager &mgr, const nlohmann::json &metadata) {
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

    // Set up EXIF data
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

    // Replace original with new file
    std::filesystem::remove(imageComp.filePath);
    std::filesystem::rename(tempFile, imageComp.filePath);

    return true;
}

nlohmann::json ReadMetadataFromPNG(const std::string &filepath) {
    FILE *fp = fopen(filepath.c_str(), "rb");
    if (!fp) {
        return nlohmann::json();
    }

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) {
        fclose(fp);
        return nlohmann::json();
    }

    png_infop info = png_create_info_struct(png);
    if (!info) {
        png_destroy_read_struct(&png, NULL, NULL);
        fclose(fp);
        return nlohmann::json();
    }

    if (setjmp(png_jmpbuf(png))) {
        png_destroy_read_struct(&png, &info, NULL);
        fclose(fp);
        return nlohmann::json();
    }

    png_init_io(png, fp);
    png_read_info(png, info);

    png_textp text_ptr;
    int num_text;
    png_get_text(png, info, &text_ptr, &num_text);

    nlohmann::json metadata;
    for (int i = 0; i < num_text; i++) {
        if (strcmp(text_ptr[i].key, "ani_metadata") == 0) {
            try {
                metadata = nlohmann::json::parse(text_ptr[i].text);
            } catch (const nlohmann::json::exception &e) {
                std::cerr << "Failed to parse metadata JSON: " << e.what() << std::endl;
            }
            break;
        }
    }

    png_destroy_read_struct(&png, &info, NULL);
    fclose(fp);

    return metadata;
}

void DeserializeEntityComponents(const EntityID entity, EntityManager &mgr, const nlohmann::json &componentData) {
    // Helper lambda to deserialize a component if it exists in the JSON
    auto deserializeComponent = [&](auto componentType) {
        using T = decltype(componentType);
        T comp;
        comp.Deserialize(componentData);
        if (mgr.HasComponent<T>(entity)) {
            mgr.GetComponent<T>(entity) = comp;
        } else {
            mgr.AddComponent<T>(entity, comp);
        }
    };

    // Deserialize each component type if present in the JSON
    if (componentData.contains("ModelComponent"))
        deserializeComponent(ModelComponent{});
    if (componentData.contains("CLipLComponent"))
        deserializeComponent(CLipLComponent{});
    if (componentData.contains("CLipGComponent"))
        deserializeComponent(CLipGComponent{});
    if (componentData.contains("T5XXLComponent"))
        deserializeComponent(T5XXLComponent{});
    if (componentData.contains("DiffusionModelComponent"))
        deserializeComponent(DiffusionModelComponent{});
    if (componentData.contains("VaeComponent"))
        deserializeComponent(VaeComponent{});
    if (componentData.contains("TaesdComponent"))
        deserializeComponent(TaesdComponent{});
    if (componentData.contains("ControlnetComponent"))
        deserializeComponent(ControlnetComponent{});
    if (componentData.contains("LoraComponent"))
        deserializeComponent(LoraComponent{});
    if (componentData.contains("LatentComponent"))
        deserializeComponent(LatentComponent{});
    if (componentData.contains("SamplerComponent"))
        deserializeComponent(SamplerComponent{});
    if (componentData.contains("CFGComponent"))
        deserializeComponent(CFGComponent{});
    if (componentData.contains("PromptComponent"))
        deserializeComponent(PromptComponent{});
    if (componentData.contains("EmbeddingComponent"))
        deserializeComponent(EmbeddingComponent{});
    if (componentData.contains("LayerSkipComponent"))
        deserializeComponent(LayerSkipComponent{});
    if (componentData.contains("ImageComponent"))
        deserializeComponent(ImageComponent{});
}

} // namespace ECS