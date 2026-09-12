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

        ConversionComponent() = default;

        const char* GetCompName() const override { return "Conversion"; }
        const char* GetCompCategory() const override { return "Tools"; }

        const nlohmann::json& GetSchema() const override {
            static const nlohmann::json j = {
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
                        {"items", get_type_method_names()},
                        {"itemCount", static_cast<int>(get_type_method_names().size())},
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
            return j;
        }

        ConversionComponent(const ConversionComponent& other) : BaseComponent(other) {
            tensorTypeRules = other.tensorTypeRules;
            convertName = other.convertName;
            outputType = other.outputType;
            nThreads = other.nThreads;
        }

        ConversionComponent& operator=(const ConversionComponent& other) {
            if (this != &other) {
                tensorTypeRules = other.tensorTypeRules;
                convertName = other.convertName;
                outputType = other.outputType;
                nThreads = other.nThreads;
            }
            return *this;
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
            nlohmann::json j;
            j[GetCompName()] = {
                {"tensorTypeRules", tensorTypeRules},
                {"convertName", convertName},
                {"outputType", outputType},
                {"nThreads", nThreads}
            };
            return j;
        }

        void Deserialize(const nlohmann::json& j) override {
            const char* key = GetCompName();
            nlohmann::json componentData;
            if (j.contains(key))
                componentData = j.at(key);
            else
                componentData = j;

            if (componentData.contains("tensorTypeRules"))
                tensorTypeRules = componentData["tensorTypeRules"];
            if (componentData.contains("convertName"))
                convertName = componentData["convertName"];
            if (componentData.contains("outputType"))
                outputType = componentData["outputType"];
            if (componentData.contains("nThreads"))
                nThreads = componentData["nThreads"];
        }

        enum sd_type_t get_output_type_enum() const {
            return static_cast<enum sd_type_t>(type_method_from_name(outputType));
        }
    };

} // namespace ECS