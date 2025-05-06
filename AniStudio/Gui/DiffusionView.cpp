#include "DiffusionView.hpp"
#include "../Events/Events.hpp"
#include "Constants.hpp"
#include "UISchema.hpp"
#include <exiv2/exiv2.hpp>

using namespace ECS;
using namespace ANI;

static void LogCallback(sd_log_level_t level, const char* text, void* data) {
	switch (level) {
	case SD_LOG_DEBUG:
		std::cout << "[DEBUG]: " << text;
		break;
	case SD_LOG_INFO:
		std::cout << "[INFO]: " << text;
		break;
	case SD_LOG_WARN:
		std::cout << "[WARNING]: " << text;
		break;
	case SD_LOG_ERROR:
		std::cerr << "[ERROR]: " << text;
		break;
	default:
		std::cerr << "[UNKNOWN LOG LEVEL]: " << text;
		break;
	}
}
static ProgressData progressData;
static void ProgressCallback(int step, int steps, float time, void* data) {
	progressData.currentStep = step;
	progressData.totalSteps = steps;
	progressData.currentTime = time;
	progressData.isProcessing = (steps > 0);
	std::cout << "Progress: Step " << step << " of " << steps << " | Time: " << time << "s" << std::endl;
}

namespace GUI {

	DiffusionView::DiffusionView(EntityManager& entityMgr) : BaseView(entityMgr) {
		viewName = "DiffusionView";
	}

	DiffusionView::~DiffusionView() {
		if(txt2imgEntity != 0)
			mgr.DestroyEntity(txt2imgEntity);
		if (img2imgEntity != 0)
			mgr.DestroyEntity(img2imgEntity);
	}

	void DiffusionView::Init() {
		sd_set_log_callback(LogCallback, nullptr);
		sd_set_progress_callback(ProgressCallback, nullptr);
		ResetEntities();
	}

	void DiffusionView::ResetEntities() {
		
		if (txt2imgEntity != 0) {
			mgr.DestroyEntity(txt2imgEntity);
			txt2imgEntity = 0;
		}
			
		if (img2imgEntity != 0) {
			mgr.DestroyEntity(img2imgEntity);
			img2imgEntity = 0;
		}
			
		// Create entity for Txt2Img mode
		txt2imgEntity = mgr.AddNewEntity();

		// Add components for Txt2Img
		mgr.AddComponent<ModelComponent>(txt2imgEntity);
		mgr.AddComponent<ClipLComponent>(txt2imgEntity);
		mgr.AddComponent<ClipGComponent>(txt2imgEntity);
		mgr.AddComponent<T5XXLComponent>(txt2imgEntity);
		mgr.AddComponent<DiffusionModelComponent>(txt2imgEntity);
		mgr.AddComponent<VaeComponent>(txt2imgEntity);
		mgr.AddComponent<LoraComponent>(txt2imgEntity);
		mgr.AddComponent<TaesdComponent>(txt2imgEntity);
		mgr.AddComponent<LatentComponent>(txt2imgEntity);
		mgr.AddComponent<SamplerComponent>(txt2imgEntity);
		mgr.AddComponent<GuidanceComponent>(txt2imgEntity);
		mgr.AddComponent<ClipSkipComponent>(txt2imgEntity);
		mgr.AddComponent<PromptComponent>(txt2imgEntity);
		mgr.AddComponent<LayerSkipComponent>(txt2imgEntity);
		mgr.AddComponent<OutputImageComponent>(txt2imgEntity);

		// Create entity for Img2Img mode 
		img2imgEntity = mgr.AddNewEntity();

		// Add components for Img2Img
		mgr.AddComponent<ModelComponent>(img2imgEntity);
		mgr.AddComponent<ClipLComponent>(img2imgEntity);
		mgr.AddComponent<ClipGComponent>(img2imgEntity);
		mgr.AddComponent<T5XXLComponent>(img2imgEntity);
		mgr.AddComponent<DiffusionModelComponent>(img2imgEntity);
		mgr.AddComponent<VaeComponent>(img2imgEntity);
		mgr.AddComponent<LoraComponent>(img2imgEntity);
		mgr.AddComponent<TaesdComponent>(img2imgEntity);
		mgr.AddComponent<LatentComponent>(img2imgEntity);
		mgr.AddComponent<SamplerComponent>(img2imgEntity);
		mgr.AddComponent<GuidanceComponent>(img2imgEntity);
		mgr.AddComponent<ClipSkipComponent>(img2imgEntity);
		mgr.AddComponent<PromptComponent>(img2imgEntity);
		mgr.AddComponent<LayerSkipComponent>(img2imgEntity);
		mgr.AddComponent<OutputImageComponent>(img2imgEntity);
		mgr.AddComponent<InputImageComponent>(img2imgEntity);

		// Default denoise value for Img2Img
		mgr.GetComponent<SamplerComponent>(img2imgEntity).denoise = 0.6f;
	}

	void DiffusionView::RenderModelLoader(const EntityID entity) {
		if (!mgr.HasComponent<ModelComponent>(entity) ||
			!mgr.HasComponent<SamplerComponent>(entity)) {
			return;
		}

		auto& modelComp = mgr.GetComponent<ModelComponent>(entity);
		auto& samplerComp = mgr.GetComponent<SamplerComponent>(entity);

		int current_type = static_cast<int>(samplerComp.current_type_method);
		ImGui::Combo("Quant Type", &current_type, type_method_items, type_method_item_count);
		samplerComp.current_type_method = static_cast<decltype(samplerComp.current_type_method)>(current_type);

		if (ImGui::BeginTable("ModelLoaderTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp)) {
			ImGui::TableSetupColumn("Model", ImGuiTableColumnFlags_WidthFixed, 52.0f);
			ImGui::TableSetupColumn("Load", ImGuiTableColumnFlags_WidthFixed, 52.0f);
			ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
			// ImGui::TableHeadersRow();

			// Row for "Checkpoint"
			ImGui::TableNextColumn();
			ImGui::Text("Model");
			ImGui::TableNextColumn();
			if (ImGui::Button("...##j6")) {
				IGFD::FileDialogConfig config;
				config.path = filePaths.checkpointDir;
				ImGuiFileDialog::Instance()->OpenDialog("LoadFileDialog", "Choose Model", ".safetensors, .ckpt, .pt, .gguf",
					config);
			}
			ImGui::SameLine();
			if (ImGui::Button("R##j6")) {
				modelComp.modelName = "";
				modelComp.modelPath = "";
			}
			ImGui::TableNextColumn();
			ImGui::Text("%s", modelComp.modelName.c_str());

			RenderVaeLoader(entity);
		}

		if (ImGuiFileDialog::Instance()->Display("LoadFileDialog", 32, ImVec2(700, 400))) {
			if (ImGuiFileDialog::Instance()->IsOk()) {
				std::string selectedFile = ImGuiFileDialog::Instance()->GetCurrentFileName();
				std::string fullPath = ImGuiFileDialog::Instance()->GetFilePathName();

				modelComp.modelName = selectedFile;
				modelComp.modelPath = fullPath;
				std::cout << "Selected file: " << modelComp.modelName << std::endl;
				std::cout << "Full path: " << modelComp.modelPath << std::endl;
				std::cout << "New model path set: " << modelComp.modelPath << std::endl;
			}

			ImGuiFileDialog::Instance()->Close();
		}
	}

	static char fileName[256] = "AniStudio"; // Buffer for file name
	static char outputDir[256] = "";         // Buffer for output directory

	void DiffusionView::RenderFilePath(const EntityID entity) {
		if (!mgr.HasComponent<OutputImageComponent>(entity)) {
			return;
		}

		auto& imageComp = mgr.GetComponent<OutputImageComponent>(entity);

		if (ImGui::BeginTable("Output Name", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp)) {
			ImGui::TableSetupColumn("Param", ImGuiTableColumnFlags_WidthFixed, 54.0f);
			ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
			// ImGui::TableHeadersRow();
			ImGui::TableNextColumn();
			ImGui::Text("FileName"); // Row for "Filename"
			ImGui::TableNextColumn();
			if (ImGui::InputText("##Filename", fileName, IM_ARRAYSIZE(fileName))) {
				isFilenameChanged = true;
			}
			ImGui::EndTable();
		}
		if (ImGui::BeginTable("Output Dir", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp)) {
			ImGui::TableSetupColumn("Param", ImGuiTableColumnFlags_WidthFixed, 54.0f); // Fixed width for Model
			ImGui::TableSetupColumn("Load", ImGuiTableColumnFlags_WidthFixed, 52.0f);
			ImGui::TableSetupColumn("Path", ImGuiTableColumnFlags_WidthStretch);
			// ImGui::TableHeadersRow();
			ImGui::TableNextColumn();
			ImGui::Text("Dir Path"); // Row for "Output Directory"
			ImGui::TableNextColumn();
			if (ImGui::Button("...##w8")) {
				IGFD::FileDialogConfig config;
				config.path = filePaths.defaultProjectPath; // Set the initial directory
				ImGuiFileDialog::Instance()->OpenDialog("LoadDirDialog", "Choose Directory", nullptr, config);
			}
			ImGui::SameLine();
			if (ImGui::Button("R##w9")) {
				imageComp.fileName = "AniStudio";
				imageComp.filePath = "filePaths.defaultProjectPath";
			}
			ImGui::TableNextColumn();
			ImGui::Text("%s", imageComp.filePath.c_str());

			ImGui::EndTable();
		}

		// Handle the file dialog display and selection
		if (ImGuiFileDialog::Instance()->Display("LoadDirDialog", 32, ImVec2(700, 400))) {
			if (ImGuiFileDialog::Instance()->IsOk()) {
				std::string selectedDir = ImGuiFileDialog::Instance()->GetCurrentPath();
				if (!selectedDir.empty()) {
					strncpy(outputDir, selectedDir.c_str(), IM_ARRAYSIZE(outputDir) - 1);
					outputDir[IM_ARRAYSIZE(outputDir) - 1] = '\0'; // Null termination
					imageComp.filePath = selectedDir;
					isFilenameChanged = true;
				}
			}
			ImGuiFileDialog::Instance()->Close();
		}

		// Update ImageComponent properties if filename or filepath changes
		if (isFilenameChanged) {
			std::string newFileName(fileName);

			// Ensure the file name has a .png extension
			if (!newFileName.empty()) {
				std::filesystem::path filePath(newFileName);
				if (filePath.extension() != ".png") {
					filePath.replace_extension(".png");
					newFileName = filePath.filename().string();
				}
			}

			// Update the ImageComponent
			if (!newFileName.empty()) {
				imageComp.fileName = newFileName;
				std::cout << "ImageComponent updated:\n";
				std::cout << "  FileName: " << imageComp.fileName << '\n';
				std::cout << "  FilePath: " << imageComp.filePath << '\n';
			}
			else {
				std::cerr << "Invalid directory or filename!" << '\n';
			}

			isFilenameChanged = false; // Reset the flag
		}
	}

	void DiffusionView::RenderLatents(const EntityID entity) {
		if (!mgr.HasComponent<LatentComponent>(entity) ||
			!mgr.HasComponent<SamplerComponent>(entity)) {
			return;
		}

		auto& latentComp = mgr.GetComponent<LatentComponent>(entity);
		auto& samplerComp = mgr.GetComponent<SamplerComponent>(entity);

		int current_rng = static_cast<int>(samplerComp.current_rng_type);
		ImGui::Combo("RNG Type", &current_rng, type_rng_items, type_rng_item_count);
		samplerComp.current_rng_type = static_cast<decltype(samplerComp.current_rng_type)>(current_rng);

		if (ImGui::BeginTable("PromptTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp)) {
			ImGui::TableSetupColumn("Param", ImGuiTableColumnFlags_WidthFixed, 52.0f);
			ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::Text("Width");
			ImGui::TableNextColumn();
			ImGui::InputInt("##Width", &latentComp.latentWidth, 8, 8);
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::Text("Height");
			ImGui::TableNextColumn();
			ImGui::InputInt("##Height", &latentComp.latentHeight, 8, 8);
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			// Batching using sdcpp just queues, does not batch asyncronously
			// ImGui::Text("Batch");
			// ImGui::TableNextColumn();
			// ImGui::InputInt("##Batch Size", &latentComp.batchSize);
			ImGui::EndTable();
			if(ImGui::Button("Swap XY")) {
				int temp = latentComp.latentWidth;
				latentComp.latentWidth = latentComp.latentHeight;
				latentComp.latentHeight = temp;
			}
		}
	}

	void DiffusionView::RenderInputImage(const EntityID entity) {
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
			if (ImGui::Button("...##load_img")) {
				IGFD::FileDialogConfig config;
				config.path = filePaths.defaultProjectPath;
				ImGuiFileDialog::Instance()->OpenDialog("LoadInputImageDialog", "Choose Image",
					".png,.jpg,.jpeg,.bmp,.tga", config);
			}

			ImGui::SameLine();
			if (ImGui::Button("X##clear_img")) {
				// Clear image
				if (imageComp.imageData) {
					Utils::ImageUtils::FreeImageData(imageComp.imageData);
					imageComp.imageData = nullptr;
				}
				if (imageComp.textureID != 0) {
					Utils::ImageUtils::DeleteTexture(imageComp.textureID);
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
					Utils::ImageUtils::FreeImageData(imageComp.imageData);
					imageComp.imageData = nullptr;
				}
				if (imageComp.textureID != 0) {
					Utils::ImageUtils::DeleteTexture(imageComp.textureID);
					imageComp.textureID = 0;
				}

				// Load new image
				int width, height, channels;
				imageComp.imageData = Utils::ImageUtils::LoadImageData(filePath, width, height, channels);

				if (imageComp.imageData) {
					// Update component data
					imageComp.width = width;
					imageComp.height = height;
					imageComp.channels = channels;
					imageComp.fileName = fileName;
					imageComp.filePath = filePath;

					// Generate texture for preview
					imageComp.textureID = Utils::ImageUtils::GenerateTexture(
						imageComp.width, imageComp.height, imageComp.channels, imageComp.imageData);

					// Update latent dimensions to match input image
					if (mgr.HasComponent<LatentComponent>(entity)) {
						auto& latentComp = mgr.GetComponent<LatentComponent>(entity);
						// Make dimensions divisible by 8 (required for stable diffusion)
						latentComp.latentWidth = (width / 8) * 8;
						latentComp.latentHeight = (height / 8) * 8;
					}
				}
			}
			ImGuiFileDialog::Instance()->Close();
		}

		// Image preview section
		ImGui::Separator();

		if (imageComp.textureID != 0 && imageComp.width > 0 && imageComp.height > 0) {
			// Display image info
			ImGui::Text("Image dimensions: %d x %d", imageComp.width, imageComp.height);

			// Create a child window for the image preview with border
			ImGui::BeginChild("ImagePreview", ImVec2(0, 300), true);

			// Calculate image size to maintain aspect ratio
			float availWidth = ImGui::GetContentRegionAvail().x;
			float aspectRatio = static_cast<float>(imageComp.width) / static_cast<float>(imageComp.height);

			ImVec2 imageSize;
			if (aspectRatio > 1.0f) {
				// Image is wider than tall
				imageSize = ImVec2(availWidth, availWidth / aspectRatio);
			}
			else {
				// Image is taller than wide or square
				imageSize = ImVec2(availWidth * aspectRatio, availWidth);
			}

			// Limit height to available space
			float availHeight = ImGui::GetContentRegionAvail().y;
			if (imageSize.y > availHeight) {
				imageSize.y = availHeight;
				imageSize.x = availHeight * aspectRatio;
			}

			// Center the image horizontally
			float xOffset = (availWidth - imageSize.x) * 0.5f;
			if (xOffset > 0) {
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + xOffset);
			}

			// Draw the image
			ImGui::Image((void*)(intptr_t)imageComp.textureID, imageSize);

			ImGui::EndChild();
		}
		else {
			ImGui::BeginChild("ImagePreview", ImVec2(0, 300), true);
			ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f),
				"No image loaded. Click the '...' button to select an input image.");
			ImGui::EndChild();
		}

		// Additional image processing options
		if (imageComp.textureID != 0) {
			if (ImGui::Button("Use image dimensions for latent size")) {
				if (mgr.HasComponent<LatentComponent>(entity)) {
					auto& latentComp = mgr.GetComponent<LatentComponent>(entity);
					// Make dimensions divisible by 8 (required for stable diffusion)
					latentComp.latentWidth = (imageComp.width / 8) * 8;
					latentComp.latentHeight = (imageComp.height / 8) * 8;
				}
			}
		}
	}

	void DiffusionView::RenderPrompts(const EntityID entity) {

		if (!mgr.HasComponent<PromptComponent>(entity)) {
			return;
		}

		auto& promptComp = mgr.GetComponent<PromptComponent>(entity);

		// Get the property map directly from the component
		auto properties = promptComp.GetPropertyMap();

		// Use the component's schema to render the UI
		if (UISchema::RenderSchema(promptComp.schema, properties)) {
		}
	}

	void DiffusionView::RenderSampler(const EntityID entity) {
		if (!mgr.HasComponent<SamplerComponent>(entity)) {
			return;
		}

		auto& samplerComp = mgr.GetComponent<SamplerComponent>(entity);

		// Get the property map directly from the component
		auto properties = samplerComp.GetPropertyMap();

		// Use the component's schema to render the UI
		if (UISchema::RenderSchema(samplerComp.schema, properties)) {
		}
	}

	//void DiffusionView::RenderClipSkip() {
	//	// Get the property map directly from the component
	//	auto properties = clipSkipComp.GetPropertyMap();

	//	// Use the component's schema to render the UI
	//	if (UISchema::RenderSchema(clipSkipComp.schema, properties)) {
	//	}
	//}

	void DiffusionView::HandleT2IEvent() {
		std::cout << "Adding new entity..." << std::endl;
		EntityID newEntity = mgr.DeserializeEntity(mgr.SerializeEntity(txt2imgEntity));
		if (newEntity == 0) {
			std::cerr << "Failed to create new entity!" << std::endl;
			return;
		}

		// Queue event
		Event event;
		event.entityID = newEntity;
		event.type = EventType::InferenceRequest;
		ANI::Events::Ref().QueueEvent(event);
		std::cout << "Inference request queued for entity: " << newEntity << std::endl;
	}

	void DiffusionView::HandleI2IEvent() {
		std::cout << "Creating new entity for img2img..." << std::endl;

		// Create a new entity
		EntityID newEntity = mgr.DeserializeEntity(mgr.SerializeEntity(txt2imgEntity));
		if (newEntity == 0) {
			std::cerr << "Failed to create new entity!" << std::endl;
			return;
		}

		// Copy all components from the template entity using direct component copying
		// instead of serialization/deserialization
		std::vector<ComponentTypeID> componentTypes = mgr.GetEntityComponents(img2imgEntity);

		for (const auto& componentId : componentTypes) {
			std::string componentName = mgr.GetComponentNameById(componentId);
			std::cout << "Copying component: " << componentName << " to new entity" << std::endl;

			// Handle each component type specifically
			if (componentName == "Model") {
				mgr.AddComponent<ModelComponent>(newEntity) = mgr.GetComponent<ModelComponent>(img2imgEntity);
			}
			else if (componentName == "CLipL") {
				mgr.AddComponent<ClipLComponent>(newEntity) = mgr.GetComponent<ClipLComponent>(img2imgEntity);
			}
			else if (componentName == "CLipG") {
				mgr.AddComponent<ClipGComponent>(newEntity) = mgr.GetComponent<ClipGComponent>(img2imgEntity);
			}
			else if (componentName == "T5XXL") {
				mgr.AddComponent<T5XXLComponent>(newEntity) = mgr.GetComponent<T5XXLComponent>(img2imgEntity);
			}
			else if (componentName == "DiffusionModel") {
				mgr.AddComponent<DiffusionModelComponent>(newEntity) = mgr.GetComponent<DiffusionModelComponent>(img2imgEntity);
			}
			else if (componentName == "Vae") {
				mgr.AddComponent<VaeComponent>(newEntity) = mgr.GetComponent<VaeComponent>(img2imgEntity);
			}
			else if (componentName == "Lora") {
				mgr.AddComponent<LoraComponent>(newEntity) = mgr.GetComponent<LoraComponent>(img2imgEntity);
			}
			else if (componentName == "Taesd") {
				mgr.AddComponent<TaesdComponent>(newEntity) = mgr.GetComponent<TaesdComponent>(img2imgEntity);
			}
			else if (componentName == "Latent") {
				mgr.AddComponent<LatentComponent>(newEntity) = mgr.GetComponent<LatentComponent>(img2imgEntity);
			}
			else if (componentName == "Sampler") {
				mgr.AddComponent<SamplerComponent>(newEntity) = mgr.GetComponent<SamplerComponent>(img2imgEntity);
			}
			else if (componentName == "Guidance") {
				mgr.AddComponent<GuidanceComponent>(newEntity) = mgr.GetComponent<GuidanceComponent>(img2imgEntity);
			}
			else if (componentName == "ClipSkip") {
				mgr.AddComponent<ClipSkipComponent>(newEntity) = mgr.GetComponent<ClipSkipComponent>(img2imgEntity);
			}
			else if (componentName == "Prompt") {
				mgr.AddComponent<PromptComponent>(newEntity) = mgr.GetComponent<PromptComponent>(img2imgEntity);
			}
			else if (componentName == "LayerSkip") {
				mgr.AddComponent<LayerSkipComponent>(newEntity) = mgr.GetComponent<LayerSkipComponent>(img2imgEntity);
			}
			else if (componentName == "OutputImage") {
				auto& srcComp = mgr.GetComponent<OutputImageComponent>(img2imgEntity);
				auto& destComp = mgr.AddComponent<OutputImageComponent>(newEntity);

				destComp.fileName = srcComp.fileName;
				destComp.filePath = srcComp.filePath;
				destComp.width = srcComp.width;
				destComp.height = srcComp.height;
				destComp.channels = srcComp.channels;
			}
			else if (componentName == "InputImage") {
				auto& srcComp = mgr.GetComponent<InputImageComponent>(img2imgEntity);
				auto& destComp = mgr.AddComponent<InputImageComponent>(newEntity);

				destComp.fileName = srcComp.fileName;
				destComp.filePath = srcComp.filePath;
				destComp.width = srcComp.width;
				destComp.height = srcComp.height;
				destComp.channels = srcComp.channels;
			}
		}

		std::cout << "Entity cloning complete. Created entity: " << newEntity << std::endl;

		// Queue event for img2img processing
		Event event;
		event.entityID = newEntity;
		event.type = EventType::Img2ImgRequest;
		ANI::Events::Ref().QueueEvent(event);
		std::cout << "Img2Img request queued for entity: " << newEntity << std::endl;
	}

	void DiffusionView::HandleUpscaleEvent() {
		Event event;
		EntityID newEntity = mgr.AddNewEntity();

		std::cout << "Initialized entity with ID: " << newEntity << "\n";

		event.entityID = newEntity;
		event.type = EventType::InferenceRequest;
		ANI::Events::Ref().QueueEvent(event);
	}

	void DiffusionView::RenderOther(const EntityID entity) {
		if (!mgr.HasComponent<SamplerComponent>(entity)) {
			return;
		}

		auto& samplerComp = mgr.GetComponent<SamplerComponent>(entity);

		ImGui::Checkbox("Free Params", &samplerComp.free_params_immediately);
		ImGui::InputInt("# Threads (CPU Only)", &samplerComp.n_threads);
	}

	void DiffusionView::RenderControlnets(const EntityID entity) {
		if (!mgr.HasComponent<ControlnetComponent>(entity)) {
			return;
		}

		auto& controlComp = mgr.GetComponent<ControlnetComponent>(entity);

		if (ImGui::BeginTable("Controlnet", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp)) {
			ImGui::TableSetupColumn("Model", ImGuiTableColumnFlags_WidthFixed, 64.0f);
			ImGui::TableSetupColumn("Load", ImGuiTableColumnFlags_WidthFixed, 52.0f);
			ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
			// ImGui::TableHeadersRow();
			ImGui::TableNextColumn();
			ImGui::Text("Controlnet");
			ImGui::TableNextColumn();
			if (ImGui::Button("...##5b")) {
				IGFD::FileDialogConfig config;
				config.path = filePaths.controlnetDir;
				ImGuiFileDialog::Instance()->OpenDialog("LoadFileDialog", "Choose Model", ".safetensors, .ckpt, .pt, .gguf",
					config);
			}
			ImGui::SameLine();
			if (ImGui::Button("R##5c")) {
				controlComp.modelName = "";
				controlComp.modelPath = "";
			}
			ImGui::TableNextColumn();
			ImGui::Text("%s", controlComp.modelName.c_str());

			if (ImGuiFileDialog::Instance()->Display("LoadFileDialog", 32, ImVec2(700, 400))) {
				if (ImGuiFileDialog::Instance()->IsOk()) {
					std::string selectedFile = ImGuiFileDialog::Instance()->GetCurrentFileName();
					std::string fullPath = ImGuiFileDialog::Instance()->GetFilePathName();

					controlComp.modelName = selectedFile;
					controlComp.modelPath = fullPath;
					std::cout << "Selected file: " << controlComp.modelName << std::endl;
					std::cout << "Full path: " << controlComp.modelPath << std::endl;
					std::cout << "New model path set: " << controlComp.modelPath << std::endl;
				}

				ImGuiFileDialog::Instance()->Close();
			}
			ImGui::EndTable();
		}

		if (ImGui::BeginTable("Control Settings", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp)) {
			ImGui::TableSetupColumn("Param", ImGuiTableColumnFlags_WidthFixed, 52.0f);
			ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
			// ImGui::TableHeadersRow();
			ImGui::TableNextColumn();
			ImGui::Text("Strength");
			ImGui::TableNextColumn();
			ImGui::InputFloat("##Strength", &controlComp.cnStrength, 0.01f, 0.1f, "%.2f");
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::Text("Start");
			ImGui::TableNextColumn();
			ImGui::InputFloat("##Start", &controlComp.applyStart, 0.01f, 0.1f, "%.2f");
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::Text("End");
			ImGui::TableNextColumn();
			ImGui::InputFloat("##End", &controlComp.applyEnd, 0.01f, 0.1f, "%.2f");
			ImGui::EndTable();
		}
	}
	void DiffusionView::RenderEmbeddings(const EntityID entity) {
		if (!mgr.HasComponent<EmbeddingComponent>(entity)) {
			return;
		}

		auto& embedComp = mgr.GetComponent<EmbeddingComponent>(entity);

		if (ImGui::BeginTable("Controlnet", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp)) {
			ImGui::TableSetupColumn("Model", ImGuiTableColumnFlags_WidthFixed, 52.0f);
			ImGui::TableSetupColumn("Load", ImGuiTableColumnFlags_WidthFixed, 52.0f);
			ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
			// ImGui::TableHeadersRow();
			ImGui::TableNextColumn();
			ImGui::Text("Embedding:");
			ImGui::TableNextColumn();
			if (ImGui::Button("...##v9")) {
				IGFD::FileDialogConfig config;
				config.path = filePaths.embedDir;
				ImGuiFileDialog::Instance()->OpenDialog("LoadFileDialog", "Choose Model", ".safetensors, .ckpt, .pt, .gguf",
					config);
			}
			ImGui::SameLine();
			if (ImGui::Button("R##v0")) {
				embedComp.modelName = "";
				embedComp.modelPath = "";
			}
			ImGui::TableNextColumn();
			ImGui::Text("%s", embedComp.modelName.c_str());

			if (ImGuiFileDialog::Instance()->Display("LoadFileDialog", 32, ImVec2(700, 400))) {
				if (ImGuiFileDialog::Instance()->IsOk()) {
					std::string selectedFile = ImGuiFileDialog::Instance()->GetCurrentFileName();
					std::string fullPath = ImGuiFileDialog::Instance()->GetFilePathName();

					embedComp.modelName = selectedFile;
					embedComp.modelPath = fullPath;
					std::cout << "Selected file: " << embedComp.modelName << std::endl;
					std::cout << "Full path: " << embedComp.modelPath << std::endl;
					std::cout << "New model path set: " << embedComp.modelPath << std::endl;
				}

				ImGuiFileDialog::Instance()->Close();
			}
			ImGui::EndTable();
		}
	}

	void DiffusionView::RenderDiffusionModelLoader(const EntityID entity) {
		if (
			!mgr.HasComponent<DiffusionModelComponent>(entity) ||
			!mgr.HasComponent<ClipLComponent>(entity) ||
			!mgr.HasComponent<ClipGComponent>(entity) ||
			!mgr.HasComponent<T5XXLComponent>(entity) ||
			!mgr.HasComponent<SamplerComponent>(entity)
			) {
			return;
		}

		auto& ckptComp = mgr.GetComponent<DiffusionModelComponent>(entity);
		auto& clipLComp = mgr.GetComponent<ClipLComponent>(entity);
		auto& clipGComp = mgr.GetComponent<ClipGComponent>(entity);
		auto& t5xxlComp = mgr.GetComponent<T5XXLComponent>(entity);
		auto& samplerComp = mgr.GetComponent<SamplerComponent>(entity);

		int current_type_method = static_cast<int>(samplerComp.current_type_method);
		ImGui::Combo("Quant Type", &current_type_method, type_method_items, type_method_item_count);
		samplerComp.current_type_method = static_cast<decltype(samplerComp.current_type_method)>(current_type_method);

		if (ImGui::BeginTable("ModelLoaderTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit)) {
			ImGui::TableSetupColumn("Model", ImGuiTableColumnFlags_WidthFixed, 52.0f);
			ImGui::TableSetupColumn("Load", ImGuiTableColumnFlags_WidthFixed, 52.0f);
			ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::Text("Unet:"); // Row for Unet
			ImGui::TableNextColumn();
			if (ImGui::Button("...##n2")) {
				IGFD::FileDialogConfig config;
				config.path = filePaths.unetDir;
				ImGuiFileDialog::Instance()->OpenDialog("LoadUnetDialog", "Choose Model", ".safetensors,.ckpt,.pt,.gguf",
					config);
			}
			ImGui::SameLine();
			if (ImGui::Button("R##n3")) {
				ckptComp.modelName = "";
				ckptComp.modelPath = "";
			}
			ImGui::TableNextColumn();
			ImGui::TextWrapped("%s", ckptComp.modelName.c_str());

			if (ImGuiFileDialog::Instance()->Display("LoadUnetDialog", 32, ImVec2(700, 400))) {
				if (ImGuiFileDialog::Instance()->IsOk()) {
					std::string selectedFile = ImGuiFileDialog::Instance()->GetCurrentFileName();
					std::string fullPath = ImGuiFileDialog::Instance()->GetFilePathName();

					ckptComp.modelName = selectedFile;
					ckptComp.modelPath = fullPath;
					std::cout << "Selected file: " << ckptComp.modelName << std::endl;
					std::cout << "Full path: " << ckptComp.modelPath << std::endl;
				}
				ImGuiFileDialog::Instance()->Close();
			}

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::Text("Clip L:"); // Row for Clip L
			ImGui::TableNextColumn();
			if (ImGui::Button("...##b7")) {
				IGFD::FileDialogConfig config;
				config.path = filePaths.encoderDir;
				ImGuiFileDialog::Instance()->OpenDialog("LoadClipLDialog", "Choose Model", ".safetensors,.ckpt,.pt,.gguf",
					config);
			}
			ImGui::SameLine();
			if (ImGui::Button("R##n6")) {
				clipLComp.modelName = "";
				clipLComp.modelPath = "";
			}
			ImGui::TableNextColumn();
			ImGui::TextWrapped("%s", clipLComp.modelName.c_str());

			if (ImGuiFileDialog::Instance()->Display("LoadClipLDialog", 32, ImVec2(700, 400))) {
				if (ImGuiFileDialog::Instance()->IsOk()) {
					std::string selectedFile = ImGuiFileDialog::Instance()->GetCurrentFileName();
					std::string fullPath = ImGuiFileDialog::Instance()->GetFilePathName();

					clipLComp.modelName = selectedFile;
					clipLComp.modelPath = fullPath;
					std::cout << "Selected file: " << clipLComp.modelName << std::endl;
					std::cout << "Full path: " << clipLComp.modelPath << std::endl;
				}
				ImGuiFileDialog::Instance()->Close();
			}

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::Text("Clip G:"); // Row for Clip G
			ImGui::TableNextColumn();
			if (ImGui::Button("...##g7")) {
				IGFD::FileDialogConfig config;
				config.path = filePaths.encoderDir;
				ImGuiFileDialog::Instance()->OpenDialog("LoadClipGDialog", "Choose Model", ".safetensors,.ckpt,.pt,.gguf",
					config);
			}
			ImGui::SameLine();
			if (ImGui::Button("R##n5")) {
				clipGComp.modelName = "";
				clipGComp.modelPath = "";
			}
			ImGui::TableNextColumn();
			ImGui::TextWrapped("%s", clipGComp.modelName.c_str());

			if (ImGuiFileDialog::Instance()->Display("LoadClipGDialog", 32, ImVec2(700, 400))) {
				if (ImGuiFileDialog::Instance()->IsOk()) {
					std::string selectedFile = ImGuiFileDialog::Instance()->GetCurrentFileName();
					std::string fullPath = ImGuiFileDialog::Instance()->GetFilePathName();

					clipGComp.modelName = selectedFile;
					clipGComp.modelPath = fullPath;
					std::cout << "Selected file: " << clipGComp.modelName << std::endl;
					std::cout << "Full path: " << clipGComp.modelPath << std::endl;
				}
				ImGuiFileDialog::Instance()->Close();
			}

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::Text("T5XXL:"); // Row for T5XXL
			ImGui::TableNextColumn();
			if (ImGui::Button("...##x6")) {
				IGFD::FileDialogConfig config;
				config.path = filePaths.encoderDir;
				ImGuiFileDialog::Instance()->OpenDialog("LoadT5XXLDialog", "Choose Model", ".safetensors,.ckpt,.pt,.gguf",
					config);
			}
			ImGui::SameLine();
			if (ImGui::Button("R##n4")) {
				t5xxlComp.modelName = "";
				t5xxlComp.modelPath = "";
			}
			ImGui::TableNextColumn();
			ImGui::TextWrapped("%s", t5xxlComp.modelName.c_str());

			if (ImGuiFileDialog::Instance()->Display("LoadT5XXLDialog", 32, ImVec2(700, 400))) {
				if (ImGuiFileDialog::Instance()->IsOk()) {
					std::string selectedFile = ImGuiFileDialog::Instance()->GetCurrentFileName();
					std::string fullPath = ImGuiFileDialog::Instance()->GetFilePathName();

					t5xxlComp.modelName = selectedFile;
					t5xxlComp.modelPath = fullPath;
					std::cout << "Selected file: " << t5xxlComp.modelName << std::endl;
					std::cout << "Full path: " << t5xxlComp.modelPath << std::endl;
				}
				ImGuiFileDialog::Instance()->Close();
			}

			// Row for Vae
			RenderVaeLoader(entity);
		}
	}

	void DiffusionView::RenderVaeLoader(const EntityID entity) {
		if (!mgr.HasComponent<VaeComponent>(entity)) {
			return;
		}

		auto& vaeComp = mgr.GetComponent<VaeComponent>(entity);

		ImGui::TableNextColumn();
		ImGui::Text("Vae: ");
		ImGui::TableNextColumn();
		if (ImGui::Button("...##4b")) {
			IGFD::FileDialogConfig config;
			config.path = filePaths.vaeDir;
			ImGuiFileDialog::Instance()->OpenDialog("LoadVaeDialog", "Choose Model", ".safetensors, .ckpt, .pt, .gguf",
				config);
		}
		ImGui::SameLine();
		if (ImGui::Button("R##f7")) {
			vaeComp.modelName = "";
			vaeComp.modelPath = "";
		}
		ImGui::TableNextColumn();
		ImGui::Text("%s", vaeComp.modelName.c_str());

		if (ImGuiFileDialog::Instance()->Display("LoadVaeDialog", 32, ImVec2(700, 400))) {
			if (ImGuiFileDialog::Instance()->IsOk()) {
				std::string selectedFile = ImGuiFileDialog::Instance()->GetCurrentFileName();
				std::string fullPath = ImGuiFileDialog::Instance()->GetFilePathName();

				vaeComp.modelName = selectedFile;
				vaeComp.modelPath = fullPath;
				std::cout << "Selected file: " << vaeComp.modelName << std::endl;
				std::cout << "Full path: " << vaeComp.modelPath << std::endl;
				std::cout << "New model path set: " << vaeComp.modelPath << std::endl;
			}

			ImGuiFileDialog::Instance()->Close();
		}
		ImGui::EndTable();

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		RenderVaeOptions(entity);
	}

	void DiffusionView::RenderVaeOptions(const EntityID entity) {
		if (!mgr.HasComponent<VaeComponent>(entity)) {
			return;
		}

		auto& vaeComp = mgr.GetComponent<VaeComponent>(entity);

		if (ImGui::BeginTable("VaeOptionsTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit)) {
			ImGui::TableSetupColumn("Param", ImGuiTableColumnFlags_WidthFixed, 64.0f); // Fixed width for Model
			ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

			ImGui::TableNextColumn();

			ImGui::Text("Tiled Vae");

			ImGui::TableNextColumn();

			ImGui::Checkbox("##TileVae", &vaeComp.isTiled);

			ImGui::TableNextRow();
			ImGui::TableNextColumn();

			ImGui::Text("Keep Vae on CPU");
			ImGui::TableNextColumn();

			ImGui::Checkbox("##KeepVaeLoaded", &vaeComp.keep_vae_on_cpu);

			ImGui::TableNextRow();
			ImGui::TableNextColumn();

			ImGui::Text("Vae Decode Only");
			ImGui::TableNextColumn();

			ImGui::Checkbox("##VaeDecodeOnly", &vaeComp.vae_decode_only);

			ImGui::EndTable();
		}
	}

	void DiffusionView::RenderQueueList() {

		ImGui::SetNextWindowSize(ImVec2(300, 500), ImGuiCond_FirstUseEver);
		if (ImGui::Begin("Queue")) {

			// Get current progress values
			int currentStep = progressData.currentStep;
			int totalSteps = progressData.totalSteps;
			float time = progressData.currentTime;
			bool isProcessing = progressData.isProcessing;

			if (isProcessing && totalSteps > 0) {
				float progress = static_cast<float>(currentStep) / totalSteps;
				std::ostringstream ss;
				ss << "Processing: " << currentStep << "/" << totalSteps << " steps (" << std::fixed << std::setprecision(1)
					<< time << "s)";
				ImGui::Text("%s", ss.str().c_str());
				ImGui::ProgressBar(progress, ImVec2(-FLT_MIN, 0));
			}
			else {
				ImGui::Text("Waiting...");
				ImGui::ProgressBar(0.0f, ImVec2(-FLT_MIN, 0));
			}
			ImGui::Separator();

			if (ImGui::InputInt("Queue #", &numQueues, 1, 4)) {
				if (numQueues < 1) {
					numQueues = 1;
				}
			}

			if (ImGui::Button("Queue", ImVec2(-FLT_MIN, 0))) {
				EntityID targetEntity = isTxt2ImgMode ? txt2imgEntity : img2imgEntity;
				if (mgr.HasComponent<LoraComponent>(targetEntity)) {
					auto& loraComp = mgr.GetComponent<LoraComponent>(targetEntity);
					loraComp.modelPath = filePaths.loraDir;
				}
				
				for (int i = 0; i < numQueues; i++) {
					if (isTxt2ImgMode) {
						HandleT2IEvent();
					}
					else {
						HandleI2IEvent();
					}
					
					// seedControl->activate();
				}
			}

			if (isPaused) {
				if (ImGui::Button("Resume", ImVec2(-FLT_MIN, 0))) {
					Event event;
					event.type = EventType::ResumeInference;
					ANI::Events::Ref().QueueEvent(event);
					isPaused = false;
				}
			}
			else {
				if (ImGui::Button("Pause", ImVec2(-FLT_MIN, 0))) {
					Event event;
					event.type = EventType::PauseInference;
					ANI::Events::Ref().QueueEvent(event);
					isPaused = true;
				}
			}

			if (ImGui::Button("Stop", ImVec2(-FLT_MIN, 0))) {
				Event event;
				event.type = EventType::StopCurrentTask;
				ANI::Events::Ref().QueueEvent(event);
			}

			if (ImGui::Button("Clear Queue", ImVec2(-FLT_MIN, 0))) {
				Event event;
				event.type = EventType::ClearInferenceQueue;
				ANI::Events::Ref().QueueEvent(event);
			}

			ImGui::Separator();

			if (ImGui::BeginTable("Queue", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp)) {
				ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 42.0f);
				ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 44.0f);
				ImGui::TableSetupColumn("Controls", ImGuiTableColumnFlags_WidthStretch);
				// ImGui::TableSetupColumn("Prompt", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableHeadersRow();

				auto sdSystem = mgr.GetSystem<SDCPPSystem>();
				if (sdSystem) {
					auto queueItems = sdSystem->GetQueueSnapshot();
					for (size_t i = 0; i < queueItems.size(); i++) {
						const auto& item = queueItems[i];

						ImGui::TableNextRow();

						// ID column
						ImGui::TableNextColumn();
						ImGui::Text("%d", static_cast<int>(item.entityID));

						// Status column
						ImGui::TableNextColumn();
						if (item.processing) {
							ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "Active");
						}
						else {
							ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.2f, 1.0f), "Queued");
						}

						// Controls column
						ImGui::TableNextColumn();
						if (!item.processing) {

							if (i > 0) {
								if (ImGui::ArrowButton(("up##" + std::to_string(i)).c_str(), ImGuiDir_Up)) {
									sdSystem->MoveInQueue(i, i - 1);
								}
								ImGui::SameLine();

							}

							if (i < queueItems.size() - 1) {
								if (ImGui::ArrowButton(("down##" + std::to_string(i)).c_str(), ImGuiDir_Down)) {
									sdSystem->MoveInQueue(i, i + 1);
								}
								ImGui::SameLine();
							}

							if (i > 0) {
								if (ImGui::Button(("Top##Top" + std::to_string(i)).c_str())) {
									if (queueItems[0].processing) {
										sdSystem->MoveInQueue(i, 1);
									}
									else {
										sdSystem->MoveInQueue(i, 0);
									}
										
								}
								ImGui::SameLine();
							}

							if (i < queueItems.size() - 1) {
								if (ImGui::Button(("Bottom##Bottom" + std::to_string(i)).c_str())) {
									sdSystem->MoveInQueue(i, queueItems.size() - 1);
								}
								ImGui::SameLine();
							}

							if (ImGui::Button(("X##Remove" + std::to_string(i)).c_str())) {
								sdSystem->RemoveFromQueue(i);
							}

						}

						// // Prompt column
						// ImGui::TableNextColumn();
						// if (prompt.length() > 50) {
						// 	ImGui::Text("%s...", prompt.substr(0, 47).c_str());
						// 	if (ImGui::IsItemHovered()) {
						// 		ImGui::SetTooltip("%s", prompt.c_str());
						// 	}
						// }
						// else {
						// 	ImGui::Text("%s", prompt.c_str());
						// }
					}
				}
				ImGui::EndTable();
			}
		}
		ImGui::End();
	}

	void DiffusionView::Render() {
		// Render queue controls
		RenderQueueList();

		// Main window
		ImGui::SetNextWindowSize(ImVec2(300, 800), ImGuiCond_FirstUseEver);
		if (ImGui::Begin("Image Generation")) {
			// Metadata controls
			if (ImGui::CollapsingHeader("Metadata Controls")) {
				RenderMetadataControls();
			}

			// Tab bar for switching between Txt2Img and Img2Img
			if (ImGui::BeginTabBar("Image")) {
				// Text-to-Image tab
				if (ImGui::BeginTabItem("Txt2Img")) {
					isTxt2ImgMode = true;

					// Render appropriate UI sections
					if (ImGui::CollapsingHeader("Output Settings")) {
						RenderFilePath(txt2imgEntity);
					}

					if (ImGui::CollapsingHeader("Model Selection")) {
						if (ImGui::BeginTabBar("Model Loader")) {
							if (ImGui::BeginTabItem("Full")) {
								RenderModelLoader(txt2imgEntity);
								ImGui::EndTabItem();
							}
							if (ImGui::BeginTabItem("Separate")) {
								RenderDiffusionModelLoader(txt2imgEntity);
								ImGui::EndTabItem();
							}
							ImGui::EndTabBar();
						}
					}

					if (ImGui::CollapsingHeader("Latent Settings")) {
						RenderLatents(txt2imgEntity);
					}

					if (ImGui::CollapsingHeader("Prompt Inputs")) {
						RenderPrompts(txt2imgEntity);
					}

					if (ImGui::CollapsingHeader("Sampler Settings")) {
						RenderSampler(txt2imgEntity);
					}

					if (ImGui::CollapsingHeader("Other Settings")) {
						RenderOther(txt2imgEntity);
					}

					ImGui::EndTabItem();
				}

				// Image-to-Image tab
				if (ImGui::BeginTabItem("Img2Img")) {
					isTxt2ImgMode = false;

					if (ImGui::CollapsingHeader("Output Settings")) {
						RenderFilePath(img2imgEntity);
					}

					if (ImGui::CollapsingHeader("Input Image")) {
						RenderInputImage(img2imgEntity);
					}

					if (ImGui::CollapsingHeader("Model Selection")) {
						if (ImGui::BeginTabBar("Model Loader")) {
							if (ImGui::BeginTabItem("Full")) {
								RenderModelLoader(img2imgEntity);
								ImGui::EndTabItem();
							}
							if (ImGui::BeginTabItem("Separate")) {
								RenderDiffusionModelLoader(img2imgEntity);
								ImGui::EndTabItem();
							}
							ImGui::EndTabBar();
						}
					}

					if (ImGui::CollapsingHeader("Latent Settings")) {
						RenderLatents(img2imgEntity);
					}

					if (ImGui::CollapsingHeader("Prompt Inputs")) {
						RenderPrompts(img2imgEntity);
					}

					if (ImGui::CollapsingHeader("Sampler Settings")) {
						RenderSampler(img2imgEntity);
					}

					if (ImGui::CollapsingHeader("Other Settings")) {
						RenderOther(img2imgEntity);
					}

					ImGui::EndTabItem();
				}
				ImGui::EndTabBar();
			}
			ImGui::End();
		}
	}

	nlohmann::json DiffusionView::Serialize() const {
		nlohmann::json entityJson = "";
		if (isTxt2ImgMode) {
			entityJson = mgr.SerializeEntity(txt2imgEntity);
		}
		else {
			entityJson = mgr.SerializeEntity(img2imgEntity);
		}
		
		return entityJson;
	}

	void DiffusionView::Deserialize(const nlohmann::json& j) {
		
		EntityID targetEntity = isTxt2ImgMode ? txt2imgEntity : img2imgEntity;

		if (targetEntity == 0) {
			std::cerr << "Error: Invalid target entity for deserialization" << std::endl;
			return;
		}

		try {
			mgr.DeserializeEntity(j, targetEntity);
			std::cout << "Successfully deserialized data to entity " << targetEntity << std::endl;
		}
		catch (const std::exception& e) {
			std::cerr << "Exception during deserialization: " << e.what() << std::endl;
		}
	}


	void DiffusionView::SaveMetadataToJson(const std::string& filepath) {
		try {
			nlohmann::json metadata = Serialize();
			std::ofstream file(filepath);
			if (file.is_open()) {
				file << metadata.dump(4);
				file.close();
				std::cout << "Metadata saved to: " << filepath << std::endl;
			}
			else {
				std::cerr << "Failed to open file for writing: " << filepath << std::endl;
			}
		}
		catch (const std::exception& e) {
			std::cerr << "Error saving metadata: " << e.what() << std::endl;
		}
	}

	void DiffusionView::LoadMetadataFromJson(const std::string& filepath) {
		try {
			std::ifstream file(filepath);
			if (file.is_open()) {
				nlohmann::json metadata;
				file >> metadata;
				Deserialize(metadata);
				file.close();
				std::cout << "Metadata loaded from: " << filepath << std::endl;
			}
			else {
				std::cerr << "Failed to open file for reading: " << filepath << std::endl;
			}
		}
		catch (const std::exception& e) {
			std::cerr << "Error loading metadata: " << e.what() << std::endl;
		}
	}

	void DiffusionView::LoadMetadataFromPNG(const std::string& imagePath) {
		std::cout << "Attempting to load metadata from: " << imagePath << std::endl;

		FILE* fp = fopen(imagePath.c_str(), "rb");
		if (!fp) {
			std::cerr << "Failed to open PNG file: " << imagePath << std::endl;
			return;
		}

		png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
		if (!png) {
			fclose(fp);
			return;
		}

		png_infop info = png_create_info_struct(png);
		if (!info) {
			png_destroy_read_struct(&png, nullptr, nullptr);
			fclose(fp);
			return;
		}

		if (setjmp(png_jmpbuf(png))) {
			png_destroy_read_struct(&png, &info, nullptr);
			fclose(fp);
			return;
		}

		png_init_io(png, fp);
		png_read_info(png, info);

		// Get text chunks
		png_textp text_ptr;
		int num_text;
		if (png_get_text(png, info, &text_ptr, &num_text) > 0) {
			for (int i = 0; i < num_text; i++) {
				if (strcmp(text_ptr[i].key, "parameters") == 0) {
					try {
						// Parse metadata from PNG
						nlohmann::json metadata = nlohmann::json::parse(text_ptr[i].text);
						std::cout << "Loading metadata: " << metadata.dump(2) << std::endl;

						// Convert nested object format to array format
						nlohmann::json convertedJson;

						// Keep the app info
						convertedJson["ID"] = metadata.value("ID", 0);
						if (metadata.contains("software"))
							convertedJson["software"] = metadata["software"];
						if (metadata.contains("timestamp"))
							convertedJson["timestamp"] = metadata["timestamp"];
						if (metadata.contains("version"))
							convertedJson["version"] = metadata["version"];

						convertedJson["components"] = nlohmann::json::array();

						// If metadata has the nested components object format, convert it
						if (metadata.contains("components") && metadata["components"].is_object()) {
							auto& componentsObj = metadata["components"];

							// Process each component type
							for (auto it = componentsObj.begin(); it != componentsObj.end(); ++it) {
								std::string componentName = it.key();
								nlohmann::json componentData = it.value();

								// Handle base component as a special case
								if (componentName == "Base_Component") {
									nlohmann::json baseComp;
									baseComp["compName"] = "Base_Component";
									convertedJson["components"].push_back(baseComp);
									continue;
								}

								// For nested objects, create an array element for each component
								if (componentData.is_object()) {
									// Remove the double nesting if present
									if (componentData.contains(componentName) && componentData[componentName].is_object()) {
										nlohmann::json arrayElement;
										arrayElement[componentName] = componentData[componentName];
										convertedJson["components"].push_back(arrayElement);
									}
									else {
										// Just add it as is
										nlohmann::json arrayElement;
										arrayElement[componentName] = componentData;
										convertedJson["components"].push_back(arrayElement);
									}
								}
							}
						}
						else if (metadata.contains("components") && metadata["components"].is_array()) {
							// If it's already in the array format, use it directly
							convertedJson["components"] = metadata["components"];
						}

						std::cout << "Converted JSON: " << convertedJson.dump(2) << std::endl;

						// Now deserialize the converted JSON using your existing method
						Deserialize(convertedJson);
						std::cout << "Successfully loaded metadata" << std::endl;
					}
					catch (const std::exception& e) {
						std::cerr << "Error loading metadata: " << e.what() << std::endl;
					}
					break;
				}
			}
		}

		png_destroy_read_struct(&png, &info, nullptr);
		fclose(fp);
	}

	void DiffusionView::RenderMetadataControls() {

		if (ImGui::Button("Save Metadata", ImVec2(-FLT_MIN, 0))) {
			IGFD::FileDialogConfig config;
			config.path = filePaths.defaultProjectPath;
			ImGuiFileDialog::Instance()->OpenDialog("SaveMetadataDialog", "Save Metadata", ".json", config);
		}

		if (ImGui::Button("Load Metadata", ImVec2(-FLT_MIN, 0))) {
			IGFD::FileDialogConfig config;
			config.path = filePaths.defaultProjectPath;
			ImGuiFileDialog::Instance()->OpenDialog("LoadMetadataDialog", "Load Metadata", ".json,.png", config);
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
				std::string extension = std::filesystem::path(filepath).extension().string();

				if (extension == ".json") {
					LoadMetadataFromJson(filepath);
				}
				else if (extension == ".png" || extension == ".jpg" || extension == ".jpeg") {
					LoadMetadataFromPNG(filepath);
				}
			}
			ImGuiFileDialog::Instance()->Close();
		}
	}
} // namespace GUI