#pragma once
#include "stable-diffusion.h"
#include "DiffusionCallbackUtils.hpp"

namespace Utils {
	// Parse sample parameters from metadata
	static bool ParseSampleParams(const nlohmann::json& comp, sd_sample_params_t& sample_params,
		std::vector<std::unique_ptr<float[]>>& floatArrayResources) {
		try {
			if (comp.contains("steps") && !comp["steps"].is_null())
				sample_params.sample_steps = comp["steps"].get<int>();
			if (comp.contains("eta") && !comp["eta"].is_null())
				sample_params.eta = comp["eta"].get<float>();
			if (comp.contains("current_sample_method") && !comp["current_sample_method"].is_null())
				sample_params.sample_method = static_cast<sample_method_t>(comp["current_sample_method"].get<int>());
			if (comp.contains("current_scheduler_method") && !comp["current_scheduler_method"].is_null())
				sample_params.scheduler = static_cast<scheduler_t>(comp["current_scheduler_method"].get<int>());
			if (comp.contains("shifted_timestep") && !comp["shifted_timestep"].is_null())
				sample_params.shifted_timestep = comp["shifted_timestep"].get<int>();
			if (comp.contains("flow_shift") && !comp["flow_shift"].is_null())
				sample_params.flow_shift = comp["flow_shift"].get<float>();

			if (comp.contains("custom_sigmas") && comp["custom_sigmas"].is_array()) {
				auto sigmasArray = comp["custom_sigmas"];
				size_t count = sigmasArray.size();
				if (count > 0) {
					auto sigmas = std::make_unique<float[]>(count);
					for (size_t i = 0; i < count; ++i) {
						sigmas[i] = sigmasArray[i].get<float>();
					}
					sample_params.custom_sigmas = sigmas.get();
					sample_params.custom_sigmas_count = static_cast<int>(count);
					floatArrayResources.push_back(std::move(sigmas));
				}
			}
			return true;
		}
		catch (...) {
			return false;
		}
	}
}