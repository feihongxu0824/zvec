cmake_minimum_required(VERSION 3.13)
cmake_policy(SET CMP0077 NEW)
project(zvec-example-c++)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- Paths to Zvec ---
# Allow custom host build directory, default to "build"
if(NOT DEFINED HOST_BUILD_DIR)
    set(HOST_BUILD_DIR "build")
endif()

get_filename_component(ZVEC_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
set(ZVEC_INCLUDE_DIR ${ZVEC_ROOT_DIR}/src/include)
set(ZVEC_LIB_DIR ${ZVEC_ROOT_DIR}/${HOST_BUILD_DIR}/lib)
set(ZVEC_LINK_VIA_CMAKE_TARGET OFF)
if(TARGET zvec_shared OR TARGET zvec_core_shared)
    set(ZVEC_LINK_VIA_CMAKE_TARGET ON)
endif()

option(BUILD_ZVEC_CORE_EXAMPLE "Build zvec-core example linked only with libzvec_core" ON)
option(BUILD_ZVEC_FULL_EXAMPLES "Build examples linked with libzvec" ON)

# Add include and library search paths
include_directories(${ZVEC_INCLUDE_DIR})
if(NOT ZVEC_LINK_VIA_CMAKE_TARGET)
    link_directories(${ZVEC_LIB_DIR})
    set(CMAKE_BUILD_RPATH ${ZVEC_LIB_DIR})
endif()

# Support multi-config builds (MSVC puts libs in Debug/Release subdirectories)
if(CMAKE_BUILD_TYPE)
    set(ZVEC_CONFIG_LIB_DIR ${ZVEC_LIB_DIR}/${CMAKE_BUILD_TYPE})
    if(EXISTS "${ZVEC_CONFIG_LIB_DIR}" AND NOT ZVEC_LINK_VIA_CMAKE_TARGET)
        link_directories(${ZVEC_CONFIG_LIB_DIR})
        list(APPEND CMAKE_BUILD_RPATH ${ZVEC_CONFIG_LIB_DIR})
    endif()
endif()
if(WIN32)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# --- Create INTERFACE target for libzvec (all-in-one C++ shared library) ---
# libzvec.so/.dylib/.dll already bundles all zvec internal components
# (zvec_db, zvec_core, zvec_ailego, zvec_turbo), so no individual dependency
# libraries need to be specified by the consumer.
add_library(zvec-lib INTERFACE)
if(TARGET zvec_shared)
    target_link_libraries(zvec-lib INTERFACE zvec_shared)
else()
    target_link_libraries(zvec-lib INTERFACE zvec)
endif()

# --- Create INTERFACE target for libzvec_core (core-only all-in-one library) ---
# The core example intentionally depends only on libzvec_core.
add_library(zvec-core-lib INTERFACE)
if(TARGET zvec_core_shared)
    target_link_libraries(zvec-core-lib INTERFACE zvec_core_shared)
else()
    target_link_libraries(zvec-core-lib INTERFACE zvec_core)
endif()

# --- Executables ---
set(ZVEC_EXAMPLE_TARGETS)

if(BUILD_ZVEC_FULL_EXAMPLES)
    add_executable(db-example db/main.cc)
    target_link_libraries(db-example PRIVATE zvec-lib)
    if(ANDROID)
        target_link_libraries(db-example PRIVATE log)
    endif()
    list(APPEND ZVEC_EXAMPLE_TARGETS db-example)

    add_executable(ailego-example ailego/main.cc)
    target_link_libraries(ailego-example PRIVATE zvec-lib)
    list(APPEND ZVEC_EXAMPLE_TARGETS ailego-example)
endif()

if(BUILD_ZVEC_CORE_EXAMPLE)
    add_executable(core-example core/main.cc)
    target_link_libraries(core-example PRIVATE zvec-core-lib)
    list(APPEND ZVEC_EXAMPLE_TARGETS core-example)
endif()

# Strip symbols to reduce executable size
if(CMAKE_BUILD_TYPE STREQUAL "Release" AND ANDROID)
    foreach(ZVEC_EXAMPLE_TARGET ${ZVEC_EXAMPLE_TARGETS})
        add_custom_command(TARGET ${ZVEC_EXAMPLE_TARGET} POST_BUILD
            COMMAND ${CMAKE_STRIP} "$<TARGET_FILE:${ZVEC_EXAMPLE_TARGET}>"
            COMMENT "Stripping symbols from ${ZVEC_EXAMPLE_TARGET}")
    endforeach()
endif()

# Optimize for size
if(CMAKE_BUILD_TYPE STREQUAL "Release" AND ANDROID AND ZVEC_EXAMPLE_TARGETS)
    set_property(TARGET ${ZVEC_EXAMPLE_TARGETS} PROPERTY COMPILE_FLAGS "-Os")
    set_property(TARGET ${ZVEC_EXAMPLE_TARGETS} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()
