# ============================================================================
# StudioCore.cmake - GUI Framework and Events
# ============================================================================

# Studio Core Sources - EXACT same patterns as your working CMakeLists.txt
file(GLOB_RECURSE GUI_SOURCES ${CMAKE_SOURCE_DIR}/src/gui/*.cpp)
file(GLOB_RECURSE EVENT_SOURCES ${CMAKE_SOURCE_DIR}/src/events/*.cpp)
file(GLOB_RECURSE GUI_UTILS_SOURCES ${CMAKE_SOURCE_DIR}/src/gui_utils/*.cpp)

set(STUDIO_CORE_SOURCES ${GUI_SOURCES} ${EVENT_SOURCES} ${GUI_UTILS_SOURCES})

# Studio Core Include Directories - from your working CMakeLists.txt
set(STUDIO_CORE_INCLUDES
    ${CMAKE_SOURCE_DIR}/src/gui
    ${CMAKE_SOURCE_DIR}/src/gui/base
    ${CMAKE_SOURCE_DIR}/src/events
    ${CMAKE_SOURCE_DIR}/src/gui_utils
    ${CMAKE_SOURCE_DIR}/external/imgui
    ${CMAKE_SOURCE_DIR}/external/imgui_nodes
    ${CMAKE_SOURCE_DIR}/external/ImNodeFlow
    ${CMAKE_SOURCE_DIR}/external/imgui_filedialog
    ${CMAKE_SOURCE_DIR}/external/ImJSchema
    ${CMAKE_SOURCE_DIR}/external/ImGuizmo
    ${CMAKE_SOURCE_DIR}/external/immvision
    ${CMAKE_SOURCE_DIR}/external/imgui_markdown
    ${Vulkan_INCLUDE_DIR}
)

if(ZEP_AVAILABLE)
    list(APPEND STUDIO_CORE_INCLUDES ${CMAKE_SOURCE_DIR}/external/zep/include)
endif()

# Create StudioCore library
add_library(StudioCore STATIC ${STUDIO_CORE_SOURCES})

target_include_directories(StudioCore PUBLIC ${STUDIO_CORE_INCLUDES})

target_link_libraries(StudioCore PUBLIC 
    ECSCore
    ImGui
    ImGui_Nodes
    ImNodeFlow
    ImJSchema
    ImGuiFileDialog
    ImGuizmo
    glfw
    GLEW::GLEW
    Vulkan::Vulkan
    ${CONAN_LIBS}
)

if(ZEP_AVAILABLE)
    target_link_libraries(StudioCore PRIVATE Zep::Zep)
    target_compile_definitions(StudioCore PUBLIC ZEP_AVAILABLE)
endif()

target_compile_definitions(StudioCore PUBLIC 
    GLEW_STATIC
    IMGUI_DEFINE_MATH_OPERATORS
)

set_target_properties(StudioCore PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/lib
    ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lib
    ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/lib
    ARCHIVE_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/lib
)

target_compile_features(StudioCore PUBLIC cxx_std_17)

message(STATUS "StudioCore library configured")