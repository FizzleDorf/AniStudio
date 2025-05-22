#include "UpscaleView.hpp"
#include "../Events/Events.hpp"
#include "Constants.hpp"
#include "ImageUtils.hpp"
#include <filesystem>

using namespace ECS;
using namespace ANI;
using namespace Utils;

namespace GUI {

	UpscaleView::UpscaleView(EntityManager& entityMgr) : BaseView(entityMgr),
		isFilenameChanged(false) {
		viewName = "UpscaleView";
		strcpy(outDirPath, Utils::FilePaths::defaultProjectPath.c_str());
	}

	UpscaleView::~UpscaleView() {
		if (upscaleEntity != 0) {
			mgr.DestroyEntity(upscaleEntity);
		}
	}

	void UpscaleView::Init() {
		ResetEntity();
	}

	void UpscaleView::ResetEntity() {
		if (upscaleEntity != 0) {
			mgr.DestroyEntity(upscaleEntity);
			upscaleEntity = 0;
		}

		// Create a new entity for upscaling
		upscaleEntity = mgr.AddNewEntity();

		// Add components
		auto& inputComp = mgr.AddComponent<InputImageComponent>(upscaleEntity);
		auto& outputComp = mgr.AddComponent<OutputImageComponent>(upscaleEntity);
		auto& esrganComp = mgr.AddComponent<EsrganComponent>(upscaleEntity);
		auto& samplerComp = mgr.AddComponent<SamplerComponent>(upscaleEntity);

		// Initialize with default values
		outputComp.fileName = "upscaled_output.png";
		outputComp.filePath = Utils::FilePaths::defaultProjectPath;
		esrganComp.upscaleFactor = 2;
		esrganComp.preserveAspectRatio = true;
		samplerComp.n_threads = std::thread::hardware_concurrency();
	}
	void UpscaleView::Render() {
		ImGui::SetNextWindowSize(ImVec2(600, 800), ImGuiCond_FirstUseEver);
		if (ImGui::Begin("Image Upscaler")) {
			
			if (ImGui::CollapsingHeader("Metadata")) {
				RenderMetadataControls();
			}

			if (ImGui::CollapsingHeader("Input Image")) {
				RenderInputImageSelector(upscaleEntity);
			}

			if (ImGui::CollapsingHeader("Upscale Model")) {
				RenderModelSelector(upscaleEntity);
			}

			if (ImGui::CollapsingHeader("Upscale Parameters")) {
				RenderUpscaleParams(upscaleEntity);
			}

			if (ImGui::CollapsingHeader("Output Settings")) {
				RenderOutputSettings(upscaleEntity);
			}

			if (ImGui::CollapsingHeader("Processing Settings")) {
				RenderThreadsSettings(upscaleEntity);
			}

			// Run button
			ImGui::Separator();
			if (ImGui::Button("Run Upscale", ImVec2(-FLT_MIN, 0))) {
				HandleUpscaleEvent();
			}
		}
		ImGui::End();
	}

	void UpscaleView::RenderInputImageSelector(const EntityID entity) {
		if (!mgr.HasComponent<InputImageComponent>(entity)) {
			return;
		}

		auto& imageComp = mgr.GetComponent<InputImageComponent>(entity);

		// Image selection UI
		if (ImGui::BeginTable("InputImageTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp)) {
			ImGui::TableSetupColumn("Image", ImGuiTableColumnFlags_WidthFixed, 52.0f);
			ImGui::TableSetupColumn("Load", ImGuiTableColumnFlags_WidthFixed, 52.0f);
			ImGui::TableSetupColumn("Path", ImGuiTableColumnFlags_WidthStretch);

			ImGui::TableNextColumn();
			ImGui::Text("Input Image");

			ImGui::TableNextColumn();
			if (ImGui::Button("...##load_img123")) {
				IGFD::FileDialogConfig config;
				config.path = Utils::FilePaths::defaultProjectPath;
				ImGuiFileDialog::Instance()->OpenDialog("LoadInputImageDialog", "Choose Image",
					".png,.jpg,.jpeg,.bmp,.tga", config);
			}

			ImGui::SameLine();
			if (ImGui::Button("X##clear_img123")) {
				// Clear image
				if (imageComp.imageData) {
					ImageUtils::FreeImageData(imageComp.imageData);
					imageComp.imageData = nullptr;
				}
				if (imageComp.textureID != 0) {
					ImageUtils::DeleteTexture(imageComp.textureID);
					imageComp.textureID = 0;
				}
				imageComp.fileName = "";
				imageComp.filePath = "";
				imageComp.width = 0;
				imageComp.height = 0;
			}

			ImGui::TableNextColumn();
			ImGui::Text("%s", imageComp.fileName.c_str());

			ImGui::EndTable();
		}

		// File dialog for loading image
		if (ImGuiFileDialog::Instance()->Display("LoadInputImageDialog", 32, ImVec2(700, 400))) {
			if (ImGuiFileDialog::Instance()->IsOk()) {
				std::string filePath = ImGuiFileDialog::Instance()->GetFilePathName();
				std::string fileName = ImGuiFileDialog::Instance()->GetCurrentFileName();

				// Clean up previous image data if it exists
				if (imageComp.imageData) {
					ImageUtils::FreeImageData(imageComp.imageData);
					imageComp.imageData = nullptr;
				}
				if (imageComp.textureID != 0) {
					ImageUtils::DeleteTexture(imageComp.textureID);
					imageComp.textureID = 0;
				}

				// Load new image
				int width, height, channels;
				imageComp.imageData = ImageUtils::LoadImageData(filePath, width, height, channels);

				if (imageComp.imageData) {
					// Update component data
					imageComp.width = width;
					imageComp.height = height;
					imageComp.channels = channels;
					imageComp.fileName = fileName;
					imageComp.filePath = filePath;

					// Generate texture for preview
					imageComp.textureID = ImageUtils::GenerateTexture(
						imageComp.width, imageComp.height, imageComp.channels, imageComp.imageData);

					// Update output filename based on input
					if (mgr.HasComponent<OutputImageComponent>(entity)) {
						auto& outputComp = mgr.GetComponent<OutputImageComponent>(entity);
						std::filesystem::path inputPath(fileName);
						std::filesystem::path stem = inputPath.stem();
						outputComp.fileName = stem.string() + "_upscaled.png";
						isFilenameChanged = true;
					}
				}
			}
			ImGuiFileDialog::Instance()->Close();
		}

		// Image preview section
		ImGui::Separator();

		if (imageComp.textureID != 0 && imageComp.width > 0 && imageComp.height > 0) {
			ImGui::Text("Image dimensions: %d x %d", imageComp.width, imageComp.height);

			ImGui::BeginChild("ImagePreview", ImVec2(0, 300), true);

			float availWidth = ImGui::GetContentRegionAvail().x;
			float aspectRatio = static_cast<float>(imageComp.width) / static_cast<float>(imageComp.height);

			ImVec2 imageSize;
			if (aspectRatio > 1.0f) {
				imageSize = ImVec2(availWidth, availWidth / aspectRatio);
			}
			else {
				imageSize = ImVec2(availWidth * aspectRatio, availWidth);
			}

			float availHeight = ImGui::GetContentRegionAvail().y;
			if (imageSize.y > availHeight) {
				imageSize.y = availHeight;
				imageSize.x = availHeight * aspectRatio;
			}

			float xOffset = (availWidth - imageSize.x) * 0.5f;
			if (xOffset > 0) {
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + xOffset);
			}

			ImGui::Image((void*)(intptr_t)imageComp.textureID, imageSize);

			ImGui::EndChild();

			// Show estimated output dimensions
			if (mgr.HasComponent<EsrganComponent>(entity)) {
				auto& esrganComp = mgr.GetComponent<EsrganComponent>(entity);
				int outWidth = imageComp.width * esrganComp.upscaleFactor;
				int outHeight = imageComp.height * esrganComp.upscaleFactor;
				ImGui::Text("Estimated output: %d x %d", outWidth, outHeight);

				// Add option to use image dimensions for latent size
				if (ImGui::Button("Use image dimensions for output")) {
					if (mgr.HasComponent<LatentComponent>(entity)) {
						auto& latentComp = mgr.GetComponent<LatentComponent>(entity);
						// Make dimensions divisible by 8 (required for stable diffusion)
						latentComp.latentWidth = (imageComp.width / 8) * 8;
						latentComp.latentHeight = (imageComp.height / 8) * 8;
					}
				}
			}
		}
		else {
			ImGui::BeginChild("ImagePreview", ImVec2(0, 300), true);
			ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f),
				"No image loaded. Click the '...' button to select an input image.");
			ImGui::EndChild();
		}
	}

	void UpscaleView::RenderModelSelector(const EntityID entity) {
		if (!mgr.HasComponent<EsrganComponent>(entity)) {
			return;
		}

		auto& esrganComp = mgr.GetComponent<EsrganComponent>(entity);

		if (ImGui::BeginTable("ModelTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp)) {
			ImGui::TableSetupColumn("Model", ImGuiTableColumnFlags_WidthFixed, 52.0f);
			ImGui::TableSetupColumn("Load", ImGuiTableColumnFlags_WidthFixed, 52.0f);
			ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);

			ImGui::TableNextColumn();
			ImGui::Text("Upscaler");

			ImGui::TableNextColumn();
			if (ImGui::Button("...##LoadModel")) {
				IGFD::FileDialogConfig config;
				config.path = Utils::FilePaths::upscaleDir;
				ImGuiFileDialog::Instance()->OpenDialog(
					"LoadModelDialog",
					"Choose Upscale Model",
					".pth,.safetensors,.pt,.bin",
					config
				);
			}

			ImGui::SameLine();
			if (ImGui::Button("R##ResetModel123")) {
				esrganComp.modelName = "";
				esrganComp.modelPath = "";
			}

			ImGui::TableNextColumn();
			ImGui::Text("%s", esrganComp.modelName.c_str());

			ImGui::EndTable();
		}

		// Handle file dialog display and selection
		if (ImGuiFileDialog::Instance()->Display("LoadModelDialog", 32, ImVec2(700, 400))) {
			if (ImGuiFileDialog::Instance()->IsOk()) {
				std::string fullPath = ImGuiFileDialog::Instance()->GetFilePathName();
				std::string fileName = ImGuiFileDialog::Instance()->GetCurrentFileName();

				esrganComp.modelPath = fullPath;
				esrganComp.modelName = fileName;
				std::cout << "Selected model: " << fullPath << std::endl;
			}
			ImGuiFileDialog::Instance()->Close();
		}
	}

	void UpscaleView::RenderUpscaleParams(const EntityID entity) {
		if (!mgr.HasComponent<EsrganComponent>(entity)) {
			return;
		}

		auto& esrganComp = mgr.GetComponent<EsrganComponent>(entity);

		// Upscale factor
		ImGui::Text("Upscale Factor:");
		ImGui::SameLine();
		int factor = esrganComp.upscaleFactor;
		if (ImGui::RadioButton("2x", factor == 2)) factor = 2;
		ImGui::SameLine();
		if (ImGui::RadioButton("3x", factor == 3)) factor = 3;
		ImGui::SameLine();
		if (ImGui::RadioButton("4x", factor == 4)) factor = 4;
		esrganComp.upscaleFactor = factor;

		// Preserve aspect ratio
		ImGui::Checkbox("Preserve Aspect Ratio", &esrganComp.preserveAspectRatio);

		// Display memory estimate
		if (mgr.HasComponent<InputImageComponent>(entity)) {
			auto& inputComp = mgr.GetComponent<InputImageComponent>(entity);
			if (inputComp.width > 0 && inputComp.height > 0) {
				int outWidth = inputComp.width * esrganComp.upscaleFactor;
				int outHeight = inputComp.height * esrganComp.upscaleFactor;
				float memoryMB = (outWidth * outHeight * 4) / (1024.0f * 1024.0f); // Assuming 4 bytes per pixel
				ImGui::Text("Estimated memory usage: %.1f MB", memoryMB);
			}
		}
	}

	void UpscaleView::RenderOutputSettings(const EntityID entity) {
		if (!mgr.HasComponent<OutputImageComponent>(entity)) {
			return;
		}

		auto& outputComp = mgr.GetComponent<OutputImageComponent>(entity);

		// Initialize buffer with current filename if changed
		if (isFilenameChanged) {
			strncpy(outFileName, outputComp.fileName.c_str(), sizeof(outFileName) - 1);
			outFileName[sizeof(outFileName) - 1] = '\0';  // Ensure null termination

			strncpy(outDirPath, outputComp.filePath.c_str(), sizeof(outDirPath) - 1);
			outDirPath[sizeof(outDirPath) - 1] = '\0';  // Ensure null termination

			isFilenameChanged = false;
		}

		// Filename input
		if (ImGui::BeginTable("OutputNameTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp)) {
			ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 54.0f);
			ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::Text("Filename");

			ImGui::TableNextColumn();
			if (ImGui::InputText("##OutFilename", outFileName, IM_ARRAYSIZE(outFileName))) {
				outputComp.fileName = outFileName;
				isFilenameChanged = true;
			}

			ImGui::EndTable();
		}

		// File extension selector
		const char* extensions[] = { ".png", ".jpg", ".jpeg", ".bmp", ".tga", ".webp" };

		if (ImGui::BeginTable("Extension", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp)) {
			ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 54.0f);
			ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

			ImGui::TableNextColumn();
			ImGui::Text("Format");

			ImGui::TableNextColumn();
			if (ImGui::BeginCombo("##ExtensionCombo", currentExtension.c_str())) {
				for (int i = 0; i < IM_ARRAYSIZE(extensions); i++) {
					bool isSelected = (currentExtensionIndex == i);
					if (ImGui::Selectable(extensions[i], isSelected)) {
						currentExtensionIndex = i;
						currentExtension = extensions[i];
						isFilenameChanged = true; // Trigger update of the file extension
					}
					if (isSelected) {
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}

			ImGui::EndTable();
		}

		// Directory selector
		if (ImGui::BeginTable("OutputDirTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp)) {
			ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 54.0f);
			ImGui::TableSetupColumn("Load", ImGuiTableColumnFlags_WidthFixed, 52.0f);
			ImGui::TableSetupColumn("Path", ImGuiTableColumnFlags_WidthStretch);

			ImGui::TableNextColumn();
			ImGui::Text("Dir Path");

			ImGui::TableNextColumn();
			if (ImGui::Button("...##ChooseDir123123")) {
				IGFD::FileDialogConfig config;
				config.path = Utils::FilePaths::defaultProjectPath;
				ImGuiFileDialog::Instance()->OpenDialog("ChooseDirDialog", "Choose Output Directory", nullptr, config);
			}

			ImGui::SameLine();
			if (ImGui::Button("R##ResetDir123123")) {
				outputComp.filePath = Utils::FilePaths::defaultProjectPath;
				strncpy(outDirPath, outputComp.filePath.c_str(), sizeof(outDirPath) - 1);
				outDirPath[sizeof(outDirPath) - 1] = '\0';  // Ensure null termination
			}

			ImGui::TableNextColumn();
			ImGui::Text("%s", outputComp.filePath.c_str());

			ImGui::EndTable();
		}

		// Handle directory selection dialog
		if (ImGuiFileDialog::Instance()->Display("ChooseDirDialog", 32, ImVec2(700, 400))) {
			if (ImGuiFileDialog::Instance()->IsOk()) {
				std::string selectedDir = ImGuiFileDialog::Instance()->GetCurrentPath();
				if (!selectedDir.empty()) {
					outputComp.filePath = selectedDir;
					strncpy(outDirPath, selectedDir.c_str(), sizeof(outDirPath) - 1);
					outDirPath[sizeof(outDirPath) - 1] = '\0';  // Ensure null termination
					isFilenameChanged = true;
				}
			}
			ImGuiFileDialog::Instance()->Close();
		}

		// Update filename with extension if changed
		if (isFilenameChanged) {
			std::string newFileName = outputComp.fileName;
			if (!newFileName.empty()) {
				std::filesystem::path filePath(newFileName);

				// If the extension is missing or different from the selected one, update it
				std::string fileExtension = filePath.has_extension() ?
					filePath.extension().string() : "";

				if (fileExtension != currentExtension) {
					filePath.replace_extension(currentExtension);
					outputComp.fileName = filePath.string();
					strncpy(outFileName, outputComp.fileName.c_str(), sizeof(outFileName) - 1);
					outFileName[sizeof(outFileName) - 1] = '\0';  // Ensure null termination
				}
			}
			isFilenameChanged = false;
		}
	}

	void UpscaleView::RenderThreadsSettings(const EntityID entity) {
		if (!mgr.HasComponent<SamplerComponent>(entity)) {
			return;
		}

		auto& samplerComp = mgr.GetComponent<SamplerComponent>(entity);

		ImGui::Text("Processing Threads:");
		ImGui::SliderInt("##Threads", &samplerComp.n_threads, 1, std::thread::hardware_concurrency(),
			"%d threads");

		// CPU thread info
		ImGui::Text("System has %d logical processors", std::thread::hardware_concurrency());

		// Display a warning if too many threads are selected
		if (samplerComp.n_threads > std::thread::hardware_concurrency() / 2) {
			ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.0f, 1.0f),
				"Warning: Using too many threads may degrade system performance");
		}
	}

	void UpscaleView::HandleUpscaleEvent() {
		std::cout << "Creating entity for upscaling..." << std::endl;

		// Create a new entity for processing by cloning the current entity
		EntityID newEntity = mgr.DeserializeEntity(mgr.SerializeEntity(upscaleEntity));

		if (newEntity == 0) {
			std::cerr << "Failed to create new entity!" << std::endl;
			mgr.DestroyEntity(newEntity);
			return;
		}

		std::cout << "Initialized entity with ID: " << newEntity << std::endl;

		// Ensure the Output component exists and is properly set up
		if (!mgr.HasComponent<OutputImageComponent>(upscaleEntity)) {
			std::cerr << "Failed to find output path!" << std::endl;
			mgr.DestroyEntity(newEntity);
			return;
		}

		// Copy the original output component to the new entity
		mgr.GetComponent<OutputImageComponent>(newEntity) = mgr.GetComponent<OutputImageComponent>(upscaleEntity);
		mgr.GetComponent<InputImageComponent>(newEntity) = mgr.GetComponent<InputImageComponent>(upscaleEntity);

		// Queue event for upscaling
		Event event;
		event.entityID = newEntity;
		event.type = EventType::UpscaleRequest;
		ANI::Events::Ref().QueueEvent(event);

		std::cout << "Upscaling request queued for entity: " << newEntity << std::endl;
	}

	void UpscaleView::RenderMetadataControls() {
		if (ImGui::Button("Save Settings##2d", ImVec2(-FLT_MIN, 0))) {
			IGFD::FileDialogConfig config;
			config.path = Utils::FilePaths::defaultProjectPath;
			ImGuiFileDialog::Instance()->OpenDialog("SaveMetadataDialog", "Save Settings", ".json", config);
		}

		if (ImGui::Button("Load Settings##r1", ImVec2(-FLT_MIN, 0))) {
			IGFD::FileDialogConfig config;
			config.path = Utils::FilePaths::defaultProjectPath;
			ImGuiFileDialog::Instance()->OpenDialog("LoadMetadataDialog", "Load Settings", ".json", config);
		}

		// Handle Save Dialog
		if (ImGuiFileDialog::Instance()->Display("SaveMetadataDialog", 32, ImVec2(700, 400))) {
			if (ImGuiFileDialog::Instance()->IsOk()) {
				std::string filepath = ImGuiFileDialog::Instance()->GetFilePathName();
				SaveMetadataToJson(filepath);
			}
			ImGuiFileDialog::Instance()->Close();
		}

		// Handle Load Dialog
		if (ImGuiFileDialog::Instance()->Display("LoadMetadataDialog", 32, ImVec2(700, 400))) {
			if (ImGuiFileDialog::Instance()->IsOk()) {
				std::string filepath = ImGuiFileDialog::Instance()->GetFilePathName();
				LoadMetadataFromJson(filepath);
			}
			ImGuiFileDialog::Instance()->Close();
		}
	}

	nlohmann::json UpscaleView::Serialize() const {
		if (upscaleEntity == 0) {
			return nlohmann::json();
		}

		return mgr.SerializeEntity(upscaleEntity);
	}

	void UpscaleView::Deserialize(const nlohmann::json& j) {
		try {
			// Create a temporary entity with the deserialized data
			EntityID tempEntity = mgr.DeserializeEntity(j);

			// Copy components from the temporary entity to our upscale entity
			if (mgr.HasComponent<InputImageComponent>(tempEntity))
				mgr.GetComponent<InputImageComponent>(upscaleEntity) = mgr.GetComponent<InputImageComponent>(tempEntity);

			if (mgr.HasComponent<OutputImageComponent>(tempEntity))
				mgr.GetComponent<OutputImageComponent>(upscaleEntity) = mgr.GetComponent<OutputImageComponent>(tempEntity);

			if (mgr.HasComponent<EsrganComponent>(tempEntity))
				mgr.GetComponent<EsrganComponent>(upscaleEntity) = mgr.GetComponent<EsrganComponent>(tempEntity);

			if (mgr.HasComponent<SamplerComponent>(tempEntity))
				mgr.GetComponent<SamplerComponent>(upscaleEntity) = mgr.GetComponent<SamplerComponent>(tempEntity);

			// Clean up the temporary entity
			mgr.DestroyEntity(tempEntity);

			isFilenameChanged = true; // Trigger UI update
			std::cout << "Successfully deserialized data to entity " << upscaleEntity << std::endl;
		}
		catch (const std::exception& e) {
			std::cerr << "Exception during deserialization: " << e.what() << std::endl;
		}
	}

	void UpscaleView::SaveMetadataToJson(const std::string& filepath) {
		try {
			nlohmann::json metadata = Serialize();
			std::ofstream file(filepath);
			if (file.is_open()) {
				file << metadata.dump(4); // Pretty print with 4-space indentation
				file.close();
				std::cout << "Settings saved to: " << filepath << std::endl;
			}
			else {
				std::cerr << "Failed to open file for writing: " << filepath << std::endl;
			}
		}
		catch (const std::exception& e) {
			std::cerr << "Error saving settings: " << e.what() << std::endl;
		}
	}

	void UpscaleView::LoadMetadataFromJson(const std::string& filepath) {
		try {
			std::ifstream file(filepath);
			if (file.is_open()) {
				nlohmann::json metadata;
				file >> metadata;
				Deserialize(metadata);
				file.close();
				std::cout << "Settings loaded from: " << filepath << std::endl;
			}
			else {
				std::cerr << "Failed to open file for reading: " << filepath << std::endl;
			}
		}
		catch (const std::exception& e) {
			std::cerr << "Error loading settings: " << e.what() << std::endl;
		}
	}

} // namespace GUI