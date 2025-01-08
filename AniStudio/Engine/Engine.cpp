#include "Engine.hpp"

using namespace ECS;
using namespace GUI;

namespace ANI {

Engine &Core = Engine::Ref();

void WindowCloseCallback(GLFWwindow *window) { Core.Quit(); }

Engine::Engine() : run(true), window(nullptr), videoWidth(SCREEN_WIDTH), videoHeight(SCREEN_HEIGHT) {}

Engine::~Engine() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
}

void Engine::Init() {
    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // For macOS

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

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
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
    
    filePaths.Init();
    mgr.Reset();
    viewMgr.Reset();

    mgr.RegisterSystem<SDCPPSystem>();
    mgr.RegisterSystem<ImageSystem>();

    viewMgr.Init();

    // Create core views
    ViewID debugViewID = viewMgr.AddNewView();
    viewMgr.AddView<DebugView>(debugViewID);

    ViewID diffusionViewID = viewMgr.AddNewView();
    viewMgr.AddView<DiffusionView>(diffusionViewID);

    ViewID imageViewID = viewMgr.AddNewView();
    viewMgr.AddView<ImageView>(imageViewID);


    ViewID pluginViewID = viewMgr.AddNewView();
    viewMgr.AddView<PluginView>(pluginViewID);

    // Initialize the views
    viewMgr.GetView<DebugView>(debugViewID).Init();
    viewMgr.GetView<DiffusionView>(diffusionViewID).Init();
    viewMgr.GetView<ImageView>(imageViewID).Init();
    viewMgr.GetView<PluginView>(pluginViewID).Init();
}

void Engine::Update(const float deltaT) {
    timeElapsed += deltaT;
    frameCount++;
    if (timeElapsed >= 1.0) {
        double fps = frameCount / timeElapsed;

        // Update the window title
        std::ostringstream titleStream;
        titleStream << "AniStudio - FPS: " << static_cast<int>(fps);
        glfwSetWindowTitle(window, titleStream.str().c_str());

        // Reset for the next second
        frameCount = 0;
        timeElapsed = 0.0;
    }
    mgr.Update(deltaT);
}

void Engine::Draw() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::DockSpaceOverViewport(ImGui::GetMainViewport()->ID);

    ShowMenuBar(window);
    viewMgr.Render();

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
