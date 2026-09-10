#include "Core.hpp"
#include "Timer.hpp"
#include "AniStudio.hpp"
#include "StudioContext.hpp"
#include "GUI.h"
#include "ViewTypes.hpp"
#include "NetClient.hpp"
#include "ProjectManagerView.hpp"
#include "DebugView.hpp"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#endif

static bool SpawnLocalServer(const std::string& exePath,
    const std::string& projectArg,
    uint16_t port) {
#ifdef _WIN32
    std::string cmd = "\"" + exePath + "\" "
        + std::to_string(port) + " "
        + "--project \"" + projectArg + "\"";
    STARTUPINFOA si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    BOOL ok = CreateProcessA(nullptr, (LPSTR)cmd.c_str(),
        nullptr, nullptr, FALSE, 0,
        nullptr, nullptr, &si, &pi);
    if (!ok) return false;
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return true;
#else
    std::string cmd = "\"" + exePath + "\" "
        + std::to_string(port) + " "
        + "--project \"" + projectArg + "\" &";
    return std::system(cmd.c_str()) == 0;
#endif
}

int main(int argc, char** argv) {
    std::string directHost;
    uint16_t    directPort = 0;
    if (argc > 1) directHost = argv[1];
    if (argc > 2) directPort = (uint16_t)std::atoi(argv[2]);

    auto& core = ANI::Core::Ref();
    core.Init();

    auto studio = core.GetStudioCore().GetStudioContext();
    if (!studio) {
        std::cerr << "[Client] StudioContext not available\n";
        return 1;
    }

    // Turn on the networking entry point. The startup screen will now show
    // Host and Join buttons. We do NOT set mode=Client yet ? that happens after
    // the user has actually connected.
    core.GetStudioCore().SetNetworkClientMode(true);

    auto net = std::make_unique<Net::NetClient>();

    auto& pmv = core.GetStudioCore().GetProjectManagerView();
    pmv.onNetworkReady = [&](const GUI::NetworkStartupResult& result) {
        if (result.mode == GUI::NetworkStartupResult::Mode::Host) {
            std::string serverExe = "AniServer";
            std::string projectArg;
            if (!result.hostProjectPath.empty()) {
                projectArg = result.hostProjectPath;
            }
            else {
                projectArg = result.hostCreatePath + "/" + result.hostProjectName;
                std::filesystem::create_directories(projectArg);
            }

            uint16_t port = 9000;
            if (!SpawnLocalServer(serverExe, projectArg, port)) {
                std::cerr << "[Client] Failed to spawn AniServer\n";
                return;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(400));

            if (!net->Connect("127.0.0.1", port, result.username)) {
                std::cerr << "[Client] Host spawn ok but connect failed\n";
                return;
            }
        }
        else {
            if (!net->Connect(result.joinAddress, result.joinPort, result.username)) {
                std::cerr << "[Client] Join failed: "
                    << result.joinAddress << ":" << result.joinPort << "\n";
                return;
            }
        }

        // Now that we're connected, switch to Client mode and flip the flag.
        // Render() will see both and stop drawing the startup screen.
        studio->mode = ANI::StudioContext::Mode::Client;
        studio->networkConnected = true;

        auto& vm = core.GetStudioCore().GetViewManager();
        GUI::WorkspaceID ws = vm.CreateView();
        vm.SetWorkspaceName(ws, "Networked");
        vm.SetActiveWorkspace(ws);

        GUI::ViewTypeID debugId = vm.GetViewType("DebugView");
        vm.AddViewByType(ws, debugId);

        auto& debugView = vm.GetView<GUI::DebugView>(ws);
        debugView.SetNetClient(net.get());
        };

    if (!directHost.empty() && directPort != 0) {
        GUI::NetworkStartupResult r;
        r.mode = GUI::NetworkStartupResult::Mode::Join;
        r.username = "user";
        r.joinAddress = directHost;
        r.joinPort = directPort;
        pmv.onNetworkReady(r);
    }

    while (core.Run()) {
        core.Update(ANI::Timer.DeltaTime());
        core.Draw();
    }

    core.Quit();
    net.reset();
    return 0;
}