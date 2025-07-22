/*
 * ZepEditorManager.hpp - GUI Layer Zep Editor Management
 * Integrates with existing StringWidgets to manage Zep editors for components
 */

#pragma once

#include "ECS.h"
#include <unordered_map>
#include <unordered_set>
#include <memory>

namespace GUI {

	// Zep editor tracking key
	struct ZepEditorKey {
		ECS::EntityID entityId;
		std::string componentName;
		std::string propertyName;

		bool operator==(const ZepEditorKey& other) const {
			return entityId == other.entityId &&
				componentName == other.componentName &&
				propertyName == other.propertyName;
		}
	};

	// Hash function for ZepEditorKey
	struct ZepEditorKeyHash {
		std::size_t operator()(const ZepEditorKey& key) const {
			std::size_t h1 = std::hash<ECS::EntityID>{}(key.entityId);
			std::size_t h2 = std::hash<std::string>{}(key.componentName);
			std::size_t h3 = std::hash<std::string>{}(key.propertyName);
			return h1 ^ (h2 << 1) ^ (h3 << 2);
		}
	};

	class ZepEditorManager {
	public:
		static ZepEditorManager& Instance() {
			static ZepEditorManager instance;
			return instance;
		}

		// Register a zep editor property for management
		void RegisterZepEditorProperty(ECS::EntityID entityId,
			const std::string& componentName,
			const std::string& propertyName,
			std::string* propertyPtr) {
			ZepEditorKey key{ entityId, componentName, propertyName };
			registeredZepEditors[key] = propertyPtr;

			std::cout << "[ZepEditorManager] Registered zep editor property: "
				<< componentName << "." << propertyName
				<< " for entity " << entityId << std::endl;
		}

		// Unregister a zep editor property (when component is removed)
		void UnregisterZepEditorProperty(ECS::EntityID entityId,
			const std::string& componentName,
			const std::string& propertyName) {
			ZepEditorKey key{ entityId, componentName, propertyName };
			auto it = registeredZepEditors.find(key);
			if (it != registeredZepEditors.end()) {
				// Clean up the zep editor through StringWidgets
				CleanupStringWidgetsZepEditor(it->second);
				registeredZepEditors.erase(it);

				std::cout << "[ZepEditorManager] Unregistered zep editor property: "
					<< componentName << "." << propertyName
					<< " for entity " << entityId << std::endl;
			}
		}

		// Unregister all zep editor properties for an entity (when entity is destroyed)
		void UnregisterZepEditorEntity(ECS::EntityID entityId) {
			auto it = registeredZepEditors.begin();
			while (it != registeredZepEditors.end()) {
				if (it->first.entityId == entityId) {
					// Clean up the zep editor through StringWidgets
					CleanupStringWidgetsZepEditor(it->second);
					std::cout << "[ZepEditorManager] Cleaned up zep editor "
						<< it->first.componentName << "." << it->first.propertyName
						<< " for entity " << entityId << std::endl;
					it = registeredZepEditors.erase(it);
				}
				else {
					++it;
				}
			}
		}

		// Update zep editor content when component data changes (called during deserialization)
		void UpdateZepEditorContent(ECS::EntityID entityId,
			const std::string& componentName,
			const std::string& propertyName) {
			ZepEditorKey key{ entityId, componentName, propertyName };
			auto it = registeredZepEditors.find(key);
			if (it != registeredZepEditors.end()) {
				UpdateStringWidgetsZepEditor(it->second);
			}
		}

		// Get zep editor property pointer for rendering (used by UISchema)
		std::string* GetZepEditorPropertyPointer(ECS::EntityID entityId,
			const std::string& componentName,
			const std::string& propertyName) {
			ZepEditorKey key{ entityId, componentName, propertyName };
			auto it = registeredZepEditors.find(key);
			return (it != registeredZepEditors.end()) ? it->second : nullptr;
		}

		// Check if a zep editor property is registered
		bool IsZepEditorPropertyRegistered(ECS::EntityID entityId,
			const std::string& componentName,
			const std::string& propertyName) {
			ZepEditorKey key{ entityId, componentName, propertyName };
			return registeredZepEditors.find(key) != registeredZepEditors.end();
		}

		// Clean up all zep editors (called on shutdown)
		void CleanupAllZepEditors() {
			for (const auto& pair : registeredZepEditors) {
				CleanupStringWidgetsZepEditor(pair.second);
			}
			registeredZepEditors.clear();
			CleanupAllStringWidgetsZepEditors();

			std::cout << "[ZepEditorManager] Cleaned up all zep editors" << std::endl;
		}

		// Debug: List all registered zep editor properties
		void DebugPrintRegisteredZepEditors() {
			std::cout << "[ZepEditorManager] Registered zep editor properties ("
				<< registeredZepEditors.size() << "):" << std::endl;
			for (const auto& pair : registeredZepEditors) {
				const auto& key = pair.first;
				std::cout << "  - Entity " << key.entityId
					<< ": " << key.componentName << "." << key.propertyName << std::endl;
			}
		}

	private:
		ZepEditorManager() = default;
		~ZepEditorManager() {
			CleanupAllZepEditors();
		}

		// Map from zep editor property keys to string property pointers
		std::unordered_map<ZepEditorKey, std::string*, ZepEditorKeyHash> registeredZepEditors;

		// Forward declarations to StringWidgets functions
		void CleanupStringWidgetsZepEditor(std::string* value);
		void UpdateStringWidgetsZepEditor(std::string* value);
		void CleanupAllStringWidgetsZepEditors();
	};

} // namespace GUI

// Implementation of forwarded StringWidgets functions
namespace GUI {
	inline void ZepEditorManager::CleanupStringWidgetsZepEditor(std::string* value) {
		UISchema::StringWidgets::CleanupEditor(value);
	}

	inline void ZepEditorManager::UpdateStringWidgetsZepEditor(std::string* value) {
		UISchema::StringWidgets::UpdateEditorContent(value);
	}

	inline void ZepEditorManager::CleanupAllStringWidgetsZepEditors() {
		UISchema::StringWidgets::Cleanup();
	}
}/*
 * ZepEditorManager.hpp - GUI Layer Editor Management
 * Integrates with existing StringWidgets to manage Zep editors for components
 */

#pragma once

#include "ECS.h"
#include <unordered_map>
#include <unordered_set>
#include <memory>

namespace GUI {

	// Component editor tracking key
	struct ComponentEditorKey {
		ECS::EntityID entityId;
		std::string componentName;
		std::string propertyName;

		bool operator==(const ComponentEditorKey& other) const {
			return entityId == other.entityId &&
				componentName == other.componentName &&
				propertyName == other.propertyName;
		}
	};

	// Hash function for ComponentEditorKey
	struct ComponentEditorKeyHash {
		std::size_t operator()(const ComponentEditorKey& key) const {
			std::size_t h1 = std::hash<ECS::EntityID>{}(key.entityId);
			std::size_t h2 = std::hash<std::string>{}(key.componentName);
			std::size_t h3 = std::hash<std::string>{}(key.propertyName);
			return h1 ^ (h2 << 1) ^ (h3 << 2);
		}
	};

	class ZepEditorManager {
	public:
		static ZepEditorManager& Instance() {
			static ZepEditorManager instance;
			return instance;
		}

		// Register a component property for editor management
		void RegisterComponentProperty(ECS::EntityID entityId,
			const std::string& componentName,
			const std::string& propertyName,
			std::string* propertyPtr) {
			ComponentEditorKey key{ entityId, componentName, propertyName };
			registeredProperties[key] = propertyPtr;

			std::cout << "[ZepEditorManager] Registered property: "
				<< componentName << "." << propertyName
				<< " for entity " << entityId << std::endl;
		}

		// Unregister a component property (when component is removed)
		void UnregisterComponentProperty(ECS::EntityID entityId,
			const std::string& componentName,
			const std::string& propertyName) {
			ComponentEditorKey key{ entityId, componentName, propertyName };
			auto it = registeredProperties.find(key);
			if (it != registeredProperties.end()) {
				// Clean up the editor through StringWidgets
				CleanupStringWidgetsEditor(it->second);
				registeredProperties.erase(it);

				std::cout << "[ZepEditorManager] Unregistered property: "
					<< componentName << "." << propertyName
					<< " for entity " << entityId << std::endl;
			}
		}

		// Unregister all properties for an entity (when entity is destroyed)
		void UnregisterEntity(ECS::EntityID entityId) {
			auto it = registeredProperties.begin();
			while (it != registeredProperties.end()) {
				if (it->first.entityId == entityId) {
					// Clean up the editor through StringWidgets
					CleanupStringWidgetsEditor(it->second);
					std::cout << "[ZepEditorManager] Cleaned up "
						<< it->first.componentName << "." << it->first.propertyName
						<< " for entity " << entityId << std::endl;
					it = registeredProperties.erase(it);
				}
				else {
					++it;
				}
			}
		}

		// Update editor content when component data changes (called during deserialization)
		void UpdateEditorContent(ECS::EntityID entityId,
			const std::string& componentName,
			const std::string& propertyName) {
			ComponentEditorKey key{ entityId, componentName, propertyName };
			auto it = registeredProperties.find(key);
			if (it != registeredProperties.end()) {
				UpdateStringWidgetsEditor(it->second);
			}
		}

		// Get property pointer for rendering (used by UISchema)
		std::string* GetPropertyPointer(ECS::EntityID entityId,
			const std::string& componentName,
			const std::string& propertyName) {
			ComponentEditorKey key{ entityId, componentName, propertyName };
			auto it = registeredProperties.find(key);
			return (it != registeredProperties.end()) ? it->second : nullptr;
		}

		// Check if a property is registered
		bool IsPropertyRegistered(ECS::EntityID entityId,
			const std::string& componentName,
			const std::string& propertyName) {
			ComponentEditorKey key{ entityId, componentName, propertyName };
			return registeredProperties.find(key) != registeredProperties.end();
		}

		// Clean up all editors (called on shutdown)
		void Cleanup() {
			for (const auto& pair : registeredProperties) {
				CleanupStringWidgetsEditor(pair.second);
			}
			registeredProperties.clear();
			CleanupAllStringWidgetsEditors();

			std::cout << "[ZepEditorManager] Cleaned up all editors" << std::endl;
		}

		// Debug: List all registered properties
		void DebugPrintRegisteredProperties() {
			std::cout << "[ZepEditorManager] Registered properties ("
				<< registeredProperties.size() << "):" << std::endl;
			for (const auto& pair : registeredProperties) {
				const auto& key = pair.first;
				std::cout << "  - Entity " << key.entityId
					<< ": " << key.componentName << "." << key.propertyName << std::endl;
			}
		}

	private:
		ZepEditorManager() = default;/*
 * ZepEditorManager.hpp - GUI Layer Editor Management
 * Manages Zep editors for components without creating dependencies between layers
 */

#pragma once

#include "ECS.h"
#include "StringWidgets.hpp"
#include "ZepUtils.hpp"
#include <unordered_map>
#include <unordered_set>
#include <memory>

		namespace GUI {

			// Component editor tracking key
			struct ComponentEditorKey {
				ECS::EntityID entityId;
				std::string componentName;
				std::string propertyName;

				bool operator==(const ComponentEditorKey& other) const {
					return entityId == other.entityId &&
						componentName == other.componentName &&
						propertyName == other.propertyName;
				}
			};

			// Hash function for ComponentEditorKey
			struct ComponentEditorKeyHash {
				std::size_t operator()(const ComponentEditorKey& key) const {
					std::size_t h1 = std::hash<ECS::EntityID>{}(key.entityId);
					std::size_t h2 = std::hash<std::string>{}(key.componentName);
					std::size_t h3 = std::hash<std::string>{}(key.propertyName);
					return h1 ^ (h2 << 1) ^ (h3 << 2);
				}
			};

			class ZepEditorManager {
			public:
				static ZepEditorManager& Instance() {
					static ZepEditorManager instance;
					return instance;
				}

				// Register a component property for editor management
				void RegisterComponentProperty(ECS::EntityID entityId,
					const std::string& componentName,
					const std::string& propertyName,
					std::string* propertyPtr) {
					ComponentEditorKey key{ entityId, componentName, propertyName };
					registeredProperties[key] = propertyPtr;

					std::cout << "[ZepEditorManager] Registered property: "
						<< componentName << "." << propertyName
						<< " for entity " << entityId << std::endl;
				}

				// Unregister a component property (when component is removed)
				void UnregisterComponentProperty(ECS::EntityID entityId,
					const std::string& componentName,
					const std::string& propertyName) {
					ComponentEditorKey key{ entityId, componentName, propertyName };
					auto it = registeredProperties.find(key);
					if (it != registeredProperties.end()) {
						// Clean up the editor
						UISchema::StringWidgets::CleanupEditor(it->second);
						registeredProperties.erase(it);

						std::cout << "[ZepEditorManager] Unregistered property: "
							<< componentName << "." << propertyName
							<< " for entity " << entityId << std::endl;
					}
				}

				// Unregister all properties for an entity (when entity is destroyed)
				void UnregisterEntity(ECS::EntityID entityId) {
					auto it = registeredProperties.begin();
					while (it != registeredProperties.end()) {
						if (it->first.entityId == entityId) {
							// Clean up the editor
							UISchema::StringWidgets::CleanupEditor(it->second);
							std::cout << "[ZepEditorManager] Cleaned up "
								<< it->first.componentName << "." << it->first.propertyName
								<< " for entity " << entityId << std::endl;
							it = registeredProperties.erase(it);
						}
						else {
							++it;
						}
					}
				}

				// Update editor content when component data changes (called during deserialization)
				void UpdateEditorContent(ECS::EntityID entityId,
					const std::string& componentName,
					const std::string& propertyName) {
					ComponentEditorKey key{ entityId, componentName, propertyName };
					auto it = registeredProperties.find(key);
					if (it != registeredProperties.end()) {
						UISchema::StringWidgets::UpdateEditorContent(it->second);
					}
				}

				// Get property pointer for rendering (used by UISchema)
				std::string* GetPropertyPointer(ECS::EntityID entityId,
					const std::string& componentName,
					const std::string& propertyName) {
					ComponentEditorKey key{ entityId, componentName, propertyName };
					auto it = registeredProperties.find(key);
					return (it != registeredProperties.end()) ? it->second : nullptr;
				}

				// Check if a property is registered
				bool IsPropertyRegistered(ECS::EntityID entityId,
					const std::string& componentName,
					const std::string& propertyName) {
					ComponentEditorKey key{ entityId, componentName, propertyName };
					return registeredProperties.find(key) != registeredProperties.end();
				}

				// Clean up all editors (called on shutdown)
				void Cleanup() {
					for (const auto& pair : registeredProperties) {
						UISchema::StringWidgets::CleanupEditor(pair.second);
					}
					registeredProperties.clear();
					UISchema::StringWidgets::Cleanup();

					std::cout << "[ZepEditorManager] Cleaned up all editors" << std::endl;
				}

				// Debug: List all registered properties
				void DebugPrintRegisteredProperties() {
					std::cout << "[ZepEditorManager] Registered properties ("
						<< registeredProperties.size() << "):" << std::endl;
					for (const auto& pair : registeredProperties) {
						const auto& key = pair.first;
						std::cout << "  - Entity " << key.entityId
							<< ": " << key.componentName << "." << key.propertyName << std::endl;
					}
				}

			private:
				ZepEditorManager() = default;
				~ZepEditorManager() {
					Cleanup();
				}

				// Map from component property keys to string property pointers
				std::unordered_map<ComponentEditorKey, std::string*, ComponentEditorKeyHash> registeredProperties;
			};

		} // namespace GUI