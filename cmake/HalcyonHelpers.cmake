# halcyon_copy_shaders(<target> [DESTINATION <dir>])
# DESTINATION defaults to "$<TARGET_FILE_DIR:target>/shaders". A custom DESTINATION
# must be paired with the HALCYON_SHADER_DIR env var at runtime.
function(halcyon_copy_shaders TARGET)
    cmake_parse_arguments(ARG "" "DESTINATION" "" ${ARGN})

    if(NOT Halcyon_SHADER_DIR)
        message(FATAL_ERROR "halcyon_copy_shaders: Halcyon_SHADER_DIR is not set")
    endif()

    if(NOT ARG_DESTINATION)
        set(ARG_DESTINATION "$<TARGET_FILE_DIR:${TARGET}>/shaders")
    endif()

    add_custom_target(${TARGET}_copy_shaders ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${Halcyon_SHADER_DIR}" "${ARG_DESTINATION}"
        COMMENT "Copying Halcyon shaders to ${ARG_DESTINATION}"
        VERBATIM)

    if(HALCYON_SHADER_TARGETS)
        add_dependencies(${TARGET}_copy_shaders ${HALCYON_SHADER_TARGETS})
    endif()
    add_dependencies(${TARGET} ${TARGET}_copy_shaders)
endfunction()
