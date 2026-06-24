cmake_minimum_required(VERSION 3.20)

# vcpkg needs to know where dependencies are installed before project() is called.
# In this example we reuse the dependencies already installed for Halcyon itself.
set(VCPKG_INSTALLED_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../vcpkg_installed")

project(HalcyonSample LANGUAGES CXX)

if(NOT VCPKG_TARGET_TRIPLET)
    if(WIN32)
        set(VCPKG_TARGET_TRIPLET "x64-windows")
    else()
        set(VCPKG_TARGET_TRIPLET "x64-linux")
    endif()
endif()

list(APPEND CMAKE_PREFIX_PATH "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}")

set(HALCYON_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")

# User-configurable: where compiled stock shaders should be written.
set(HALCYON_SHADER_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders" CACHE PATH "Output directory for Halcyon compiled shaders")

# In-source build specific settings
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

target_compile_features(HalcyonSample PRIVATE cxx_std_20)

target_include_directories(HalcyonSample PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(HalcyonSample PRIVATE Halcyon)

# In-source build specific settings
if(HALCYON_SHADER_TARGETS)
    add_dependencies(HalcyonSample ${HALCYON_SHADER_TARGETS})
endif()

# Copy runtime assets next to the executable.
add_custom_command(TARGET HalcyonSample POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/textures"
        "$<TARGET_FILE_DIR:HalcyonSample>/assets/textures"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/models"
        "$<TARGET_FILE_DIR:HalcyonSample>/assets/models"
    COMMENT "Copying sample assets..."
    VERBATIM
)