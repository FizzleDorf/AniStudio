#include "SDCPPSettingsTab.hpp"
#include <imgui.h>
#include <algorithm>

namespace ECS {

    SDCPPSettingsTab::SDCPPSettingsTab(SDCPPSettingsComponent& comp) : m_comp(comp) {}

    bool SDCPPSettingsTab::FilterPass(const std::string& section) const {
        if (m_filter.empty()) return true;
        std::string lower = section;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        std::string f = m_filter;
        std::transform(f.begin(), f.end(), f.begin(), ::tolower);
        return lower.find(f) != std::string::npos;
    }

    static void RenderPropertyWithTooltip(const char* label, void* data_ptr, const nlohmann::json& prop_schema) {

    }

    static void ShowTooltip(const nlohmann::json& prop_schema) {
        if (prop_schema.contains("description") && prop_schema["description"].is_string()) {
            ImGui::SameLine();
            ImGui::TextDisabled("(?)");
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s", prop_schema["description"].get<std::string>().c_str());
            }
        }
    }

    void SDCPPSettingsTab::Render() {
        if (ImGui::BeginChild("SDCPPSettings", ImVec2(0, 0), false)) {
            ImGui::Text("Global SDCPP settings applied at context creation");
            ImGui::Separator();

            if (FilterPass("General")) RenderGeneralSettings();
            if (FilterPass("Model")) RenderModelSettings();
            if (FilterPass("Advanced")) RenderAdvancedSettings();
            RenderActionButtons();
        }
        ImGui::EndChild();
    }

    void SDCPPSettingsTab::RenderGeneralSettings() {
        auto& props = m_comp.schema["properties"];

        ImGui::Checkbox("Enable mmap", &m_comp.enable_mmap);
        ShowTooltip(props["enable_mmap"]);

        char maxVramBuf[256];
        strncpy(maxVramBuf, m_comp.max_vram.c_str(), sizeof(maxVramBuf) - 1);
        maxVramBuf[sizeof(maxVramBuf) - 1] = '\0';
        if (ImGui::InputText("Max VRAM", maxVramBuf, sizeof(maxVramBuf))) {
            m_comp.max_vram = maxVramBuf;
        }
        ShowTooltip(props["max_vram"]);

        ImGui::Checkbox("Stream Layers", &m_comp.stream_layers);
        ShowTooltip(props["stream_layers"]);

        ImGui::Checkbox("Eager Load", &m_comp.eager_load);
        ShowTooltip(props["eager_load"]);

        char backendBuf[128];
        strncpy(backendBuf, m_comp.backend.c_str(), sizeof(backendBuf) - 1);
        backendBuf[sizeof(backendBuf) - 1] = '\0';
        if (ImGui::InputText("Backend", backendBuf, sizeof(backendBuf))) {
            m_comp.backend = backendBuf;
        }
        ShowTooltip(props["backend"]);

        char paramsBackendBuf[128];
        strncpy(paramsBackendBuf, m_comp.params_backend.c_str(), sizeof(paramsBackendBuf) - 1);
        paramsBackendBuf[sizeof(paramsBackendBuf) - 1] = '\0';
        if (ImGui::InputText("Params Backend", paramsBackendBuf, sizeof(paramsBackendBuf))) {
            m_comp.params_backend = paramsBackendBuf;
        }
        ShowTooltip(props["params_backend"]);
    }

    void SDCPPSettingsTab::RenderModelSettings() {
        auto& props = m_comp.schema["properties"];

        char splitModeBuf[128];
        strncpy(splitModeBuf, m_comp.split_mode.c_str(), sizeof(splitModeBuf) - 1);
        splitModeBuf[sizeof(splitModeBuf) - 1] = '\0';
        if (ImGui::InputText("Split Mode", splitModeBuf, sizeof(splitModeBuf))) {
            m_comp.split_mode = splitModeBuf;
        }
        ShowTooltip(props["split_mode"]);

        ImGui::Checkbox("Auto Fit", &m_comp.auto_fit);
        ShowTooltip(props["auto_fit"]);

        char rpcServersBuf[256];
        strncpy(rpcServersBuf, m_comp.rpc_servers.c_str(), sizeof(rpcServersBuf) - 1);
        rpcServersBuf[sizeof(rpcServersBuf) - 1] = '\0';
        if (ImGui::InputText("RPC Servers", rpcServersBuf, sizeof(rpcServersBuf))) {
            m_comp.rpc_servers = rpcServersBuf;
        }
        ShowTooltip(props["rpc_servers"]);

        if (ImGui::Combo("LoRA Apply Mode", &m_comp.lora_apply_mode, lora_apply_mode_items, lora_apply_mode_item_count)) {
        }
        ShowTooltip(props["lora_apply_mode"]);
    }

    void SDCPPSettingsTab::RenderAdvancedSettings() {
        auto& props = m_comp.schema["properties"];
        
        ImGui::Checkbox("Flash Attention", &m_comp.diffusion_flash_attn);
        ShowTooltip(props["flash_attn"]);

        ImGui::Checkbox("Diffusion Flash Attention", &m_comp.diffusion_flash_attn);
        ShowTooltip(props["diffusion_flash_attn"]);

        ImGui::Checkbox("Diffusion Conv Direct", &m_comp.diffusion_conv_direct);
        ShowTooltip(props["diffusion_conv_direct"]);

        ImGui::Checkbox("VAE Conv Direct", &m_comp.vae_conv_direct);
        ShowTooltip(props["vae_conv_direct"]);

        ImGui::Checkbox("Force SDXL VAE Conv Scale", &m_comp.force_sdxl_vae_conv_scale);
        ShowTooltip(props["force_sdxl_vae_conv_scale"]);
    }

    void SDCPPSettingsTab::RenderActionButtons() {
        if (ImGui::Button("Save Settings")) SaveSettings();
        ImGui::SameLine();
        if (ImGui::Button("Reset to Defaults")) ResetToDefaults();
        ImGui::SameLine();
        if (ImGui::Button("Revert Changes")) RestoreFromBackup();
        if (m_comp.HasUnsavedChanges()) {
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Unsaved changes");
        }
    }

}