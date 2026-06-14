add_executable(HalcyonSample
    main.cpp
    Game/GameInit.cpp
    Game/Systems/ControlSystem.cpp
    Game/Systems/RotationSystem.cpp
    Game/Systems/SpawnSystem.cpp
)

target_include_directories(HalcyonSample PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(HalcyonSample PRIVATE Halcyon)

# Copy assets and compiled shaders next to the sample executable on every build
add_custom_target(copy_sample_assets ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets/shaders"
        "${CMAKE_CURRENT_BINARY_DIR}/assets/shaders"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets/textures"
        "${CMAKE_CURRENT_BINARY_DIR}/assets/textures"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets/models"
        "${CMAKE_CURRENT_BINARY_DIR}/assets/models"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_BINARY_DIR}/shaders"
        "${CMAKE_CURRENT_BINARY_DIR}/shaders"
    DEPENDS ${HALCYON_SHADER_TARGETS}
    COMMENT "Copying sample assets and shaders..."
    VERBATIM
)

add_dependencies(HalcyonSample copy_sample_assets)
