# ============================================================================
# PluginCore.cmake - Plugin Infrastructure
# ============================================================================

# Plugin Core Sources - EXACT same patterns as your working CMakeLists.txt
file(GLOB_RECURSE PLUGIN_SOURCES ${CMAKE_SOURCE_DIR}/src/base_plugins/*.cpp)

# Plugin Core Include Directories - from your working CMakeLists.txt
set(PLUGIN_CORE_INCLUDES
    ${CMAKE_SOURCE_DIR}/src/base_plugins
    ${CMAKE_SOURCE_DIR}/src/utils
)

# Create PluginCore library
add_library(PluginCore STATIC ${PLUGIN_SOURCES})

target_include_directories(PluginCore PUBLIC ${PLUGIN_CORE_INCLUDES})

target_link_libraries(PluginCore PUBLIC 
    ECSCore
    StudioCore
    ${CONAN_LIBS}
)

if(WIN32)
    target_compile_definitions(PluginCore PUBLIC 
        "ANI_CORE_API=" 
        "PLUGIN_API=__declspec(dllexport)"
        GLEW_STATIC
    )
else()
    target_compile_definitions(PluginCore PUBLIC 
        "ANI_CORE_API=" 
        "PLUGIN_API=__attribute__((visibility(\"default\")))"
        GLEW_STATIC
    )
endif()

set_target_properties(PluginCore PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/lib
    ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lib
    ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/lib
    ARCHIVE_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/lib
)

target_compile_features(PluginCore PUBLIC cxx_std_17)

# Plugin helper functions
function(create_plugin PLUGIN_NAME)
    set(PLUGIN_DIR "${CMAKE_BINARY_DIR}/plugins/${PLUGIN_NAME}")
    file(MAKE_DIRECTORY ${PLUGIN_DIR})
    file(MAKE_DIRECTORY ${PLUGIN_DIR}/staging)
    message(STATUS "Created plugin directory: ${PLUGIN_DIR}")
endfunction()

function(add_plugin_target PLUGIN_NAME PLUGIN_SOURCES)
    create_plugin(${PLUGIN_NAME})
    
    add_library(${PLUGIN_NAME} SHARED ${PLUGIN_SOURCES})
    
    target_include_directories(${PLUGIN_NAME} PRIVATE
        ${PLUGIN_CORE_INCLUDES}
        ${CMAKE_SOURCE_DIR}/src/ecs
        ${CMAKE_SOURCE_DIR}/src/gui
    )
    
    target_link_libraries(${PLUGIN_NAME} PRIVATE
        PluginCore
        ECSCore  
        StudioCore
    )
    
    if(WIN32)
        target_compile_definitions(${PLUGIN_NAME} PRIVATE 
            "PLUGIN_API=__declspec(dllexport)"
            GLEW_STATIC
        )
    else()
        target_compile_definitions(${PLUGIN_NAME} PRIVATE 
            "PLUGIN_API=__attribute__((visibility(\"default\")))"
            GLEW_STATIC
        )
    endif()
    
    set_target_properties(${PLUGIN_NAME} PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins/${PLUGIN_NAME}
        LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/plugins/${PLUGIN_NAME}
        LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/plugins/${PLUGIN_NAME}
        LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/plugins/${PLUGIN_NAME}
        LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/plugins/${PLUGIN_NAME}
    )
    
    target_compile_features(${PLUGIN_NAME} PRIVATE cxx_std_17)
    message(STATUS "Created plugin target: ${PLUGIN_NAME}")
endfunction()

message(STATUS "PluginCore library configured")