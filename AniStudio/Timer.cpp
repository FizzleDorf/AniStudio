#include <pch.h>
#include "Timer.h"

namespace ANI {
	ANI::T_Timer::T_Timer() : deltaTime(0.0f), lastFrame(0.0f) {

	}

	ANI::T_Timer::~T_Timer() {

	}

	void ANI::T_Timer::Tick(){
		deltaTime = glfwGetTime() - lastFrame;
		lastFrame = glfwGetTime();
	}

	void ANI::T_Timer::Init(){

	}
}

