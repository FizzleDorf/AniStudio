#include "GuiSettings.hpp"
#include "ImGuiFileDialog.h"
#include "ImGuiFileDialogConfig.h"

using json = nlohmann::json;

//ComfyUI Path Args
extern FlagOption inputOptions[] = {
    {"venv_path", "Specify the path to the virtual environment (Default is AniStudio/comfy).", ""},
    {"ComfyUi_Install_Path", "Specify the path to a ComfyUI install (Default is AniStudio/comfy).", ""},
    {"--listen [IP]", "Specify the IP address to listen on (default: 127.0.0.1). If --listen is provided without an argument, it defaults to 0.0.0.0. (listens on all)", ""},
    {"--port PORT", "Set the listen port.", ""},
    {"--tls-keyfile TLS_KEYFILE", "Path to TLS (SSL) key file. Enables TLS, makes app accessible at https://... requires --tls-certfile to function", ""},
    {"--tls-certfile TLS_CERTFILE", "Path to TLS (SSL) certificate file. Enables TLS, makes app accessible at https://... requires --tls-keyfile to function", ""},
    {"--enable-cors-header [ORIGIN]", "Enable CORS (Cross-Origin Resource Sharing) with optional origin or allow all with default '*'.", ""},
    {"--max-upload-size MAX_UPLOAD_SIZE", "Set the maximum upload size in MB.", ""},
    {"--extra-model-paths-config PATH [PATH ...]", "Load one or more extra_model_paths.yaml files.", ""},
    {"--output-directory OUTPUT_DIRECTORY", "Set the ComfyUI output directory.", ""},
    {"--temp-directory TEMP_DIRECTORY", "Set the ComfyUI temp directory (default is in the ComfyUI directory).", ""},
    {"--input-directory INPUT_DIRECTORY", "Set the ComfyUI input directory.", ""},
    {"--cuda-device DEVICE_ID", "Set the id of the cuda device this instance will use.", ""},
    {"--directml [DIRECTML_DEVICE]", "Use torch-directml.", ""},
    {"--preview-method [none,auto,latent2rgb,taesd]", "Default preview method for sampler nodes.", ""}
};

//ComfyUI bool Args
extern BoolOption boolOptions[] = {
    {"--auto-launch", "Automatically launch ComfyUI in the default browser.", false},
    {"--disable-auto-launch", "Disable auto launching the browser.", false},
    {"--cuda-malloc", "Enable cudaMallocAsync (enabled by default for torch 2.0 and up).", false},
    {"--disable-cuda-malloc", "Disable cudaMallocAsync.", false},
    {"--force-fp32", "Force fp32 (If this makes your GPU work better please report it).", false},
    {"--force-fp16", "Force fp16.", false},
    {"--bf16-unet", "Run the UNET in bf16. This should only be used for testing stuff.", false},
    {"--fp16-unet", "Store unet weights in fp16.", false},
    {"--fp8_e4m3fn-unet", "Store unet weights in fp8_e4m3fn.", false},
    {"--fp8_e5m2-unet", "Store unet weights in fp8_e5m2.", false},
    {"--fp16-vae", "Run the VAE in fp16, might cause black images.", false},
    {"--fp32-vae", "Run the VAE in full precision fp32.", false},
    {"--bf16-vae", "Run the VAE in bf16.", false},
    {"--cpu-vae", "Run the VAE on the CPU.", false},
    {"--fp8_e4m3fn-text-enc", "Store text encoder weights in fp8 (e4m3fn variant).", false},
    {"--fp8_e5m2-text-enc", "Store text encoder weights in fp8 (e5m2 variant).", false},
    {"--fp16-text-enc", "Store text encoder weights in fp16.", false},
    {"--fp32-text-enc", "Store text encoder weights in fp32.", false},
    {"--force-channels-last", "Force channels last format when inferencing the models.", false},
    {"--disable-ipex-optimize", "Disables ipex.optimize when loading models with Intel GPUs.", false},
    {"--use-split-cross-attention", "Use the split cross attention optimization. Ignored when xformers is used.", false},
    {"--use-quad-cross-attention", "Use the sub-quadratic cross attention optimization . Ignored when xformers is used.", false},
    {"--use-pytorch-cross-attention", "Use the new pytorch 2.0 cross attention function.", false},
    {"--disable-xformers", "Disable xformers.", false},
    {"--force-upcast-attention", "Force enable attention upcasting, please report if it fixes black images.", false},
    {"--dont-upcast-attention", "Disable all upcasting of attention. Should be unnecessary except for debugging.", false},
    {"--gpu-only", "Store and run everything (text encoders/CLIP models, etc... on the GPU).", false},
    {"--highvram", "By default models will be unloaded to CPU memory after being used. This option keeps them in GPU memory.", false},
    {"--normalvram", "Used to force normal vram use if lowvram gets automatically enabled.", false},
    {"--lowvram", "Split the unet in parts to use less vram.", false},
    {"--novram", "When lowvram isn't enough.", false},
    {"--cpu", "To use the CPU for everything (slow).", false},
    {"--disable-smart-memory", "Force ComfyUI to aggressively offload to regular ram instead of keeping models in vram when it can.", false},
    {"--deterministic", "Make pytorch use slower deterministic algorithms when it can. Note that this might not make images deterministic in all cases.", false},
    {"--dont-print-server", "Don't print server output.", false},
    {"--quick-test-for-ci", "Quick test for CI.", false},
    {"--windows-standalone-build", "Windows standalone build: Enable convenient things that most people using the standalone windows build will probably enjoy (like auto opening the page on startup).", false},
    {"--disable-metadata", "Disable saving prompt metadata in files.", false},
    {"--multi-user", "Enables per-user storage.", false},
    {"--verbose", "Enables more debug prints.", false}
};

void GuiSettings::Render() {
    if (viewState.showSettingsView) {
        ImGui::SetNextWindowSize(ImVec2(700, 400), ImGuiCond_FirstUseEver);
        ImGui::Begin("Settings");

        if (ImGui::BeginTabBar("Startup Options"))
        {
            // AniStudio Settings
            if (ImGui::BeginTabItem("AniStudio Settings"))
            {
                // Content for AniStudio Settings
                ImGui::EndTabItem();
            }

            // ComfyUI Settings
            if (ImGui::BeginTabItem("ComfyUI Settings"))
            {
                if (ImGui::Button("Install comfy-cli"))
                {
                    InstallVenv(inputOptions[0].value);
                }

                ImGui::SameLine();

                if (ImGui::Button("Install ComfyUI"))
                {
                    InstallComfyUI();//inputOptions[1].value);
                }

                ImGui::SameLine();

                if (ImGui::Button("Save Configuration"))
                {
                    SaveOptionsToFile("comfyui_options.json");
                }

                ImGui::NewLine();

                if (ImGui::BeginTabBar("ComfyUI Startup Options"))
                {
                    // "General" tab
                    if (ImGui::BeginTabItem("General"))
                    {
                        

                        ShowBoolOptionsTable(boolOptions, sizeof(boolOptions) / sizeof(BoolOption), "General Settings Table");
                        ImGui::EndTabItem();
                    }

                    // "Paths" tab
                    if (ImGui::BeginTabItem("Paths"))
                    {
                        ShowFlagPathsTable(inputOptions, sizeof(inputOptions) / sizeof(FlagOption), "PathsTable");
                        ImGui::EndTabItem();
                    }
                    ImGui::EndTabBar();
                }
                ImGui::EndTabItem();
            }

            // SDCPP Settings
            if (ImGui::BeginTabItem("SDCPP Startup Options"))
            {
                ImGui::NewLine();

                if (ImGui::Button("Save Configuration")) {
                    SaveOptionsToFile("sdcpp_options.json");
                }

                ImGui::NewLine();

                if (ImGui::BeginTabBar("SDCPP Startup Options"))
                {
                    // "General" tab
                    if (ImGui::BeginTabItem("General"))
                    {
                        // Optionally add content here
                        ImGui::EndTabItem();
                    }

                    // "Paths" tab
                    if (ImGui::BeginTabItem("Paths"))
                    {
                        // Optionally add content here
                        ImGui::EndTabItem();
                    }
                    ImGui::EndTabBar();
                }
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        ImGui::End();
    }
}

void GuiSettings::InstallVenv(const std::string& venvPath)
{
    // Check if the virtual environment directory already exists
    if (std::filesystem::exists(venvPath)) {
        std::cerr << "Virtual environment already exists at " << venvPath << std::endl;
        return;
    }

    // Command to create a virtual environment
    std::string createVenvCmd = "python -m venv \"" + venvPath + "\"";
    if (std::system(createVenvCmd.c_str()) != 0) {
        std::cerr << "Failed to create virtual environment at " << venvPath << std::endl;
        return;
    }

    // Construct paths for the pip executable and other relevant scripts
    std::string pipExecutable;

#ifdef _WIN32
    pipExecutable = venvPath + "\\Scripts\\pip.exe";
#else
    pipExecutable = venvPath + "/bin/pip";
#endif

    // Check if comfy-cli is already installed
    std::string checkComfyCliCmd = pipExecutable + " show comfy-cli";
    if (std::system(checkComfyCliCmd.c_str()) == 0) {
        std::cerr << "comfy-cli is already installed in the virtual environment at " << venvPath << std::endl;
        return;
    }

    // Command to install comfy-cli within the virtual environment
    std::string installCmd = pipExecutable + " install comfy-cli";
    if (std::system(installCmd.c_str()) != 0) {
        std::cerr << "Failed to install comfy-cli in the virtual environment at " << venvPath << std::endl;
        return;
    }

    std::cout << "comfy-cli successfully installed in the virtual environment at " << venvPath << std::endl;
}

void GuiSettings::InstallComfyUI()
{
    std::system("comfy install");
}

// Function to display the flag options table with checkboxes and tooltips
void GuiSettings::ShowBoolOptionsTable(BoolOption* options, int count, const char* tableTitle)
{
    int columns = 3; // Number of columns for boolean options

    if (ImGui::BeginTable(tableTitle, columns, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
    {
        ImGui::TableSetupColumn("Flag");
        ImGui::TableSetupColumn("Flag");
        ImGui::TableSetupColumn("Flag");
        ImGui::TableHeadersRow();

        int currentColumn = 0;
        for (int i = 0; i < count; i++)
        {
            if (currentColumn == 0)
                ImGui::TableNextRow();

            ImGui::TableNextColumn();
            bool* checkboxValue = &options[i].enabled;
            ImGui::Checkbox(options[i].flag, checkboxValue);
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::TextUnformatted(options[i].description);
                ImGui::EndTooltip();
            }

            currentColumn = (currentColumn + 1) % columns;
        }

        ImGui::EndTable();
    }
};

void GuiSettings::ShowFlagPathsTable(FlagOption* options, int count, const char* tableTitle)
{
    static int selectedOptionIndex = -1; // Store the index of the currently active dialog

    if (ImGui::BeginTable(tableTitle, 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
    {
        // Set column headers
        ImGui::TableSetupColumn("Flag");
        ImGui::TableSetupColumn("Path");
        ImGui::TableHeadersRow();

        // Populate options
        for (int i = 0; i < count; i++)
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();

            ImGui::TextUnformatted(options[i].flag); // Display flag name

            ImGui::TableNextColumn();

            // Display a button to open the directory dialog
            if (ImGui::Button(("...##" + std::to_string(i)).c_str())) {
                selectedOptionIndex = i; // Track which button was pressed
                IGFD::FileDialogConfig config;
                config.path = "../../.."; // Starting path
                ImGuiFileDialog::Instance()->OpenDialog("ChooseDirDlgKey", "Choose Directory", nullptr, config);
            }

            ImGui::SameLine();

            // Display the text input for the path
            ImGui::InputText(("##PathInput" + std::to_string(i)).c_str(), options[i].value, 255);
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::TextUnformatted(options[i].description);
                ImGui::EndTooltip();
            }
        }

        ImGui::EndTable();
    }

    // Handle the directory dialog
    if (ImGuiFileDialog::Instance()->Display("ChooseDirDlgKey")) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string selectedPath = ImGuiFileDialog::Instance()->GetCurrentPath();
            if (selectedOptionIndex >= 0 && selectedOptionIndex < count) {
                strncpy(options[selectedOptionIndex].value, selectedPath.c_str(), 255);
                options[selectedOptionIndex].value[254] = '\0'; // Ensure null termination
            }
        }
        ImGuiFileDialog::Instance()->Close();
    }
}


json GuiSettings::SerializeOptions() {
    json j;
    j["flags"] = json::array();
    j["booleans"] = json::array();

    for (const auto& opt : inputOptions) {
        json option;
        option["flag"] = opt.flag;
        option["description"] = opt.description;
        option["value"] = opt.value;
        j["flags"].push_back(option);
    }

    for (const auto& opt : boolOptions) {
        json option;
        option["flag"] = opt.flag;
        option["description"] = opt.description;
        option["enabled"] = opt.enabled;
        j["booleans"].push_back(option);
    }

    return j;
}

void GuiSettings::SaveOptionsToFile(const std::string& filename) {
    std::filesystem::path dir = "configs";
    if (!std::filesystem::exists(dir)) {
        std::filesystem::create_directory(dir);
    }

    json j = SerializeOptions();
    std::ofstream file(dir / filename);
    if (file.is_open()) {
        file << j.dump(4);  // Pretty print with 4 spaces
        file.close();
    }
}