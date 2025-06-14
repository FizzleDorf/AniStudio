#include "Engine.hpp"
#include "PluginManager.hpp"
#include "PluginRegistry.hpp"
#include "PluginInterface.hpp"
#include "PluginAPI.hpp"
#include <filesystem>
#include <iostream>

using namespace ECS;
using namespace GUI;
using namespace Plugin;

namespace ANI {
    static std::string iniFilePath;
    Engine &Core = Engine::Ref();
void WindowCloseCallback(GLFWwindow *window) { Core.Quit(); }

Engine::Engine() : run(true), window(nullptr),
videoWidth(SCREEN_WIDTH), videoHeight(SCREEN_HEIGHT),
fpsSum(0.0), frameCount(0), timeElapsed(0.0) {
}

Engine::~Engine() {
    // std::string relativePath = Utils::FilePaths::dataPath + "/imgui.ini";
    // std::string iniFilePath = std::filesystem::absolute(relativePath).string();
    // ImGui::SaveIniSettingsToDisk(iniFilePath.c_str());
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
}

void Engine::Quit() { run = false; }

void Engine::Init() {
	Utils::FilePaths::LoadFilePathDefaults();
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
	glfwSwapInterval(1);

	if (glewInit() != GLEW_OK) {
		throw std::runtime_error("Failed to initialize GLEW");
	}
	glViewport(0, 0, videoWidth, videoHeight);

	// Initialize ImGui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	iniFilePath = std::filesystem::absolute(Utils::FilePaths::ImguiStatePath).string();

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

	LoadStyleFromFile(style, "../data/defaults/style.json");

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 330");

	// Initialize FilePaths
	Utils::FilePaths::Init();

	// Invalidate ID 0 for all entities and viewlists
	const EntityID temp = mgr.AddNewEntity();
	mgr.DestroyEntity(temp);
	auto tempView = viewManager.CreateView();
	viewManager.DestroyView(tempView);

	// Register Views
	viewManager.RegisterViewType<DebugView>("DebugView");
	viewManager.RegisterViewType<SettingsView>("SettingsView");
	viewManager.RegisterViewType<DiffusionView>("DiffusionView");
	viewManager.RegisterViewType<ImageView>("ImageView");
	viewManager.RegisterViewType<NodeGraphView>("NodeGraphView");
	viewManager.RegisterViewType<ConvertView>("ConvertView");
	viewManager.RegisterViewType<ViewListManagerView>("ViewListManagerView");
	viewManager.RegisterViewType<SequencerView>("SequencerView");
	viewManager.RegisterViewType<PluginView>("PluginView");
	viewManager.RegisterViewType<NodeView>("NodeView");
	viewManager.RegisterViewType<UpscaleView>("UpscaleView");
	viewManager.RegisterViewType<VideoView>("VideoView");
	viewManager.RegisterViewType<VideoView>("VideoSequencerView");
	viewManager.RegisterViewType<ZepView>("ZepView");

	// Register Component Names
	mgr.RegisterComponentName<ModelComponent>("Model");
	mgr.RegisterComponentName<ClipLComponent>("ClipL");
	mgr.RegisterComponentName<ClipGComponent>("ClipG");
	mgr.RegisterComponentName<T5XXLComponent>("T5XXL");
	mgr.RegisterComponentName<DiffusionModelComponent>("DiffusionModel");
	mgr.RegisterComponentName<LatentComponent>("Latent");
	mgr.RegisterComponentName<LoraComponent>("Lora");
	mgr.RegisterComponentName<PromptComponent>("Prompt");
	mgr.RegisterComponentName<SamplerComponent>("Sampler");
	mgr.RegisterComponentName<GuidanceComponent>("Guidance");
	mgr.RegisterComponentName<EsrganComponent>("Esrgan");
	mgr.RegisterComponentName<ClipSkipComponent>("ClipSkip");
	mgr.RegisterComponentName<VaeComponent>("Vae");
	mgr.RegisterComponentName<ImageComponent>("Image");
	mgr.RegisterComponentName<InputImageComponent>("InputImage");
	mgr.RegisterComponentName<OutputImageComponent>("OutputImage");
	mgr.RegisterComponentName<EmbeddingComponent>("Embedding");
	mgr.RegisterComponentName<ControlnetComponent>("Controlnet");
	mgr.RegisterComponentName<LayerSkipComponent>("LayerSkip");
	mgr.RegisterComponentName<VideoComponent>("Video");
	mgr.RegisterComponentName<InputVideoComponent>("InputVideo");
	mgr.RegisterComponentName<OutputVideoComponent>("OutputVideo");

	// Register core systems
	mgr.RegisterSystem<SDCPPSystem>();
	mgr.RegisterSystem<ImageSystem>();
	mgr.RegisterSystem<VideoSystem>();

	const auto upscaleViewID = viewManager.CreateView();
	viewManager.AddView<UpscaleView>(upscaleViewID, UpscaleView(mgr));
	viewManager.GetView<UpscaleView>(upscaleViewID).Init();
	
	const auto zepViewID = viewManager.CreateView();
	viewManager.AddView<ZepView>(zepViewID, ZepView(mgr));
	viewManager.GetView<ZepView>(zepViewID).Init();

	const auto videoViewID = viewManager.CreateView();
	viewManager.AddView<VideoView>(videoViewID, VideoView(mgr));
	viewManager.GetView<VideoView>(videoViewID).Init();

	const auto diffusionViewID = viewManager.CreateView();
	viewManager.AddView<DiffusionView>(diffusionViewID, DiffusionView(mgr));
	viewManager.AddView<ImageView>(diffusionViewID, ImageView(mgr));

	viewManager.GetView<DiffusionView>(diffusionViewID).Init();
	viewManager.GetView<ImageView>(diffusionViewID).Init();

	const auto pluginViewID = viewManager.CreateView();
	viewManager.AddView<PluginView>(pluginViewID, PluginView(mgr, pluginManager));
	viewManager.GetView<PluginView>(pluginViewID).Init();

	pluginManager.Init();
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
    mgr.Update(deltaT);
    viewManager.Update(deltaT);
	pluginManager.Update(deltaT);
}

void Engine::Draw() {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	ImGui::DockSpaceOverViewport(ImGui::GetMainViewport()->ID);

	try {
		ShowMenuBar(window);
		viewManager.Render();
	}
	catch (const std::exception& e) {
		std::cerr << "RENDER CRASH: " << e.what() << std::endl;
		if (ImGui::Begin("RENDER ERROR")) {
			ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Render Error: %s", e.what());
		}
		ImGui::End();
	}
	catch (...) {
		std::cerr << "UNKNOWN RENDER CRASH" << std::endl;
		if (ImGui::Begin("RENDER ERROR")) {
			ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Unknown render error");
		}
		ImGui::End();
	}

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}

	glfwSwapBuffers(window);
}

// Plugin interface functions - exported by the main executable
extern "C" {
	ANI_CORE_API ECS::EntityManager* GetHostEntityManager() {
		return &Core.GetEntityManager();
	}

	ANI_CORE_API GUI::ViewManager* GetHostViewManager() {
		return &Core.GetViewManager();
	}

	ANI_CORE_API ImGuiContext* GetHostImGuiContext() {
		return ImGui::GetCurrentContext();
	}

	ANI_CORE_API ImGuiMemAllocFunc GetHostImGuiAllocFunc() {
		ImGuiMemAllocFunc allocFunc;
		ImGuiMemFreeFunc freeFunc;
		void* userData;
		ImGui::GetAllocatorFunctions(&allocFunc, &freeFunc, &userData);
		return allocFunc;
	}

	ANI_CORE_API ImGuiMemFreeFunc GetHostImGuiFreeFunc() {
		ImGuiMemAllocFunc allocFunc;
		ImGuiMemFreeFunc freeFunc;
		void* userData;
		ImGui::GetAllocatorFunctions(&allocFunc, &freeFunc, &userData);
		return freeFunc;
	}

	ANI_CORE_API void* GetHostImGuiUserData() {
		ImGuiMemAllocFunc allocFunc;
		ImGuiMemFreeFunc freeFunc;
		void* userData;
		ImGui::GetAllocatorFunctions(&allocFunc, &freeFunc, &userData);
		return userData;
	}
}

} // namespace ANI