# Add source files specific to AniStudio
add_library(AniStudioLib
    App.cpp
    pch.h
    pch.cpp
)

# Include the directory containing the precompiled header file
target_include_directories(AniStudioLib PRIVATE "${CMAKE_SOURCE_DIR}")

# Link the AniStudio library to the main executable
# target_link_libraries(${PROJECT_NAME} PRIVATE AniStudioLib)
