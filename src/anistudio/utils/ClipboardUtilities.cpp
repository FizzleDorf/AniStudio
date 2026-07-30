#include "ClipboardUtilities.hpp"
#include "PropertyTypes.hpp"
#include "ECS.h"
#include "BaseComponent.hpp"
#include "PngMetadataUtils.hpp"
#include <iostream>
#include <imgui.h>

namespace GUI {
    namespace Clipboard {

        static Type s_type = Type::None;
        static nlohmann::json s_data;

        static nlohmann::json PropertyVariantToJson(const Engine::PropertyVariant& var) {
            if (auto* p = std::get_if<bool*>(&var)) return **p;
            if (auto* p = std::get_if<int*>(&var)) return **p;
            if (auto* p = std::get_if<float*>(&var)) return **p;
            if (auto* p = std::get_if<double*>(&var)) return **p;
            if (auto* p = std::get_if<std::string*>(&var)) return **p;
            if (auto* p = std::get_if<ImVec2*>(&var)) return nlohmann::json::array({ (*p)->x, (*p)->y });
            if (auto* p = std::get_if<ImVec4*>(&var)) return nlohmann::json::array({ (*p)->x, (*p)->y, (*p)->z, (*p)->w });
            if (auto* p = std::get_if<std::vector<std::string>*>(&var)) return nlohmann::json(**p);
            return nullptr;
        }

        static bool JsonToPropertyVariant(const nlohmann::json& jsonVal, Engine::PropertyVariant& var) {
            if (auto* p = std::get_if<bool*>(&var)) {
                if (jsonVal.is_boolean()) { **p = jsonVal.get<bool>(); return true; }
            }
            else if (auto* p = std::get_if<int*>(&var)) {
                if (jsonVal.is_number_integer()) { **p = jsonVal.get<int>(); return true; }
            }
            else if (auto* p = std::get_if<float*>(&var)) {
                if (jsonVal.is_number()) { **p = jsonVal.get<float>(); return true; }
            }
            else if (auto* p = std::get_if<double*>(&var)) {
                if (jsonVal.is_number()) { **p = jsonVal.get<double>(); return true; }
            }
            else if (auto* p = std::get_if<std::string*>(&var)) {
                if (jsonVal.is_string()) { **p = jsonVal.get<std::string>(); return true; }
            }
            else if (auto* p = std::get_if<ImVec2*>(&var)) {
                if (jsonVal.is_array() && jsonVal.size() >= 2) {
                    (*p)->x = jsonVal[0].get<float>();
                    (*p)->y = jsonVal[1].get<float>();
                    return true;
                }
            }
            else if (auto* p = std::get_if<ImVec4*>(&var)) {
                if (jsonVal.is_array() && jsonVal.size() >= 4) {
                    (*p)->x = jsonVal[0].get<float>();
                    (*p)->y = jsonVal[1].get<float>();
                    (*p)->z = jsonVal[2].get<float>();
                    (*p)->w = jsonVal[3].get<float>();
                    return true;
                }
            }
            else if (auto* p = std::get_if<std::vector<std::string>*>(&var)) {
                if (jsonVal.is_array()) {
                    **p = jsonVal.get<std::vector<std::string>>();
                    return true;
                }
            }
            return false;
        }

        static nlohmann::json GetWrappedClipboardData() {
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

        static void SetWrappedClipboardData(const nlohmann::json& data) {
            std::string clipboardString = data.dump();
            ImGui::SetClipboardText(clipboardString.c_str());
        }

        void CopyEntity(ECS::EntityManager& mgr, ECS::EntityID entity) {
            if (!mgr.IsEntityValid(entity)) return;
            nlohmann::json entityData = mgr.SerializeEntity(entity);
            nlohmann::json wrappedData;
            wrappedData["dataType"] = "entity";
            wrappedData["data"] = entityData;
            wrappedData["source"] = "entity";
            SetWrappedClipboardData(wrappedData);
            s_type = Type::Entity;
            s_data = entityData;
        }

        void CopyComponent(ECS::EntityManager& mgr, ECS::EntityID entity, const std::string& compName) {
            auto cid = mgr.GetComponentTypeIdByName(compName);
            if (cid == 0 || !mgr.HasComponentById(entity, cid)) return;
            auto* comp = mgr.GetComponentById(entity, cid);
            if (!comp) return;
            nlohmann::json compData = comp->Serialize();
            nlohmann::json wrappedData;
            wrappedData["dataType"] = "component";
            wrappedData["componentName"] = compName;
            wrappedData["componentData"] = compData;
            wrappedData["source"] = "entity";
            SetWrappedClipboardData(wrappedData);
            s_type = Type::Component;
            s_data = compData;
            s_data["__componentName"] = compName;
        }

        void CopyProperty(ECS::EntityManager& mgr, ECS::EntityID entity, const std::string& compName, const std::string& propName) {
            auto cid = mgr.GetComponentTypeIdByName(compName);
            if (cid == 0 || !mgr.HasComponentById(entity, cid)) return;
            auto* comp = mgr.GetComponentById(entity, cid);
            if (!comp) return;
            auto props = comp->GetPropertyMap();
            auto it = props.find(propName);
            if (it == props.end()) return;
            nlohmann::json propData;
            propData["value"] = PropertyVariantToJson(it->second);
            nlohmann::json wrappedData;
            wrappedData["dataType"] = "property";
            wrappedData["componentName"] = compName;
            wrappedData["propertyName"] = propName;
            wrappedData["propertyData"] = propData;
            wrappedData["source"] = "entity";
            SetWrappedClipboardData(wrappedData);
            s_type = Type::Property;
            s_data = propData;
            s_data["__componentName"] = compName;
            s_data["__propertyName"] = propName;
        }

        static nlohmann::json LoadMetadataFromImage(const std::string& imagePath) {
            return Utils::PngMetadata::ReadMetadataFromPNG(imagePath);
        }

        void CopyImageMetadata(ECS::EntityManager& mgr, const std::string& imagePath) {
            nlohmann::json metadata = LoadMetadataFromImage(imagePath);
            if (metadata.is_null() || !metadata.is_object()) {
                std::cerr << "[Clipboard] Failed to read metadata from: " << imagePath << std::endl;
                return;
            }
            nlohmann::json wrappedData;
            wrappedData["dataType"] = "entity";
            wrappedData["data"] = metadata;
            wrappedData["source"] = "metadata";
            SetWrappedClipboardData(wrappedData);
            s_type = Type::Entity;
            s_data = metadata;
            std::cout << "[Clipboard] Copied image metadata from: " << imagePath << std::endl;
        }

        void CopyComponentFromImageMetadata(ECS::EntityManager& mgr, const std::string& imagePath, const std::string& compName) {
            nlohmann::json metadata = LoadMetadataFromImage(imagePath);
            if (metadata.is_null() || !metadata.is_object()) {
                std::cerr << "[Clipboard] Failed to read metadata from: " << imagePath << std::endl;
                return;
            }
            nlohmann::json componentData;
            bool found = false;
            if (metadata.contains("components") && metadata["components"].is_array()) {
                for (const auto& comp : metadata["components"]) {
                    if (comp.contains(compName)) {
                        componentData = comp[compName];
                        found = true;
                        break;
                    }
                }
            }
            else if (metadata.contains(compName)) {
                componentData = metadata[compName];
                found = true;
            }
            if (!found) {
                std::cerr << "[Clipboard] Component " << compName << " not found in metadata." << std::endl;
                return;
            }
            nlohmann::json wrappedData;
            wrappedData["dataType"] = "component";
            wrappedData["componentName"] = compName;
            wrappedData["componentData"] = componentData;
            wrappedData["source"] = "metadata";
            SetWrappedClipboardData(wrappedData);
            s_type = Type::Component;
            s_data = componentData;
            s_data["__componentName"] = compName;
            std::cout << "[Clipboard] Copied component " << compName << " from image metadata." << std::endl;
        }

        void CopyPropertyFromImageMetadata(ECS::EntityManager& mgr, const std::string& imagePath, const std::string& compName, const std::string& propName) {
            nlohmann::json metadata = LoadMetadataFromImage(imagePath);
            if (metadata.is_null() || !metadata.is_object()) {
                std::cerr << "[Clipboard] Failed to read metadata from: " << imagePath << std::endl;
                return;
            }
            nlohmann::json componentData;
            bool found = false;
            if (metadata.contains("components") && metadata["components"].is_array()) {
                for (const auto& comp : metadata["components"]) {
                    if (comp.contains(compName)) {
                        componentData = comp[compName];
                        found = true;
                        break;
                    }
                }
            }
            else if (metadata.contains(compName)) {
                componentData = metadata[compName];
                found = true;
            }
            if (!found) {
                std::cerr << "[Clipboard] Component " << compName << " not found in metadata." << std::endl;
                return;
            }
            if (!componentData.contains(propName)) {
                std::cerr << "[Clipboard] Property " << propName << " not found in component " << compName << std::endl;
                return;
            }
            nlohmann::json propData;
            propData["value"] = componentData[propName];
            nlohmann::json wrappedData;
            wrappedData["dataType"] = "property";
            wrappedData["componentName"] = compName;
            wrappedData["propertyName"] = propName;
            wrappedData["propertyData"] = propData;
            wrappedData["source"] = "metadata";
            SetWrappedClipboardData(wrappedData);
            s_type = Type::Property;
            s_data = propData;
            s_data["__componentName"] = compName;
            s_data["__propertyName"] = propName;
            std::cout << "[Clipboard] Copied property " << propName << " from image metadata." << std::endl;
        }

        Type GetType() {
            return s_type;
        }

        bool HasEntity() { return s_type == Type::Entity; }
        bool HasComponent() { return s_type == Type::Component; }
        bool HasProperty() { return s_type == Type::Property; }

        nlohmann::json GetData() {
            return s_data;
        }

        bool PasteEntity(ECS::EntityManager& mgr, ECS::EntityID target, std::function<void(ECS::EntityID, const std::string&)> addComponent) {
            if (s_type != Type::Entity || s_data.is_null()) return false;
            if (!mgr.IsEntityValid(target)) return false;
            auto comps = mgr.GetEntityComponents(target);
            for (auto cid : comps) mgr.RemoveComponentById(target, cid);
            ECS::EntityID temp = mgr.DeserializeEntity(s_data);
            if (temp == 0) {
                std::cerr << "[Clipboard] Failed to deserialize entity for paste" << std::endl;
                return false;
            }
            auto tempComps = mgr.GetEntityComponents(temp);
            for (auto cid : tempComps) {
                auto* srcComp = mgr.GetComponentById(temp, cid);
                if (!srcComp) continue;
                std::string name = mgr.GetComponentNameById(cid);
                if (addComponent) {
                    addComponent(target, name);
                    auto* dstComp = mgr.GetComponentById(target, cid);
                    if (dstComp) {
                        dstComp->Deserialize(srcComp->Serialize());
                        dstComp->RefreshSchema();
                    }
                }
            }
            mgr.DestroyEntity(temp);
            return true;
        }

        bool PasteComponent(ECS::EntityManager& mgr, ECS::EntityID target, const std::string& compName) {
            if (s_type != Type::Component || s_data.is_null()) return false;
            if (!mgr.IsEntityValid(target)) return false;
            std::string storedName = s_data.value("__componentName", "");
            if (storedName != compName) return false;
            auto cid = mgr.GetComponentTypeIdByName(compName);
            if (cid == 0) return false;
            auto* comp = mgr.GetComponentById(target, cid);
            if (!comp) return false;
            nlohmann::json data = s_data;
            data.erase("__componentName");
            comp->Deserialize(data);
            comp->RefreshSchema();
            return true;
        }

        bool PasteProperty(ECS::EntityManager& mgr, ECS::EntityID target, const std::string& compName, const std::string& propName) {
            if (s_type != Type::Property || s_data.is_null()) return false;
            if (!mgr.IsEntityValid(target)) return false;
            std::string storedComp = s_data.value("__componentName", "");
            std::string storedProp = s_data.value("__propertyName", "");
            if (storedComp != compName || storedProp != propName) return false;
            auto cid = mgr.GetComponentTypeIdByName(compName);
            if (cid == 0 || !mgr.HasComponentById(target, cid)) return false;
            auto* comp = mgr.GetComponentById(target, cid);
            if (!comp) return false;
            auto props = comp->GetPropertyMap();
            auto it = props.find(propName);
            if (it == props.end()) return false;
            if (!JsonToPropertyVariant(s_data["value"], it->second)) return false;
            comp->RefreshSchema();
            return true;
        }

        bool PasteComponentFromEntity(ECS::EntityManager& mgr, ECS::EntityID target, const std::string& compName, std::function<void(ECS::EntityID, const std::string&)> addComponent) {
            if (s_type != Type::Entity || s_data.is_null()) return false;
            if (!mgr.IsEntityValid(target)) return false;
            nlohmann::json entityData = s_data;
            nlohmann::json componentData;
            bool found = false;
            if (entityData.contains("components") && entityData["components"].is_array()) {
                for (const auto& comp : entityData["components"]) {
                    if (comp.contains(compName)) {
                        componentData = comp[compName];
                        found = true;
                        break;
                    }
                }
            }
            else if (entityData.contains(compName)) {
                componentData = entityData[compName];
                found = true;
            }
            if (!found) return false;
            auto cid = mgr.GetComponentTypeIdByName(compName);
            if (cid == 0) return false;
            if (!mgr.HasComponentById(target, cid)) {
                if (addComponent) addComponent(target, compName);
                if (!mgr.HasComponentById(target, cid)) return false;
            }
            auto* comp = mgr.GetComponentById(target, cid);
            if (!comp) return false;
            comp->Deserialize(componentData);
            comp->RefreshSchema();
            return true;
        }

        bool PastePropertyFromEntity(ECS::EntityManager& mgr, ECS::EntityID target, const std::string& compName, const std::string& propName) {
            if (s_type != Type::Entity || s_data.is_null()) return false;
            if (!mgr.IsEntityValid(target)) return false;
            nlohmann::json entityData = s_data;
            nlohmann::json componentData;
            bool found = false;
            if (entityData.contains("components") && entityData["components"].is_array()) {
                for (const auto& comp : entityData["components"]) {
                    if (comp.contains(compName)) {
                        componentData = comp[compName];
                        found = true;
                        break;
                    }
                }
            }
            else if (entityData.contains(compName)) {
                componentData = entityData[compName];
                found = true;
            }
            if (!found) return false;
            if (!componentData.contains(propName)) return false;
            auto cid = mgr.GetComponentTypeIdByName(compName);
            if (cid == 0 || !mgr.HasComponentById(target, cid)) return false;
            auto* comp = mgr.GetComponentById(target, cid);
            if (!comp) return false;
            auto props = comp->GetPropertyMap();
            auto it = props.find(propName);
            if (it == props.end()) return false;
            if (!JsonToPropertyVariant(componentData[propName], it->second)) return false;
            comp->RefreshSchema();
            return true;
        }

        std::vector<std::string> GetEntityComponentNames(ECS::EntityManager& mgr) {
            std::vector<std::string> result;
            if (s_type != Type::Entity || s_data.is_null()) return result;
            nlohmann::json entityData = s_data;
            if (entityData.contains("components") && entityData["components"].is_array()) {
                for (const auto& comp : entityData["components"]) {
                    if (comp.is_object()) {
                        for (auto it = comp.begin(); it != comp.end(); ++it) {
                            result.push_back(it.key());
                        }
                    }
                }
            }
            else {
                for (auto it = entityData.begin(); it != entityData.end(); ++it) {
                    if (it.value().is_object()) {
                        result.push_back(it.key());
                    }
                }
            }
            return result;
        }

        std::vector<std::string> GetComponentPropertyNames(ECS::EntityManager& mgr, const std::string& compName) {
            std::vector<std::string> result;
            if (s_type != Type::Entity || s_data.is_null()) return result;
            nlohmann::json entityData = s_data;
            nlohmann::json componentData;
            if (entityData.contains("components") && entityData["components"].is_array()) {
                for (const auto& comp : entityData["components"]) {
                    if (comp.contains(compName)) {
                        componentData = comp[compName];
                        break;
                    }
                }
            }
            else if (entityData.contains(compName)) {
                componentData = entityData[compName];
            }
            if (componentData.is_object()) {
                for (auto it = componentData.begin(); it != componentData.end(); ++it) {
                    result.push_back(it.key());
                }
            }
            return result;
        }

        bool EntityClipboardHasComponent(ECS::EntityManager& mgr, const std::string& compName) {
            if (s_type != Type::Entity || s_data.is_null()) return false;
            nlohmann::json entityData = s_data;
            if (entityData.contains("components") && entityData["components"].is_array()) {
                for (const auto& comp : entityData["components"]) {
                    if (comp.contains(compName)) return true;
                }
            }
            else if (entityData.contains(compName)) {
                return true;
            }
            return false;
        }

        bool EntityClipboardHasProperty(ECS::EntityManager& mgr, const std::string& compName, const std::string& propName) {
            if (s_type != Type::Entity || s_data.is_null()) return false;
            nlohmann::json entityData = s_data;
            nlohmann::json componentData;
            if (entityData.contains("components") && entityData["components"].is_array()) {
                for (const auto& comp : entityData["components"]) {
                    if (comp.contains(compName)) {
                        componentData = comp[compName];
                        break;
                    }
                }
            }
            else if (entityData.contains(compName)) {
                componentData = entityData[compName];
            }
            return componentData.is_object() && componentData.contains(propName);
        }

        void AddComponent(ECS::EntityManager& mgr, ECS::EntityID target, const std::string& compName, std::function<void(ECS::EntityID, const std::string&)> addComponent) {
            if (!addComponent) return;
            auto cid = mgr.GetComponentTypeIdByName(compName);
            if (cid == 0 || mgr.HasComponentById(target, cid)) return;
            addComponent(target, compName);
        }

        void RemoveComponent(ECS::EntityManager& mgr, ECS::EntityID target, const std::string& compName) {
            auto cid = mgr.GetComponentTypeIdByName(compName);
            if (cid == 0) return;
            if (mgr.HasComponentById(target, cid)) mgr.RemoveComponentById(target, cid);
        }

        void ResetComponent(ECS::EntityManager& mgr, ECS::EntityID target, const std::string& compName, std::function<void(ECS::EntityID, const std::string&)> addComponent) {
            RemoveComponent(mgr, target, compName);
            AddComponent(mgr, target, compName, addComponent);
        }

        void ResetAllComponents(ECS::EntityManager& mgr, ECS::EntityID target, const std::vector<std::string>& compNames, std::function<void(ECS::EntityID, const std::string&)> addComponent) {
            for (const auto& name : compNames) ResetComponent(mgr, target, name, addComponent);
        }

    } // namespace Clipboard
} // namespace GUI