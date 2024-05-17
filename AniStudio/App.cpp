#include "pch.h"
#include "Core/Ani.hpp"

#include "ECS/Base/Types.hpp"
#include "ECS/Base/EntityManager.hpp"


class TestComp1 : public ECS::BaseComponent {

};

int main(int argc, char** argv) {
	
	ECS::EntityManager mgr;
	auto id = mgr.AddNewEntity();
	


	std::cout << id << " "  << std::endl;

	auto id2 = mgr.AddNewEntity();

	auto typeID1 = ECS::CompType<TestComp1>();

	std::cout << id << " " <<  id2 << " " <<  std::endl;

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