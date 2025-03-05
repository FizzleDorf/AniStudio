#include <iostream>
#include <fstream>
#include <filesystem>

#include <ImJSchema/ImJSchema.h>

#include <SDL.h>
#include <imgui.h>

#if !SDL_VERSION_ATLEAST(2,0,17)
#error This backend requires SDL 2.0.17+ because of SDL_RenderGeometry() function
#endif

#include <../res/bindings/imgui_impl_sdl2.h>
#include <../res/bindings/imgui_impl_sdlrenderer2.h>

namespace IJS = ImJSchema;



bool BeginFullScreen(const char* name, bool* p_open = nullptr, ImGuiWindowFlags _flags = 0)
{
    static bool use_work_area = true;
    static ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | _flags;

    // We demonstrate using the full viewport area or the work area (without menu-bars, task-bars etc.)
    // Based on your use case you may want one or the other.
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos( use_work_area ? viewport->WorkPos : viewport->Pos);
    ImGui::SetNextWindowSize(use_work_area ? viewport->WorkSize : viewport->Size);

    return ImGui::Begin(name, p_open, flags);
}

IJS::json _value;
IJS::json _cache;
IJS::json _schema = IJS::json::parse(
    R"foo({
    "$defs": {
        "normalized_number": {
            "type" : "number",
            "maximum": 1.0,
            "minimum": 0.0,
            "default": 0.0,
            "ui:widget" : "slider"
        }
    },
    "ui:order" : ["albedo", "metallic", "roughness", "emissive", "emissiveFactor", "unlit"],
    "description" : "Execute using \n\n   schenity schema.json",
    "properties" : {
        "metallic"  : { "$ref": "#/$defs/normalized_number" },
        "roughness" : { "$ref": "#/$defs/normalized_number" },
        "albedo"    : { "type" : "string", "ui:widget" : "color" },
        "emissive"  : { "type" : "string", "ui:widget" : "color" },
        "emissiveFactor" : {
            "type" : "number",
            "minimum" : 0.0,
            "ui:speed" : 0.01,
            "ui:widget" : "drag"
        },
        "unlit" : {
            "type" : "boolean"
        }
    },
    "type": "object"
})foo"
);

int runApp()
{
    BeginFullScreen("Object");

    int ret = 255;

    auto width = ImGui::GetContentRegionAvail().x / 2;

    ImVec2 ButtonSize = {width,100};


    if(IJS::drawSchemaWidget("object",
                              _value,
                              _schema,
                              _cache))
    {
        // dont do anything when the user
        // changes values
    }
    auto height = ImGui::GetContentRegionAvail().y;
    ImGui::Dummy({5,height-ButtonSize.y});

    if( ImGui::Button("Cancel", ButtonSize) )
    {
        ret = 1;
    }
    ImGui::SameLine();
    if( ImGui::Button("Confirm", ButtonSize) )
    {
        std::cout << _value.dump();
        ret = 0;
    }
    ImGui::End();

    return ret;
}



int main(int argc, char** argv)
{
    if(argc==2)
    {
        std::string inputFile= argv[1];
        if( std::filesystem::exists(inputFile))
        {
            std::ifstream in(inputFile);
            auto cpy = _schema;

            try
            {
                in >> _schema;
            } catch( std::exception & E)
            {
                _schema = cpy;
                _schema["description"] = std::string("Error parsing json:  ") + E.what();
            }
        }
    }
    // we need to make sure we expand all the references
    // otherwise it will throw an error
    // when it tries to read
    IJS::jsonExpandAllReferences(_schema);


    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

// From 2.0.18: Enable native IME.
#ifdef SDL_HINT_IME_SHOW_UI
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

    int windowWidth  = 64*8;
    int windowHeight = 64*10;

    // Create window with SDL_Renderer graphics context
    SDL_WindowFlags window_flags = static_cast<SDL_WindowFlags>(SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI );
    SDL_Window* window = SDL_CreateWindow("schenity: Like Zenity, but with Schemas", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, windowWidth, windowHeight, window_flags);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);

    if (renderer == nullptr)
    {
        SDL_Log("Error creating SDL_Renderer!");
        return 0;
    }
    //SDL_RendererInfo info;
    //SDL_GetRendererInfo(renderer, &info);
    //SDL_Log("Current SDL_Renderer: %s", info.name);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != nullptr);

    // Our state
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);


    // Main loop
    bool done = false;
    int returnValue=0;

    while (!done)
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }

        // Start the Dear ImGui frame
        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        returnValue = runApp();
        if(returnValue != 255)
            done = true;

        // Rendering
        ImGui::Render();
        SDL_RenderSetScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
        SDL_SetRenderDrawColor(renderer, static_cast<Uint8>(clear_color.x * 255), static_cast<Uint8>(clear_color.y * 255), static_cast<Uint8>(clear_color.z * 255), static_cast<Uint8>(clear_color.w * 255));
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData());
        SDL_RenderPresent(renderer);
    }
    // Cleanup
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return returnValue;
}

