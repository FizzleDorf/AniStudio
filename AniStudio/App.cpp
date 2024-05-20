#include "pch.h"
#include "Core/Ani.hpp"
#include "ECS/ECS.hpp"

class TestComp1 : public ECS::BaseComponent {
	int A = 5;
};

class TestComp2 : public ECS::BaseComponent {
	int A = 5;
};

class TestSystem1 : public ECS::BaseSystem {
	TestSystem1() {
		AddComponentSignature<TestComp1>();
	}
};

class TestSystem2 : public ECS::BaseSystem {
	TestSystem2() {
		AddComponentSignature<TestComp2>();
	}
};

class TestSystem3 : public ECS::BaseSystem {
	TestSystem3() {
		AddComponentSignature<TestComp1>();
		AddComponentSignature<TestComp2>();
	}
};




int main(int argc, char** argv) {
	
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
	
	mgr.Update();

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