#include "Engine.hpp"

using namespace ECS;

namespace ANI {

Engine &Core = Engine::Ref();

void WindowCloseCallback(GLFWwindow *window) { Core.Quit(); }

Engine::Engine()
    : mgr(EntityManager::Ref()), run(true), window(nullptr), videoWidth(SCREEN_WIDTH),
      videoHeight(SCREEN_HEIGHT) {
    mgr.Reset();
    vMgr = std::make_unique<ViewManager>();   
}

Engine::~Engine() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
}

void Engine::Init() {
    mgr.Reset();
    vMgr->Init(mgr);

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
}

void Engine::Update(const float deltaT) {
    if (deltaT > 0) {
        frameCount++;
        fpsSum += 1.0 / deltaT;

        if (frameCount >= 60) { // Update every 60 frames
            int averageFPS = static_cast<int>(fpsSum / frameCount);

            std::ostringstream titleStream;
            titleStream << "AniStudio - FPS: " << averageFPS;

            glfwSetWindowTitle(window, titleStream.str().c_str());

            frameCount = 0;
            fpsSum = 0.0;
        }
    }
    mgr.Update();
    vMgr->Update();
}

void Engine::Draw() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::DockSpaceOverViewport(ImGui::GetMainViewport()->ID);

    ShowMenuBar(window);
    vMgr->Render();

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
