#pragma once
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <functional>
#include "EntityManager.hpp"

namespace GUI {
	namespace Clipboard {

		enum class Type { None, Entity, Component, Property };

		void CopyEntity(ECS::EntityManager& mgr, ECS::EntityID entity);
		void CopyComponent(ECS::EntityManager& mgr, ECS::EntityID entity, const std::string& compName);
		void CopyProperty(ECS::EntityManager& mgr, ECS::EntityID entity, const std::string& compName, const std::string& propName);
		void CopyImageMetadata(ECS::EntityManager& mgr, const std::string& imagePath);
		void CopyComponentFromImageMetadata(ECS::EntityManager& mgr, const std::string& imagePath, const std::string& compName);
		void CopyPropertyFromImageMetadata(ECS::EntityManager& mgr, const std::string& imagePath, const std::string& compName, const std::string& propName);

		Type GetType();
		bool HasEntity();
		bool HasComponent();
		bool HasProperty();
		nlohmann::json GetData();

		bool PasteEntity(ECS::EntityManager& mgr, ECS::EntityID target, std::function<void(ECS::EntityID, const std::string&)> addComponent);
		bool PasteComponent(ECS::EntityManager& mgr, ECS::EntityID target, const std::string& compName);
		bool PasteProperty(ECS::EntityManager& mgr, ECS::EntityID target, const std::string& compName, const std::string& propName);

		bool PasteComponentFromEntity(ECS::EntityManager& mgr, ECS::EntityID target, const std::string& compName, std::function<void(ECS::EntityID, const std::string&)> addComponent);
		bool PastePropertyFromEntity(ECS::EntityManager& mgr, ECS::EntityID target, const std::string& compName, const std::string& propName);

		std::vector<std::string> GetEntityComponentNames(ECS::EntityManager& mgr);
		std::vector<std::string> GetComponentPropertyNames(ECS::EntityManager& mgr, const std::string& compName);
		bool EntityClipboardHasComponent(ECS::EntityManager& mgr, const std::string& compName);
		bool EntityClipboardHasProperty(ECS::EntityManager& mgr, const std::string& compName, const std::string& propName);

		void AddComponent(ECS::EntityManager& mgr, ECS::EntityID target, const std::string& compName, std::function<void(ECS::EntityID, const std::string&)> addComponent);
		void RemoveComponent(ECS::EntityManager& mgr, ECS::EntityID target, const std::string& compName);
		void ResetComponent(ECS::EntityManager& mgr, ECS::EntityID target, const std::string& compName, std::function<void(ECS::EntityID, const std::string&)> addComponent);
		void ResetAllComponents(ECS::EntityManager& mgr, ECS::EntityID target, const std::vector<std::string>& compNames, std::function<void(ECS::EntityID, const std::string&)> addComponent);

	}
}