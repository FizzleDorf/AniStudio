#include "pch.h"
#include "Core/Ani.hpp"
#include "ECS/ECS.hpp"
//#include "imgui/imgui.h"

class TestComp1 : public ECS::BaseComponent {
public:
	int A;
	TestComp1(int a = 5) : A(a) {}
};

class TestComp2 : public ECS::BaseComponent {
public:
	int A;
	TestComp2(int a = 5) : A(a) {}
};

struct TestSystem1 : public ECS::BaseSystem {
	TestSystem1() {
		AddComponentSignature<TestComp1>();
	}
};

struct TestSystem2 : public ECS::BaseSystem {
	TestSystem2() {
		AddComponentSignature<TestComp2>();
	}
};

struct TestSystem3 : public ECS::BaseSystem {
	TestSystem3() {
		AddComponentSignature<TestComp1>();
		AddComponentSignature<TestComp2>();	
	}
};
/*
* OS Specific init

struct OSInit {
	void Init() {
#if defined(_WIN32) || defined(_WIN64)
		// Windows-specific initialization
		WindowsInit();
#elif defined(__linux__)
		// Linux-specific initialization
		LinuxInit();
#elif defined(__APPLE__)
		// macOS-specific initialization
		MacOSInit();
#else
		// Other OS-specific initialization
		DefaultInit();
#endif
	}
	*/

int main(int argc, char** argv) {
	/*
	ECS::EntityManager mgr;
	
	mgr.RegisterSystem<TestSystem1>();
	mgr.RegisterSystem<TestSystem2>();
	mgr.RegisterSystem<TestSystem3>();

	auto entity1 = mgr.AddNewEntity();
	ECS::Entity ent(entity1, &mgr);
	
	ent.AddComponent<TestComp1>();

	auto entity2 = mgr.AddNewEntity();
	ent.AddComponent<TestComp2>(entity2);
	
	auto entity3 = mgr.AddNewEntity();
	ent.AddComponent<TestComp1>(entity3);
	ent.AddComponent<TestComp2>(entity3);
	*/
	bool p_open = true;
	//ImGui::Begin("My First Tool", &p_open, ImGuiWindowFlags_MenuBar);
	//if (ImGui::BeginMenuBar())
	//{
	//	if (ImGui::BeginMenu("File"))
	//	{
	//		if (ImGui::MenuItem("Open..", "Ctrl+O")) { /* Do stuff */ }
	//		if (ImGui::MenuItem("Save", "Ctrl+S")) { /* Do stuff */ }
	//		if (ImGui::MenuItem("Close", "Ctrl+W")) { p_open = false; }
	//		ImGui::EndMenu();
	//	}
	//	ImGui::EndMenuBar();
	//}
	
	ANI::Core.Init();
	ANI::Timer.Init();
	ANI::Event.Init();
	

	while (ANI::Core.Run()) {
		//mgr.Update();
		ANI::Timer.Tick();
		ANI::Event.Poll();
		ANI::Core.Update();
	}

	return EXIT_SUCCESS;
}