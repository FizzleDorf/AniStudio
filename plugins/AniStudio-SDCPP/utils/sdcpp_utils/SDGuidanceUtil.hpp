#pragma once
#include "stable-diffusion.h"
#include "DiffusionCallbackUtils.hpp"

namespace Utils {
	// Parse guidance parameters from metadata
	static bool ParseGuidanceParams(const nlohmann::json& comp, sd_guidance_params_t& guidance) {
		try {
			if (comp.contains("txt_cfg") && !comp["txt_cfg"].is_null())
				guidance.txt_cfg = comp["txt_cfg"].get<float>();
			if (comp.contains("img_cfg") && !comp["img_cfg"].is_null())
				guidance.img_cfg = comp["img_cfg"].get<float>();
			if (comp.contains("distilled_guidance") && !comp["distilled_guidance"].is_null())
				guidance.distilled_guidance = comp["distilled_guidance"].get<float>();
			return true;
		}
		catch (...) {
			return false;
		}
	}
}