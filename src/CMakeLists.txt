include(${PROJECT_ROOT_DIR}/cmake/bazel.cmake)
include(${PROJECT_ROOT_DIR}/cmake/option.cmake)

# Retrieve version from git repository
git_version(ZVEC_VERSION ${CMAKE_CURRENT_SOURCE_DIR})

# Add repository
cc_directory(ailego)
cc_directory(turbo)
cc_directory(core)
cc_directory(db)
cc_directory(binding)

# =============================================================================
# Build ALL-IN-ONE C++ Shared Libraries
# =============================================================================
# Merges zvec internal static libraries into shared libraries while preserving
# C++ symbols for direct C++ linking.

include(GNUInstallDirs)
find_package(Threads REQUIRED)

function(zvec_get_cxx_export_build_def OUTPUT_VAR TARGET_NAME)
    if(TARGET_NAME STREQUAL zvec)
        set(${OUTPUT_VAR} ZVEC_DB_BUILD_SHARED PARENT_SCOPE)
    elseif(TARGET_NAME STREQUAL zvec_core)
        set(${OUTPUT_VAR} ZVEC_CORE_BUILD_SHARED PARENT_SCOPE)
    elseif(TARGET_NAME STREQUAL zvec_ailego)
        set(${OUTPUT_VAR} ZVEC_AILEGO_BUILD_SHARED PARENT_SCOPE)
    elseif(TARGET_NAME STREQUAL zvec_turbo)
        set(${OUTPUT_VAR} ZVEC_TURBO_BUILD_SHARED PARENT_SCOPE)
    else()
        set(${OUTPUT_VAR} "" PARENT_SCOPE)
    endif()
endfunction()

function(zvec_clone_target_objects_for_shared CLONE_TARGET SOURCE_TARGET)
    get_target_property(ZVEC_CLONE_SRCS ${SOURCE_TARGET} SOURCES)
    if(NOT ZVEC_CLONE_SRCS)
        message(FATAL_ERROR "Target ${SOURCE_TARGET} has no sources to clone")
    endif()

    get_target_property(ZVEC_CLONE_SOURCE_DIR ${SOURCE_TARGET} SOURCE_DIR)
    set(ZVEC_CLONE_ABS_SRCS)
    foreach(ZVEC_CLONE_SRC IN LISTS ZVEC_CLONE_SRCS)
        if(ZVEC_CLONE_SRC MATCHES "^\\$<")
            list(APPEND ZVEC_CLONE_ABS_SRCS "${ZVEC_CLONE_SRC}")
        elseif(IS_ABSOLUTE "${ZVEC_CLONE_SRC}")
            list(APPEND ZVEC_CLONE_ABS_SRCS "${ZVEC_CLONE_SRC}")
        else()
            list(APPEND ZVEC_CLONE_ABS_SRCS "${ZVEC_CLONE_SOURCE_DIR}/${ZVEC_CLONE_SRC}")
        endif()
    endforeach()

    add_library(${CLONE_TARGET} OBJECT ${ZVEC_CLONE_ABS_SRCS})
    set_target_properties(${CLONE_TARGET} PROPERTIES
        POSITION_INDEPENDENT_CODE ON
    )

    foreach(ZVEC_CLONE_PROP C_STANDARD CXX_STANDARD C_STANDARD_REQUIRED
                             CXX_STANDARD_REQUIRED C_EXTENSIONS CXX_EXTENSIONS)
        get_target_property(ZVEC_CLONE_PROP_VALUE ${SOURCE_TARGET} ${ZVEC_CLONE_PROP})
        if(ZVEC_CLONE_PROP_VALUE)
            set_target_properties(${CLONE_TARGET} PROPERTIES
                ${ZVEC_CLONE_PROP} "${ZVEC_CLONE_PROP_VALUE}"
            )
        endif()
    endforeach()

    foreach(ZVEC_CLONE_PROP INCLUDE_DIRECTORIES INTERFACE_INCLUDE_DIRECTORIES)
        get_target_property(ZVEC_CLONE_PROP_VALUE ${SOURCE_TARGET} ${ZVEC_CLONE_PROP})
        if(ZVEC_CLONE_PROP_VALUE)
            target_include_directories(${CLONE_TARGET} PRIVATE ${ZVEC_CLONE_PROP_VALUE})
        endif()
    endforeach()

    foreach(ZVEC_CLONE_PROP SYSTEM_INCLUDE_DIRECTORIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES)
        get_target_property(ZVEC_CLONE_PROP_VALUE ${SOURCE_TARGET} ${ZVEC_CLONE_PROP})
        if(ZVEC_CLONE_PROP_VALUE)
            target_include_directories(${CLONE_TARGET} SYSTEM PRIVATE ${ZVEC_CLONE_PROP_VALUE})
        endif()
    endforeach()

    foreach(ZVEC_CLONE_PROP COMPILE_DEFINITIONS INTERFACE_COMPILE_DEFINITIONS)
        get_target_property(ZVEC_CLONE_PROP_VALUE ${SOURCE_TARGET} ${ZVEC_CLONE_PROP})
        if(ZVEC_CLONE_PROP_VALUE)
            target_compile_definitions(${CLONE_TARGET} PRIVATE ${ZVEC_CLONE_PROP_VALUE})
        endif()
    endforeach()

    foreach(ZVEC_CLONE_PROP COMPILE_OPTIONS INTERFACE_COMPILE_OPTIONS)
        get_target_property(ZVEC_CLONE_PROP_VALUE ${SOURCE_TARGET} ${ZVEC_CLONE_PROP})
        if(ZVEC_CLONE_PROP_VALUE)
            target_compile_options(${CLONE_TARGET} PRIVATE ${ZVEC_CLONE_PROP_VALUE})
        endif()
    endforeach()

    if(ARGN)
        target_compile_definitions(${CLONE_TARGET} PRIVATE ${ARGN})
    endif()
endfunction()

function(zvec_collect_link_dependencies OUTPUT_VAR)
    set(ZVEC_COLLECT_SKIP_LIBS ${ARGN})
    set(ZVEC_COLLECT_DEPS)

    foreach(ZVEC_COLLECT_LIB IN LISTS ZVEC_COLLECT_SKIP_LIBS)
        foreach(ZVEC_COLLECT_PROP LINK_LIBRARIES INTERFACE_LINK_LIBRARIES)
            get_target_property(ZVEC_COLLECT_PROP_VALUE ${ZVEC_COLLECT_LIB} ${ZVEC_COLLECT_PROP})
            if(NOT ZVEC_COLLECT_PROP_VALUE)
                continue()
            endif()

            foreach(ZVEC_COLLECT_DEP IN LISTS ZVEC_COLLECT_PROP_VALUE)
                if(ZVEC_COLLECT_DEP IN_LIST ZVEC_COLLECT_SKIP_LIBS)
                    continue()
                endif()
                list(APPEND ZVEC_COLLECT_DEPS ${ZVEC_COLLECT_DEP})
            endforeach()
        endforeach()
    endforeach()

    if(ZVEC_COLLECT_DEPS)
        list(REMOVE_DUPLICATES ZVEC_COLLECT_DEPS)
    endif()
    set(${OUTPUT_VAR} ${ZVEC_COLLECT_DEPS} PARENT_SCOPE)
endfunction()

function(zvec_add_all_in_one_shared TARGET_NAME OUTPUT_NAME)
    cmake_parse_arguments(ZVEC_ALLIN "" "" "LIBS" ${ARGN})
    if(NOT ZVEC_ALLIN_LIBS)
        message(FATAL_ERROR "zvec_add_all_in_one_shared requires LIBS")
    endif()

    foreach(ZVEC_ALLIN_LIB ${ZVEC_ALLIN_LIBS})
        if(NOT TARGET ${ZVEC_ALLIN_LIB})
            message(FATAL_ERROR "Target ${ZVEC_ALLIN_LIB} is required by ${TARGET_NAME}")
        endif()
    endforeach()

    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}_stub.cc
        "// Auto-generated stub for ${TARGET_NAME}\n"
    )

    add_library(${TARGET_NAME} SHARED
        ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}_stub.cc
    )

    set_target_properties(${TARGET_NAME} PROPERTIES
        OUTPUT_NAME "${OUTPUT_NAME}"
        POSITION_INDEPENDENT_CODE ON
        WINDOWS_EXPORT_ALL_SYMBOLS OFF
    )
    if(WIN32)
        set_target_properties(${TARGET_NAME} PROPERTIES
            ARCHIVE_OUTPUT_NAME "${TARGET_NAME}"
        )
    endif()
    target_compile_features(${TARGET_NAME} PUBLIC cxx_std_17)

    if(MSVC)
        foreach(ZVEC_ALLIN_LIB ${ZVEC_ALLIN_LIBS})
            zvec_get_cxx_export_build_def(ZVEC_ALLIN_BUILD_DEF ${ZVEC_ALLIN_LIB})
            zvec_clone_target_objects_for_shared(
                ${TARGET_NAME}_${ZVEC_ALLIN_LIB}_objects
                ${ZVEC_ALLIN_LIB}
                ${ZVEC_ALLIN_BUILD_DEF}
            )
            target_sources(${TARGET_NAME} PRIVATE
                $<TARGET_OBJECTS:${TARGET_NAME}_${ZVEC_ALLIN_LIB}_objects>
            )
        endforeach()

        zvec_collect_link_dependencies(ZVEC_ALLIN_LINK_DEPS ${ZVEC_ALLIN_LIBS})
        target_link_libraries(${TARGET_NAME}
            PRIVATE
                ${ZVEC_ALLIN_LINK_DEPS}
                Threads::Threads
        )
    elseif(APPLE)
        foreach(ZVEC_ALLIN_LIB ${ZVEC_ALLIN_LIBS})
            list(APPEND ZVEC_ALLIN_WA_OPTIONS
                -Wl,-force_load,$<TARGET_FILE:${ZVEC_ALLIN_LIB}>
            )
        endforeach()

        target_link_options(${TARGET_NAME} PRIVATE ${ZVEC_ALLIN_WA_OPTIONS})
        target_link_libraries(${TARGET_NAME}
            PRIVATE
                ${ZVEC_ALLIN_LIBS}
                Threads::Threads
                ${CMAKE_DL_LIBS}
        )
    else()
        target_link_libraries(${TARGET_NAME}
            PRIVATE
                -Wl,--whole-archive
                    ${ZVEC_ALLIN_LIBS}
                -Wl,--no-whole-archive
                Threads::Threads
                ${CMAKE_DL_LIBS}
        )

        # Try static linking of libstdc++ and libgcc for better portability.
        set(ALLIN_STATIC_LIBS_FOUND FALSE)
        execute_process(
            COMMAND ${CMAKE_CXX_COMPILER} -print-file-name=libstdc++.a
            OUTPUT_VARIABLE ALLIN_LIBSTDCPP_PATH
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        execute_process(
            COMMAND ${CMAKE_CXX_COMPILER} -print-file-name=libgcc.a
            OUTPUT_VARIABLE ALLIN_LIBGCC_PATH
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        if(EXISTS "${ALLIN_LIBSTDCPP_PATH}" AND EXISTS "${ALLIN_LIBGCC_PATH}")
            if(IS_ABSOLUTE "${ALLIN_LIBSTDCPP_PATH}" AND IS_ABSOLUTE "${ALLIN_LIBGCC_PATH}")
                set(ALLIN_STATIC_LIBS_FOUND TRUE)
            endif()
        endif()
        if(ALLIN_STATIC_LIBS_FOUND)
            target_link_options(${TARGET_NAME} PRIVATE -static-libstdc++ -static-libgcc)
            message(STATUS "${TARGET_NAME}: Using static libstdc++ and libgcc")
        else()
            message(STATUS "${TARGET_NAME}: Using dynamic linking for libstdc++ and libgcc")
        endif()
    endif()

    target_include_directories(${TARGET_NAME}
        PUBLIC
            $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/src/generated>
            $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src/include>
            $<INSTALL_INTERFACE:include>
        PRIVATE
            ${PROJECT_SOURCE_DIR}/src
    )

    set(ZVEC_ALLIN_USE_DEFS)
    foreach(ZVEC_ALLIN_LIB ${ZVEC_ALLIN_LIBS})
        if(ZVEC_ALLIN_LIB STREQUAL zvec)
            list(APPEND ZVEC_ALLIN_USE_DEFS ZVEC_DB_USE_SHARED)
        elseif(ZVEC_ALLIN_LIB STREQUAL zvec_core)
            list(APPEND ZVEC_ALLIN_USE_DEFS ZVEC_CORE_USE_SHARED)
        elseif(ZVEC_ALLIN_LIB STREQUAL zvec_ailego)
            list(APPEND ZVEC_ALLIN_USE_DEFS ZVEC_AILEGO_USE_SHARED)
        elseif(ZVEC_ALLIN_LIB STREQUAL zvec_turbo)
            list(APPEND ZVEC_ALLIN_USE_DEFS ZVEC_TURBO_USE_SHARED)
        endif()
    endforeach()
    if(ZVEC_ALLIN_USE_DEFS)
        list(REMOVE_DUPLICATES ZVEC_ALLIN_USE_DEFS)
        target_compile_definitions(${TARGET_NAME} INTERFACE ${ZVEC_ALLIN_USE_DEFS})
    endif()

    # Strip symbols in release builds to reduce library size.
    if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
        if(UNIX AND NOT APPLE)
            # add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            #     COMMAND ${CMAKE_STRIP} $<TARGET_FILE:${TARGET_NAME}>
            #     COMMENT "Stripping symbols from $<TARGET_FILE_NAME:${TARGET_NAME}>"
            # )
        elseif(APPLE)
            add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
                COMMAND /usr/bin/strip -x $<TARGET_FILE:${TARGET_NAME}>
                COMMENT "Stripping symbols from $<TARGET_FILE_NAME:${TARGET_NAME}>"
            )
        endif()
    endif()

    install(TARGETS ${TARGET_NAME}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endfunction()

function(zvec_enable_cxx_exports TARGET_NAME)
    if(NOT TARGET ${TARGET_NAME})
        return()
    endif()

    foreach(ZVEC_EXPORT_DEF IN LISTS ARGN)
        target_compile_definitions(${TARGET_NAME} PRIVATE ${ZVEC_EXPORT_DEF})
    endforeach()
endfunction()

if((NOT MSVC) AND (WIN32 OR BUILD_ZVEC_AILEGO_SHARED OR BUILD_ZVEC_CORE_SHARED OR BUILD_ZVEC_SHARED))
    if(BUILD_ZVEC_SHARED)
        zvec_enable_cxx_exports(zvec ZVEC_DB_BUILD_SHARED)
    endif()
    if(BUILD_ZVEC_SHARED OR BUILD_ZVEC_CORE_SHARED)
        zvec_enable_cxx_exports(zvec_core ZVEC_CORE_BUILD_SHARED)
        zvec_enable_cxx_exports(zvec_turbo ZVEC_TURBO_BUILD_SHARED)
    endif()
    if(BUILD_ZVEC_SHARED OR BUILD_ZVEC_CORE_SHARED OR BUILD_ZVEC_AILEGO_SHARED)
        zvec_enable_cxx_exports(zvec_ailego ZVEC_AILEGO_BUILD_SHARED)
    endif()
endif()

if(BUILD_ZVEC_AILEGO_SHARED)
    zvec_add_all_in_one_shared(zvec_ailego_shared zvec_ailego
        LIBS
            zvec_ailego
    )
endif()

if(BUILD_ZVEC_CORE_SHARED)
    zvec_add_all_in_one_shared(zvec_core_shared zvec_core
        LIBS
            zvec_core
            zvec_ailego
            zvec_turbo
    )
endif()

if(BUILD_ZVEC_SHARED)
    zvec_add_all_in_one_shared(zvec_shared zvec
        LIBS
            zvec
            zvec_core
            zvec_ailego
            zvec_turbo
    )
endif()
