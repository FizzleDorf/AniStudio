#pragma once
#include "BaseComponent.hpp"
#include "PropertyTypes.hpp"
#include "DiffusionOptions.hpp"
#include "UISchema.hpp"
#include <string>
#include <nlohmann/json.hpp>

namespace ECS {

    class SDCPPSettingsComponent : public BaseComponent {
    public:
        bool enable_mmap = true;
        bool flash_attn = true;
        std::string max_vram = "-1";
        bool eager_load = false;
        std::string backend;
        std::string params_backend;
        std::string split_mode;
        bool auto_fit = false;
        std::string rpc_servers;
        std::string lora_apply_mode = "LORA_APPLY_AUTO";
        bool diffusion_flash_attn = false;
        bool diffusion_conv_direct = false;
        bool vae_conv_direct = false;
        bool force_sdxl_vae_conv_scale = false;
        int log_level = 1;
        std::string model_args;
        int preview_mode = 2;
        int preview_interval = 1;
        bool disable_prefetch = false;
        bool disable_segmented_compute = false;
        float linear_scale = 0.0f;
        float attn_scale = 0.0f;
        bool sage_attn = false;

        SDCPPSettingsComponent() = default;

        const char* GetCompName() const override { return "SDCPP"; }
        const char* GetCompCategory() const override { return "SDCPP"; }

        const nlohmann::json& GetSchema() const override {
            static const nlohmann::json j = {
                {"title", "Global SDCPP Settings"},
                {"type", "object"},
                {"propertyOrder", {
                    "enable_mmap", "max_vram", "eager_load",
                    "backend", "params_backend", "split_mode", "auto_fit",
                    "rpc_servers", "lora_apply_mode",
                    "flash_attn", "diffusion_flash_attn",
                    "diffusion_conv_direct", "vae_conv_direct", "force_sdxl_vae_conv_scale",
                    "sage_attn",
                    "log_level", "model_args",
                    "preview_mode", "preview_interval",
                    "disable_prefetch", "disable_segmented_compute",
                    "linear_scale", "attn_scale"
                }},
                {"properties", {
                    {"enable_mmap", {{"type","boolean"},{"title","Enable mmap"},{"ui:widget","checkbox"}}},
                    {"max_vram", {{"type","string"},{"title","Max VRAM"},{"ui:widget","text"}}},
                    {"eager_load", {{"type","boolean"},{"title","Eager Load"},{"ui:widget","checkbox"}}},
                    {"backend", {{"type","string"},{"title","Backend"},{"ui:widget","text"}}},
                    {"params_backend", {{"type","string"},{"title","Params Backend"},{"ui:widget","text"}}},
                    {"split_mode", {{"type","string"},{"title","Split Mode"},{"ui:widget","text"}}},
                    {"auto_fit", {{"type","boolean"},{"title","Auto Fit"},{"ui:widget","checkbox"}}},
                    {"rpc_servers", {{"type","string"},{"title","RPC Servers"},{"ui:widget","text"}}},
                    {"lora_apply_mode", {{"type","string"},{"title","LoRA Apply Mode"},{"ui:widget","combo"},{"items", get_lora_apply_mode_names()},{"itemCount", static_cast<int>(get_lora_apply_mode_names().size())}}},
                    {"flash_attn", {{"type","boolean"},{"title","Flash Attention"},{"ui:widget","checkbox"}}},
                    {"diffusion_flash_attn", {{"type","boolean"},{"title","Diffusion Flash Attention"},{"ui:widget","checkbox"}}},
                    {"diffusion_conv_direct", {{"type","boolean"},{"title","Diffusion Conv Direct"},{"ui:widget","checkbox"}}},
                    {"vae_conv_direct", {{"type","boolean"},{"title","VAE Conv Direct"},{"ui:widget","checkbox"}}},
                    {"force_sdxl_vae_conv_scale", {{"type","boolean"},{"title","Force SDXL VAE Conv Scale"},{"ui:widget","checkbox"}}},
                    {"sage_attn", {{"type","boolean"},{"title","Sage Attention"},{"ui:widget","checkbox"}}},
                    {"log_level", {{"type","integer"},{"title","Log Level"},{"ui:widget","combo"},{"items",{"DEBUG","INFO","WARN","ERROR"}},{"itemCount",4}}},
                    {"model_args", {{"type","string"},{"title","Model Args"},{"ui:widget","text"}}},
                    {"preview_mode", {{"type","integer"},{"title","Preview Mode"},{"ui:widget","combo"},{"items",{"None","Proj","TAE","VAE"}},{"itemCount",4}}},
                    {"preview_interval", {{"type","integer"},{"title","Preview Interval"},{"ui:widget","input_int"},{"ui:options",{{"min",-100},{"max",100}}}}},
                    {"disable_prefetch", {{"type","boolean"},{"title","Disable Prefetch"},{"ui:widget","checkbox"}}},
                    {"disable_segmented_compute", {{"type","boolean"},{"title","Disable Segmented Compute"},{"ui:widget","checkbox"}}},
                    {"linear_scale", {{"type","number"},{"title","Linear Scale"},{"ui:widget","input_float"},{"ui:options",{{"step",0.01f},{"min",0.0f},{"max",10.0f}}}}},
                    {"attn_scale", {{"type","number"},{"title","Attention Scale"},{"ui:widget","input_float"},{"ui:options",{{"step",0.01f},{"min",0.0f},{"max",10.0f}}}}}
                }}
            };
            return j;
        }

        SDCPPSettingsComponent(const SDCPPSettingsComponent& other) = default;
        SDCPPSettingsComponent& operator=(const SDCPPSettingsComponent& other) = default;

        void ResetToDefaults() {
            *this = SDCPPSettingsComponent();
        }

        nlohmann::json Serialize() const override {
            return {
                {"enable_mmap", enable_mmap},
                {"max_vram", max_vram},
                {"flash_attn", flash_attn},
                {"eager_load", eager_load},
                {"backend", backend},
                {"params_backend", params_backend},
                {"split_mode", split_mode},
                {"auto_fit", auto_fit},
                {"rpc_servers", rpc_servers},
                {"lora_apply_mode", lora_apply_mode},
                {"diffusion_flash_attn", diffusion_flash_attn},
                {"diffusion_conv_direct", diffusion_conv_direct},
                {"vae_conv_direct", vae_conv_direct},
                {"force_sdxl_vae_conv_scale", force_sdxl_vae_conv_scale},
                {"log_level", log_level},
                {"model_args", model_args},
                {"preview_mode", preview_mode},
                {"preview_interval", preview_interval},
                {"disable_prefetch", disable_prefetch},
                {"disable_segmented_compute", disable_segmented_compute},
                {"linear_scale", linear_scale},
                {"attn_scale", attn_scale},
                {"sage_attn", sage_attn}
            };
        }

        void Deserialize(const nlohmann::json& j) override {
            if (j.contains("enable_mmap")) enable_mmap = j["enable_mmap"].get<bool>();
            if (j.contains("max_vram")) max_vram = j["max_vram"].get<std::string>();
            if (j.contains("flash_attn")) flash_attn = j["flash_attn"].get<bool>();
            if (j.contains("eager_load")) eager_load = j["eager_load"].get<bool>();
            if (j.contains("backend")) backend = j["backend"].get<std::string>();
            if (j.contains("params_backend")) params_backend = j["params_backend"].get<std::string>();
            if (j.contains("split_mode")) split_mode = j["split_mode"].get<std::string>();
            if (j.contains("auto_fit")) auto_fit = j["auto_fit"].get<bool>();
            if (j.contains("rpc_servers")) rpc_servers = j["rpc_servers"].get<std::string>();
            if (j.contains("lora_apply_mode")) lora_apply_mode = j["lora_apply_mode"].get<std::string>();
            if (j.contains("diffusion_flash_attn")) diffusion_flash_attn = j["diffusion_flash_attn"].get<bool>();
            if (j.contains("diffusion_conv_direct")) diffusion_conv_direct = j["diffusion_conv_direct"].get<bool>();
            if (j.contains("vae_conv_direct")) vae_conv_direct = j["vae_conv_direct"].get<bool>();
            if (j.contains("force_sdxl_vae_conv_scale")) force_sdxl_vae_conv_scale = j["force_sdxl_vae_conv_scale"].get<bool>();
            if (j.contains("log_level")) log_level = j["log_level"].get<int>();
            if (j.contains("model_args")) model_args = j["model_args"].get<std::string>();
            if (j.contains("preview_mode")) preview_mode = j["preview_mode"].get<int>();
            if (j.contains("preview_interval")) preview_interval = j["preview_interval"].get<int>();
            if (j.contains("disable_prefetch")) disable_prefetch = j["disable_prefetch"].get<bool>();
            if (j.contains("disable_segmented_compute")) disable_segmented_compute = j["disable_segmented_compute"].get<bool>();
            if (j.contains("linear_scale")) linear_scale = j["linear_scale"].get<float>();
            if (j.contains("attn_scale")) attn_scale = j["attn_scale"].get<float>();
            if (j.contains("sage_attn")) sage_attn = j["sage_attn"].get<bool>();
        }

        std::unordered_map<std::string, Engine::PropertyVariant> GetPropertyMap() override {
            return {
                {"enable_mmap", &enable_mmap},
                {"max_vram", &max_vram},
                {"flash_attn", &flash_attn},
                {"eager_load", &eager_load},
                {"backend", &backend},
                {"params_backend", &params_backend},
                {"split_mode", &split_mode},
                {"auto_fit", &auto_fit},
                {"rpc_servers", &rpc_servers},
                {"lora_apply_mode", &lora_apply_mode},
                {"diffusion_flash_attn", &diffusion_flash_attn},
                {"diffusion_conv_direct", &diffusion_conv_direct},
                {"vae_conv_direct", &vae_conv_direct},
                {"force_sdxl_vae_conv_scale", &force_sdxl_vae_conv_scale},
                {"log_level", &log_level},
                {"model_args", &model_args},
                {"preview_mode", &preview_mode},
                {"preview_interval", &preview_interval},
                {"disable_prefetch", &disable_prefetch},
                {"disable_segmented_compute", &disable_segmented_compute},
                {"linear_scale", &linear_scale},
                {"attn_scale", &attn_scale},
                {"sage_attn", &sage_attn}
            };
        }
    };

} // namespace ECS