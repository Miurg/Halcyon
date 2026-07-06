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

# halcyon_stage_dir(<target> <source_dir> <dest_subdir>)
# Copies source_dir to "<target output dir>/dest_subdir", re-running only when a file
# under source_dir changes. Dependency-tracked (unlike POST_BUILD), so edits are picked
# up without a full rebuild, and large asset trees are not re-copied on every build.
function(halcyon_stage_dir TARGET SRC DEST_SUBDIR)
    file(GLOB_RECURSE _files CONFIGURE_DEPENDS "${SRC}/*")
    string(MAKE_C_IDENTIFIER "${DEST_SUBDIR}" _id)
    set(_stamp "${CMAKE_CURRENT_BINARY_DIR}/${TARGET}_${_id}.stamp")

    add_custom_command(
        OUTPUT "${_stamp}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${SRC}" "$<TARGET_FILE_DIR:${TARGET}>/${DEST_SUBDIR}"
        COMMAND ${CMAKE_COMMAND} -E touch "${_stamp}"
        DEPENDS ${_files}
        COMMENT "Staging ${DEST_SUBDIR} for ${TARGET}"
        VERBATIM)

    add_custom_target(${TARGET}_stage_${_id} DEPENDS "${_stamp}")
    add_dependencies(${TARGET} ${TARGET}_stage_${_id})
endfunction()
