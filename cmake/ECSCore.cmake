# ============================================================================
# ECSCore.cmake - ECS Component System (FIXED STB)
# ============================================================================

# ECS Core Sources - EXACT same patterns as your working CMakeLists.txt
file(GLOB_RECURSE SYSTEM_SOURCES ${CMAKE_SOURCE_DIR}/src/ecs/systems/*.cpp)
file(GLOB_RECURSE UTILS_SOURCES ${CMAKE_SOURCE_DIR}/src/utils/*.cpp)

set(ECS_CORE_SOURCES ${SYSTEM_SOURCES} ${UTILS_SOURCES})

# ECS Core Include Directories - from your working CMakeLists.txt
# PUT LOCAL STB FIRST so it's found before stable-diffusion.cpp's version
set(ECS_CORE_INCLUDES
    ${CMAKE_SOURCE_DIR}/external/stb
    ${CMAKE_SOURCE_DIR}/src/ecs
    ${CMAKE_SOURCE_DIR}/src/ecs/base
    ${CMAKE_SOURCE_DIR}/src/ecs/systems
    ${CMAKE_SOURCE_DIR}/src/ecs/components
    ${CMAKE_SOURCE_DIR}/src/ecs/components/SDCPPComponents
    ${CMAKE_SOURCE_DIR}/src/ecs/components/ImageComponents
    ${CMAKE_SOURCE_DIR}/src/ecs/components/VideoComponents
    ${CMAKE_SOURCE_DIR}/src/ecs/components/ScriptComponents
    ${CMAKE_SOURCE_DIR}/src/utils
    ${CMAKE_SOURCE_DIR}/external/glm
    ${CMAKE_SOURCE_DIR}/external/nlohmann_json
    ${CMAKE_SOURCE_DIR}/external/imgui_filedialog
    ${CMAKE_SOURCE_DIR}/external/imgui_nodes
    ${CMAKE_SOURCE_DIR}/external/immvision
    ${CMAKE_SOURCE_DIR}/external/imgui_markdown
    ${CMAKE_SOURCE_DIR}/external/stable-diffusion.cpp
    ${CONAN_INCLUDE_DIRS}
)

# Create ECSCore library with SYSTEM_SOURCES AND UTILS_SOURCES
add_library(ECSCore STATIC ${ECS_CORE_SOURCES})

# CRITICAL: Prevent STB symbol conflicts and ensure std::min is available
target_compile_definitions(ECSCore PUBLIC 
    GLEW_STATIC
    IMGUI_DEFINE_MATH_OPERATORS
    NOMINMAX
    # DO NOT define STB_IMAGE_IMPLEMENTATION here - only in stb_image_impl.cpp
)

# Force the correct STB include path by using target_include_directories with BEFORE
target_include_directories(ECSCore BEFORE PUBLIC 
    ${CMAKE_SOURCE_DIR}/external/stb
)

target_include_directories(ECSCore PUBLIC 
    ${ECS_CORE_INCLUDES}
    ${CONAN_INCLUDE_DIRS}
)

# Exclude stable-diffusion.cpp STB files to prevent conflicts
target_compile_definitions(ECSCore PRIVATE
    STB_IMAGE_IMPLEMENTATION_DONE
    STB_IMAGE_WRITE_IMPLEMENTATION_DONE
)

# NOW link libraries AFTER the target is created
target_link_libraries(ECSCore PUBLIC 
    opencv::opencv 
    ${CONAN_LIBS}
    stable-diffusion
    ZLIB::ZLIB
    PNG::PNG
    exiv2::exiv2
    ffmpeg::avcodec
    ffmpeg::avformat
    ffmpeg::avutil
    ffmpeg::swscale
    pybind11::embed
)

if(SD_CUDA)
    target_compile_definitions(ECSCore PUBLIC SD_USE_CUDA)
    target_link_libraries(ECSCore PRIVATE CUDA::cudart)
endif()

set_target_properties(ECSCore PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/lib
    ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lib
    ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/lib
    ARCHIVE_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/lib
)

target_compile_features(ECSCore PUBLIC cxx_std_17)

message(STATUS "ECSCore library configured with systems and utils (STB conflict resolved)")