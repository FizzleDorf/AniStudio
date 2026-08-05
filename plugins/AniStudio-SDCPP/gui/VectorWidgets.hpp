#pragma once

#include <imgui.h>
#include <string>
#include <vector>
#include <utility>
#include "SDCPPComponents.h"

namespace VectorWidgets {

	bool RenderLoraList(std::vector<ECS::LoraComponent::LoraEntry>& entries);
	bool RenderEmbeddingsPairList(std::vector<std::pair<std::string, std::string>>& entries);
	bool RenderFilePathList(std::vector<std::string>& paths);
	bool RenderFloatList(std::vector<float>& floats);
	bool RenderStringList(std::vector<std::string>& strings);

} // namespace VectorWidgets