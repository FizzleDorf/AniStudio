#include "Serialization.hpp"
#include <filesystem>

namespace ECS {

nlohmann::json SerializeEntityComponents(EntityID entity) {
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

bool WriteMetadataToPNG(EntityID entity, const nlohmann::json &metadata) {
    const auto &imageComp = mgr.GetComponent<ImageComponent>(entity);
    if (!std::filesystem::exists(imageComp.filePath)) {
        std::cerr << "Image file not found: " << imageComp.filePath << std::endl;
        return false;
    }

    // Open PNG file for reading
    FILE *fp = fopen(imageComp.filePath.c_str(), "rb");
    if (!fp) {
        std::cerr << "Failed to open PNG file for reading" << std::endl;
        return false;
    }

    // Initialize PNG read structures
    png_structp pngRead = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!pngRead) {
        fclose(fp);
        return false;
    }

    png_infop infoRead = png_create_info_struct(pngRead);
    if (!infoRead) {
        png_destroy_read_struct(&pngRead, NULL, NULL);
        fclose(fp);
        return false;
    }

    // Set up error handling
    if (setjmp(png_jmpbuf(pngRead))) {
        png_destroy_read_struct(&pngRead, &infoRead, NULL);
        fclose(fp);
        return false;
    }

    png_init_io(pngRead, fp);
    png_read_info(pngRead, infoRead);

    // Read existing metadata
    png_textp text_ptr;
    int num_text;
    png_get_text(pngRead, infoRead, &text_ptr, &num_text);

    // Create temporary file for writing
    std::string tempPath = imageComp.filePath + ".tmp";
    FILE *fpWrite = fopen(tempPath.c_str(), "wb");
    if (!fpWrite) {
        png_destroy_read_struct(&pngRead, &infoRead, NULL);
        fclose(fp);
        return false;
    }

    // Initialize PNG write structures
    png_structp pngWrite = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!pngWrite) {
        png_destroy_read_struct(&pngRead, &infoRead, NULL);
        fclose(fp);
        fclose(fpWrite);
        return false;
    }

    png_infop infoWrite = png_create_info_struct(pngWrite);
    if (!infoWrite) {
        png_destroy_write_struct(&pngWrite, NULL);
        png_destroy_read_struct(&pngRead, &infoRead, NULL);
        fclose(fp);
        fclose(fpWrite);
        return false;
    }

    // Set up error handling for write
    if (setjmp(png_jmpbuf(pngWrite))) {
        png_destroy_write_struct(&pngWrite, &infoWrite);
        png_destroy_read_struct(&pngRead, &infoRead, NULL);
        fclose(fp);
        fclose(fpWrite);
        return false;
    }

    png_init_io(pngWrite, fpWrite);

    // Copy PNG header
    png_set_IHDR(pngWrite, infoWrite, png_get_image_width(pngRead, infoRead), png_get_image_height(pngRead, infoRead),
                 png_get_bit_depth(pngRead, infoRead), png_get_color_type(pngRead, infoRead),
                 png_get_interlace_type(pngRead, infoRead), png_get_compression_type(pngRead, infoRead),
                 png_get_filter_type(pngRead, infoRead));

    // Add metadata text chunk
    std::string metadataStr = metadata.dump();
    png_text text[1];
    text[0].compression = PNG_TEXT_COMPRESSION_zTXt;
    text[0].key = const_cast<char *>("ani_metadata");
    text[0].text = const_cast<char *>(metadataStr.c_str());
    text[0].text_length = metadataStr.length();
    text[0].itxt_length = 0;
    text[0].lang = NULL;
    text[0].lang_key = NULL;

    png_set_text(pngWrite, infoWrite, text, 1);
    png_write_info(pngWrite, infoWrite);

    // Copy image data
    png_bytep row = new png_byte[png_get_rowbytes(pngRead, infoRead)];
    for (uint32_t y = 0; y < png_get_image_height(pngRead, infoRead); y++) {
        png_read_row(pngRead, row, NULL);
        png_write_row(pngWrite, row);
    }
    delete[] row;

    // Finish writing
    png_write_end(pngWrite, infoWrite);

    // Clean up
    png_destroy_write_struct(&pngWrite, &infoWrite);
    png_destroy_read_struct(&pngRead, &infoRead, NULL);
    fclose(fp);
    fclose(fpWrite);

    // Replace original file with new file
    std::filesystem::remove(imageComp.filePath);
    std::filesystem::rename(tempPath, imageComp.filePath);

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

void DeserializeEntityComponents(EntityID entity, const nlohmann::json &componentData) {
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