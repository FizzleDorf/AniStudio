#include "pch.h"
#include "Core/Ani.hpp"
#include "ECS/ECS.hpp"


class TestSystem1 : public ECS::BaseSystem {

};

class TestSystem2 : public ECS::BaseSystem {

};

class TestSystem3 : public ECS::BaseSystem {

};

class TestComp1 : public ECS::BaseComponent {

};

class TestComp2 : public ECS::BaseComponent {

};


int main(int argc, char** argv) {
	
	ECS::EntityManager mgr;

	mgr.RegisterSystem<TestSystem1>();
	mgr.RegisterSystem<TestSystem2>();
	mgr.RegisterSystem<TestSystem3>();

	auto entity1 = mgr.AddNewEntity();
	ECS::EntityID ent(entity1, &mgr);
	
	ent.addComponent()

	auto entity1 = mgr.AddNewEntity();
	ECS::EntityID ent(entity1, &mgr);
		
		
		
	std::cout << id << " "  << std::endl;

	ent.AddComponent<TestComp1>();

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