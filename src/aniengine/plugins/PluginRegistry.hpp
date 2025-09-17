#pragma once
#include <string>
#include <functional>
#include <vector>
#include <memory>

// Forward declaration for ImGui
struct ImGuiContext;

namespace ECS {
	class EntityManager;
	using EntityID = size_t;
	using ComponentTypeID = size_t;
	using SystemTypeID = size_t;
}

namespace GUI {
	class ViewManager;
	class BaseView;
	using ViewTypeID = size_t;
}

namespace Plugins {

	// Component and System descriptors remain unchanged
	struct ComponentDescriptor {
		std::string name;
		size_t size;
		void(*constructor)(void*, ECS::EntityID);
		void(*destructor)(void*);
	};

	struct SystemDescriptor {
		std::string name;
		void*(*creator)(ECS::EntityManager*);
		void(*destructor)(void*);
		void(*updater)(void*, float);
		std::vector<ECS::ComponentTypeID> requiredComponents;
	};

	struct ViewDescriptor {
		std::string name;
		std::string category;
		std::function<std::unique_ptr<GUI::BaseView>(ECS::EntityManager&, ImGuiContext*)> factory;
	};

	// Plugin Registry Interface - updated to handle ImGui context
	class IPluginRegistry {
	public:
		virtual ~IPluginRegistry() = default;

		// Registration methods - plugins call these directly
		virtual ECS::ComponentTypeID RegisterComponent(const ComponentDescriptor& desc) = 0;
		virtual ECS::SystemTypeID RegisterSystem(const SystemDescriptor& desc) = 0;
		virtual GUI::ViewTypeID RegisterView(const ViewDescriptor& desc) = 0;

		// Utility methods
		virtual const std::string& GetCurrentPluginName() const = 0;
		virtual ImGuiContext* GetImGuiContext() const = 0;
	};

} // namespace Plugins