cmake_minimum_required (VERSION 3.20)

if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,
  $<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif() 

project ("Halcyon" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE MY_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

find_package(Vulkan REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(tinyobjloader CONFIG REQUIRED)
find_path(TINYGLTF_INCLUDE_DIRS "tiny_gltf.h")
find_path(STB_INCLUDE_DIRS "stb_image.h") 
find_package(VulkanMemoryAllocator CONFIG REQUIRED)

add_executable("${CMAKE_PROJECT_NAME}" ${MY_SOURCES})
target_precompile_headers("${CMAKE_PROJECT_NAME}" PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/pch.h")

target_link_libraries("${CMAKE_PROJECT_NAME}" PRIVATE Vulkan::Vulkan)
target_link_libraries("${CMAKE_PROJECT_NAME}" PRIVATE glm::glm)
target_link_libraries("${CMAKE_PROJECT_NAME}" PRIVATE glfw)
target_include_directories("${CMAKE_PROJECT_NAME}" PRIVATE ${TINYGLTF_INCLUDE_DIRS})
target_link_libraries("${CMAKE_PROJECT_NAME}" PRIVATE tinyobjloader::tinyobjloader)
target_include_directories("${CMAKE_PROJECT_NAME}" PRIVATE ${STB_INCLUDE_DIRS})
target_link_libraries("${CMAKE_PROJECT_NAME}" PRIVATE GPUOpen::VulkanMemoryAllocator)

if(MSVC)
    target_compile_options("${CMAKE_PROJECT_NAME}" PRIVATE "/Zc:__cplusplus" "/permissive-")
    target_compile_definitions("${CMAKE_PROJECT_NAME}" PRIVATE NOMINMAX)
endif()

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
    
    get_filename_component(ABS_SOURCE_PATH "${ARG_SOURCES}" ABSOLUTE)

    file(MAKE_DIRECTORY ${OUT_DIR})

    add_custom_command(
        OUTPUT ${OUT_FILE}
        COMMAND ${SLANGC_EXECUTABLE} 
                "${ABS_SOURCE_PATH}"
                -target spirv 
                -profile spirv_1_4 
                -emit-spirv-directly 
                -fvk-use-entrypoint-name 
                ${ENTRY_FLAGS}
                -o ${OUT_FILE}
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        DEPENDS "${ABS_SOURCE_PATH}"
        COMMENT "Compiling Slang shader: ${ARG_OUTPUT_NAME}"
        VERBATIM
    )

    add_custom_target(${TARGET_NAME} ALL DEPENDS ${OUT_FILE})
endfunction()

add_slang_shader_target(
    shader
    SOURCES "assets/shaders/shader.slang"
    ENTRY vertMain fragMain
)
add_slang_shader_target(
    shadow
    SOURCES "assets/shaders/shadow.slang"
    ENTRY vertShadow
)

add_slang_shader_target(
    fxaa
    SOURCES "assets/shaders/FXAA.slang"
    ENTRY vertMain fragMain
)

add_slang_shader_target(
    frustum_culling
    SOURCES "assets/shaders/frustum_culling.slang"
    ENTRY computeMain
)

add_dependencies("${CMAKE_PROJECT_NAME}" shader)
add_dependencies("${CMAKE_PROJECT_NAME}" shadow)
add_dependencies("${CMAKE_PROJECT_NAME}" fxaa)
add_dependencies("${CMAKE_PROJECT_NAME}" frustum_culling)

add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/assets/textures"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/assets/textures"
    "$<TARGET_FILE_DIR:${PROJECT_NAME}>/assets/textures"
    COMMENT "Copy textures..."
)

add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/assets/models"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/assets/models"
    "$<TARGET_FILE_DIR:${PROJECT_NAME}>/assets/models"
    COMMENT "Copy models..."
)


