
include(${PROJECT_ROOT_DIR}/cmake/bazel.cmake)
include(${PROJECT_ROOT_DIR}/cmake/option.cmake)

file(GLOB_RECURSE ALL_TEST_SRCS *_test.cc)

set(CORE_UTILITY_TEST_LIBS zvec_ailego core_framework core_utility)
if(NOT BUILD_ZVEC_CORE_ONLY)
  list(APPEND CORE_UTILITY_TEST_LIBS
    Arrow::arrow_depends
    Arrow::parquet_static)
endif()

foreach(CC_SRCS ${ALL_TEST_SRCS})
  get_filename_component(CC_TARGET ${CC_SRCS} NAME_WE)
  cc_gtest(
    NAME ${CC_TARGET} STRICT
    LIBS ${CORE_UTILITY_TEST_LIBS}
    SRCS ${CC_SRCS}
    INCS . ${PROJECT_ROOT_DIR}/src/core/
  )
  cc_test_suite(zvec_ailego ${CC_TARGET})
endforeach()
