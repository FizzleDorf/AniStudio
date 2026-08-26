// QueueView.cpp
#include "QueueView.hpp"
#include "Events.hpp"
#include "DiffusionCallbackUtils.hpp"
#include "FileDialogUtil.hpp"
#include "FileDialogFilters.hpp"
#include "BaseDiffusionView.hpp"
#include "ClipboardUtilities.hpp"
#include "FilePathSystem.hpp"
#include "ProjectSystem.hpp"
#include <fstream>
#include <iomanip>
#include <sstream>
#include <filesystem>
#include <any>

using namespace ECS;
using namespace ANI;

namespace GUI {

    struct ViewQueueInfo {
        EntityID entity = 0;
        std::string taskType;
        bool valid = false;
    };

    static ViewQueueInfo GetViewQueueInfo(BaseView* view) {
        ViewQueueInfo info;
        if (!view) return info;

        if (auto* dv = dynamic_cast<BaseDiffusionView*>(view)) {
            info.entity = dv->GetActiveEntity();
            info.taskType = dv->GetTaskType();
            info.valid = (info.entity != 0 && !info.taskType.empty());
            return info;
        }
        return info;
    }

    QueueView::QueueView(ECS::EntityManager& mgr, ViewManager& vm)
        : BaseView(mgr, vm) {
        viewName = "QueueView";
        windowOpen = true;
        m_queueLoaded = false;
    }

    QueueView::~QueueView() {
    }

    void QueueView::Init() {
    }

    bool QueueView::IsDiffusionView(BaseView* view) const {
        if (!view) return false;
        for (size_t i = 0; i < DIFFUSION_VIEWS_COUNT; ++i) {
            if (view->viewName == DIFFUSION_VIEWS[i]) {
                return true;
            }
        }
        return false;
    }

    void QueueView::RefreshViewList() {
        m_availableViews.clear();

        WorkspaceID currentWorkspace = GetViewManager().GetActiveWorkspace();

        auto allViews = GetViewManager().GetAllViews();
        for (auto* view : allViews) {
            if (!view) continue;

            if (view->GetID() != currentWorkspace) continue;

            if (IsDiffusionView(view)) {
                m_availableViews.push_back(view);
            }
        }

        if (m_selectedViewIndex >= (int)m_availableViews.size())
            m_selectedViewIndex = m_availableViews.empty() ? -1 : 0;
    }

    void QueueView::RenderViewSelector() {
        RefreshViewList();

        if (m_availableViews.empty()) {
            ImGui::TextDisabled("No diffusion views available in current workspace.");
            return;
        }

        std::vector<std::string> displayNames;
        std::vector<const char*> displayNamePtrs;
        for (auto* view : m_availableViews) {
            auto info = GetViewQueueInfo(view);
            std::string task = info.valid ? info.taskType : "N/A";
            displayNames.push_back(view->viewName + " (ID:" + std::to_string(view->GetID()) + ") [" + task + "]");
            displayNamePtrs.push_back(displayNames.back().c_str());
        }

        if (ImGui::Combo("Select View", &m_selectedViewIndex, displayNamePtrs.data(), (int)displayNamePtrs.size())) {
        }
    }

    void QueueView::QueueFromSelectedView() {
        if (m_selectedViewIndex < 0 || m_selectedViewIndex >= (int)m_availableViews.size())
            return;
        auto* view = m_availableViews[m_selectedViewIndex];
        auto info = GetViewQueueInfo(view);
        if (!info.valid) {
            std::cerr << "[QueueView] Selected view does not provide a valid entity/task.\n";
            return;
        }

        EntityID newEntity = m_entityManager.CloneEntity(info.entity);
        if (newEntity == 0) {
            std::cerr << "[QueueView] Failed to clone entity.\n";
            return;
        }

        auto sys = m_entityManager.GetSystem<ECS::SDCPPSystem>();
        if (!sys) {
            m_entityManager.DestroyEntity(newEntity);
            return;
        }
        ECS::SDCPPSystem::TaskType type;
        if (info.taskType == "Inference") type = ECS::SDCPPSystem::TaskType::Inference;
        else if (info.taskType == "Img2Img") type = ECS::SDCPPSystem::TaskType::Img2Img;
        else if (info.taskType == "Edit") type = ECS::SDCPPSystem::TaskType::Edit;
        else if (info.taskType == "Upscaling") type = ECS::SDCPPSystem::TaskType::Upscaling;
        else if (info.taskType == "Conversion") type = ECS::SDCPPSystem::TaskType::Conversion;
        else if (info.taskType == "Img2Vid") type = ECS::SDCPPSystem::TaskType::Img2Vid;
        else {
            m_entityManager.DestroyEntity(newEntity);
            return;
        }

        sys->QueueTask(newEntity, type);
        std::cout << "[QueueView] Queued entity " << newEntity << " as " << info.taskType << "\n";
    }

    void QueueView::RenderQueueItemContextMenu(const ECS::SDCPPSystem::QueueItem& item, size_t index) {
        std::string popupId = "QueueItemContextMenu##" + std::to_string(index);
        if (ImGui::BeginPopup(popupId.c_str())) {
            ImGui::Text("Entity ID: %d", static_cast<int>(item.entityID));
            ImGui::Text("Status: %s", item.processing ? "Active" : "Queued");
            ImGui::Separator();

            if (ImGui::MenuItem("Copy Entity Metadata")) {
                GUI::Clipboard::CopyEntity(m_entityManager, item.entityID);
            }

            ImGui::EndPopup();
        }
    }

    void QueueView::RenderQueueList() {
        const auto& progressData = DiffusionCallbackUtils::GetProgressData();
        int currentStep = progressData.currentStep;
        int totalSteps = progressData.totalSteps;
        float time = progressData.currentTime;
        bool isProcessing = progressData.isProcessing;

        if (isProcessing && totalSteps > 0) {
            float progress = static_cast<float>(currentStep) / totalSteps;
            std::ostringstream ss;
            ss << "Processing: " << currentStep << "/" << totalSteps << " steps ("
                << std::fixed << std::setprecision(1) << time << "s)";
            ImGui::Text("%s", ss.str().c_str());
            ImGui::ProgressBar(progress, ImVec2(-FLT_MIN, 0));
        }
        else {
            ImGui::Text("Waiting...");
            ImGui::ProgressBar(0.0f, ImVec2(-FLT_MIN, 0));
        }
        ImGui::Separator();

        if (ImGui::InputInt("Queue #", &numQueues, 1, 4)) {
            if (numQueues < 1) numQueues = 1;
        }

        auto sys = m_entityManager.GetSystem<ECS::SDCPPSystem>();
        if (sys) isPaused = sys->IsPaused();

        float contentWidth = ImGui::GetContentRegionAvail().x;
        float spacing = ImGui::GetStyle().ItemSpacing.x;

        int numButtons1 = 2;
        float buttonWidth1 = (contentWidth - spacing * (numButtons1 - 1)) / numButtons1;
        if (buttonWidth1 < 0) buttonWidth1 = 0;

        if (ImGui::Button("Queue", ImVec2(buttonWidth1, 0))) {
            for (int i = 0; i < numQueues; ++i)
                QueueFromSelectedView();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(buttonWidth1, 0))) {
            ANI::Events::Ref().QueueEvent("CancelCurrentDiffusionTask");
        }

        ImGui::Separator();

        int numButtons2 = 4;
        float buttonWidth2 = (contentWidth - spacing * (numButtons2 - 1)) / numButtons2;
        if (buttonWidth2 < 0) buttonWidth2 = 0;

        if (isPaused) {
            if (ImGui::Button("Resume", ImVec2(buttonWidth2, 0))) {
                ANI::Events::Ref().QueueEvent("ResumeDiffusionWorker");
                isPaused = false;
            }
        }
        else {
            if (ImGui::Button("Pause", ImVec2(buttonWidth2, 0))) {
                ANI::Events::Ref().QueueEvent("PauseDiffusionWorker");
                isPaused = true;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Stop", ImVec2(buttonWidth2, 0))) {
            ANI::Events::Ref().QueueEvent("StopCurrentDiffusionTask");
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear Queue", ImVec2(buttonWidth2, 0))) {
            ANI::Events::Ref().QueueEvent("ClearDiffusionQueue");
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear All", ImVec2(buttonWidth2, 0))) {
            ANI::Events::Ref().QueueEvent("ClearAllDiffusionTasks");
        }

        ImGui::Separator();

        if (ImGui::BeginTable("QueueItems", 4,
            ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 75.0f);
            ImGui::TableSetupColumn("Entity ID", ImGuiTableColumnFlags_WidthFixed, 75.0f);
            ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 75.0f);
            ImGui::TableSetupColumn("Controls", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();

            if (sys) {
                auto items = sys->GetQueueSnapshot();
                for (size_t i = 0; i < items.size(); ++i) {
                    const auto& item = items[i];
                    ImGui::TableNextRow();

                    ImGui::PushID(static_cast<int>(i));

                    std::string popupId = "QueueItemContextMenu##" + std::to_string(i);

                    // Status
                    ImGui::TableNextColumn();
                    if (item.processing)
                        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "Active");
                    else
                        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.2f, 1.0f), "Queued");
                    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                        ImGui::OpenPopup(popupId.c_str());
                    }

                    // Entity ID
                    ImGui::TableNextColumn();
                    ImGui::Text("%05d", static_cast<int>(item.entityID));
                    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                        ImGui::OpenPopup(popupId.c_str());
                    }

                    // Type
                    ImGui::TableNextColumn();
                    switch (item.taskType) {
                    case ECS::SDCPPSystem::TaskType::Inference: ImGui::Text("Txt2Img"); break;
                    case ECS::SDCPPSystem::TaskType::Img2Img:   ImGui::Text("Img2Img"); break;
                    case ECS::SDCPPSystem::TaskType::Edit:      ImGui::Text("Edit"); break;
                    case ECS::SDCPPSystem::TaskType::Upscaling: ImGui::Text("Upscale"); break;
                    case ECS::SDCPPSystem::TaskType::Conversion:ImGui::Text("Convert"); break;
                    case ECS::SDCPPSystem::TaskType::Img2Vid:   ImGui::Text("Img2Vid"); break;
                    default: ImGui::Text("Unknown");
                    }
                    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                        ImGui::OpenPopup(popupId.c_str());
                    }

                    // Controls
                    ImGui::TableNextColumn();
                    if (!item.processing) {
                        bool controlHovered = false;
                        if (i > 0) {
                            if (ImGui::ArrowButton(("up##" + std::to_string(i)).c_str(), ImGuiDir_Up)) {
                                auto moveData = std::make_pair(i, i - 1);
                                ANI::Events::Ref().QueueEventWithData("MoveInDiffusionQueue", moveData);
                            }
                            if (ImGui::IsItemHovered()) controlHovered = true;
                            ImGui::SameLine();
                        }
                        if (i < items.size() - 1) {
                            if (ImGui::ArrowButton(("down##" + std::to_string(i)).c_str(), ImGuiDir_Down)) {
                                auto moveData = std::make_pair(i, i + 1);
                                ANI::Events::Ref().QueueEventWithData("MoveInDiffusionQueue", moveData);
                            }
                            if (ImGui::IsItemHovered()) controlHovered = true;
                            ImGui::SameLine();
                        }
                        if (i > 0) {
                            if (ImGui::Button(("top##" + std::to_string(i)).c_str())) {
                                size_t target = items[0].processing ? 1 : 0;
                                auto moveData = std::make_pair(i, target);
                                ANI::Events::Ref().QueueEventWithData("MoveInDiffusionQueue", moveData);
                            }
                            if (ImGui::IsItemHovered()) controlHovered = true;
                            ImGui::SameLine();
                        }
                        if (i < items.size() - 1) {
                            if (ImGui::Button(("end##" + std::to_string(i)).c_str())) {
                                auto moveData = std::make_pair(i, items.size() - 1);
                                ANI::Events::Ref().QueueEventWithData("MoveInDiffusionQueue", moveData);
                            }
                            if (ImGui::IsItemHovered()) controlHovered = true;
                            ImGui::SameLine();
                        }
                        if (ImGui::Button(("X##" + std::to_string(i)).c_str()))
                            ANI::Events::Ref().QueueEventWithData("RemoveFromDiffusionQueue", i);
                        if (ImGui::IsItemHovered()) controlHovered = true;

                        if (controlHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                            ImGui::OpenPopup(popupId.c_str());
                        }
                    }
                    else {
                        ImGui::TextDisabled("Processing");
                        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                            ImGui::OpenPopup(popupId.c_str());
                        }
                    }

                    // Render the context menu popup
                    RenderQueueItemContextMenu(item, i);

                    ImGui::PopID();
                }
            }
            ImGui::EndTable();
        }
    }

    void QueueView::RenderMenuBar() {
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Save Queue As...")) {
                    SaveQueue();
                }
                if (ImGui::MenuItem("Load Queue...")) {
                    LoadQueue();
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Quick Save")) {
                    QuickSave();
                }
                if (ImGui::MenuItem("Quick Load")) {
                    QuickLoad();
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }
    }

    void QueueView::SaveQueue() {
        auto sys = m_entityManager.GetSystem<ECS::SDCPPSystem>();
        if (!sys) return;

        auto tasks = sys->GetQueueTasksWithMetadata();
        if (tasks.empty()) {
            std::cout << "[QueueView] Queue is empty, nothing to save.\n";
            return;
        }

        nlohmann::json j = nlohmann::json::array();
        for (const auto& [taskType, entityData] : tasks) {
            nlohmann::json entry;
            entry["taskType"] = static_cast<int>(taskType);
            entry["entityData"] = entityData;
            j.push_back(entry);
        }

        std::string defaultPath = std::filesystem::current_path().string() + "/data/saved_queues/";
        std::filesystem::create_directories(defaultPath);
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S");
        std::string defaultName = "queue_" + ss.str() + ".json";

        std::string outPath;
        if (!FileDialog::SaveFile("Save Queue", FileDialog::FilterType::ALL_FILES, defaultName, outPath, defaultPath)) {
            std::cout << "[QueueView] Save cancelled.\n";
            return;
        }

        std::ofstream file(outPath);
        if (file.is_open()) {
            file << j.dump(4);
            std::cout << "[QueueView] Saved queue to " << outPath << "\n";
        }
        else {
            std::cerr << "[QueueView] Failed to save queue.\n";
        }
    }

    void QueueView::LoadQueue() {
        auto sys = m_entityManager.GetSystem<ECS::SDCPPSystem>();
        if (!sys) {
            std::cerr << "[QueueView] SDCPPSystem not available.\n";
            return;
        }

        std::string defaultPath = std::filesystem::current_path().string() + "/data/saved_queues/";
        std::string selected;
        if (!FileDialog::OpenFile("Load Queue File", FileDialog::FilterType::ALL_FILES, selected, defaultPath)) {
            std::cout << "[QueueView] Load cancelled.\n";
            return;
        }

        std::ifstream file(selected);
        if (!file.is_open()) {
            std::cerr << "[QueueView] Failed to open file: " << selected << "\n";
            return;
        }

        nlohmann::json j;
        try { file >> j; }
        catch (const std::exception& e) {
            std::cerr << "[QueueView] Error parsing JSON: " << e.what() << "\n";
            return;
        }

        if (!j.is_array()) {
            std::cerr << "[QueueView] Invalid format: expected array.\n";
            return;
        }

        sys->ClearAllTasks();

        int loadedCount = 0;
        for (const auto& entry : j) {
            if (!entry.contains("taskType") || !entry.contains("entityData")) {
                std::cerr << "[QueueView] Skipping invalid entry.\n";
                continue;
            }
            int taskInt = entry["taskType"];
            auto taskType = static_cast<ECS::SDCPPSystem::TaskType>(taskInt);
            const auto& entityData = entry["entityData"];
            sys->QueueTaskFromSerialized(entityData, taskType);
            loadedCount++;
        }
        std::cout << "[QueueView] Loaded " << loadedCount << " tasks from " << selected << "\n";
        m_queueLoaded = true;
    }

    void QueueView::QuickSave() {
        if (!m_queueLoaded) return;
        try {
            auto filePathSys = m_entityManager.GetSystem<ECS::FilePathSystem>();
            if (!filePathSys) return;

            std::string dataPath = filePathSys->GetPath("ProjectDataPath");
            if (dataPath.empty()) {
                dataPath = filePathSys->GetPath("DefaultProject");
            }
            if (dataPath.empty()) {
                std::cout << "[QueueView] No data path found, cannot quick save.\n";
                return;
            }

            auto sys = m_entityManager.GetSystem<ECS::SDCPPSystem>();
            if (!sys) return;

            auto tasks = sys->GetQueueTasksWithMetadata();
            if (tasks.empty()) {
                std::cout << "[QueueView] Queue is empty, nothing to quick save.\n";
                return;
            }

            std::filesystem::create_directories(dataPath);
            std::string filename = viewName + ".json";
            std::string filepath = (std::filesystem::path(dataPath) / filename).string();

            nlohmann::json j = nlohmann::json::array();
            for (const auto& [taskType, entityData] : tasks) {
                nlohmann::json entry;
                entry["taskType"] = static_cast<int>(taskType);
                entry["entityData"] = entityData;
                j.push_back(entry);
            }

            std::ofstream file(filepath);
            if (file.is_open()) {
                file << j.dump(4);
                std::cout << "[QueueView] Quick saved queue to " << filepath << "\n";
            }
            else {
                std::cerr << "[QueueView] Failed to quick save queue to " << filepath << "\n";
            }
        }
        catch (const std::exception& e) {
            std::cerr << "[QueueView] Exception during QuickSave: " << e.what() << "\n";
        }
    }

    void QueueView::QuickLoad() {
        try {
            auto filePathSys = m_entityManager.GetSystem<ECS::FilePathSystem>();
            if (!filePathSys) return;

            std::string dataPath = filePathSys->GetPath("ProjectDataPath");
            if (dataPath.empty()) {
                dataPath = filePathSys->GetPath("DefaultProject");
            }
            if (dataPath.empty()) {
                std::cout << "[QueueView] No data path found, cannot quick load.\n";
                return;
            }

            std::string filename = viewName + ".json";
            std::string filepath = (std::filesystem::path(dataPath) / filename).string();

            if (!std::filesystem::exists(filepath)) {
                std::cout << "[QueueView] No quick save file found at: " << filepath << "\n";
                return;
            }

            auto sys = m_entityManager.GetSystem<ECS::SDCPPSystem>();
            if (!sys) {
                std::cerr << "[QueueView] SDCPPSystem not available.\n";
                return;
            }

            std::ifstream file(filepath);
            if (!file.is_open()) {
                std::cerr << "[QueueView] Failed to open file: " << filepath << "\n";
                return;
            }

            nlohmann::json j;
            try { file >> j; }
            catch (const std::exception& e) {
                std::cerr << "[QueueView] Error parsing JSON: " << e.what() << "\n";
                return;
            }

            if (!j.is_array()) {
                std::cerr << "[QueueView] Invalid format: expected array.\n";
                return;
            }

            sys->ClearAllTasks();

            int loadedCount = 0;
            for (const auto& entry : j) {
                if (!entry.contains("taskType") || !entry.contains("entityData")) {
                    std::cerr << "[QueueView] Skipping invalid entry.\n";
                    continue;
                }
                int taskInt = entry["taskType"];
                auto taskType = static_cast<ECS::SDCPPSystem::TaskType>(taskInt);
                const auto& entityData = entry["entityData"];
                sys->QueueTaskFromSerialized(entityData, taskType);
                loadedCount++;
            }

            std::cout << "[QueueView] Quick loaded " << loadedCount << " tasks from " << filepath << "\n";
            m_queueLoaded = true;
        }
        catch (const std::exception& e) {
            std::cerr << "[QueueView] Exception during QuickLoad: " << e.what() << "\n";
        }
    }

    nlohmann::json QueueView::Serialize() const {
        nlohmann::json j;

        j["selectedViewIndex"] = m_selectedViewIndex;

        auto sys = m_entityManager.GetSystem<ECS::SDCPPSystem>();
        if (sys) {
            auto tasks = sys->GetQueueTasksWithMetadata();
            nlohmann::json queueData = nlohmann::json::array();
            for (const auto& [taskType, entityData] : tasks) {
                nlohmann::json entry;
                entry["taskType"] = static_cast<int>(taskType);
                entry["entityData"] = entityData;
                queueData.push_back(entry);
            }
            j["queueData"] = queueData;
        }
        return j;
    }

    void QueueView::Deserialize(const nlohmann::json& j) {
        if (j.contains("selectedViewIndex")) {
            m_selectedViewIndex = j["selectedViewIndex"].get<int>();
        }

        if (j.contains("queueData") && j["queueData"].is_array()) {
            auto sys = m_entityManager.GetSystem<ECS::SDCPPSystem>();
            if (!sys) {
                std::cerr << "[QueueView] SDCPPSystem not available for deserialization.\n";
                return;
            }

            sys->ClearAllTasks();

            int loadedCount = 0;
            for (const auto& entry : j["queueData"]) {
                if (!entry.contains("taskType") || !entry.contains("entityData")) {
                    std::cerr << "[QueueView] Skipping invalid queue entry.\n";
                    continue;
                }
                int taskInt = entry["taskType"];
                auto taskType = static_cast<ECS::SDCPPSystem::TaskType>(taskInt);
                const auto& entityData = entry["entityData"];
                sys->QueueTaskFromSerialized(entityData, taskType);
                loadedCount++;
            }
            std::cout << "[QueueView] Deserialized " << loadedCount << " tasks from viewstate.\n";
            m_queueLoaded = true;
        }
    }

    void QueueView::Render() {
        ImGui::SetNextWindowSize(ImVec2(600, 700), ImGuiCond_FirstUseEver);
        if (ImGui::Begin(GetWindowTitle().c_str(), &windowOpen, ImGuiWindowFlags_MenuBar)) {
            RenderMenuBar();
            RenderViewSelector();
            RenderQueueList();
        }
        ImGui::End();

        if (!windowOpen) {
            std::unordered_map<std::string, std::any> eventData;
            eventData["workspaceID"] = GetID();
            eventData["viewTypeName"] = viewName;
            ANI::Events::Ref().QueueEventWithData("RemoveView", eventData);
        }
    }

} // namespace GUI