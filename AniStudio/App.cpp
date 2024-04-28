#include "pch.h"
#include "Ani.h"

#include "Types.h"

namespace ECS {
	struct BaseComponent {
	public:
		virtual ~BaseComponent(){}
	};
}

class TestComp1 : public ECS::BaseComponent {

};

class TestComp2 : public ECS::BaseComponent {

};

int main(int argc, char** argv) {
	
	auto typeID1 = ECS::CompType<TestComp1>();
	auto typeID2 = ECS::CompType<TestComp1>();

	auto typeID3 = ECS::CompType<TestComp2>();

	std::cout << typeID1 << " " << typeID2 << " " << typeID3 << std::endl;

	ANI::Core.Init();
	ANI::Timer.Init();
	ANI::Event.Init();

	while (ANI::Core.Run()) {
		ANI::Core.Tick();
		ANI::Core.Poll();
		ANI::Core.Update();
	}

	return EXIT_SUCCESS;
}