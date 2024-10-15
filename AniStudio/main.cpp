#include "Engine/Engine.hpp"

int main(int argc, char* argv[]) {

    // Initialize the engine
    ANI::Core.Init();

    // Main loop
    while (ANI::Core.Run()) {
        
        ANI::Core.Update();
    }

    return 0;
}
