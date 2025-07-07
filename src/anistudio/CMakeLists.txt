cmake_minimum_required(VERSION 3.12)
project(AniStudio VERSION 1.0.0 LANGUAGES CXX)

# Build options
option(ANISTUDIO_BUILD_SHARED "Build AniStudio as shared library" OFF)
option(ANISTUDIO_ENABLE_ZEP "Enable Zep text editor" ON)
option(ANISTUDIO_BUILD_EXAMPLES "Build AniStudio examples" OFF)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Platform specific settings
if(WIN32)
    add_definitions(-DNOMINMAX)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

if(UNIX AND NOT APPLE)
    set(OpenGL_GL_PREFERENCE GLVND)
endif()

# Find dependencies if building standalone
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    # Standalone build - find all dependencies
    include(${CMAKE_BINARY_DIR}/conan/conan_toolchain.cmake OPTIONAL)
    
    find_package(glew REQUIRED)
    find_package(glfw3 REQUIRED)
    find_package(exiv2 CONFIG REQUIRED)
    
    # Set external directory
    set(EXTERNAL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external")
    
    # Find or build AniEngineCore
    find_package(AniEngineCore QUIET)
    if(NOT AniEngineCore_FOUND)
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../aniengine")
            message(STATUS "AniEngineCore not found, building from source")
            add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../aniengine ${CMAKE_BINARY_DIR}/aniengine)
        else()
            message(FATAL_ERROR "AniEngineCore not found and source not available")
        endif()
    endif()
else()
    # Parent project build - use root external directory
    set(EXTERNAL_DIR "${CMAKE_SOURCE_DIR}/external")
endif()

# Add external subdirectories - REQUIRED FIRST
message(STATUS "Adding external dependencies from: ${EXTERNAL_DIR}")

# Required ImGui - only add if not already added
if(NOT TARGET ImGui AND EXISTS "${EXTERNAL_DIR}/imgui/CMakeLists.txt")
    add_subdirectory("${EXTERNAL_DIR}/imgui" "${CMAKE_BINARY_DIR}/imgui")
    message(STATUS "Added ImGui: ${EXTERNAL_DIR}/imgui")
elseif(TARGET ImGui)
    message(STATUS "ImGui already available")
else()
    message(FATAL_ERROR "ImGui not found at ${EXTERNAL_DIR}/imgui. Required for AniStudio.")
endif()

# Required ImGuiFileDialog - only add if not already added
if(NOT TARGET ImGuiFileDialog AND EXISTS "${EXTERNAL_DIR}/imgui_filedialog/CMakeLists.txt")
    add_subdirectory("${EXTERNAL_DIR}/imgui_filedialog" "${CMAKE_BINARY_DIR}/imgui_filedialog")
    message(STATUS "Added ImGuiFileDialog: ${EXTERNAL_DIR}/imgui_filedialog")
elseif(TARGET ImGuiFileDialog)
    message(STATUS "ImGuiFileDialog already available")
else()
    message(FATAL_ERROR "ImGuiFileDialog not found at ${EXTERNAL_DIR}/imgui_filedialog. Required for AniStudio.")
endif()

# Add ImGui extensions - only add if not already added
if(NOT TARGET ImGui_Nodes AND EXISTS "${EXTERNAL_DIR}/imgui_nodes/CMakeLists.txt")
    add_subdirectory("${EXTERNAL_DIR}/imgui_nodes" "${CMAKE_BINARY_DIR}/imgui_nodes")
    message(STATUS "Added ImGui_Nodes: ${EXTERNAL_DIR}/imgui_nodes")
elseif(TARGET ImGui_Nodes)
    message(STATUS "ImGui_Nodes already available")
endif()

if(NOT TARGET ImNodeFlow AND EXISTS "${EXTERNAL_DIR}/ImNodeFlow/CMakeLists.txt")
    add_subdirectory("${EXTERNAL_DIR}/ImNodeFlow" "${CMAKE_BINARY_DIR}/ImNodeFlow")
    message(STATUS "Added ImNodeFlow: ${EXTERNAL_DIR}/ImNodeFlow")
elseif(TARGET ImNodeFlow)
    message(STATUS "ImNodeFlow already available")
endif()

if(NOT TARGET ImGuizmo AND EXISTS "${EXTERNAL_DIR}/ImGuizmo/CMakeLists.txt")
    add_subdirectory("${EXTERNAL_DIR}/ImGuizmo" "${CMAKE_BINARY_DIR}/ImGuizmo")
    message(STATUS "Added ImGuizmo: ${EXTERNAL_DIR}/ImGuizmo")
elseif(TARGET ImGuizmo)
    message(STATUS "ImGuizmo already available")
endif()

# Add NodeEditor - only add if not already added
if(NOT TARGET ax::NodeEditor AND EXISTS "${EXTERNAL_DIR}/node-editor/CMakeLists.txt")
    add_subdirectory("${EXTERNAL_DIR}/node-editor" "${CMAKE_BINARY_DIR}/node-editor")
    message(STATUS "Added NodeEditor: ${EXTERNAL_DIR}/node-editor")
elseif(TARGET ax::NodeEditor)
    message(STATUS "NodeEditor already available")
endif()

# Add ImSequencer - only add if not already added
if(NOT TARGET ImSequencer AND EXISTS "${EXTERNAL_DIR}/ImSequencer/CMakeLists.txt")
    add_subdirectory("${EXTERNAL_DIR}/ImSequencer" "${CMAKE_BINARY_DIR}/ImSequencer")
    message(STATUS "Added ImSequencer: ${EXTERNAL_DIR}/ImSequencer")
elseif(TARGET ImSequencer)
    message(STATUS "ImSequencer already available")
endif()

# Add immvision (skip if missing dependencies) - only add if not already added
if(NOT TARGET immvision AND EXISTS "${EXTERNAL_DIR}/immvision/CMakeLists.txt" AND EXISTS "${EXTERNAL_DIR}/immvision/../cmake/activate_warnings.cmake")
    add_subdirectory("${EXTERNAL_DIR}/immvision" "${CMAKE_BINARY_DIR}/immvision")
    message(STATUS "Added immvision: ${EXTERNAL_DIR}/immvision")
elseif(TARGET immvision)
    message(STATUS "immvision already available")
else()
    message(STATUS "Skipping immvision - missing dependencies or incomplete setup")
endif()

# Add Zep text editor (optional but preferred) - only add if not already added
if(ANISTUDIO_ENABLE_ZEP AND NOT TARGET Zep::Zep AND EXISTS "${EXTERNAL_DIR}/zep/CMakeLists.txt")
    # Configure Zep build options
    set(BUILD_IMGUI OFF CACHE BOOL "Build ImGui Demo" FORCE)
    set(BUILD_QT OFF CACHE BOOL "Build Qt Demo" FORCE) 
    set(BUILD_TESTS OFF CACHE BOOL "Build tests" FORCE)
    set(ZEP_FEATURE_CPP_FILE_SYSTEM ON CACHE BOOL "Enable cpp file system" FORCE)
    
    add_subdirectory("${EXTERNAL_DIR}/zep" "${CMAKE_BINARY_DIR}/zep")
    message(STATUS "Added Zep: ${EXTERNAL_DIR}/zep")
    set(ZEP_AVAILABLE TRUE)
elseif(TARGET Zep::Zep)
    message(STATUS "Zep already available")
    set(ZEP_AVAILABLE TRUE)
else()
    set(ZEP_AVAILABLE FALSE)
    if(ANISTUDIO_ENABLE_ZEP)
        message(WARNING "Zep not found at ${EXTERNAL_DIR}/zep - text editor features will be limited")
    endif()
endif()

# Header-only libraries
set(IMGUI_MARKDOWN_FOUND FALSE)
if(EXISTS "${EXTERNAL_DIR}/imgui_markdown")
    set(IMGUI_MARKDOWN_FOUND TRUE)
    message(STATUS "Found imgui_markdown (header-only)")
endif()

# imgui_tex_inspect
set(IMGUI_TEX_INSPECT_FOUND FALSE)
if(EXISTS "${EXTERNAL_DIR}/imgui_tex_inspect")
    set(IMGUI_TEX_INSPECT_FOUND TRUE)
    file(GLOB IMGUI_TEX_INSPECT_SOURCES "${EXTERNAL_DIR}/imgui_tex_inspect/*.cpp")
    if(IMGUI_TEX_INSPECT_SOURCES)
        list(APPEND ANISTUDIO_SOURCES ${IMGUI_TEX_INSPECT_SOURCES})
    endif()
    message(STATUS "Found imgui_tex_inspect (manual inclusion)")
endif()

# Collect source files
file(GLOB_RECURSE ANISTUDIO_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/gui/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/gui/base/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/gui/project/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/gui_utils/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/events/*.cpp"
)

# Add PluginManager source from aniplugins directory
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../aniplugins/PluginManager.cpp")
    list(APPEND ANISTUDIO_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../aniplugins/PluginManager.cpp")
    message(STATUS "Added PluginManager.cpp to AniStudioCore")
else()
    message(FATAL_ERROR "PluginManager.cpp not found at ${CMAKE_CURRENT_SOURCE_DIR}/../aniplugins/")
endif()

# Add the main AniStudio implementation
list(APPEND ANISTUDIO_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/AniStudio.cpp")

# Create the library
if(ANISTUDIO_BUILD_SHARED)
    add_library(AniStudioCore SHARED ${ANISTUDIO_SOURCES})
    target_compile_definitions(AniStudioCore PRIVATE ANI_STUDIO_EXPORTS)
    target_compile_definitions(AniStudioCore PUBLIC ANI_STUDIO_API=__declspec(dllimport))
else()
    add_library(AniStudioCore STATIC ${ANISTUDIO_SOURCES})
    target_compile_definitions(AniStudioCore PUBLIC ANI_STUDIO_API=)
endif()

# Set library properties
set_target_properties(AniStudioCore PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Include directories
target_include_directories(AniStudioCore PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/gui>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/gui/base>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/gui/project>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/gui_utils>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/events>
    $<INSTALL_INTERFACE:include/anistudio>
)

# External includes
target_include_directories(AniStudioCore PUBLIC
    "${EXTERNAL_DIR}/glm"
    "${EXTERNAL_DIR}/nlohmann_json"
    "${EXTERNAL_DIR}/cr/cr"
    "${EXTERNAL_DIR}/stable-diffusion.cpp"
    "${EXTERNAL_DIR}/stb_image"
    "${EXTERNAL_DIR}/imgui_filedialog"
    "${EXTERNAL_DIR}/imgui_nodes"
    "${EXTERNAL_DIR}/ImNodeFlow/include"
    "${EXTERNAL_DIR}/ImGuizmo"
    "${EXTERNAL_DIR}/ImJSchema"
    "${EXTERNAL_DIR}/imgui_markdown"
)

# Zep include path
if(ZEP_AVAILABLE)
    target_include_directories(AniStudioCore PUBLIC
        "${EXTERNAL_DIR}/zep/include"
    )
endif()

# Plugin interface include
target_include_directories(AniStudioCore PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../aniplugins>
    $<INSTALL_INTERFACE:include/aniplugins>
)

# Core required libraries
target_link_libraries(AniStudioCore PUBLIC
    AniEngineCore
    glfw
    GLEW::GLEW
    exiv2::exiv2
    ImGui
)

# Link ImGuiFileDialog (REQUIRED)
if(TARGET ImGuiFileDialog)
    target_link_libraries(AniStudioCore PUBLIC ImGuiFileDialog)
    target_compile_definitions(AniStudioCore PUBLIC IMGUI_FILEDIALOG_AVAILABLE)
    message(STATUS "Linking ImGuiFileDialog")
else()
    message(FATAL_ERROR "ImGuiFileDialog target not available but required")
endif()

# Link optional dependencies if available
if(TARGET ImGui_Nodes)
    target_link_libraries(AniStudioCore PUBLIC ImGui_Nodes)
    target_compile_definitions(AniStudioCore PUBLIC IMGUI_NODES_AVAILABLE)
    message(STATUS "Linking ImGui_Nodes")
endif()

if(TARGET ImNodeFlow)
    target_link_libraries(AniStudioCore PUBLIC ImNodeFlow)
    target_compile_definitions(AniStudioCore PUBLIC IMNODEFLOW_AVAILABLE)
    message(STATUS "Linking ImNodeFlow")
endif()

if(TARGET ImGuizmo)
    target_link_libraries(AniStudioCore PUBLIC ImGuizmo)
    target_compile_definitions(AniStudioCore PUBLIC IMGUIZMO_AVAILABLE)
    message(STATUS "Linking ImGuizmo")
endif()

if(TARGET ImJSchema)
    target_link_libraries(AniStudioCore PUBLIC ImJSchema)
    target_compile_definitions(AniStudioCore PUBLIC IMJSCHEMA_AVAILABLE)
    message(STATUS "Linking ImJSchema")
endif()

if(TARGET immvision)
    target_link_libraries(AniStudioCore PUBLIC immvision)
    target_compile_definitions(AniStudioCore PUBLIC IMMVISION_AVAILABLE)
    message(STATUS "Linking immvision")
else()
    message(STATUS "immvision not available - skipping")
endif()

# Link NodeEditor if available
if(TARGET ax::NodeEditor)
    target_link_libraries(AniStudioCore PUBLIC ax::NodeEditor)
    target_compile_definitions(AniStudioCore PUBLIC NODE_EDITOR_AVAILABLE)
    message(STATUS "Linking NodeEditor")
endif()

# Link ImSequencer if available
if(TARGET ImSequencer)
    target_link_libraries(AniStudioCore PUBLIC ImSequencer)
    target_compile_definitions(AniStudioCore PUBLIC IMSEQUENCER_AVAILABLE)
    message(STATUS "Linking ImSequencer")
endif()

# Link stb_image
if(TARGET stb_image)
    target_link_libraries(AniStudioCore PUBLIC stb_image)
    message(STATUS "Linking stb_image")
endif()

# Zep text editor
if(ZEP_AVAILABLE AND TARGET Zep::Zep)
    target_link_libraries(AniStudioCore PUBLIC Zep::Zep)
    target_compile_definitions(AniStudioCore PUBLIC ZEP_AVAILABLE)
    message(STATUS "Linking Zep text editor")
endif()

# Compile definitions
target_compile_definitions(AniStudioCore PUBLIC
    GLEW_STATIC
    IMGUI_DEFINE_MATH_OPERATORS
    $<$<CONFIG:Debug>:DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
)

# Install configuration (when building standalone)
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    include(GNUInstallDirs)
    
    # Install the library
    install(TARGETS AniStudioCore
        EXPORT AniStudioCoreTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
    
    # Install headers
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/anistudio
        FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
    )
    
    # Install plugin headers
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../aniplugins/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/aniplugins
        FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
    )
    
    # Install public headers from include directory
    if(EXISTS ${CMAKE_SOURCE_DIR}/../../include)
        install(DIRECTORY ${CMAKE_SOURCE_DIR}/../../include/
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
            FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
        )
    endif()
    
    # Create and install config files
    include(CMakePackageConfigHelpers)
    
    configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/AniStudioCoreConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/AniStudioCoreConfig.cmake"
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/AniStudioCore
    )
    
    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/AniStudioCoreConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )
    
    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/AniStudioCoreConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/AniStudioCoreConfigVersion.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/AniStudioCore
    )
    
    install(EXPORT AniStudioCoreTargets
        FILE AniStudioCoreTargets.cmake
        NAMESPACE AniStudioCore::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/AniStudioCore
    )
endif()

# Examples (optional)
if(ANISTUDIO_BUILD_EXAMPLES AND CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    add_subdirectory(examples)
endif()

# Print configuration summary
message(STATUS "")
message(STATUS "AniStudio configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Library type: ${BUILD_SHARED_LIBS}")
message(STATUS "  External dir: ${EXTERNAL_DIR}")
message(STATUS "")
message(STATUS "Features:")
message(STATUS "  Zep editor: ${ZEP_AVAILABLE}")
message(STATUS "  ImGui Markdown: ${IMGUI_MARKDOWN_FOUND}")
message(STATUS "  ImGui Tex Inspect: ${IMGUI_TEX_INSPECT_FOUND}")
message(STATUS "  Examples: ${ANISTUDIO_BUILD_EXAMPLES}")
message(STATUS "")

# Check for required targets
if(NOT TARGET ImGuiFileDialog)
    message(FATAL_ERROR "ImGuiFileDialog target not found - check external/imgui_filedialog")
endif()

if(NOT TARGET ImGui)
    message(FATAL_ERROR "ImGui target not found - check external/imgui")
endif()

message(STATUS "All required targets found - ready to build")