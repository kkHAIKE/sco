function (build_external_project dist_dir target url_name url_hash)

    set(TARGET_DIR "${CMAKE_CURRENT_BINARY_DIR}/ExternalProjects/${target}")
    set(${dist_dir} "${TARGET_DIR}/dist" PARENT_SCOPE)

    set(CMAKELIST_CONTENT "
        cmake_minimum_required(VERSION ${CMAKE_MINIMUM_REQUIRED_VERSION})
        project(build_external_project)
        include(ExternalProject)
        ExternalProject_add(${target}
            URL \"${url_name}\"
            URL_HASH SHA256=${url_hash}
            CMAKE_GENERATOR \"${CMAKE_GENERATOR}\"
            CMAKE_GENERATOR_PLATFORM \"${CMAKE_GENERATOR_PLATFORM}\"
            CMAKE_GENERATOR_TOOLSET \"${CMAKE_GENERATOR_TOOLSET}\"
            CMAKE_GENERATOR_INSTANCE \"${CMAKE_GENERATOR_INSTANCE}\"
            CMAKE_ARGS ${ARGN}
                -D CMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
                -D CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                -D \"CMAKE_INSTALL_PREFIX=${TARGET_DIR}/dist\"
        )
        add_custom_target(build_external_project)
        add_dependencies(build_external_project ${target})
    ")

    file(WRITE "${TARGET_DIR}/CMakeLists.txt" "${CMAKELIST_CONTENT}")

    file(MAKE_DIRECTORY "${TARGET_DIR}" "${TARGET_DIR}/build")

    execute_process(COMMAND ${CMAKE_COMMAND}
        -G "${CMAKE_GENERATOR}"
        -A "${CMAKE_GENERATOR_PLATFORM}"
        -T "${CMAKE_GENERATOR_TOOLSET}"
        ..
        WORKING_DIRECTORY "${TARGET_DIR}/build")

    execute_process(COMMAND ${CMAKE_COMMAND}
        --build .
        --config ${CMAKE_BUILD_TYPE}
        WORKING_DIRECTORY "${TARGET_DIR}/build")

endfunction()
