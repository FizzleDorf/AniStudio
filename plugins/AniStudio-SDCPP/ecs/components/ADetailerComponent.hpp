#pragma once

#include "BaseComponent.hpp"
#include "stable-diffusion.h"
#include "DiffusionOptions.hpp"
#include <string>
#include <unordered_map>
#include <vector>

namespace ECS {

    struct ADetailerComponent : public BaseComponent {
        std::string detector_path;
        std::string ad_prompt;
        std::string ad_negative_prompt;
        int input_size = 640;
        float confidence = 0.3f;
        float nms = 0.45f;
        int max_detections = 100;
        int mask_k_largest = 0;
        float mask_min_ratio = 0.0f;
        float mask_max_ratio = 1.0f;
        int dilate_erode = 4;
        int x_offset = 0;
        int y_offset = 0;
        std::string mask_mode = "none";
        bool merge_masks = false;
        bool invert_mask = false;
        int mask_blur = 4;
        int inpaint_padding = 32;
        int inpaint_width = 0;
        int inpaint_height = 0;
        float denoising_strength = -1.0f;
        int steps = 0;
        float cfg_scale = -1.0f;
        int sample_method = 0;
        int scheduler = 0;
        std::string sort_by = "none";
        std::string extra_ad_args_override;

        ADetailerComponent() {
            compName = "ADetailer";
            compCategory = "ADetailer";

            std::vector<std::string> maskModeItems = { "none", "merge", "merge_invert" };
            std::vector<std::string> sortByItems = { "none", "left_to_right", "center_to_edge", "area" };

            schema = {
                {"title", "ADetailer Settings"},
                {"type", "object"},
                {"propertyOrder", {
                    "detector_path", "ad_prompt", "ad_negative_prompt",
                    "input_size", "confidence", "nms", "max_detections",
                    "mask_k_largest", "mask_min_ratio", "mask_max_ratio",
                    "dilate_erode", "x_offset", "y_offset",
                    "mask_mode", "merge_masks", "invert_mask", "mask_blur",
                    "inpaint_padding", "inpaint_width", "inpaint_height",
                    "denoising_strength", "steps", "cfg_scale",
                    "sample_method", "scheduler", "sort_by", "extra_ad_args_override"
                }},
                {"properties", {
                    {"detector_path", {
                        {"type", "string"},
                        {"title", "Detector Path"},
                        {"ui:widget", "file_selector"},
                        {"ui:options", {
                            {"mode", "file"},
                            {"filters", ".safetensors,.pt"},
                            {"filterName", "YOLO Detectors"},
                            {"buttonText", "Browse..."},
                            {"resetButtonText", "Clear"}
                        }}
                    }},
                    {"ad_prompt", {
                        {"type", "string"},
                        {"title", "AD Prompt Override"},
                        {"ui:widget", "textarea"}
                    }},
                    {"ad_negative_prompt", {
                        {"type", "string"},
                        {"title", "AD Negative Prompt Override"},
                        {"ui:widget", "textarea"}
                    }},
                    {"input_size", {
                        {"type", "integer"},
                        {"title", "Input Size"},
                        {"ui:widget", "input_int"},
                        {"ui:options", {{"min", 32}, {"max", 1024}, {"step", 32}}}
                    }},
                    {"confidence", {
                        {"type", "number"},
                        {"title", "Confidence"},
                        {"ui:widget", "input_float"},
                        {"ui:options", {{"step", 0.01f}, {"min", 0.0f}, {"max", 1.0f}}}
                    }},
                    {"nms", {
                        {"type", "number"},
                        {"title", "NMS IoU"},
                        {"ui:widget", "input_float"},
                        {"ui:options", {{"step", 0.01f}, {"min", 0.0f}, {"max", 1.0f}}}
                    }},
                    {"max_detections", {
                        {"type", "integer"},
                        {"title", "Max Detections"},
                        {"ui:widget", "input_int"},
                        {"ui:options", {{"min", 1}, {"max", 1000}}}
                    }},
                    {"mask_k_largest", {
                        {"type", "integer"},
                        {"title", "Keep K Largest"},
                        {"ui:widget", "input_int"},
                        {"ui:options", {{"min", 0}, {"max", 100}}}
                    }},
                    {"mask_min_ratio", {
                        {"type", "number"},
                        {"title", "Min Bbox Ratio"},
                        {"ui:widget", "input_float"},
                        {"ui:options", {{"step", 0.01f}, {"min", 0.0f}, {"max", 1.0f}}}
                    }},
                    {"mask_max_ratio", {
                        {"type", "number"},
                        {"title", "Max Bbox Ratio"},
                        {"ui:widget", "input_float"},
                        {"ui:options", {{"step", 0.01f}, {"min", 0.0f}, {"max", 1.0f}}}
                    }},
                    {"dilate_erode", {
                        {"type", "integer"},
                        {"title", "Dilate/Erode"},
                        {"ui:widget", "input_int"},
                        {"ui:options", {{"min", -100}, {"max", 100}}}
                    }},
                    {"x_offset", {
                        {"type", "integer"},
                        {"title", "X Offset"},
                        {"ui:widget", "input_int"},
                        {"ui:options", {{"min", -1000}, {"max", 1000}}}
                    }},
                    {"y_offset", {
                        {"type", "integer"},
                        {"title", "Y Offset"},
                        {"ui:widget", "input_int"},
                        {"ui:options", {{"min", -1000}, {"max", 1000}}}
                    }},
                    {"mask_mode", {
                        {"type", "string"},
                        {"title", "Mask Mode"},
                        {"ui:widget", "combo"},
                        {"items", maskModeItems}
                    }},
                    {"merge_masks", {
                        {"type", "boolean"},
                        {"title", "Merge Masks"},
                        {"ui:widget", "checkbox"}
                    }},
                    {"invert_mask", {
                        {"type", "boolean"},
                        {"title", "Invert Mask"},
                        {"ui:widget", "checkbox"}
                    }},
                    {"mask_blur", {
                        {"type", "integer"},
                        {"title", "Mask Blur"},
                        {"ui:widget", "input_int"},
                        {"ui:options", {{"min", 0}, {"max", 100}}}
                    }},
                    {"inpaint_padding", {
                        {"type", "integer"},
                        {"title", "Inpaint Padding"},
                        {"ui:widget", "input_int"},
                        {"ui:options", {{"min", 0}, {"max", 512}}}
                    }},
                    {"inpaint_width", {
                        {"type", "integer"},
                        {"title", "Inpaint Width"},
                        {"ui:widget", "input_int"},
                        {"ui:options", {{"min", 0}, {"max", 2048}}}
                    }},
                    {"inpaint_height", {
                        {"type", "integer"},
                        {"title", "Inpaint Height"},
                        {"ui:widget", "input_int"},
                        {"ui:options", {{"min", 0}, {"max", 2048}}}
                    }},
                    {"denoising_strength", {
                        {"type", "number"},
                        {"title", "Denoising Strength"},
                        {"ui:widget", "input_float"},
                        {"ui:options", {{"step", 0.01f}, {"min", -1.0f}, {"max", 1.0f}}}
                    }},
                    {"steps", {
                        {"type", "integer"},
                        {"title", "Steps"},
                        {"ui:widget", "input_int"},
                        {"ui:options", {{"min", 0}, {"max", 150}}}
                    }},
                    {"cfg_scale", {
                        {"type", "number"},
                        {"title", "CFG Scale"},
                        {"ui:widget", "input_float"},
                        {"ui:options", {{"step", 0.1f}, {"min", -1.0f}, {"max", 30.0f}}}
                    }},
                    {"sample_method", {
                        {"type", "integer"},
                        {"title", "Sampler"},
                        {"ui:widget", "combo"},
                        {"items", sample_method_items},
                        {"itemCount", sample_method_item_count}
                    }},
                    {"scheduler", {
                        {"type", "integer"},
                        {"title", "Scheduler"},
                        {"ui:widget", "combo"},
                        {"items", scheduler_method_items},
                        {"itemCount", scheduler_method_item_count}
                    }},
                    {"sort_by", {
                        {"type", "string"},
                        {"title", "Sort By"},
                        {"ui:widget", "combo"},
                        {"items", sortByItems}
                    }},
                    {"extra_ad_args_override", {
                        {"type", "string"},
                        {"title", "Extra AD Args Override"},
                        {"ui:widget", "textarea"}
                    }}
                }}
            };
        }

        std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
            return {
                {"detector_path", &detector_path},
                {"ad_prompt", &ad_prompt},
                {"ad_negative_prompt", &ad_negative_prompt},
                {"input_size", &input_size},
                {"confidence", &confidence},
                {"nms", &nms},
                {"max_detections", &max_detections},
                {"mask_k_largest", &mask_k_largest},
                {"mask_min_ratio", &mask_min_ratio},
                {"mask_max_ratio", &mask_max_ratio},
                {"dilate_erode", &dilate_erode},
                {"x_offset", &x_offset},
                {"y_offset", &y_offset},
                {"mask_mode", &mask_mode},
                {"merge_masks", &merge_masks},
                {"invert_mask", &invert_mask},
                {"mask_blur", &mask_blur},
                {"inpaint_padding", &inpaint_padding},
                {"inpaint_width", &inpaint_width},
                {"inpaint_height", &inpaint_height},
                {"denoising_strength", &denoising_strength},
                {"steps", &steps},
                {"cfg_scale", &cfg_scale},
                {"sample_method", &sample_method},
                {"scheduler", &scheduler},
                {"sort_by", &sort_by},
                {"extra_ad_args_override", &extra_ad_args_override}
            };
        }

        ADetailerComponent& operator=(const ADetailerComponent& other) {
            if (this != &other) {
                detector_path = other.detector_path;
                ad_prompt = other.ad_prompt;
                ad_negative_prompt = other.ad_negative_prompt;
                input_size = other.input_size;
                confidence = other.confidence;
                nms = other.nms;
                max_detections = other.max_detections;
                mask_k_largest = other.mask_k_largest;
                mask_min_ratio = other.mask_min_ratio;
                mask_max_ratio = other.mask_max_ratio;
                dilate_erode = other.dilate_erode;
                x_offset = other.x_offset;
                y_offset = other.y_offset;
                mask_mode = other.mask_mode;
                merge_masks = other.merge_masks;
                invert_mask = other.invert_mask;
                mask_blur = other.mask_blur;
                inpaint_padding = other.inpaint_padding;
                inpaint_width = other.inpaint_width;
                inpaint_height = other.inpaint_height;
                denoising_strength = other.denoising_strength;
                steps = other.steps;
                cfg_scale = other.cfg_scale;
                sample_method = other.sample_method;
                scheduler = other.scheduler;
                sort_by = other.sort_by;
                extra_ad_args_override = other.extra_ad_args_override;
            }
            return *this;
        }

        nlohmann::json Serialize() const override {
            return { {compName, {
                {"detector_path", detector_path},
                {"ad_prompt", ad_prompt},
                {"ad_negative_prompt", ad_negative_prompt},
                {"input_size", input_size},
                {"confidence", confidence},
                {"nms", nms},
                {"max_detections", max_detections},
                {"mask_k_largest", mask_k_largest},
                {"mask_min_ratio", mask_min_ratio},
                {"mask_max_ratio", mask_max_ratio},
                {"dilate_erode", dilate_erode},
                {"x_offset", x_offset},
                {"y_offset", y_offset},
                {"mask_mode", mask_mode},
                {"merge_masks", merge_masks},
                {"invert_mask", invert_mask},
                {"mask_blur", mask_blur},
                {"inpaint_padding", inpaint_padding},
                {"inpaint_width", inpaint_width},
                {"inpaint_height", inpaint_height},
                {"denoising_strength", denoising_strength},
                {"steps", steps},
                {"cfg_scale", cfg_scale},
                {"sample_method", sample_method},
                {"scheduler", scheduler},
                {"sort_by", sort_by},
                {"extra_ad_args_override", extra_ad_args_override}
            }} };
        }

        void Deserialize(const nlohmann::json& j) override {
            BaseComponent::Deserialize(j);
            nlohmann::json componentData;
            if (j.contains(compName)) {
                componentData = j.at(compName);
            }
            else {
                for (auto it = j.begin(); it != j.end(); ++it) {
                    if (it.key() == compName) {
                        componentData = it.value();
                        break;
                    }
                }
                if (componentData.empty()) {
                    componentData = j;
                }
            }

            if (componentData.contains("detector_path")) detector_path = componentData["detector_path"].get<std::string>();
            if (componentData.contains("ad_prompt")) ad_prompt = componentData["ad_prompt"].get<std::string>();
            if (componentData.contains("ad_negative_prompt")) ad_negative_prompt = componentData["ad_negative_prompt"].get<std::string>();
            if (componentData.contains("input_size")) input_size = componentData["input_size"];
            if (componentData.contains("confidence")) confidence = componentData["confidence"];
            if (componentData.contains("nms")) nms = componentData["nms"];
            if (componentData.contains("max_detections")) max_detections = componentData["max_detections"];
            if (componentData.contains("mask_k_largest")) mask_k_largest = componentData["mask_k_largest"];
            if (componentData.contains("mask_min_ratio")) mask_min_ratio = componentData["mask_min_ratio"];
            if (componentData.contains("mask_max_ratio")) mask_max_ratio = componentData["mask_max_ratio"];
            if (componentData.contains("dilate_erode")) dilate_erode = componentData["dilate_erode"];
            if (componentData.contains("x_offset")) x_offset = componentData["x_offset"];
            if (componentData.contains("y_offset")) y_offset = componentData["y_offset"];
            if (componentData.contains("mask_mode")) mask_mode = componentData["mask_mode"].get<std::string>();
            if (componentData.contains("merge_masks")) merge_masks = componentData["merge_masks"];
            if (componentData.contains("invert_mask")) invert_mask = componentData["invert_mask"];
            if (componentData.contains("mask_blur")) mask_blur = componentData["mask_blur"];
            if (componentData.contains("inpaint_padding")) inpaint_padding = componentData["inpaint_padding"];
            if (componentData.contains("inpaint_width")) inpaint_width = componentData["inpaint_width"];
            if (componentData.contains("inpaint_height")) inpaint_height = componentData["inpaint_height"];
            if (componentData.contains("denoising_strength")) denoising_strength = componentData["denoising_strength"];
            if (componentData.contains("steps")) steps = componentData["steps"];
            if (componentData.contains("cfg_scale")) cfg_scale = componentData["cfg_scale"];
            if (componentData.contains("sample_method")) sample_method = componentData["sample_method"];
            if (componentData.contains("scheduler")) scheduler = componentData["scheduler"];
            if (componentData.contains("sort_by")) sort_by = componentData["sort_by"].get<std::string>();
            if (componentData.contains("extra_ad_args_override")) extra_ad_args_override = componentData["extra_ad_args_override"].get<std::string>();
        }

        std::string build_extra_ad_args() const {
            if (!extra_ad_args_override.empty()) {
                return extra_ad_args_override;
            }
            std::string args;
            args += "input_size=" + std::to_string(input_size);
            args += ",confidence=" + std::to_string(confidence);
            args += ",nms=" + std::to_string(nms);
            args += ",max_detections=" + std::to_string(max_detections);
            args += ",mask_k_largest=" + std::to_string(mask_k_largest);
            args += ",mask_min_ratio=" + std::to_string(mask_min_ratio);
            args += ",mask_max_ratio=" + std::to_string(mask_max_ratio);
            args += ",dilate_erode=" + std::to_string(dilate_erode);
            args += ",x_offset=" + std::to_string(x_offset);
            args += ",y_offset=" + std::to_string(y_offset);
            args += ",mask_mode=" + mask_mode;
            args += ",merge_masks=" + std::string(merge_masks ? "true" : "false");
            args += ",invert_mask=" + std::string(invert_mask ? "true" : "false");
            args += ",mask_blur=" + std::to_string(mask_blur);
            args += ",inpaint_padding=" + std::to_string(inpaint_padding);
            if (inpaint_width > 0) args += ",inpaint_width=" + std::to_string(inpaint_width);
            if (inpaint_height > 0) args += ",inpaint_height=" + std::to_string(inpaint_height);
            if (denoising_strength >= 0.0f) args += ",denoising_strength=" + std::to_string(denoising_strength);
            if (steps > 0) args += ",steps=" + std::to_string(steps);
            if (cfg_scale >= 0.0f) args += ",cfg_scale=" + std::to_string(cfg_scale);
            if (sample_method >= 0 && sample_method < sample_method_item_count) {
                args += ",sample_method=" + std::string(sample_method_items[sample_method]);
            }
            if (scheduler >= 0 && scheduler < scheduler_method_item_count) {
                args += ",scheduler=" + std::string(scheduler_method_items[scheduler]);
            }
            if (!sort_by.empty() && sort_by != "none") args += ",sort_by=" + sort_by;
            return args;
        }

        sd_adetailer_params_t to_adetailer_params() const {
            sd_adetailer_params_t params{};
            params.prompt = ad_prompt.empty() ? nullptr : ad_prompt.c_str();
            params.negative_prompt = ad_negative_prompt.empty() ? nullptr : ad_negative_prompt.c_str();
            params.extra_ad_args = build_extra_ad_args().c_str();
            return params;
        }
    };

} // namespace ECS