# Add plugins
add_library(CustomPlugin SHARED
    plugins/CustomPlugin/src/CustomPlugin.cpp
)

target_include_directories(CustomPlugin PRIVATE
    ${CMAKE_SOURCE_DIR}/AniStudio
    ${CMAKE_SOURCE_DIR}/AniStudio/ECS
    ${CMAKE_SOURCE_DIR}/AniStudio/GUI
    ${CMAKE_SOURCE_DIR}/plugins/CustomPlugin/include
    ${CONAN_INCLUDE_DIRS}
)

target_link_libraries(CustomPlugin PRIVATE
    ${CONAN_LIBS}
)

set_target_properties(CustomPlugin PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
)