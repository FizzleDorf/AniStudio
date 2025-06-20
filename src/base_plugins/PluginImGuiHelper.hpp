#pragma once

#include <imgui.h>
#include <atomic>
#include <iostream>

namespace Plugin {

	class ImGuiContextManager {
	private:
		static std::atomic<ImGuiContext*> s_sharedContext;
		static std::atomic<bool> s_contextValid;
		static ImGuiMemAllocFunc s_allocFunc;
		static ImGuiMemFreeFunc s_freeFunc;
		static void* s_userData;

	public:
		// Set the shared context from the host application
		static void SetSharedContext(ImGuiContext* context,
			ImGuiMemAllocFunc allocFunc = nullptr,
			ImGuiMemFreeFunc freeFunc = nullptr,
			void* userData = nullptr) {
			s_sharedContext.store(context);
			s_contextValid.store(context != nullptr);
			s_allocFunc = allocFunc;
			s_freeFunc = freeFunc;
			s_userData = userData;

			if (context) {
				// Set the context for this DLL's ImGui calls
				ImGui::SetCurrentContext(context);

				// Set memory functions if provided
				if (allocFunc && freeFunc) {
					ImGui::SetAllocatorFunctions(allocFunc, freeFunc, userData);
				}

				std::cout << "Plugin ImGui context set successfully: " << context << std::endl;
			}
			else {
				std::cout << "Plugin ImGui context cleared" << std::endl;
			}
		}

		// Get the current shared context
		static ImGuiContext* GetSharedContext() {
			return s_sharedContext.load();
		}

		// Check if context is valid and available
		static bool IsContextValid() {
			ImGuiContext* context = s_sharedContext.load();
			if (!context || !s_contextValid.load()) {
				return false;
			}

			// Verify the context is still current
			return ImGui::GetCurrentContext() == context;
		}

		// Ensure context is set before ImGui operations
		static bool EnsureContext() {
			if (!IsContextValid()) {
				ImGuiContext* context = s_sharedContext.load();
				if (context) {
					ImGui::SetCurrentContext(context);
					return true;
				}
				return false;
			}
			return true;
		}

		// Safe ImGui operation wrapper
		template<typename Func>
		static auto SafeImGuiCall(Func&& func) -> decltype(func()) {
			if (EnsureContext()) {
				return func();
			}
			else {
				std::cerr << "Plugin: ImGui context not available for operation" << std::endl;
				return decltype(func()){}; // Return default-constructed value
			}
		}

		// Cleanup
		static void Cleanup() {
			s_sharedContext.store(nullptr);
			s_contextValid.store(false);
			s_allocFunc = nullptr;
			s_freeFunc = nullptr;
			s_userData = nullptr;
		}
	};

	// Static member definitions
	std::atomic<ImGuiContext*> ImGuiContextManager::s_sharedContext{ nullptr };
	std::atomic<bool> ImGuiContextManager::s_contextValid{ false };
	ImGuiMemAllocFunc ImGuiContextManager::s_allocFunc = nullptr;
	ImGuiMemFreeFunc ImGuiContextManager::s_freeFunc = nullptr;
	void* ImGuiContextManager::s_userData = nullptr;

} // namespace Plugin