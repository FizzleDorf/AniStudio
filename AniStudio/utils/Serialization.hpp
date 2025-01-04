#pragma once

#include "EntityManager.hpp"
#include "ImageComponent.hpp"
#include "SDCPPComponents.h"
#include <nlohmann/json.hpp>
#include <png.h>

namespace ECS {

// Serialize all components attached to an entity
nlohmann::json SerializeEntityComponents(EntityID entity);

// Write metadata to PNG file
bool WriteMetadataToPNG(EntityID entity, const nlohmann::json &metadata);

// Read metadata from PNG file
nlohmann::json ReadMetadataFromPNG(const std::string &filepath);

// Deserialize components from JSON and attach to entity
void DeserializeEntityComponents(EntityID entity, const nlohmann::json &componentData);

} // namespace ECS