#include "DragDropUtils.hpp"
#include "FileFormats.hpp"
#include <iostream>
#include <mutex>
#include <queue>
#include <filesystem>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <ole2.h>
#include <GLFW/glfw3native.h>
#include <imgui.h>
#endif

#ifdef __linux__
#include <imgui.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <X11/Xlib.h>
#endif

namespace GUI {
    namespace DragDrop {

        static std::vector<std::string> g_droppedFiles;
        static std::mutex g_dropMutex;
        static bool g_hasNewDrop = false;
        static void* g_windowHandle = nullptr;
        static bool g_isExternalDragActive = false;
        static std::vector<std::string> g_pendingDropFiles;

#ifdef _WIN32

        class DropTarget : public IDropTarget {
        private:
            LONG m_refCount;
            bool m_isDragging;

        public:
            DropTarget() : m_refCount(1), m_isDragging(false) {}

            HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override {
                if (riid == IID_IUnknown || riid == IID_IDropTarget) {
                    *ppvObject = this;
                    AddRef();
                    return S_OK;
                }
                *ppvObject = nullptr;
                return E_NOINTERFACE;
            }

            ULONG STDMETHODCALLTYPE AddRef() override {
                return InterlockedIncrement(&m_refCount);
            }

            ULONG STDMETHODCALLTYPE Release() override {
                LONG count = InterlockedDecrement(&m_refCount);
                if (count == 0) {
                    delete this;
                }
                return count;
            }

            HRESULT STDMETHODCALLTYPE DragEnter(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) override {
                FORMATETC fmte = { CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
                if (pDataObj->QueryGetData(&fmte) != S_OK) {
                    *pdwEffect = DROPEFFECT_NONE;
                    return S_OK;
                }

                m_isDragging = true;
                g_isExternalDragActive = true;

                ImGuiIO& io = ImGui::GetIO();
                io.MouseDown[0] = true;
                io.MousePos = ImVec2(static_cast<float>(pt.x), static_cast<float>(pt.y));

                *pdwEffect = DROPEFFECT_COPY;
                return S_OK;
            }

            HRESULT STDMETHODCALLTYPE DragOver(DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) override {
                ImGuiIO& io = ImGui::GetIO();
                io.MousePos = ImVec2(static_cast<float>(pt.x), static_cast<float>(pt.y));

                *pdwEffect = DROPEFFECT_COPY;
                return S_OK;
            }

            HRESULT STDMETHODCALLTYPE DragLeave() override {
                m_isDragging = false;
                g_isExternalDragActive = false;

                ImGuiIO& io = ImGui::GetIO();
                io.MouseDown[0] = false;

                return S_OK;
            }

            HRESULT STDMETHODCALLTYPE Drop(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) override {
                FORMATETC fmte = { CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
                STGMEDIUM stgm;

                if (SUCCEEDED(pDataObj->GetData(&fmte, &stgm))) {
                    HDROP hdrop = static_cast<HDROP>(stgm.hGlobal);
                    UINT fileCount = DragQueryFileW(hdrop, 0xFFFFFFFF, nullptr, 0);

                    std::lock_guard<std::mutex> lock(g_dropMutex);
                    g_droppedFiles.clear();
                    g_pendingDropFiles.clear();

                    for (UINT i = 0; i < fileCount; ++i) {
                        wchar_t wpath[MAX_PATH];
                        if (DragQueryFileW(hdrop, i, wpath, MAX_PATH) > 0) {
                            int size_needed = WideCharToMultiByte(CP_UTF8, 0, wpath, -1, nullptr, 0, nullptr, nullptr);
                            if (size_needed > 0) {
                                std::string path(size_needed - 1, 0);
                                WideCharToMultiByte(CP_UTF8, 0, wpath, -1, &path[0], size_needed, nullptr, nullptr);
                                g_droppedFiles.push_back(path);
                                g_pendingDropFiles.push_back(path);
                            }
                        }
                    }

                    ReleaseStgMedium(&stgm);
                    g_hasNewDrop = true;
                }

                m_isDragging = false;
                g_isExternalDragActive = false;

                ImGuiIO& io = ImGui::GetIO();
                io.MouseDown[0] = false;
                io.MousePos = ImVec2(static_cast<float>(pt.x), static_cast<float>(pt.y));

                *pdwEffect = DROPEFFECT_COPY;
                return S_OK;
            }
        };

        static DropTarget* g_dropTarget = nullptr;
        static bool g_comInitialized = false;

        void InstallDropHandler(HWND hwnd) {
            if (!hwnd) {
                std::cerr << "[DragDrop] ERROR: Cannot install drop handler, HWND is null!" << std::endl;
                return;
            }

            if (!g_comInitialized) {
                HRESULT hr = OleInitialize(nullptr);
                if (SUCCEEDED(hr) || hr == S_FALSE) {
                    g_comInitialized = true;
                }
                else {
                    std::cerr << "[DragDrop] OleInitialize failed!" << std::endl;
                    return;
                }
            }

            g_dropTarget = new DropTarget();
            HRESULT hr = RegisterDragDrop(hwnd, g_dropTarget);
            if (SUCCEEDED(hr)) {
                std::cout << "[DragDrop] RegisterDragDrop succeeded for HWND: " << hwnd << std::endl;
            }
            else {
                std::cerr << "[DragDrop] RegisterDragDrop failed!" << std::endl;
                g_dropTarget->Release();
                g_dropTarget = nullptr;
            }
        }

        void UninstallDropHandler(HWND hwnd) {
            if (hwnd) {
                RevokeDragDrop(hwnd);
            }
            if (g_dropTarget) {
                g_dropTarget->Release();
                g_dropTarget = nullptr;
            }
            if (g_comInitialized) {
                OleUninitialize();
                g_comInitialized = false;
            }
        }

        class DropSource : public IDropSource {
            LONG m_refCount;
        public:
            DropSource() : m_refCount(1) {}
            ~DropSource() {}

            HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override {
                if (riid == IID_IUnknown || riid == IID_IDropSource) {
                    *ppv = this;
                    AddRef();
                    return S_OK;
                }
                *ppv = nullptr;
                return E_NOINTERFACE;
            }

            ULONG STDMETHODCALLTYPE AddRef() override {
                return InterlockedIncrement(&m_refCount);
            }

            ULONG STDMETHODCALLTYPE Release() override {
                LONG count = InterlockedDecrement(&m_refCount);
                if (count == 0) {
                    delete this;
                }
                return count;
            }

            HRESULT STDMETHODCALLTYPE QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState) override {
                if (fEscapePressed)
                    return DRAGDROP_S_CANCEL;
                if (!(grfKeyState & MK_LBUTTON))
                    return DRAGDROP_S_DROP;
                return S_OK;
            }

            HRESULT STDMETHODCALLTYPE GiveFeedback(DWORD dwEffect) override {
                return DRAGDROP_S_USEDEFAULTCURSORS;
            }
        };

        class DataObject : public IDataObject {
            LONG m_refCount;
            std::vector<std::string> m_files;
            STGMEDIUM m_stgMedium;
            FORMATETC m_formatEtc;
        public:
            DataObject(const std::vector<std::string>& files) : m_refCount(1), m_files(files) {
                ZeroMemory(&m_stgMedium, sizeof(STGMEDIUM));
                ZeroMemory(&m_formatEtc, sizeof(FORMATETC));
                m_formatEtc.cfFormat = CF_HDROP;
                m_formatEtc.ptd = nullptr;
                m_formatEtc.dwAspect = DVASPECT_CONTENT;
                m_formatEtc.lindex = -1;
                m_formatEtc.tymed = TYMED_HGLOBAL;

                if (files.empty()) return;
                size_t totalSize = 1;
                for (const auto& f : files) {
                    totalSize += f.length() + 1;
                }
                totalSize++;

                HGLOBAL hDrop = GlobalAlloc(GHND, sizeof(DROPFILES) + totalSize * sizeof(WCHAR));
                if (hDrop) {
                    DROPFILES* pDrop = (DROPFILES*)GlobalLock(hDrop);
                    pDrop->pFiles = sizeof(DROPFILES);
                    pDrop->fWide = TRUE;
                    pDrop->fNC = FALSE;
                    char* pData = (char*)(pDrop + 1);
                    for (const auto& f : files) {
                        std::wstring wpath = std::wstring(f.begin(), f.end());
                        wcscpy_s((wchar_t*)pData, (totalSize - (pData - (char*)pDrop)) / sizeof(wchar_t), wpath.c_str());
                        pData += (wpath.length() + 1) * sizeof(wchar_t);
                    }
                    *(wchar_t*)pData = 0;
                    GlobalUnlock(hDrop);
                    m_stgMedium.tymed = TYMED_HGLOBAL;
                    m_stgMedium.hGlobal = hDrop;
                    m_stgMedium.pUnkForRelease = nullptr;
                }
            }

            ~DataObject() {
                ReleaseStgMedium(&m_stgMedium);
            }

            HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override {
                if (riid == IID_IUnknown || riid == IID_IDataObject) {
                    *ppv = this;
                    AddRef();
                    return S_OK;
                }
                *ppv = nullptr;
                return E_NOINTERFACE;
            }

            ULONG STDMETHODCALLTYPE AddRef() override {
                return InterlockedIncrement(&m_refCount);
            }

            ULONG STDMETHODCALLTYPE Release() override {
                LONG count = InterlockedDecrement(&m_refCount);
                if (count == 0) {
                    delete this;
                }
                return count;
            }

            HRESULT STDMETHODCALLTYPE GetData(FORMATETC* pFormatEtc, STGMEDIUM* pMedium) override {
                if (pFormatEtc->cfFormat == CF_HDROP && pFormatEtc->tymed & TYMED_HGLOBAL) {
                    *pMedium = m_stgMedium;
                    return S_OK;
                }
                return DV_E_FORMATETC;
            }

            HRESULT STDMETHODCALLTYPE GetDataHere(FORMATETC* pFormatEtc, STGMEDIUM* pMedium) override { return E_NOTIMPL; }
            HRESULT STDMETHODCALLTYPE QueryGetData(FORMATETC* pFormatEtc) override { return E_NOTIMPL; }
            HRESULT STDMETHODCALLTYPE GetCanonicalFormatEtc(FORMATETC* pFormatEtcIn, FORMATETC* pFormatEtcOut) override { return E_NOTIMPL; }
            HRESULT STDMETHODCALLTYPE SetData(FORMATETC* pFormatEtc, STGMEDIUM* pMedium, BOOL fRelease) override { return E_NOTIMPL; }
            HRESULT STDMETHODCALLTYPE EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC** ppEnumFormatEtc) override { return E_NOTIMPL; }
            HRESULT STDMETHODCALLTYPE DAdvise(FORMATETC* pFormatEtc, DWORD advf, IAdviseSink* pAdvSink, DWORD* pdwConnection) override { return E_NOTIMPL; }
            HRESULT STDMETHODCALLTYPE DUnadvise(DWORD dwConnection) override { return E_NOTIMPL; }
            HRESULT STDMETHODCALLTYPE EnumDAdvise(IEnumSTATDATA** ppEnumAdvise) override { return E_NOTIMPL; }
        };

        bool StartExternalDragFilesWindows(const std::vector<std::string>& filePaths, void* windowHandle) {
            if (filePaths.empty()) return false;

            if (!g_comInitialized) {
                HRESULT hr = OleInitialize(nullptr);
                if (FAILED(hr) && hr != S_FALSE) {
                    std::cerr << "[DragDrop] OleInitialize failed for drag-out!" << std::endl;
                    return false;
                }
                g_comInitialized = true;
            }

            DataObject* pDataObject = new DataObject(filePaths);
            DropSource* pDropSource = new DropSource();

            DWORD dwEffect = DROPEFFECT_COPY | DROPEFFECT_MOVE;
            HWND hwnd = (HWND)windowHandle;
            HRESULT hr = ::DoDragDrop(pDataObject, pDropSource, dwEffect, &dwEffect);

            pDataObject->Release();
            pDropSource->Release();

            return SUCCEEDED(hr);
        }

#endif

        void GlfwDropCallback(GLFWwindow* window, int count, const char** paths) {
            std::lock_guard<std::mutex> lock(g_dropMutex);
            g_droppedFiles.clear();
            for (int i = 0; i < count; ++i) {
                if (paths[i]) {
                    g_droppedFiles.push_back(std::string(paths[i]));
                }
            }
            g_hasNewDrop = true;
        }

        void InitializeFileDrop(GLFWwindow* window) {
            if (!window) {
                std::cerr << "[DragDrop] ERROR: Cannot initialize file drop, window is null!" << std::endl;
                return;
            }

            SetWindowHandle(window);

#ifdef _WIN32
            HWND hwnd = glfwGetWin32Window(window);
            if (hwnd) {
                InstallDropHandler(hwnd);
                SetWindowHandle(hwnd);
            }
            else {
                std::cerr << "[DragDrop] ERROR: Failed to get HWND from GLFW window!" << std::endl;
                glfwSetDropCallback(window, GlfwDropCallback);
            }
#else
            glfwSetDropCallback(window, GlfwDropCallback);
#endif
        }

        void ShutdownFileDrop(GLFWwindow* window) {
#ifdef _WIN32
            HWND hwnd = window ? glfwGetWin32Window(window) : nullptr;
            UninstallDropHandler(hwnd);
#endif
        }

        bool PollFileDrop(std::vector<std::string>& outFiles) {
            std::lock_guard<std::mutex> lock(g_dropMutex);
            if (g_hasNewDrop && !g_droppedFiles.empty()) {
                outFiles = g_droppedFiles;
                g_hasNewDrop = false;
                g_droppedFiles.clear();
                return true;
            }
            return false;
        }

        bool IsExternalDragActive() {
            return g_isExternalDragActive;
        }

        const std::vector<std::string>& GetPendingDropFiles() {
            return g_pendingDropFiles;
        }

        void ClearPendingDropFiles() {
            g_pendingDropFiles.clear();
        }

        bool BeginDragSource(const char* payloadType, const nlohmann::json& payload, ImGuiID sourceID) {
            if (IsExternalDragActive()) {
                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceExtern)) {
                    ImGui::SetDragDropPayload(payloadType, nullptr, 0);
                    ImGui::Text("Drop files here");
                    ImGui::EndDragDropSource();
                    if (g_hasNewDrop && !g_pendingDropFiles.empty()) {
                        return true;
                    }
                    return true;
                }
                return false;
            }

            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
                std::string payloadStr = payload.dump();
                ImGui::SetDragDropPayload(payloadType, payloadStr.c_str(), payloadStr.size() + 1);
                ImGui::EndDragDropSource();
                return true;
            }
            return false;
        }

        bool AcceptDragDrop(const char* payloadType, nlohmann::json& outPayload) {
            if (ImGui::BeginDragDropTarget()) {
                const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(payloadType);
                if (payload) {
                    if (IsExternalDragActive() && g_hasNewDrop && !g_pendingDropFiles.empty()) {
                        ImGui::EndDragDropTarget();
                        return true;
                    }

                    try {
                        std::string payloadStr(static_cast<const char*>(payload->Data), payload->DataSize - 1);
                        outPayload = nlohmann::json::parse(payloadStr);
                        ImGui::EndDragDropTarget();
                        return true;
                    }
                    catch (...) {
                        std::cerr << "[DragDrop] Failed to parse payload" << std::endl;
                    }
                }
                ImGui::EndDragDropTarget();
            }
            return false;
        }

        // ===== FIXED AcceptFileDrop =====
        bool AcceptFileDrop(std::vector<std::string>& outFiles) {
            // 1. Check for OS file drop (GLFW or OLE)
            if (PollFileDrop(outFiles)) {
                return true;
            }

            // 2. Internal drag-drop for multiple files
            std::vector<std::string> multiFiles;
            if (AcceptMultipleFileDrop(multiFiles)) {
                outFiles = multiFiles;
                return true;
            }

            // 3. Internal drag-drop for single file
            std::string singleFile, fileType;
            if (AcceptFilePathDrop(singleFile, fileType)) {
                outFiles.push_back(singleFile);
                return true;
            }

            return false;
        }

        bool BeginEntityDrag(ECS::EntityID entity) {
            nlohmann::json payload;
            payload["entityID"] = entity;
            return BeginDragSource(PAYLOAD_ENTITY, payload);
        }

        bool BeginFilePathDrag(const std::string& filePath, const std::string& fileType) {
            nlohmann::json payload;
            payload["filePath"] = filePath;
            if (!fileType.empty()) payload["fileType"] = fileType;
            else payload["fileType"] = GuessMediaType(filePath);
            return BeginDragSource(PAYLOAD_FILE_PATH, payload);
        }

        bool BeginMultipleFileDrag(const std::vector<std::string>& filePaths) {
            nlohmann::json payload;
            payload["files"] = filePaths;
            return BeginDragSource(PAYLOAD_FILE_MULTIPLE, payload);
        }

        bool AcceptEntityDrop(ECS::EntityID& outEntity) {
            nlohmann::json payload;
            if (AcceptDragDrop(PAYLOAD_ENTITY, payload)) {
                if (payload.contains("entityID")) {
                    outEntity = payload["entityID"].get<ECS::EntityID>();
                    return true;
                }
            }
            return false;
        }

        bool AcceptFilePathDrop(std::string& outFilePath, std::string& outFileType) {
            nlohmann::json payload;
            if (AcceptDragDrop(PAYLOAD_FILE_PATH, payload)) {
                if (payload.contains("filePath")) {
                    outFilePath = payload["filePath"].get<std::string>();
                    outFileType = payload.value("fileType", GuessMediaType(outFilePath));
                    return true;
                }
            }
            return false;
        }

        bool AcceptMultipleFileDrop(std::vector<std::string>& outFilePaths) {
            nlohmann::json payload;
            if (AcceptDragDrop(PAYLOAD_FILE_MULTIPLE, payload)) {
                if (payload.contains("files") && payload["files"].is_array()) {
                    outFilePaths = payload["files"].get<std::vector<std::string>>();
                    return true;
                }
            }
            return false;
        }

        std::string GuessMediaType(const std::string& filePath) {
            std::string ext = std::filesystem::path(filePath).extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            const auto& formats = FileFormats::GetAllFormats();
            auto it = formats.find(ext);
            if (it != formats.end()) {
                if (it->second.isImage) return "image";
                if (it->second.isVideo) return "video";
                if (it->second.isAudio) return "audio";
            }
            if (ext == ".safetensors" || ext == ".ckpt" || ext == ".pt" || ext == ".gguf" || ext == ".onnx") {
                return "model";
            }
            if (ext == ".obj" || ext == ".fbx" || ext == ".gltf" || ext == ".glb" || ext == ".stl") {
                return "model";
            }
            return "unknown";
        }

        bool IsMediaFile(const std::string& filePath) {
            std::string type = GuessMediaType(filePath);
            return type != "unknown" && type != "model";
        }

        bool IsImageFile(const std::string& filePath) {
            return GuessMediaType(filePath) == "image";
        }

        bool IsVideoFile(const std::string& filePath) {
            return GuessMediaType(filePath) == "video";
        }

        bool IsAudioFile(const std::string& filePath) {
            return GuessMediaType(filePath) == "audio";
        }

        bool IsModelFile(const std::string& filePath) {
            return GuessMediaType(filePath) == "model";
        }

#ifdef __linux__
        class LinuxDragContext {
        public:
            GtkWidget* window;
            GtkWidget* dragWidget;

            LinuxDragContext(void* winHandle) {
                GdkDisplay* display = gdk_display_get_default();
                GdkScreen* screen = gdk_display_get_default_screen(display);
                window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
                gtk_window_set_default_size(GTK_WINDOW(window), 1, 1);
                gtk_widget_realize(window);

                dragWidget = gtk_drawing_area_new();
                gtk_container_add(GTK_CONTAINER(window), dragWidget);
                gtk_widget_show_all(window);
            }

            ~LinuxDragContext() {
                gtk_widget_destroy(window);
            }
        };

        static std::unique_ptr<LinuxDragContext> g_linuxDragContext;

        bool StartExternalDragFilesLinux(const std::vector<std::string>& filePaths, void* windowHandle) {
            if (filePaths.empty()) return false;

            if (!g_linuxDragContext) {
                g_linuxDragContext = std::make_unique<LinuxDragContext>(windowHandle);
            }

            GtkWidget* widget = g_linuxDragContext->dragWidget;

            GtkTargetList* targetList = gtk_target_list_new(NULL, 0);
            gtk_target_list_add_uri_targets(targetList, 0);

            GtkTargetEntry* targets;
            int nTargets;
            targets = gtk_target_table_new_from_list(targetList, &nTargets);

            GdkEvent* event = gdk_event_new(GDK_BUTTON_PRESS);

            GdkDragContext* context = gtk_drag_begin_with_coordinates(
                widget,
                targetList,
                GDK_ACTION_COPY | GDK_ACTION_MOVE,
                1,
                event,
                -1, -1
            );

            gdk_event_free(event);
            gtk_target_table_free(targets, nTargets);
            gtk_target_list_unref(targetList);

            if (!context) return false;

            std::vector<std::string> uris;
            for (const auto& path : filePaths) {
                std::string uri = "file://" + std::filesystem::absolute(path).string();
                uris.push_back(uri);
            }

            GtkSelectionData* selectionData = gtk_selection_data_new();
            gtk_selection_data_set_uris(selectionData, uris.size(), uris.data());

            gtk_drag_set_icon_default(context);

            bool result = gtk_drag_check_threshold(widget, 0, 0, 0, 0);

            gtk_selection_data_free(selectionData);

            return result;
        }
#endif

        bool StartExternalDragFile(const std::string& filePath, void* windowHandle) {
            return StartExternalDragFiles({ filePath }, windowHandle);
        }

        bool StartExternalDragFiles(const std::vector<std::string>& filePaths, void* windowHandle) {
            if (filePaths.empty()) return false;

#ifdef _WIN32
            return StartExternalDragFilesWindows(filePaths, windowHandle);
#elif defined(__linux__)
            return StartExternalDragFilesLinux(filePaths, windowHandle);
#else
            return false;
#endif
        }

        void SetWindowHandle(void* handle) {
            g_windowHandle = handle;
        }

        void* GetWindowHandle() {
            return g_windowHandle;
        }

    }
}