#include "Core.hpp"
#include "Events.hpp"
#include "AniStudioSystems.hpp"
#include "Log.hpp"

#include <sstream>
#include <chrono>
#include <filesystem>

namespace ANI {

    void WindowCloseCallback(GLFWwindow* window) {
        Core::Ref().Quit();
    }

    Core::Core() : m_isRunning(true), m_window(nullptr),
        m_videoWidth(SCREEN_WIDTH), m_videoHeight(SCREEN_HEIGHT),
        m_fpsSum(0.0), m_frameCount(0), m_timeElapsed(0.0) {
        ANI_LOG_DEBUG("Core constructed");
    }

    Core::~Core() {
        ANI_LOG_INFO("Core destructor - shutting down StudioCore");
        try {
            m_studioCore.Shutdown();
        }
        catch (const std::exception& e) {
            ANI_LOG_ERROR("Exception during StudioCore shutdown: %s", e.what());
        }
        CleanupWindow();
    }

    void Core::Quit() {
        ANI_LOG_INFO("Quit requested");
        m_isRunning = false;
        m_studioCore.SetRunning(false);
    }

    void Core::Init() {
        ANI_LOG_INFO("Initializing Core");

        if (!InitializeWindow()) {
            throw std::runtime_error("Failed to initialize window");
        }
        ANI_LOG_INFO("Window and ImGui fully initialized");

        m_studioCore.SetImGuiContext(GetImGuiContext());
        ANI_LOG_DEBUG("ImGui context set");

        if (!m_studioCore.Initialize()) {
            throw std::runtime_error("Failed to initialize StudioCore");
        }
        ANI_LOG_INFO("StudioCore fully initialized");

        m_studioCore.SetWindowHandle(m_window);
        ANI_LOG_DEBUG("Window handle set");

        RegisterEventHandlers();
        ANI_LOG_INFO("Core initialization complete");
    }

    void Core::RegisterEventHandlers() {
        Events::Ref().RegisterEvent("Quit", [this]() {
            ANI_LOG_DEBUG("Quit event triggered");
            this->Quit();
            });

        ANI_LOG_DEBUG("All event handlers registered");
    }

    bool Core::InitializeWindow() {
        ANI_LOG_DEBUG("Initializing GLFW");
        if (!glfwInit()) {
            ANI_LOG_ERROR("Failed to initialize GLFW");
            return false;
        }
        ANI_LOG_DEBUG("GLFW initialized");

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

        ANI_LOG_DEBUG("Creating window (%dx%d)", m_videoWidth, m_videoHeight);
        m_window = glfwCreateWindow(m_videoWidth, m_videoHeight, "AniStudio", nullptr, nullptr);
#ifdef _WIN32
        HWND testHwnd = glfwGetWin32Window(m_window);
        ANI_LOG_TRACE("Immediate HWND check: %p", static_cast<void*>(testHwnd));
#endif

        if (!m_window) {
            ANI_LOG_ERROR("Failed to create GLFW window");
            glfwTerminate();
            return false;
        }
        ANI_LOG_DEBUG("Window created successfully (ptr=%p)", static_cast<void*>(m_window));

        glfwMakeContextCurrent(m_window);
        glfwSetWindowCloseCallback(m_window, WindowCloseCallback);
        glfwSwapInterval(1);
        ANI_LOG_TRACE("Window context set");

        ANI_LOG_DEBUG("Initializing GLEW");
        GLenum err = glewInit();
        if (err != GLEW_OK) {
            ANI_LOG_ERROR("Failed to initialize GLEW: %s", glewGetErrorString(err));
            return false;
        }
        ANI_LOG_DEBUG("GLEW initialized");

        glViewport(0, 0, m_videoWidth, m_videoHeight);
        ANI_LOG_TRACE("Viewport set");

        ANI_LOG_TRACE("IMGUI_CHECKVERSION()");
        IMGUI_CHECKVERSION();

        ANI_LOG_DEBUG("Creating ImGui context");
        ImGuiContext* ctx = ImGui::CreateContext();
        ANI_LOG_DEBUG("ImGui context created: %p", static_cast<void*>(ctx));

        if (!ctx) {
            ANI_LOG_ERROR("ImGui context is null");
            return false;
        }

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        ANI_LOG_TRACE("Enabled docking by default");

        std::string iniFilePath = "imgui.ini";

        try {
            std::filesystem::path iniDir = std::filesystem::path(iniFilePath).parent_path();
            if (!iniDir.empty() && !std::filesystem::exists(iniDir)) {
                std::filesystem::create_directories(iniDir);
                ANI_LOG_TRACE("Created directory for INI file: %s", iniDir.string().c_str());
            }
        }
        catch (const std::exception& e) {
            ANI_LOG_WARN("Could not create INI directory: %s", e.what());
        }

        static std::string tempIniPath = iniFilePath;
        io.IniFilename = tempIniPath.c_str();
        ANI_LOG_TRACE("INI file path set to: %s", io.IniFilename);

        ANI_LOG_DEBUG("Initializing GLFW backend");
        bool glfwOk = ImGui_ImplGlfw_InitForOpenGL(m_window, true);
        if (!glfwOk) {
            ANI_LOG_ERROR("ImGui GLFW backend initialization failed");
            return false;
        }
        ANI_LOG_DEBUG("GLFW backend initialized");

        ANI_LOG_DEBUG("Initializing OpenGL3 backend");
        bool gl3Ok = ImGui_ImplOpenGL3_Init("#version 330");
        if (!gl3Ok) {
            ANI_LOG_ERROR("ImGui OpenGL3 backend initialization failed");
            return false;
        }
        ANI_LOG_DEBUG("OpenGL3 backend initialized");

        if (io.Fonts->Fonts.Size == 0) {
            io.Fonts->AddFontDefault();
            ANI_LOG_TRACE("Default font added");
        }
        ANI_LOG_TRACE("Font count: %d", io.Fonts->Fonts.Size);

        const char* iconPath = "assets/favicom.jpg";
        if (std::filesystem::exists(iconPath)) {
            int width, height, channels;
            unsigned char* data = stbi_load(iconPath, &width, &height, &channels, 4);
            if (data) {
                GLFWimage icon;
                icon.width = width;
                icon.height = height;
                icon.pixels = data;
                glfwSetWindowIcon(m_window, 1, &icon);
                stbi_image_free(data);
                ANI_LOG_TRACE("Window icon set from %s", iconPath);
            }
            else {
                ANI_LOG_WARN("Failed to load window icon '%s': %s",
                    iconPath, stbi_failure_reason());
            }
        }

        ANI_LOG_INFO("Window initialization complete");
        return true;
    }

    void Core::CleanupWindow() {
        if (m_window) {
            ANI_LOG_DEBUG("Cleaning up ImGui and GLFW");
            try {
                ImGui_ImplOpenGL3_Shutdown();
                ImGui_ImplGlfw_Shutdown();
                ImGui::DestroyContext();
                glfwDestroyWindow(m_window);
                m_window = nullptr;
            }
            catch (const std::exception& e) {
                ANI_LOG_ERROR("Exception during window cleanup: %s", e.what());
            }
        }
        glfwTerminate();
        ANI_LOG_DEBUG("Window cleanup complete");
    }

    void Core::Update(const float deltaT) {
        if (!m_isRunning) return;

        m_timeElapsed += deltaT;
        m_frameCount++;
        if (m_timeElapsed >= 1.0) {
            double fps = m_frameCount / m_timeElapsed;
            std::ostringstream titleStream;
            titleStream << "AniStudio - FPS: " << static_cast<int>(fps);
            glfwSetWindowTitle(m_window, titleStream.str().c_str());
            m_frameCount = 0;
            m_timeElapsed = 0.0;
        }

        glfwMakeContextCurrent(m_window);

        if (!ANI::OpenGLContextHelper::VerifyContext()) {
            ANI_LOG_ERROR("OpenGL context lost before update");
            return;
        }

        try {
            m_studioCore.Update(deltaT);
        }
        catch (const std::exception& e) {
            ANI_LOG_ERROR("Update error: %s", e.what());
        }
    }

    void Core::Draw() {
        if (!m_isRunning) return;

        try {
            glfwPollEvents();

            glfwMakeContextCurrent(m_window);

            if (!ANI::OpenGLContextHelper::VerifyContext()) {
                ANI_LOG_ERROR("OpenGL context lost before render");
                return;
            }

            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            m_studioCore.Render();

            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            ImGuiIO& io = ImGui::GetIO();
            if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
                GLFWwindow* backup_current_context = glfwGetCurrentContext();
                ImGui::UpdatePlatformWindows();
                ImGui::RenderPlatformWindowsDefault();
                glfwMakeContextCurrent(backup_current_context);
            }
        }
        catch (const std::exception& e) {
            ANI_LOG_ERROR("Render error: %s", e.what());
        }

        glfwSwapBuffers(m_window);
    }

} // namespace ANI