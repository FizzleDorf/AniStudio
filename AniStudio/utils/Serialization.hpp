#pragma once

#include "EntityManager.hpp"
#include "ImageComponent.hpp"
#include "SDCPPComponents.h"
#include <nlohmann/json.hpp>
#include <png.h>

namespace ECS {
    // Add EntityManager reference parameter
    nlohmann::json SerializeEntityComponents(EntityID entityID, EntityManager& mgr);
    bool WriteMetadataToPNG(EntityID entity, const nlohmann::json& metadata, EntityManager& mgr);
}