#include "pch.h"
#include "Core/Ani.h"

#include "ECS/Base/Types.h"
#include "ECS/Base/EntityManager.h"


class TestComp1 : public ECS::BaseComponent {

};

int main(int argc, char** argv) {
	
	ECS::EntityManager mgr;
	auto id = mgr.AddNewEntity();
	auto typeID1 = ECS::CompType<TestComp1>();

	std::cout << id  << " " << typeID1 << std::endl;

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