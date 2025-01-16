#include "Engine.hpp"
#include "guis.h"
#include <filesystem>
#include <iostream>

using namespace ECS;
using namespace GUI;
using namespace Plugin;

namespace ANI {

static std::string iniFilePath;

Engine &Core = Engine::Ref();

void WindowCloseCallback(GLFWwindow *window) { Core.Quit(); }

Engine::Engine()
    : run(true), window(nullptr), videoWidth(SCREEN_WIDTH), videoHeight(SCREEN_HEIGHT), fpsSum(0.0), frameCount(0),
      timeElapsed(0.0) {}

Engine::~Engine() {
    // std::string relativePath = filePaths.dataPath + "/imgui.ini";
    // std::string iniFilePath = std::filesystem::absolute(relativePath).string();
    // ImGui::SaveIniSettingsToDisk(iniFilePath.c_str());
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
}

void Engine::Init() {
    filePaths.LoadFilePathDefaults();
    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    window = glfwCreateWindow(videoWidth, videoHeight, "AniStudio", nullptr, nullptr);
    if (!window) {
        throw std::runtime_error("Failed to create GLFW window");
    }

    glfwMakeContextCurrent(window);
    glfwSetWindowCloseCallback(window, WindowCloseCallback);
    glfwSwapInterval(1); // Enable vsync

    if (glewInit() != GLEW_OK) {
        throw std::runtime_error("Failed to initialize GLEW");
    }
    glViewport(0, 0, videoWidth, videoHeight);
    
    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    iniFilePath = std::filesystem::absolute(filePaths.ImguiStatePath).string();
    // ImGui::LoadIniSettingsFromDisk(iniFilePath.c_str());

    ImGuiIO &io = ImGui::GetIO();
    io.IniFilename = iniFilePath.c_str();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    ImGui::StyleColorsDark();
    ImGuiStyle &style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
    
    

    // Initialize core systems
    filePaths.Init();
    entityManager.Reset();
    viewManager.Reset();
    viewManager.Init();
    pluginManager.Init();

    // Register core systems
    entityManager.RegisterSystem<ECS::SDCPPSystem>();
    entityManager.RegisterSystem<ECS::ImageSystem>();

    // Create core views
    auto debugViewID = viewManager.CreateView();
    viewManager.AddView<DebugView>(debugViewID, DebugView(entityManager));

    auto diffusionViewID = viewManager.CreateView();
    viewManager.AddView<DiffusionView>(diffusionViewID, DiffusionView(entityManager));

    auto imageViewID = viewManager.CreateView();
    viewManager.AddView<ImageView>(imageViewID, ImageView(entityManager));

    // auto nodeGraphViewID = viewManager.CreateView();
    // viewManager.AddView<NodeGraphView>(nodeGraphViewID, NodeGraphView(entityManager));

    /*auto pluginViewID = viewManager.CreateView();
    viewManager.AddView<PluginView>(pluginViewID, PluginView(entityManager, pluginManager));*/

    //// Initialize views
    viewManager.GetView<DebugView>(debugViewID).Init();
    viewManager.GetView<DiffusionView>(diffusionViewID).Init();
    viewManager.GetView<ImageView>(imageViewID).Init();
    // viewManager.GetView<NodeGraphView>(nodeGraphViewID).Init();
    //viewManager.GetView<PluginView>(pluginViewID).Init();


}

void Engine::Update(const float deltaT) {
    // fps display
    timeElapsed += deltaT;
    frameCount++;
    if (timeElapsed >= 1.0) {
        double fps = frameCount / timeElapsed;
        std::ostringstream titleStream;
        titleStream << "AniStudio - FPS: " << static_cast<int>(fps);
        glfwSetWindowTitle(window, titleStream.str().c_str());
        frameCount = 0;
        timeElapsed = 0.0;
    }
    
    // Update managers
    entityManager.Update(deltaT);
    pluginManager.Update(deltaT);
}

void Engine::Draw() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::DockSpaceOverViewport(ImGui::GetMainViewport()->ID);

    // Render Views before ImGui's Render
    ShowMenuBar(window);
    viewManager.Render();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }

    glfwSwapBuffers(window);
}

void Engine::Quit() { run = false; }

} // namespace ANI