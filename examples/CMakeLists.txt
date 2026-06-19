cmake_minimum_required(VERSION 3.20)

set(VCPKG_INSTALLED_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../vcpkg_installed")

project(HalcyonSample LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Path to Halcyon root from this example directory.
set(HALCYON_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")

list(APPEND CMAKE_PREFIX_PATH "${HALCYON_ROOT}/vcpkg_installed/x64-windows")

# Example: the user sets the shader output directory before adding Halcyon.
# When this example is built standalone, this controls where compiled shaders go.
set(HALCYON_SHADER_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders" CACHE PATH "Output directory for Halcyon compiled shaders")

if(NOT TARGET Halcyon)
    add_subdirectory("${HALCYON_ROOT}" Halcyon)
endif()

add_executable(HalcyonSample
    main.cpp
    Game/GameInit.cpp
    Game/Systems/ControlSystem.cpp
    Game/Systems/RotationSystem.cpp
    Game/Systems/SpawnSystem.cpp
)

target_include_directories(HalcyonSample PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(HalcyonSample PRIVATE Halcyon)

# Copy sample textures and models next to the executable on every build
add_custom_target(copy_sample_assets ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${HALCYON_ROOT}/assets/textures"
        "${CMAKE_CURRENT_BINARY_DIR}/assets/textures"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${HALCYON_ROOT}/assets/models"
        "${CMAKE_CURRENT_BINARY_DIR}/assets/models"
    DEPENDS ${HALCYON_SHADER_TARGETS}
    COMMENT "Copying sample assets..."
    VERBATIM
)

add_dependencies(HalcyonSample copy_sample_assets)
