#include "TextEditorFontUtil.hpp"
#include <imgui.h>
#include <unordered_map>
#include <string>

namespace TextEditorUtil {

    static ECS::FontSettingsComponent* g_fontComp = nullptr;
    static ECS::TextEditorSettingsComponent* g_settingsComp = nullptr;

    void SetFontComponent(ECS::FontSettingsComponent* comp) {
        g_fontComp = comp;
    }

    void SetSettingsComponent(ECS::TextEditorSettingsComponent* comp) {
        g_settingsComp = comp;
    }

    static std::unordered_map<std::string, ImFont*> s_editorFontCache;

    static ImFont* loadEditorFont(const std::string& fontName) {
        if (fontName.empty()) return nullptr;
        auto it = s_editorFontCache.find(fontName);
        if (it != s_editorFontCache.end()) {
            return it->second;
        }

        if (!g_fontComp) return nullptr;

        std::string fontPath;
        for (const auto& entry : g_fontComp->availableFonts) {
            if (entry.name == fontName) {
                fontPath = entry.path;
                break;
            }
        }
        if (fontPath.empty()) return nullptr;

        ImGuiIO& io = ImGui::GetIO();
        float fontSize = 16.0f;
        ImFontConfig config;
        config.OversampleH = 3;
        config.OversampleV = 1;
        config.PixelSnapH = true;
        strncpy(config.Name, fontName.c_str(), sizeof(config.Name) - 1);
        config.Name[sizeof(config.Name) - 1] = '\0';
        ImFont* font = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), fontSize, &config);
        if (font) {
            s_editorFontCache[fontName] = font;
            return font;
        }
        return nullptr;
    }

    void pushEditorFont() {
        if (!g_settingsComp) {
            ImGuiIO& io = ImGui::GetIO();
            if (io.FontDefault) {
                ImGui::PushFont(io.FontDefault);
            }
            return;
        }

        if (g_settingsComp->useCustomFont) {
            const std::string& fontName = g_settingsComp->editorFontName;
            if (!fontName.empty()) {
                ImFont* font = loadEditorFont(fontName);
                if (font) {
                    ImGui::PushFont(font);
                    return;
                }
            }
        }

        ImGuiIO& io = ImGui::GetIO();
        if (io.FontDefault) {
            ImGui::PushFont(io.FontDefault);
        }
    }

    void popEditorFont() {
        ImGui::PopFont();
    }

} // namespace TextEditorUtil