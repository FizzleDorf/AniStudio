cmake_minimum_required(VERSION 3.10)



################################################################################
# The Project Name will the the name of the folder. Change it if you
# dont like it.
################################################################################
project( ImJSchema  CXX )
################################################################################

if(PROJECT_IS_TOP_LEVEL)
    set(IMJSCHEMA_BUILD_EXECUTABLES ON  CACHE FILEPATH "Build the sample application")
else()
    set(IMJSCHEMA_BUILD_EXECUTABLES OFF CACHE FILEPATH "Build the sample application")
endif()


################################################################################
# Build the Interface library
#
################################################################################
add_library(ImJSchema INTERFACE)
add_library(ImJSchema::ImJSchema ALIAS ImJSchema)

target_include_directories(ImJSchema
                            INTERFACE
                               "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
)

target_link_libraries(ImJSchema
                        INTERFACE
                            )
target_compile_features( ImJSchema
                          INTERFACE
                              cxx_std_17)

################################################################################



################################################################################
if(IMJSCHEMA_BUILD_EXECUTABLES)

    set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_BINARY_DIR};${CMAKE_MODULE_PATH}")

    find_package(nlohmann_json REQUIRED)
    find_package(imgui REQUIRED)

    ################################################################################

    if(${EMSCRIPTEN})
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -s USE_SDL=2")
        file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/index.html DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

        set(SDL_TARGETS "")
    else()

        include(cmake/CompilerWarnings.cmake)
        myproject_set_project_warnings(ImJSchema  TRUE "" "" "" "")

        find_package(SDL2   REQUIRED)
        set(SDL_TARGETS SDL2::SDL2)
    endif()

    add_library(imguiLib INTERFACE)
    target_include_directories( imguiLib
                                   SYSTEM INTERFACE
                                   "${imgui_INCLUDE_DIRS}"
                                    ${CMAKE_BINARY_DIR}/imgui_src
                                    ${CMAKE_BINARY_DIR}/imgui_src/misc/cpp)

    ################################################################################
    # Build the executable
    #
    ################################################################################
    add_executable( example example.cpp imgui.cpp )
    target_link_libraries( example
                            PRIVATE
                                ImJSchema::ImJSchema
                                nlohmann_json::nlohmann_json
                                imguiLib
                                ${SDL_TARGETS})
    #-------------------------------------------------------------------------------
    add_executable( schenity schenity.cpp imgui.cpp )
    target_link_libraries( schenity
                            PRIVATE
                                ImJSchema::ImJSchema
                                ${SDL_TARGETS}
                                nlohmann_json::nlohmann_json
                                imguiLib )
    ################################################################################


    if(${EMSCRIPTEN})
    else()
        enable_testing()
        add_subdirectory(test)
    endif()


endif()
