#pragma once

#include "BaseComponent.hpp"
#include "PropertyTypes.hpp"
#include "DiffusionOptions.hpp"
#include "stable-diffusion.h"
#include <string>
#include <vector>

namespace ECS {

    struct ConversionComponent : public BaseComponent {
        std::string tensorTypeRules = "";
        bool convertName = true;
        std::string outputType = "F16";
        int nThreads = -1;

        ConversionComponent() {
            compName = "Conversion";
            compCategory = "Tools";

            schema = {
                {"title", "Model Conversion Settings"},
                {"type", "object"},
                {"propertyOrder", {"tensorTypeRules", "convertName", "outputType", "nThreads"}},
                {"properties", {
                    {"tensorTypeRules", {
                        {"type", "string"},
                        {"title", "Tensor Type Rules"},
                        {"description", "Optional rules for tensor type conversion"}
                    }},
                    {"convertName", {
                        {"type", "boolean"},
                        {"title", "Convert Layer Names"},
                        {"description", "Whether to convert layer names during conversion"},
                        {"default", true}
                    }},
                    {"outputType", {
                        {"type", "string"},
                        {"title", "Output Type"},
                        {"description", "Quantization type for the converted model"},
                        {"ui:widget", "combo"},
                        {"items", type_method_items},
                        {"itemCount", type_method_item_count},
                        {"default", "F16"}
                    }},
                    {"nThreads", {
                        {"type", "integer"},
                        {"title", "Threads"},
                        {"description", "Number of threads to use for conversion (-1 = auto)"},
                        {"ui:widget", "input_int"},
                        {"ui:options", {
                            {"min", -1},
                            {"max", 64},
                            {"step", 1}
                        }},
                        {"default", -1}
                    }}
                }}
            };
        }

        std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
            return {
                {"tensorTypeRules", &tensorTypeRules},
                {"convertName", &convertName},
                {"outputType", &outputType},
                {"nThreads", &nThreads}
            };
        }

        nlohmann::json Serialize() const override {
            return { {compName, {
                {"tensorTypeRules", tensorTypeRules},
                {"convertName", convertName},
                {"outputType", outputType},
                {"nThreads", nThreads}
            }} };
        }

        void Deserialize(const nlohmann::json& j) override {
            if (j.contains(compName)) {
                auto compData = j.at(compName);
                if (compData.contains("tensorTypeRules"))
                    tensorTypeRules = compData["tensorTypeRules"];
                if (compData.contains("convertName"))
                    convertName = compData["convertName"];
                if (compData.contains("outputType"))
                    outputType = compData["outputType"];
                if (compData.contains("nThreads"))
                    nThreads = compData["nThreads"];
            }
        }

        enum sd_type_t get_output_type_enum() const {
            return str_to_sd_type(outputType.c_str());
        }
    };

} // namespace ECS