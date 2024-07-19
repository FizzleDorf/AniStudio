#include "pch.h"
#include "Core/Ani.hpp"
#include "ECS/ECS.hpp"
#include "imgui.h"

int main(int argc, char** argv) {
	
	ANI::Core.Init();
	ANI::Timer.Init();
	ANI::Event.Init();
	

	while (ANI::Core.Run()) {
		ANI::Timer.Tick();
		ANI::Event.Poll();
		ANI::Core.Update();
	}

	return EXIT_SUCCESS;
}