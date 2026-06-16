cmake_minimum_required(VERSION 3.20)

if(POLICY CMP0141)
    cmake_policy(SET CMP0141 NEW)
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project(Halcyon LANGUAGES CXX VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

# === Dependencies ===
find_package(Vulkan REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(tinyobjloader CONFIG REQUIRED)
find_path(TINYGLTF_INCLUDE_DIRS "tiny_gltf.h")
find_path(STB_INCLUDE_DIRS "stb_image.h")
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(Jolt CONFIG REQUIRED)
find_package(Ktx CONFIG REQUIRED)

option(HALCYON_TRACY "Enable Tracy profiler" OFF)

FetchContent_Declare(
    Orhescyon
    GIT_REPOSITORY https://github.com/Miurg/Orhescyon.git
    GIT_TAG v0.2.0
)
FetchContent_MakeAvailable(Orhescyon)

if(HALCYON_TRACY)
    FetchContent_Declare(
        tracy
        GIT_REPOSITORY https://github.com/wolfpld/tracy.git
        GIT_TAG v0.13.1
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
    )
    FetchContent_MakeAvailable(tracy)
endif()

# === Options ===
option(BUILD_SHARED_LIBS "Build Halcyon as a shared library" OFF)
option(HALCYON_BUILD_EXAMPLES "Build Halcyon examples" ON)

# === Library ===
file(GLOB_RECURSE HALCYON_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_library(${PROJECT_NAME} ${HALCYON_SOURCES})
add_library(Halcyon::Halcyon ALIAS ${PROJECT_NAME})

if(BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE HALCYON_BUILD_SHARED HALCYON_EXPORTS)
    target_compile_definitions(${PROJECT_NAME} INTERFACE HALCYON_BUILD_SHARED)
endif()

set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/src/ThirdPartyImpl.cpp" PROPERTIES SKIP_PRECOMPILE_HEADERS ON)
target_precompile_headers(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/pch.h")

# === Linking and Includes ===
target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${TINYGLTF_INCLUDE_DIRS}
    ${STB_INCLUDE_DIRS}
     ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    Vulkan::Vulkan
    glm::glm
    glfw
    tinyobjloader::tinyobjloader
    GPUOpen::VulkanMemoryAllocator
    imgui::imgui
    Jolt::Jolt
    KTX::ktx
    Orhescyon
)

if(HALCYON_TRACY)
    target_link_libraries(${PROJECT_NAME} PRIVATE TracyClient)
    target_compile_definitions(${PROJECT_NAME} PRIVATE TRACY_ENABLE)
endif()

# === Defines and Compiler Options ===
target_compile_definitions(${PROJECT_NAME} PUBLIC ORHESCYON_HIGH_CHECK)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    HALCYON_DEV_BUILD
    HALCYON_SHADER_SRC_DIR="assets/shaders"
)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE "/Zc:__cplusplus" "/permissive-")
    target_compile_definitions(${PROJECT_NAME} PRIVATE NOMINMAX)
endif()

# === Shader Compilation ===
find_program(SLANGC_EXECUTABLE NAMES slangc REQUIRED)
message(STATUS "Found Slang compiler: ${SLANGC_EXECUTABLE}")

function(add_slang_shader_target TARGET_NAME)
    set(oneValueArgs OUTPUT_NAME)
    set(multiValueArgs SOURCES ENTRY)
    cmake_parse_arguments(ARG "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT ARG_ENTRY)
        set(ARG_ENTRY vertMain fragMain)
    endif()

    set(ENTRY_FLAGS "")
    foreach(ENTRY_POINT ${ARG_ENTRY})
        list(APPEND ENTRY_FLAGS "-entry" "${ENTRY_POINT}")
    endforeach()

    if(NOT ARG_OUTPUT_NAME)
        set(ARG_OUTPUT_NAME "${TARGET_NAME}")
    endif()

    set(OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders")
    set(OUT_FILE "${OUT_DIR}/${ARG_OUTPUT_NAME}.spv")

    file(MAKE_DIRECTORY "${OUT_DIR}")

    add_custom_command(
        OUTPUT "${OUT_FILE}"
        COMMAND ${SLANGC_EXECUTABLE}
                "${ARG_SOURCES}"
                -target spirv
                -profile spirv_1_6
                -emit-spirv-directly
                -fvk-use-entrypoint-name
                -capability SPV_KHR_non_semantic_info+SPV_GOOGLE_user_type+spvDerivativeControl+spvImageQuery+spvImageGatherExtended+spvSparseResidency+spvMinLod+spvFragmentFullyCoveredEXT+spvShaderNonUniformEXT
                ${ENTRY_FLAGS}
                -o "${OUT_FILE}"
        WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}"
        DEPENDS "${ARG_SOURCES}"
        COMMENT "Compiling Slang shader: ${ARG_OUTPUT_NAME}"
        VERBATIM
    )

    add_custom_target(${TARGET_NAME} ALL DEPENDS "${OUT_FILE}")
endfunction()

set(HALCYON_SHADER_TARGETS "")

# Auto gather shader files and create targets
file(GLOB_RECURSE SLANG_SHADER_FILES "${CMAKE_CURRENT_LIST_DIR}/assets/shaders/*.slang")
foreach(SHADER_FILE ${SLANG_SHADER_FILES})
    get_filename_component(SHADER_NAME "${SHADER_FILE}" NAME_WE)
    string(TOLOWER "${SHADER_NAME}" SHADER_NAME_LOWER)

    # Read the shader file content to find entry points
    file(READ "${SHADER_FILE}" SHADER_CONTENT)
    
    set(ENTRY_POINTS "")
    foreach(PREFIX vert frag compute)
        string(FIND "${SHADER_CONTENT}" "${PREFIX}Main" FOUND_POS)
        if(NOT FOUND_POS EQUAL -1)
            list(APPEND ENTRY_POINTS "${PREFIX}Main")
        endif()
    endforeach()

    if(NOT ENTRY_POINTS)
        message(WARNING "No known entry points found in ${SHADER_NAME}.slang, skipping")
        continue()
    endif()

    message(STATUS "Shader ${SHADER_NAME}: entries = ${ENTRY_POINTS}")

    add_slang_shader_target(
        "shader_${SHADER_NAME_LOWER}"
        SOURCES "${SHADER_FILE}"
        OUTPUT_NAME "${SHADER_NAME_LOWER}"
        ENTRY ${ENTRY_POINTS}
    )

    list(APPEND HALCYON_SHADER_TARGETS "shader_${SHADER_NAME_LOWER}")
endforeach()

if(HALCYON_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# === Installation ===
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS ${PROJECT_NAME}
    EXPORT HalcyonTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(TARGETS Orhescyon
    EXPORT HalcyonTargets
)

install(DIRECTORY ${orhescyon_SOURCE_DIR}/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING
    PATTERN "*.hpp"
    PATTERN "*.h"
)

install(EXPORT HalcyonTargets
    FILE HalcyonTargets.cmake
    NAMESPACE Halcyon::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Halcyon
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/HalcyonConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/HalcyonConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Halcyon
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/HalcyonConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/HalcyonConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/HalcyonConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Halcyon
)
