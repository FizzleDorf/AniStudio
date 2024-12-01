#include "Engine/Engine.hpp"
#include "Events/Events.hpp"
#include "Timer/Timer.hpp"

int main(int argc, char* argv[]) {

    // Initialize the engine
    ANI::Core.Init();
    ANI::Events::Ref().Init(ANI::Core.Window());
    ANI::Timer.Ref().Init();

    // Main loop
    while (ANI::Core.Run()) { 
        ANI::Timer.Ref().Tick();
        ANI::Events::Ref().Poll();
        ANI::Core.Update(ANI::Timer.Ref().DeltaTime());
        ANI::Core.Draw();
    }
    
    return 0;
}
