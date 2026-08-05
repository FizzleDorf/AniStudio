#include "VectorWidgets.hpp"
#include "FileDialogUtil.hpp"
#include <imgui.h>
#include <filesystem>
#include <cstring>

namespace VectorWidgets {

    bool RenderLoraList(std::vector<ECS::LoraComponent::LoraEntry>& entries) {
        bool modified = false;

        ImGui::Text("LoRAs");
        ImGui::SameLine();
        if (ImGui::Button("Add LoRA")) {
            entries.push_back({});
            modified = true;
        }

        for (size_t i = 0; i < entries.size(); ) {
            auto& entry = entries[i];
            ImGui::PushID(static_cast<int>(i));

            bool open = ImGui::CollapsingHeader(("LoRA #" + std::to_string(i)).c_str(), ImGuiTreeNodeFlags_DefaultOpen);
            if (open) {
                std::string path = entry.path;
                ImGui::Text("Path: %s", path.c_str());
                ImGui::SameLine();
                if (ImGui::Button("Browse...")) {
                    std::string outPath;
                    if (FileDialog::OpenFile("Select LoRA File", FileDialog::FilterType::DIFFUSION_MODEL, outPath)) {
                        entry.path = outPath;
                        modified = true;
                    }
                }

                float mult = entry.multiplier;
                if (ImGui::InputFloat("Multiplier", &mult, 0.1f, 0.5f, "%.2f")) {
                    entry.multiplier = mult;
                    modified = true;
                }

                bool highNoise = entry.is_high_noise;
                if (ImGui::Checkbox("High Noise", &highNoise)) {
                    entry.is_high_noise = highNoise;
                    modified = true;
                }

                if (ImGui::Button("Remove")) {
                    entries.erase(entries.begin() + i);
                    modified = true;
                    ImGui::PopID();
                    continue;
                }
            }
            ImGui::PopID();
            ++i;
        }
        return modified;
    }

    bool RenderEmbeddingsPairList(std::vector<std::pair<std::string, std::string>>& entries) {
        bool modified = false;

        ImGui::Text("Embeddings");
        ImGui::SameLine();
        if (ImGui::Button("Add Embedding")) {
            entries.push_back({});
            modified = true;
        }

        for (size_t i = 0; i < entries.size(); ) {
            auto& entry = entries[i];
            ImGui::PushID(static_cast<int>(i));

            bool open = ImGui::CollapsingHeader(("Embedding #" + std::to_string(i)).c_str(), ImGuiTreeNodeFlags_DefaultOpen);
            if (open) {
                char nameBuf[256];
                strncpy(nameBuf, entry.first.c_str(), sizeof(nameBuf) - 1);
                nameBuf[sizeof(nameBuf) - 1] = '\0';
                if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf))) {
                    entry.first = nameBuf;
                    modified = true;
                }

                std::string path = entry.second;
                ImGui::Text("Path: %s", path.c_str());
                ImGui::SameLine();
                if (ImGui::Button("Browse...")) {
                    std::string outPath;
                    if (FileDialog::OpenFile("Select Embedding File", FileDialog::FilterType::DIFFUSION_MODEL, outPath)) {
                        entry.second = outPath;
                        modified = true;
                    }
                }

                if (ImGui::Button("Remove")) {
                    entries.erase(entries.begin() + i);
                    modified = true;
                    ImGui::PopID();
                    continue;
                }
            }
            ImGui::PopID();
            ++i;
        }
        return modified;
    }

    bool RenderFilePathList(std::vector<std::string>& paths) {
        bool modified = false;

        ImGui::Text("Files");
        ImGui::SameLine();
        if (ImGui::Button("Add File")) {
            paths.push_back("");
            modified = true;
        }

        for (size_t i = 0; i < paths.size(); ) {
            std::string& path = paths[i];
            ImGui::PushID(static_cast<int>(i));

            ImGui::Text("File %zu: %s", i, path.c_str());
            ImGui::SameLine();
            if (ImGui::Button("Browse...")) {
                std::string outPath;
                if (FileDialog::OpenFile("Select File", FileDialog::FilterType::ALL_FILES, outPath)) {
                    path = outPath;
                    modified = true;
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Remove")) {
                paths.erase(paths.begin() + i);
                modified = true;
                ImGui::PopID();
                continue;
            }
            ImGui::PopID();
            ++i;
        }
        return modified;
    }

    bool RenderFloatList(std::vector<float>& floats) {
        bool modified = false;

        ImGui::Text("Floats");
        ImGui::SameLine();
        if (ImGui::Button("Add Float")) {
            floats.push_back(0.0f);
            modified = true;
        }

        for (size_t i = 0; i < floats.size(); ) {
            float& val = floats[i];
            ImGui::PushID(static_cast<int>(i));

            if (ImGui::InputFloat(("Value " + std::to_string(i)).c_str(), &val, 0.1f, 0.5f, "%.3f")) {
                modified = true;
            }
            ImGui::SameLine();
            if (ImGui::Button("Remove")) {
                floats.erase(floats.begin() + i);
                modified = true;
                ImGui::PopID();
                continue;
            }
            ImGui::PopID();
            ++i;
        }
        return modified;
    }

    bool RenderStringList(std::vector<std::string>& strings) {
        bool modified = false;

        ImGui::Text("Strings");
        ImGui::SameLine();
        if (ImGui::Button("Add String")) {
            strings.push_back("");
            modified = true;
        }

        for (size_t i = 0; i < strings.size(); ) {
            std::string& str = strings[i];
            ImGui::PushID(static_cast<int>(i));

            char buf[256];
            strncpy(buf, str.c_str(), sizeof(buf) - 1);
            buf[sizeof(buf) - 1] = '\0';
            if (ImGui::InputText(("Item " + std::to_string(i)).c_str(), buf, sizeof(buf))) {
                str = buf;
                modified = true;
            }
            ImGui::SameLine();
            if (ImGui::Button("Remove")) {
                strings.erase(strings.begin() + i);
                modified = true;
                ImGui::PopID();
                continue;
            }
            ImGui::PopID();
            ++i;
        }
        return modified;
    }

} // namespace VectorWidgets