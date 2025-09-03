#pragma once
#include <string>
#include <functional>
#include <vector>

namespace ECS {
	class EntityManager;
	using EntityID = size_t;
	using ComponentTypeID = size_t;
	using SystemTypeID = size_t;
}

namespace GUI {
	class ViewManager;
	using ViewTypeID = size_t;
}

namespace Plugins {

	// Descriptors remain the same
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
		void*(*creator)(ECS::EntityManager*);
		void(*destructor)(void*);
	};

	// Plugin Registry Interface - passed directly to plugins
	class IPluginRegistry {
	public:
		virtual ~IPluginRegistry() = default;

		// Registration methods - plugins call these directly
		virtual ECS::ComponentTypeID RegisterComponent(const ComponentDescriptor& desc) = 0;
		virtual ECS::SystemTypeID RegisterSystem(const SystemDescriptor& desc) = 0;
		virtual GUI::ViewTypeID RegisterView(const ViewDescriptor& desc) = 0;

		// Utility methods
		virtual const std::string& GetCurrentPluginName() const = 0;
	};

} // namespace Plugins