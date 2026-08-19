#include "OpenGLWrapper.hpp"
#include "Core.hpp"
#include "Events.hpp"
#include "guiSystems.h"
#include "DragDropUtils.hpp"
#include <iostream>
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
        std::cout << "[Core] Constructor called" << '\n';
    }

    Core::~Core() {
        std::cout << "[Core] Destructor - calling StudioCore shutdown..." << '\n';
        try {
            m_studioCore.Shutdown();
        }
        catch (const std::exception& e) {
            std::cerr << "[Core] Exception during StudioCore shutdown: " << e.what() << '\n';
        }
        CleanupWindow();
    }

    void Core::Quit() {
        std::cout << "[Core] Quit called - setting run to false" << '\n';
        m_isRunning = false;
        m_studioCore.SetRunning(false);
    }

    void Core::Init() {
        std::cout << "[Core] Initializing..." << '\n';

        if (!InitializeWindow()) {
            throw std::runtime_error("Failed to initialize window");
        }
        std::cout << "[Core] Window and ImGui fully initialized" << '\n';

        if (!m_studioCore.Initialize()) {
            throw std::runtime_error("Failed to initialize StudioCore");
        }
        std::cout << "[Core] StudioCore basic initialization complete" << '\n';

        // Pass the GLFW window pointer to StudioCore
        m_studioCore.SetWindowHandle(m_window);

        m_studioCore.SetImGuiContext(GetImGuiContext());
        std::cout << "[Core] Window handle and ImGui context set" << '\n';

        std::cout << "[Core] Completing StudioCore initialization..." << '\n';
        m_studioCore.CompleteInitialization();
        std::cout << "[Core] StudioCore fully initialized" << '\n';

        std::cout << "[Core] Registering event handlers..." << '\n';
        RegisterEventHandlers();
        std::cout << "[Core] Event handlers registered" << '\n';

        std::cout << "[Core] Initialization complete!" << '\n';
    }

    void Core::RegisterEventHandlers() {
        // ... (unchanged, as in previous versions)
    }

    bool Core::InitializeWindow() {
        std::cout << "[Core] Initializing GLFW..." << '\n';
        if (!glfwInit()) {
            std::cerr << "[Core] Failed to initialize GLFW" << '\n';
            return false;
        }
        std::cout << "[Core] GLFW initialized successfully" << '\n';

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

        std::cout << "[Core] Creating window..." << '\n';
        m_window = glfwCreateWindow(m_videoWidth, m_videoHeight, "AniStudio", nullptr, nullptr);
#ifdef _WIN32
        HWND testHwnd = glfwGetWin32Window(m_window);
        std::cout << "[Core] Immediate HWND check: " << testHwnd << '\n';
#endif
        
        if (!m_window) {
            std::cerr << "[Core] Failed to create GLFW window" << '\n';
            glfwTerminate();
            return false;
        }
        std::cout << "[Core] Window created successfully, pointer: " << m_window << '\n';

        glfwMakeContextCurrent(m_window);
        glfwSetWindowCloseCallback(m_window, WindowCloseCallback);
        glfwSwapInterval(1);
        std::cout << "[Core] Window context set" << '\n';

        std::cout << "[Core] Initializing GLEW..." << '\n';
        GLenum err = glewInit();
        if (err != GLEW_OK) {
            std::cerr << "[Core] Failed to initialize GLEW: " << glewGetErrorString(err) << '\n';
            return false;
        }
        std::cout << "[Core] GLEW initialized successfully" << '\n';

        glViewport(0, 0, m_videoWidth, m_videoHeight);
        std::cout << "[Core] Viewport set" << '\n';

        std::cout << "[Core] Calling IMGUI_CHECKVERSION()..." << '\n';
        IMGUI_CHECKVERSION();
        std::cout << "[Core] Version check passed" << '\n';

        std::cout << "[Core] Calling ImGui::CreateContext()..." << '\n';
        ImGuiContext* ctx = ImGui::CreateContext();
        std::cout << "[Core] ImGui context created: " << ctx << '\n';

        if (!ctx) {
            std::cerr << "[Core] ERROR: ImGui context is NULL!" << '\n';
            return false;
        }

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        std::cout << "[Core] Enabled docking by default" << '\n';

        std::cout << "[Core] Setting temporary INI file path..." << '\n';
        std::string iniFilePath = "imgui.ini";

        try {
            std::filesystem::path iniDir = std::filesystem::path(iniFilePath).parent_path();
            if (!iniDir.empty() && !std::filesystem::exists(iniDir)) {
                std::filesystem::create_directories(iniDir);
                std::cout << "[Core] Created directory for temporary INI file" << '\n';
            }
        }
        catch (const std::exception& e) {
            std::cerr << "[Core] Warning: Could not create INI directory: " << e.what() << '\n';
        }

        static std::string tempIniPath = iniFilePath;
        io.IniFilename = tempIniPath.c_str();
        std::cout << "[Core] Temporary INI file path set to: " << io.IniFilename << '\n';

        std::cout << "[Core] Initializing GLFW backend..." << '\n';
        bool glfwOk = ImGui_ImplGlfw_InitForOpenGL(m_window, true);
        std::cout << "[Core] GLFW backend result: " << (glfwOk ? "SUCCESS" : "FAILED") << '\n';
        if (!glfwOk) return false;

        std::cout << "[Core] Initializing OpenGL3 backend..." << '\n';
        bool gl3Ok = ImGui_ImplOpenGL3_Init("#version 330");
        std::cout << "[Core] OpenGL3 backend result: " << (gl3Ok ? "SUCCESS" : "FAILED") << '\n';
        if (!gl3Ok) return false;

        std::cout << "[Core] Adding default font..." << '\n';
        if (io.Fonts->Fonts.Size == 0) {
            io.Fonts->AddFontDefault();
            std::cout << "[Core] Default font added" << '\n';
        }
        std::cout << "[Core] Font count: " << io.Fonts->Fonts.Size << '\n';

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
            }
            else {
                std::cerr << "[Core] Failed to load window icon: " << stbi_failure_reason() << std::endl;
            }
        }

        std::cout << "[Core] Window initialization COMPLETE" << '\n';
        return true;
    }

    void Core::CleanupWindow() {
        if (m_window) {
            std::cout << "[Core] Cleaning up ImGui and GLFW..." << '\n';
            try {
                ImGui_ImplOpenGL3_Shutdown();
                ImGui_ImplGlfw_Shutdown();
                ImGui::DestroyContext();
                glfwDestroyWindow(m_window);
                m_window = nullptr;
            }
            catch (const std::exception& e) {
                std::cerr << "[Core] Exception during window cleanup: " << e.what() << '\n';
            }
        }
        glfwTerminate();
        std::cout << "[Core] Window cleanup complete" << '\n';
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
            std::cerr << "[Core] ERROR: OpenGL context lost before update!" << '\n';
            return;
        }

        try {
            m_studioCore.Update(deltaT);
        }
        catch (const std::exception& e) {
            std::cerr << "[Core] Update error: " << e.what() << '\n';
        }
    }

    void Core::Draw() {
        if (!m_isRunning) return;

        try {
            glfwPollEvents();

            glfwMakeContextCurrent(m_window);

            if (!ANI::OpenGLContextHelper::VerifyContext()) {
                std::cerr << "[Core] ERROR: OpenGL context lost before render!" << '\n';
                return;
            }

#ifdef _WIN32
            {
                auto& entityMgr = m_studioCore.GetEntityManager();
                auto textureSystem = entityMgr.GetSystem<ECS::TextureSystem>();
                if (textureSystem && textureSystem->HasPendingTextures()) {
                    textureSystem->CreatePendingTextures();
                }
            }
#endif

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
            std::cerr << "[Core] Render error: " << e.what() << '\n';
        }

        glfwSwapBuffers(m_window);
    }

} // namespace ANI