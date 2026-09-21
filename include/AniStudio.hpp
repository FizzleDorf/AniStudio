/*
        d8888          d8b  .d8888b.  888                  888 d8b
       d88888          Y8P d88P  Y88b 888                  888 Y8P
      d88P888              Y88b.      888                  888
     d88P 888 88888b.  888  "Y888b.   888888 888  888  .d88888 888  .d88b.
    d88P  888 888 "88b 888     "Y88b. 888    888  888 d88" 888 888 d88""88b
   d88P   888 888  888 888       "888 888    888  888 888  888 888 888  888
  d8888888888 888  888 888 Y88b  d88P Y88b.  Y88b 888 Y88b 888 888 Y88..88P
 d88P     888 888  888 888  "Y8888P"   "Y888  "Y88888  "Y88888 888  "Y88P"

 * This file is part of AniStudio.
 * Copyright (C) 2025 FizzleDorf (AnimAnon)
 *
 * This software is dual-licensed under the GNU Lesser General Public License v3.0 (LGPL-3.0)
 * and a commercial license. You may choose to use it under either license.
 *
 * For the LGPL-3.0, see the LICENSE-LGPL-3.0.txt file in the repository.
 * For commercial license information, please contact legal@kframe.ai.
 */

#pragma once

#define ANI_STUDIO_API

#include "OpenGLWrapper.hpp"
#include "AniEngine.hpp"
#include "StudioContext.hpp"
#include "GUI.h"
#include "ProjectSystem.hpp"
#include "ImGuiStateUtils.hpp"
#include "WindowState.hpp"
#include "AniStudioSystems.hpp"
#include "StudioPluginManager.hpp"
#include "ErrorBus.hpp"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <memory>
#include <set>
#include <unordered_map>
#include <vector>
#include <string>

namespace GUI {
    class MenuBar;
    class ProjectManagerView;
    class SettingsView;
}

namespace Net { class NetClient; }

namespace ANI {

    class ANI_STUDIO_API StudioCore {
    public:
        StudioCore();
        ~StudioCore();

        // ---------------------------------------------------------------------
        // Lifecycle
        // ---------------------------------------------------------------------
        // Full studio init (core + GUI). Requires SetImGuiContext() and
        // SetWindowHandle() to have been called by the host beforehand for
        // GUI mode.
        bool Initialize();

        // Headless / server mode: engine, ECS, systems, events only.
        bool InitializeCoreOnly();

        // GUI layer: views, settings tabs, plugin manager, menu bar.
        // Called from Initialize(); can be called explicitly if you need to
        // build the core first and bring the GUI up later.
        bool InitializeGUI();

        void Shutdown();
        void Update(float deltaTime);
        void Render();

        // ---------------------------------------------------------------------
        // Accessors
        // ---------------------------------------------------------------------
        ECS::EntityManager& GetEntityManager() {
            if (!studioContext || !studioContext->entityManager) {
                throw std::runtime_error("StudioContext or EntityManager not initialized");
            }
            return *studioContext->entityManager;
        }

        GUI::ViewManager& GetViewManager() {
            if (!studioContext || !studioContext->viewManager) {
                throw std::runtime_error("StudioContext or ViewManager not initialized");
            }
            return *studioContext->viewManager;
        }

        ECS::ProjectSystem& GetProjectSystem() {
            auto system = GetEntityManager().GetSystem<ECS::ProjectSystem>();
            if (!system) {
                throw std::runtime_error("ProjectSystem not registered with EntityManager");
            }
            return *system;
        }

        GUI::ProjectManagerView& GetProjectManagerView();
        GUI::SettingsView& GetSettingsView();

        std::shared_ptr<StudioContext> GetStudioContext() const { return studioContext; }

        void SetMode(StudioContext::Mode m) {
            if (studioContext) studioContext->mode = m;
        }
        StudioContext::Mode GetMode() const {
            return studioContext ? studioContext->mode : StudioContext::Mode::Local;
        }
        void SetNetworkClientMode(bool on);

        static std::unique_ptr<StudioCore> CreateWithContext(std::shared_ptr<StudioContext> existingContext);

        // ---------------------------------------------------------------------
        // State
        // ---------------------------------------------------------------------
        bool IsRunning() const { return running && engineCore.IsRunning(); }
        void SetRunning(bool isRunning) {
            running = isRunning;
            engineCore.SetRunning(isRunning);
        }

        bool IsInitialized() const { return initialized; }

        // ---------------------------------------------------------------------
        // Host integration
        // ---------------------------------------------------------------------
        void SetWindowHandle(void* window);
        void SetImGuiContext(void* context);

        // ---------------------------------------------------------------------
        // Core wiring (called internally; exposed for advanced hosts)
        // ---------------------------------------------------------------------
        void SetCoreCallbacks();
        void SetCoreEvents();

        // ---------------------------------------------------------------------
        // Workspace
        // ---------------------------------------------------------------------
        void SetActiveWorkspace(GUI::WorkspaceID workspaceID);
        GUI::WorkspaceID GetActiveWorkspace() const;

        // ---------------------------------------------------------------------
        // Project lifecycle (called from ProjectSystem callbacks)
        // ---------------------------------------------------------------------
        void OnProjectLoaded(const std::string& projectPath);
        void OnProjectCreated(const std::string& projectPath);
        void OnProjectClosed();

    private:
        // ---------------------------------------------------------------------
        // State
        // ---------------------------------------------------------------------
        bool initialized;
        bool running;
        bool m_isShuttingDown;
        bool m_showMissingPathsPopup;

        std::shared_ptr<StudioContext> studioContext;

        void* windowHandle;
        void* imguiContext;

        EngineCore engineCore;

        std::unique_ptr<GUI::MenuBar> m_menuBar;
        std::unique_ptr<GUI::ProjectManagerView> m_projectManagerView;
        std::unique_ptr<GUI::SettingsView> m_settingsView;
        bool m_showProjectManagerView = false;

        std::vector<std::string> m_missingKeys;

        Utils::WindowState m_windowState;

        // ErrorBus members for Popups
        std::vector<ANI::ErrorBus::Entry> m_pendingErrors;
        bool m_showErrorPopup = false;

        // Seeds / repairs all core FilePathSystem keys.
        void EnsureCorePaths();

        // Points ImGui's io.IniFilename at the path stored in FilePathSystem.
        // Requires ImGui::SetCurrentContext() to have already been called.
        void ConfigureImGuiIniPath();

        // Creates the plugin manager, scans the plugin directory, and wires
        // the engine/studio contexts. Requires a valid ImGui context.
        void InitializeStudioPlugins();

        // Registers the standard settings tabs (general, style, render,
        // fonts, text editor) on the SettingsSystem.
        void RegisterSettingsTabs();

        // Wires ProjectSystem callbacks (load/created/closed/view-state).
        void SetupProjectCallbacks();

        // ---------------------------------------------------------------------
        // Window state helpers
        // ---------------------------------------------------------------------
        void InitializeWindowState();
        void SyncWindowStateFromGLFW();
        void ApplyWindowStateToGLFW();
        std::string GetDefaultWindowStatePath() const;
    };

} // namespace ANI