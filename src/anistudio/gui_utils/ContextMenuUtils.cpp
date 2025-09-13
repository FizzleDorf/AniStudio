#include "ContextMenuUtils.hpp"
#include "PngMetadataUtils.hpp"
#include "ImageComponent.hpp"
#include <png.h>
#include <filesystem>
#include <fstream>

namespace Utils {

	ContextMenuUtils::ContextMenuUtils(ECS::EntityManager& entityMgr)
		: entityManager(entityMgr) {
	}

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

			// Check if entity has image component with file path
			std::string imagePath;
			if (entityManager.HasComponent<ECS::ImageComponent>(entityId)) {
				const auto& imageComp = entityManager.GetComponent<ECS::ImageComponent>(entityId);
				imagePath = imageComp.filePath;
			}

			if (!imagePath.empty() && std::filesystem::exists(imagePath)) {
				// Parse metadata from image file
				nlohmann::json metadata = ParseImageMetadata(imagePath);
				if (!metadata.empty()) {
					RenderMetadataComponentMenu(metadata);
					ImGui::Separator();
				}
			}

			// Current entity operations
			RenderEntityComponentMenu(entityId);
			ImGui::Separator();

			// Paste operations
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
							// Invalid JSON, continue
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
				// Array format
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
				// Object format
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
			// Copy entire entity
			if (ImGui::MenuItem("Copy Entire Entity")) {
				CopyEntityFromMetadata(metadata);
			}

			ImGui::Separator();

			// Get components from metadata
			std::vector<std::string> components = ExtractComponentsFromMetadata(metadata);

			if (components.empty()) {
				ImGui::TextDisabled("No components found");
			}
			else {
				// Individual component copy options
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
			// Copy entire entity
			if (ImGui::MenuItem("Copy Entity")) {
				CopyEntity(entityId);
			}

			ImGui::Separator();

			// Individual components
			auto componentIds = entityManager.GetEntityComponents(entityId);
			if (componentIds.empty()) {
				ImGui::TextDisabled("No components");
			}
			else {
				for (ECS::ComponentTypeID compId : componentIds) {
					std::string componentName = entityManager.GetComponentNameById(compId);
					if (ImGui::MenuItem(componentName.c_str())) {
						CopyComponent(entityId, compId);
					}
				}
			}

			ImGui::EndMenu();
		}
	}

	void ContextMenuUtils::RenderPasteMenu(ECS::EntityID entityId) {
		if (!HasValidClipboardData()) {
			ImGui::TextDisabled("Nothing to paste");
			return;
		}

		if (ImGui::BeginMenu("Paste")) {
			nlohmann::json clipboardData = GetClipboardData();

			if (clipboardData.contains("dataType")) {
				std::string dataType = clipboardData["dataType"];

				if (dataType == "entity") {
					if (ImGui::MenuItem("Paste as New Entity")) {
						PasteEntity();
					}
					if (ImGui::MenuItem("Paste to This Entity")) {
						if (clipboardData.contains("data")) {
							entityManager.DeserializeEntity(clipboardData["data"], entityId);
						}
					}
				}
				else if (dataType == "component") {
					if (clipboardData.contains("componentName")) {
						std::string componentName = clipboardData["componentName"];
						if (ImGui::MenuItem(("Paste " + componentName).c_str())) {
							PasteComponent(entityId);
						}
					}
				}
			}

			ImGui::EndMenu();
		}
	}

	void ContextMenuUtils::CopyComponentFromMetadata(const nlohmann::json& metadata, const std::string& componentName) {
		nlohmann::json componentData;

		// Extract component data from metadata
		if (metadata.contains("components")) {
			if (metadata["components"].is_array()) {
				// Array format
				for (const auto& component : metadata["components"]) {
					if (component.contains(componentName)) {
						componentData = component[componentName];
						break;
					}
				}
			}
			else if (metadata["components"].is_object()) {
				// Object format
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

	bool ContextMenuUtils::PasteComponent(ECS::EntityID targetEntityId) {
		if (!CanPasteComponent() || !entityManager.IsEntityValid(targetEntityId)) {
			return false;
		}

		nlohmann::json clipboardData = GetClipboardData();
		if (!clipboardData.contains("componentName") || !clipboardData.contains("componentData")) {
			return false;
		}

		std::string componentName = clipboardData["componentName"];
		nlohmann::json componentData = clipboardData["componentData"];

		if (!IsComponentRegistered(componentName)) {
			return false;
		}

		ECS::ComponentTypeID componentId = entityManager.GetComponentTypeIdByName(componentName);
		bool hadComponent = entityManager.HasComponentById(targetEntityId, componentId);

		if (!hadComponent) {
			nlohmann::json tempEntityData = {
				{"components", {{componentName, nlohmann::json::object()}}}
			};
			entityManager.DeserializeEntity(tempEntityData, targetEntityId);
		}

		auto* component = entityManager.GetComponentById(targetEntityId, componentId);
		if (component) {
			component->Deserialize(componentData);
			return true;
		}

		return false;
	}

	bool ContextMenuUtils::PasteEntityToExisting(ECS::EntityID targetEntityId) {
		if (!CanPasteEntity() || !entityManager.IsEntityValid(targetEntityId)) {
			return false;
		}

		nlohmann::json clipboardData = GetClipboardData();
		if (!clipboardData.contains("data")) {
			return false;
		}

		entityManager.DeserializeEntity(clipboardData["data"], targetEntityId);
		return true;
	}

	bool ContextMenuUtils::CanPasteComponent() const {
		nlohmann::json clipboardData = GetClipboardData();
		return clipboardData.contains("dataType") && clipboardData["dataType"] == "component";
	}

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

	ECS::EntityID ContextMenuUtils::PasteEntity() {
		if (!CanPasteEntity()) {
			return 0;
		}

		nlohmann::json clipboardData = GetClipboardData();
		if (!clipboardData.contains("data")) {
			return 0;
		}

		return entityManager.DeserializeEntity(clipboardData["data"]);
	}

	bool ContextMenuUtils::CanPasteEntity() const {
		nlohmann::json clipboardData = GetClipboardData();
		return clipboardData.contains("dataType") && clipboardData["dataType"] == "entity";
	}

	bool ContextMenuUtils::HasValidClipboardData() const {
		nlohmann::json clipboardData = GetClipboardData();
		return clipboardData.contains("dataType");
	}

	std::string ContextMenuUtils::GetClipboardPreview() const {
		nlohmann::json clipboardData = GetClipboardData();
		if (!clipboardData.contains("dataType")) {
			return "No data";
		}

		std::string dataType = clipboardData["dataType"];
		if (dataType == "entity") {
			return "Entity";
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

	void ContextMenuUtils::SetClipboardData(const nlohmann::json& data) {
		std::string clipboardString = data.dump();
		ImGui::SetClipboardText(clipboardString.c_str());
	}

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

	bool ContextMenuUtils::IsComponentRegistered(const std::string& componentName) const {
		return entityManager.IsComponentNameRegistered(componentName);
	}

	// Legacy methods for DiffusionView compatibility
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

} // namespace Utils