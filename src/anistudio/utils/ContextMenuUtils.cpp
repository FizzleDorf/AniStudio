#include "ContextMenuUtils.hpp"
#include "PngMetadataUtils.hpp"
#include "ImageComponent.hpp"
#include "PropertyTypes.hpp"
#include <png.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>

namespace Utils {

	ContextMenuUtils::ContextMenuUtils(ECS::EntityManager& entityMgr)
		: entityManager(entityMgr) {
	}

	// ===== MAIN RENDER FUNCTIONS =====

	void ContextMenuUtils::RenderImageContextMenu(ECS::EntityID entityId) {
		if (!entityManager.IsEntityValid(entityId)) {
			return;
		}

		std::string popupId = "ImageContextMenu##" + std::to_string(entityId);

		if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
			ImGui::OpenPopup(popupId.c_str());
		}

		if (ImGui::BeginPopup(popupId.c_str())) {
			ImGui::Text("Entity: %zu", entityId);
			ImGui::Separator();

			std::string imagePath;
			if (entityManager.HasComponent<ECS::ImageComponent>(entityId)) {
				const auto& imageComp = entityManager.GetComponent<ECS::ImageComponent>(entityId);
				imagePath = imageComp.filePath;
			}

			if (!imagePath.empty() && std::filesystem::exists(imagePath)) {
				nlohmann::json metadata = ParseImageMetadata(imagePath);
				if (!metadata.empty()) {
					RenderMetadataComponentMenu(metadata);
					RenderMetadataValueMenu(metadata);
					ImGui::Separator();
				}
			}

			RenderEntityComponentMenu(entityId);
			ImGui::Separator();
			RenderPasteMenu(entityId);

			ImGui::EndPopup();
		}
	}

	void ContextMenuUtils::RenderImageContextMenuWithPath(const std::string& imagePath) {
		std::string popupId = "ImagePathContextMenu##" + std::filesystem::path(imagePath).filename().string();

		if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
			ImGui::OpenPopup(popupId.c_str());
		}

		if (ImGui::BeginPopup(popupId.c_str())) {
			ImGui::Text("Image: %s", std::filesystem::path(imagePath).filename().string().c_str());
			ImGui::Separator();

			if (std::filesystem::exists(imagePath)) {
				nlohmann::json metadata = ParseImageMetadata(imagePath);
				if (!metadata.empty()) {
					RenderMetadataComponentMenu(metadata);
					RenderMetadataValueMenu(metadata);
				}
				else {
					ImGui::TextDisabled("No metadata found");
				}
			}
			else {
				ImGui::TextDisabled("File not found");
			}

			ImGui::EndPopup();
		}
	}

	void ContextMenuUtils::RenderComponentContextMenu(ECS::EntityID entityId, ECS::ComponentTypeID componentId) {
		std::string componentName = entityManager.GetComponentNameById(componentId);
		std::string popupId = "ComponentContextMenu##" + std::to_string(entityId) + "_" + std::to_string(componentId);

		if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
			ImGui::OpenPopup(popupId.c_str());
		}

		if (ImGui::BeginPopup(popupId.c_str())) {
			ImGui::Text("Component: %s", componentName.c_str());
			ImGui::Text("Entity: %zu", entityId);
			ImGui::Separator();

			if (ImGui::MenuItem("Copy Component")) {
				CopyComponent(entityId, componentId);
			}

			if (ImGui::MenuItem("Copy Entity")) {
				CopyEntity(entityId);
			}

			ImGui::Separator();

			RenderValueCopyMenu(entityId, componentId, componentName);

			ImGui::Separator();
			RenderPasteMenu(entityId);

			ImGui::EndPopup();
		}
	}

	void ContextMenuUtils::RenderComponentContextMenu(ECS::EntityID entityId, const std::string& componentName) {
		ECS::ComponentTypeID componentId = entityManager.GetComponentTypeIdByName(componentName);
		if (componentId != ECS::MAX_COMPONENT_COUNT) {
			RenderComponentContextMenu(entityId, componentId);
		}
	}

	// ===== COPY OPERATIONS =====

	void ContextMenuUtils::CopyEntity(ECS::EntityID entityId) {
		if (!entityManager.IsEntityValid(entityId)) {
			return;
		}

		nlohmann::json entityData = entityManager.SerializeEntity(entityId);

		nlohmann::json clipboardData;
		clipboardData["dataType"] = "entity";
		clipboardData["data"] = entityData;
		clipboardData["source"] = "entity";

		SetClipboardData(clipboardData);
	}

	void ContextMenuUtils::CopyComponent(ECS::EntityID entityId, ECS::ComponentTypeID componentId) {
		if (!entityManager.IsEntityValid(entityId) || !entityManager.HasComponentById(entityId, componentId)) {
			return;
		}

		auto* component = entityManager.GetComponentByIdConst(entityId, componentId);
		if (!component) return;

		std::string componentName = entityManager.GetComponentNameById(componentId);
		nlohmann::json componentData = component->Serialize();

		nlohmann::json clipboardData;
		clipboardData["dataType"] = "component";
		clipboardData["componentName"] = componentName;
		clipboardData["componentData"] = componentData;
		clipboardData["source"] = "entity";

		SetClipboardData(clipboardData);
	}

	void ContextMenuUtils::CopyComponent(ECS::EntityID entityId, const std::string& componentName) {
		ECS::ComponentTypeID componentId = entityManager.GetComponentTypeIdByName(componentName);
		if (componentId != ECS::MAX_COMPONENT_COUNT) {
			CopyComponent(entityId, componentId);
		}
	}

	// ===== PASTE OPERATIONS =====

	bool ContextMenuUtils::PasteEntity(ECS::EntityID targetEntityId) {
		if (!HasClipboardEntity() || !entityManager.IsEntityValid(targetEntityId)) {
			return false;
		}

		nlohmann::json clipboardData = GetClipboardData();
		if (!clipboardData.contains("data")) {
			return false;
		}

		entityManager.DeserializeEntity(clipboardData["data"], targetEntityId);
		return true;
	}

	bool ContextMenuUtils::PasteComponent(ECS::EntityID targetEntityId, const std::string& componentName) {
		if (!HasClipboardEntity() || !entityManager.IsEntityValid(targetEntityId)) {
			return false;
		}

		nlohmann::json componentData = GetClipboardComponent(componentName);
		if (componentData.empty()) {
			return false;
		}

		// Check if component is registered
		if (!entityManager.IsComponentNameRegistered(componentName)) {
			return false;
		}

		// Get or create component in target entity
		ECS::ComponentTypeID componentId = entityManager.GetComponentTypeIdByName(componentName);
		bool hadComponent = entityManager.HasComponentById(targetEntityId, componentId);

		if (!hadComponent) {
			// Create a simple component structure to add to entity
			nlohmann::json tempEntityData;
			tempEntityData["components"] = nlohmann::json::array();

			nlohmann::json compJson;
			compJson[componentName] = nlohmann::json::object(); // Empty object to create component
			tempEntityData["components"].push_back(compJson);

			entityManager.DeserializeEntity(tempEntityData, targetEntityId);
		}

		// Apply component data
		auto* component = entityManager.GetComponentById(targetEntityId, componentId);
		if (component) {
			// Create proper JSON structure for deserialization
			nlohmann::json properJson;
			properJson[componentName] = componentData;
			component->Deserialize(properJson);
			return true;
		}

		return false;
	}

	bool ContextMenuUtils::PasteValue(ECS::EntityID targetEntityId, const std::string& componentName, const std::string& propertyName) {
		if (!HasClipboardEntity() || !entityManager.IsEntityValid(targetEntityId)) {
			return false;
		}

		// Get value from clipboard
		std::string clipboardValue = GetClipboardValue(componentName, propertyName);
		if (clipboardValue.empty()) {
			return false;
		}

		// Get component from target entity
		ECS::ComponentTypeID componentId = entityManager.GetComponentTypeIdByName(componentName);
		if (componentId == ECS::MAX_COMPONENT_COUNT) {
			return false;
		}

		auto* component = entityManager.GetComponentById(targetEntityId, componentId);
		if (!component) {
			return false;
		}

		// Get current component data
		nlohmann::json componentData = component->Serialize();

		// Update the specific property
		if (componentData.contains(propertyName)) {
			// Try to preserve type
			const auto& currentValue = componentData[propertyName];

			if (currentValue.is_string()) {
				componentData[propertyName] = clipboardValue;
			}
			else if (currentValue.is_number_integer()) {
				try {
					componentData[propertyName] = std::stoi(clipboardValue);
				}
				catch (...) {
					componentData[propertyName] = clipboardValue;
				}
			}
			else if (currentValue.is_number_float()) {
				try {
					componentData[propertyName] = std::stof(clipboardValue);
				}
				catch (...) {
					componentData[propertyName] = clipboardValue;
				}
			}
			else if (currentValue.is_boolean()) {
				std::string lowerValue = clipboardValue;
				std::transform(lowerValue.begin(), lowerValue.end(), lowerValue.begin(), ::tolower);
				if (lowerValue == "true" || lowerValue == "1" || lowerValue == "yes") {
					componentData[propertyName] = true;
				}
				else if (lowerValue == "false" || lowerValue == "0" || lowerValue == "no") {
					componentData[propertyName] = false;
				}
				else {
					componentData[propertyName] = clipboardValue;
				}
			}
			else {
				componentData[propertyName] = clipboardValue;
			}
		}
		else {
			// Property doesn't exist, add as string
			componentData[propertyName] = clipboardValue;
		}

		// Create proper JSON structure for deserialization
		nlohmann::json properJson;
		properJson[componentName] = componentData;

		// Deserialize back to component
		component->Deserialize(properJson);
		return true;
	}

	// ===== CLIPBOARD UTILITIES =====

	bool ContextMenuUtils::HasClipboardEntity() const {
		nlohmann::json clipboardData = GetClipboardData();
		return clipboardData.contains("dataType") && clipboardData["dataType"] == "entity";
	}

	std::string ContextMenuUtils::GetClipboardPreview() const {
		nlohmann::json clipboardData = GetClipboardData();
		if (!clipboardData.contains("dataType")) {
			return "No data";
		}

		std::string dataType = clipboardData["dataType"];
		if (dataType == "entity") {
			std::string source = clipboardData.value("source", "unknown");
			int componentCount = 0;
			if (clipboardData.contains("data") && clipboardData["data"].contains("components")) {
				const auto& components = clipboardData["data"]["components"];
				if (components.is_array()) {
					componentCount = components.size();
				}
			}
			return "Entity (" + std::to_string(componentCount) + " components)";
		}
		else if (dataType == "component") {
			if (clipboardData.contains("componentName")) {
				return "Component: " + clipboardData["componentName"].get<std::string>();
			}
			return "Component";
		}

		return "Unknown";
	}

	void ContextMenuUtils::ClearClipboard() {
		ImGui::SetClipboardText("");
	}

	std::vector<std::string> ContextMenuUtils::GetClipboardComponents() const {
		std::vector<std::string> components;

		if (!HasClipboardEntity()) {
			return components;
		}

		nlohmann::json clipboardData = GetClipboardData();
		if (!clipboardData.contains("data") || !clipboardData["data"].contains("components")) {
			return components;
		}

		const auto& comps = clipboardData["data"]["components"];

		if (comps.is_array()) {
			for (const auto& component : comps) {
				if (component.is_object()) {
					for (auto it = component.begin(); it != component.end(); ++it) {
						std::string key = it.key();
						// Skip metadata fields
						if (key == "Base_Component" || key == "compName") {
							continue;
						}
						components.push_back(key);
					}
				}
			}
		}

		// Remove duplicates
		std::sort(components.begin(), components.end());
		components.erase(std::unique(components.begin(), components.end()), components.end());

		return components;
	}

	std::vector<std::string> ContextMenuUtils::GetCommonProperties(ECS::EntityID targetEntityId, const std::string& componentName) const {
		std::vector<std::string> commonProperties;

		if (!HasClipboardEntity() || !entityManager.IsEntityValid(targetEntityId)) {
			return commonProperties;
		}

		// Get component from clipboard
		nlohmann::json clipboardComponent = GetClipboardComponent(componentName);
		if (clipboardComponent.empty()) {
			return commonProperties;
		}

		// Get component from target entity
		ECS::ComponentTypeID componentId = entityManager.GetComponentTypeIdByName(componentName);
		if (componentId == ECS::MAX_COMPONENT_COUNT) {
			return commonProperties;
		}

		auto* component = entityManager.GetComponentByIdConst(targetEntityId, componentId);
		if (!component) {
			return commonProperties;
		}

		nlohmann::json targetComponentData = component->Serialize();

		// Find properties that exist in both
		for (auto it = clipboardComponent.begin(); it != clipboardComponent.end(); ++it) {
			std::string propName = it.key();

			// Skip metadata fields
			if (propName == "compName" || propName == "entityID" ||
				propName == "compCategory" || propName == "schema") {
				continue;
			}

			// Check if property exists in target
			if (targetComponentData.contains(propName)) {
				commonProperties.push_back(propName);
			}
		}

		return commonProperties;
	}

	// ===== PUBLIC METHOD TO GET CLIPBOARD DATA =====

	nlohmann::json ContextMenuUtils::GetClipboardData() const {
		const char* clipboardText = ImGui::GetClipboardText();
		if (!clipboardText) {
			return nlohmann::json();
		}

		try {
			return nlohmann::json::parse(std::string(clipboardText));
		}
		catch (...) {
			return nlohmann::json();
		}
	}

	// ===== PRIVATE HELPER FUNCTIONS =====

	nlohmann::json ContextMenuUtils::ParseImageMetadata(const std::string& imagePath) {
		nlohmann::json metadata;

		if (!std::filesystem::exists(imagePath)) {
			return metadata;
		}

		std::string extension = std::filesystem::path(imagePath).extension().string();
		std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

		if (extension == ".png") {
			FILE* fp = fopen(imagePath.c_str(), "rb");
			if (!fp) return metadata;

			png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
			if (!png) {
				fclose(fp);
				return metadata;
			}

			png_infop info = png_create_info_struct(png);
			if (!info) {
				png_destroy_read_struct(&png, nullptr, nullptr);
				fclose(fp);
				return metadata;
			}

			if (setjmp(png_jmpbuf(png))) {
				png_destroy_read_struct(&png, &info, nullptr);
				fclose(fp);
				return metadata;
			}

			png_init_io(png, fp);
			png_read_info(png, info);

			png_textp text_ptr;
			int num_text;
			if (png_get_text(png, info, &text_ptr, &num_text) > 0) {
				for (int i = 0; i < num_text; i++) {
					if (strcmp(text_ptr[i].key, "parameters") == 0) {
						try {
							metadata = nlohmann::json::parse(text_ptr[i].text);
							break;
						}
						catch (...) {
						}
					}
				}
			}

			png_destroy_read_struct(&png, &info, nullptr);
			fclose(fp);
		}

		return metadata;
	}

	std::vector<std::string> ContextMenuUtils::ExtractComponentsFromMetadata(const nlohmann::json& metadata) {
		std::vector<std::string> components;

		if (metadata.contains("components")) {
			if (metadata["components"].is_array()) {
				for (const auto& component : metadata["components"]) {
					if (component.is_object()) {
						for (auto it = component.begin(); it != component.end(); ++it) {
							if (it.key() != "compName") {
								components.push_back(it.key());
							}
						}
					}
				}
			}
			else if (metadata["components"].is_object()) {
				for (auto it = metadata["components"].begin(); it != metadata["components"].end(); ++it) {
					if (it.key() != "Base_Component") {
						components.push_back(it.key());
					}
				}
			}
		}

		return components;
	}

	void ContextMenuUtils::RenderMetadataComponentMenu(const nlohmann::json& metadata) {
		if (ImGui::BeginMenu("Copy from Metadata")) {
			if (ImGui::MenuItem("Copy Entire Entity")) {
				CopyEntityFromMetadata(metadata);
			}

			ImGui::Separator();

			std::vector<std::string> components = ExtractComponentsFromMetadata(metadata);

			if (components.empty()) {
				ImGui::TextDisabled("No components found");
			}
			else {
				for (const std::string& componentName : components) {
					if (ImGui::MenuItem(componentName.c_str())) {
						CopyComponentFromMetadata(metadata, componentName);
					}
				}
			}

			ImGui::EndMenu();
		}
	}

	void ContextMenuUtils::RenderEntityComponentMenu(ECS::EntityID entityId) {
		if (ImGui::BeginMenu("Copy from Entity")) {
			if (ImGui::MenuItem("Copy Entity")) {
				CopyEntity(entityId);
			}

			ImGui::Separator();

			auto componentIds = entityManager.GetEntityComponents(entityId);
			if (componentIds.empty()) {
				ImGui::TextDisabled("No components");
			}
			else {
				for (ECS::ComponentTypeID compId : componentIds) {
					std::string componentName = entityManager.GetComponentNameById(compId);
					if (ImGui::BeginMenu(componentName.c_str())) {
						if (ImGui::MenuItem("Copy Component")) {
							CopyComponent(entityId, compId);
						}
						ImGui::EndMenu();
					}
				}
			}

			ImGui::EndMenu();
		}
	}

	void ContextMenuUtils::RenderPasteMenu(ECS::EntityID entityId) {
		if (!HasClipboardEntity()) {
			ImGui::TextDisabled("Nothing to paste");
			return;
		}

		if (ImGui::BeginMenu("Paste")) {
			// Option 1: Paste entire entity
			if (ImGui::MenuItem("Paste Entity (Replace All)")) {
				PasteEntity(entityId);
			}

			// Option 2: Paste specific components
			std::vector<std::string> clipboardComponents = GetClipboardComponents();
			if (!clipboardComponents.empty()) {
				if (ImGui::BeginMenu("Paste Component")) {
					for (const std::string& componentName : clipboardComponents) {
						if (ImGui::MenuItem(componentName.c_str())) {
							PasteComponent(entityId, componentName);
						}
					}
					ImGui::EndMenu();
				}
			}

			// Option 3: Paste specific values - FIXED VERSION
			if (ImGui::BeginMenu("Paste Value")) {
				// Get clipboard data
				nlohmann::json clipboardData = GetClipboardData();

				if (clipboardData.contains("data") && clipboardData["data"].contains("components")) {
					const auto& components = clipboardData["data"]["components"];

					if (components.is_array()) {
						// Create a map to track components already shown
						std::set<std::string> shownComponents;

						// First pass: Show all components from clipboard
						for (const auto& component : components) {
							if (!component.is_object()) continue;

							for (auto it = component.begin(); it != component.end(); ++it) {
								std::string componentName = it.key();

								// Skip metadata fields
								if (componentName == "Base_Component" || componentName == "compName") {
									continue;
								}

								// Skip if already shown
								if (shownComponents.find(componentName) != shownComponents.end()) {
									continue;
								}
								shownComponents.insert(componentName);

								// Check if this component exists in target entity
								ECS::ComponentTypeID compId = entityManager.GetComponentTypeIdByName(componentName);
								bool hasComponent = (compId != ECS::MAX_COMPONENT_COUNT) &&
									entityManager.HasComponentById(entityId, compId);

								if (hasComponent) {
									// Get the component data
									const auto& compData = it.value();

									if (compData.is_object()) {
										// Create menu item for this component
										if (ImGui::BeginMenu(componentName.c_str())) {
											// Show all properties in this component
											for (auto propIt = compData.begin(); propIt != compData.end(); ++propIt) {
												std::string propName = propIt.key();

												// Skip internal fields
												if (propName == "compName" || propName == "entityID" ||
													propName == "compCategory" || propName == "schema") {
													continue;
												}

												// Get value for preview
												std::string valueStr;
												if (propIt.value().is_string()) {
													valueStr = propIt.value().get<std::string>();
												}
												else if (propIt.value().is_number()) {
													valueStr = std::to_string(propIt.value().get<double>());
												}
												else if (propIt.value().is_boolean()) {
													valueStr = propIt.value().get<bool>() ? "true" : "false";
												}
												else {
													valueStr = propIt.value().dump();
												}

												std::string label = propName;
												if (!valueStr.empty()) {
													if (valueStr.length() > 20) {
														valueStr = valueStr.substr(0, 17) + "...";
													}
													label += " (\"" + valueStr + "\")";
												}

												if (ImGui::MenuItem(label.c_str())) {
													PasteValue(entityId, componentName, propName);
												}
											}
											ImGui::EndMenu();
										}
									}
								}
							}
						}
					}
				}

				ImGui::EndMenu();
			}

			ImGui::EndMenu();
		}
	}

	void ContextMenuUtils::RenderValueCopyMenu(ECS::EntityID entityId, ECS::ComponentTypeID componentId, const std::string& componentName) {
		if (ImGui::BeginMenu("Copy Values")) {
			auto* component = entityManager.GetComponentByIdConst(entityId, componentId);
			if (!component) {
				ImGui::TextDisabled("No component");
			}
			else {
				nlohmann::json componentData = component->Serialize();

				for (auto it = componentData.begin(); it != componentData.end(); ++it) {
					std::string propName = it.key();

					if (propName == "compName" || propName == "entityID" ||
						propName == "compCategory" || propName == "schema") {
						continue;
					}

					std::string label = propName + ": ";
					if (it.value().is_string()) {
						std::string value = it.value().get<std::string>();
						if (value.length() > 30) {
							value = value.substr(0, 27) + "...";
						}
						label += value;
					}
					else if (it.value().is_number()) {
						std::stringstream ss;
						ss << it.value();
						label += ss.str();
					}

					if (ImGui::MenuItem(label.c_str())) {
						// Create minimal entity with just this value
						nlohmann::json entityData;
						entityData["dataType"] = "entity";
						entityData["data"] = nlohmann::json::object();
						entityData["data"]["components"] = nlohmann::json::array();

						nlohmann::json componentJson;
						componentJson[componentName] = nlohmann::json::object();
						componentJson[componentName][propName] = it.value();
						entityData["data"]["components"].push_back(componentJson);

						SetClipboardData(entityData);
					}
				}
			}

			ImGui::EndMenu();
		}
	}

	void ContextMenuUtils::RenderMetadataValueMenu(const nlohmann::json& metadata) {
		if (ImGui::BeginMenu("Copy Values from Metadata")) {
			std::vector<std::string> components = ExtractComponentsFromMetadata(metadata);

			if (components.empty()) {
				ImGui::TextDisabled("No components found");
			}
			else {
				for (const std::string& componentName : components) {
					if (ImGui::BeginMenu(componentName.c_str())) {

						if (metadata.contains("components")) {
							nlohmann::json componentData;

							if (metadata["components"].is_array()) {
								for (const auto& component : metadata["components"]) {
									if (component.contains(componentName)) {
										componentData = component[componentName];
										break;
									}
								}
							}
							else if (metadata["components"].is_object()) {
								if (metadata["components"].contains(componentName)) {
									componentData = metadata["components"][componentName];
								}
							}

							if (!componentData.empty() && componentData.is_object()) {
								for (auto it = componentData.begin(); it != componentData.end(); ++it) {
									std::string propName = it.key();

									if (propName == "compName" || propName == "entityID" ||
										propName == "compCategory" || propName == "schema") {
										continue;
									}

									std::string label = propName + ": ";
									if (it.value().is_string()) {
										std::string value = it.value().get<std::string>();
										if (value.length() > 30) {
											value = value.substr(0, 27) + "...";
										}
										label += value;
									}

									if (ImGui::MenuItem(label.c_str())) {
										// Create minimal entity with just this value
										nlohmann::json entityData;
										entityData["dataType"] = "entity";
										entityData["data"] = nlohmann::json::object();
										entityData["data"]["components"] = nlohmann::json::array();

										nlohmann::json componentJson;
										componentJson[componentName] = nlohmann::json::object();
										componentJson[componentName][propName] = it.value();
										entityData["data"]["components"].push_back(componentJson);

										SetClipboardData(entityData);
									}
								}
							}
						}

						ImGui::EndMenu();
					}
				}
			}

			ImGui::EndMenu();
		}
	}

	void ContextMenuUtils::CopyComponentFromMetadata(const nlohmann::json& metadata, const std::string& componentName) {
		nlohmann::json componentData;

		if (metadata.contains("components")) {
			if (metadata["components"].is_array()) {
				for (const auto& component : metadata["components"]) {
					if (component.contains(componentName)) {
						componentData = component[componentName];
						break;
					}
				}
			}
			else if (metadata["components"].is_object()) {
				if (metadata["components"].contains(componentName)) {
					componentData = metadata["components"][componentName];
				}
			}
		}

		if (!componentData.empty()) {
			nlohmann::json clipboardData;
			clipboardData["dataType"] = "component";
			clipboardData["componentName"] = componentName;
			clipboardData["componentData"] = componentData;
			clipboardData["source"] = "metadata";

			SetClipboardData(clipboardData);
		}
	}

	void ContextMenuUtils::CopyEntityFromMetadata(const nlohmann::json& metadata) {
		nlohmann::json clipboardData;
		clipboardData["dataType"] = "entity";
		clipboardData["data"] = metadata;
		clipboardData["source"] = "metadata";

		SetClipboardData(clipboardData);
	}

	// ===== CLIPBOARD OPERATIONS =====

	void ContextMenuUtils::SetClipboardData(const nlohmann::json& data) {
		std::string clipboardString = data.dump();
		ImGui::SetClipboardText(clipboardString.c_str());
	}

	std::string ContextMenuUtils::GetClipboardValue(const std::string& componentName, const std::string& propertyName) const {
		nlohmann::json componentData = GetClipboardComponent(componentName);
		if (componentData.empty() || !componentData.contains(propertyName)) {
			return "";
		}

		const auto& value = componentData[propertyName];
		if (value.is_string()) {
			return value.get<std::string>();
		}
		else if (value.is_number()) {
			return value.dump();
		}
		else if (value.is_boolean()) {
			return value.get<bool>() ? "true" : "false";
		}

		return "";
	}

	nlohmann::json ContextMenuUtils::GetClipboardComponent(const std::string& componentName) const {
		nlohmann::json result;

		if (!HasClipboardEntity()) {
			return result;
		}

		nlohmann::json clipboardData = GetClipboardData();
		if (!clipboardData.contains("data") || !clipboardData["data"].contains("components")) {
			return result;
		}

		const auto& components = clipboardData["data"]["components"];

		if (components.is_array()) {
			for (const auto& component : components) {
				if (component.contains(componentName)) {
					result = component[componentName];
					break;
				}
			}
		}

		return result;
	}

}