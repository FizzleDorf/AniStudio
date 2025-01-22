#include "QueueView.hpp"
#include "../Events/Events.hpp"

namespace GUI {

void QueueView::Render() {
    ImGui::SetNextWindowSize(ImVec2(300, 500), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Queue")) {
        // Queue Controls
        if (ImGui::BeginTable("QueueControls", 1, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchSame)) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();

            if (ImGui::InputInt("Queue Count", &numQueues, 1, 4)) {
                if (numQueues < 1)
                    numQueues = 1;
            }

            if (ImGui::Button("Queue Generation")) {
                QueueGeneration(numQueues);
            }

            ImGui::SameLine();
            if (ImGui::Button("Stop")) {
                StopCurrentGeneration();
            }

            ImGui::SameLine();
            if (ImGui::Button("Clear")) {
                ClearQueue();
            }

            ImGui::EndTable();
        }

        // Queue List
        if (ImGui::BeginTable("InferenceQueue", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp)) {
            ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 42.0f);
            ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 42.0f);
            ImGui::TableSetupColumn("Controls", ImGuiTableColumnFlags_WidthFixed, 120.0f);
            ImGui::TableSetupColumn("Move", ImGuiTableColumnFlags_WidthFixed, 42.0f);
            ImGui::TableHeadersRow();

            auto &sdSystem = mgr.GetSystem<ECS::SDCPPSystem>();
            if (sdSystem) {
                auto queueItems = sdSystem->GetQueueSnapshot();
                for (size_t i = 0; i < queueItems.size(); i++) {
                    const auto &item = queueItems[i];

                    ImGui::TableNextRow();
                    
                    // ID column with metadata tooltip
                    ImGui::TableNextColumn();
                    ImGui::Text("%d", static_cast<int>(item.entityID));
                    if (ImGui::IsItemHovered()) {
                        /*ImGui::BeginTooltip();
                        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                        ImGui::TextUnformatted("Prompt:");
                        ImGui::TextColored(ImVec4(0.8f, 0.8f, 1.0f, 1.0f), "%s", prompt.posPrompt.c_str());
                        ImGui::Spacing();
                        ImGui::TextUnformatted("Negative:");
                        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.8f, 1.0f), "%s", prompt.negPrompt.c_str());
                        ImGui::Spacing();
                        ImGui::Text("Size: %dx%d", latent.latentWidth, latent.latentHeight);
                        ImGui::Text("Steps: %d", sampler.steps);
                        ImGui::Text("CFG: %.1f", cfg.cfg);
                        ImGui::Text("Seed: %d", sampler.seed);
                        ImGui::PopTextWrapPos();
                        ImGui::EndTooltip();*/
                    }

                    // Status column
                    ImGui::TableNextColumn();
                    if (item.processing) {
                        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "Active");
                    } else {
                        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.2f, 1.0f), "Queued");
                    }

                    // Controls column
                    ImGui::TableNextColumn();
                    if (!item.processing) {
                        // Small buttons next to each other
                        if (ImGui::Button(("X##" + std::to_string(i)).c_str())) {
                            sdSystem->RemoveFromQueue(i);
                        }
                        ImGui::SameLine();
                        if (i > 1 && ImGui::Button(("Top##" + std::to_string(i)).c_str())) {
                            sdSystem->MoveInQueue(i, 1); // Move to position 1 (right after current)
                        }
                        ImGui::SameLine();
                        if (i < queueItems.size() - 1 && ImGui::Button(("Bottom##" + std::to_string(i)).c_str())) {
                            sdSystem->MoveInQueue(i, queueItems.size() - 1); // Move to end
                        }
                    }

                    // Move column
                    ImGui::TableNextColumn();
                    if (!item.processing) {
                        if (i > 0) {
                            if (ImGui::ArrowButton(("up##" + std::to_string(i)).c_str(), ImGuiDir_Up)) {
                                sdSystem->MoveInQueue(i, i - 1);
                            }
                            if (i < queueItems.size() - 1) {
                                ImGui::SameLine();
                            }
                        }
                        if (i < queueItems.size() - 1) {
                            if (ImGui::ArrowButton(("down##" + std::to_string(i)).c_str(), ImGuiDir_Down)) {
                                sdSystem->MoveInQueue(i, i + 1);
                            }
                        }
                    }
                }
            }
            ImGui::EndTable();
        }
    }
    ImGui::End();
}

void QueueView::QueueGeneration(int count) {
    auto &diffusionView = vMgr.GetView<DiffusionView>(GetID());

    for (int i = 0; i < count; i++) {
        diffusionView.HandleT2IEvent();
        ANI::Event event;
        event.entityID = diffusionView.GetCurrentEntity();
        event.type = ANI::EventType::InferenceRequest;
        ANI::Events::Ref().QueueEvent(event);
        std::cout << "Inference request queued for entity: " << diffusionView.GetCurrentEntity() << std::endl;
    }
}

void QueueView::StopCurrentGeneration() {
    ANI::Event event;
    event.type = ANI::EventType::Interrupt;
    ANI::Events::Ref().QueueEvent(event);
}

void QueueView::ClearQueue() {
    auto &sdSystem = mgr.GetSystem<ECS::SDCPPSystem>();
    if (sdSystem) {
        auto queueItems = sdSystem->GetQueueSnapshot();
        for (size_t i = queueItems.size(); i > 0; --i) {
            sdSystem->RemoveFromQueue(i - 1);
        }
    }
}

//nlohmann::json QueueView::Serialize() const {
//    nlohmann::json j = BaseView::Serialize();
//    j["numQueues"] = numQueues;
//    return j;
//}
//
//void QueueView::Deserialize(const nlohmann::json &j) {
//    BaseView::Deserialize(j);
//    if (j.contains("numQueues"))
//        numQueues = j["numQueues"];
//}

} // namespace GUI