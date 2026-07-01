# halcyon_copy_shaders(<target> [DESTINATION <dir>])
# DESTINATION defaults to "$<TARGET_FILE_DIR:target>/shaders" (resolved at runtime
# automatically). A custom DESTINATION must be paired with the HALCYON_SHADER_DIR env var.
function(halcyon_copy_shaders TARGET)
    cmake_parse_arguments(ARG "" "DESTINATION" "" ${ARGN})

    if(NOT Halcyon_SHADER_DIR)
        message(FATAL_ERROR "halcyon_copy_shaders: Halcyon_SHADER_DIR is not set")
    endif()

    if(NOT ARG_DESTINATION)
        set(ARG_DESTINATION "$<TARGET_FILE_DIR:${TARGET}>/shaders")
    endif()

    add_custom_command(TARGET ${TARGET} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${Halcyon_SHADER_DIR}" "${ARG_DESTINATION}"
        COMMENT "Copying Halcyon shaders to ${ARG_DESTINATION}"
        VERBATIM)
endfunction()
