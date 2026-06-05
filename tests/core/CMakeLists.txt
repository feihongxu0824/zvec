include(${PROJECT_ROOT_DIR}/cmake/bazel.cmake)
include(${PROJECT_ROOT_DIR}/cmake/option.cmake)

include_directories(${PROJECT_ROOT_DIR})

cc_directories(algorithm)
cc_directories(framework)
cc_directories(metric)
cc_directories(utility)
cc_directories(interface)
cc_directories(quantizer)
