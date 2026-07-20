#pragma once
#include "stable-diffusion.h"

namespace Utils {
	// Parse SLG parameters from metadata
	static bool ParseSLGParams(const nlohmann::json& comp, sd_slg_params_t& slg,
		std::vector<std::unique_ptr<int[]>>& intArrayResources) {
		try {
			if (comp.contains("layer_start") && !comp["layer_start"].is_null())
				slg.layer_start = comp["layer_start"].get<float>();
			if (comp.contains("layer_end") && !comp["layer_end"].is_null())
				slg.layer_end = comp["layer_end"].get<float>();
			if (comp.contains("scale") && !comp["scale"].is_null())
				slg.scale = comp["scale"].get<float>();

			if (comp.contains("layers") && comp["layers"].is_array() &&
				comp.contains("layer_count") && !comp["layer_count"].is_null()) {
				size_t layer_count = comp["layer_count"].get<size_t>();
				if (layer_count > 0 && comp["layers"].size() >= layer_count) {
					auto layers_array = std::make_unique<int[]>(layer_count);
					for (size_t i = 0; i < layer_count; i++) {
						layers_array[i] = comp["layers"][i].get<int>();
					}
					slg.layers = layers_array.get();
					slg.layer_count = layer_count;
					intArrayResources.push_back(std::move(layers_array));
				}
			}
			return true;
		}
		catch (...) {
			return false;
		}
	}
}