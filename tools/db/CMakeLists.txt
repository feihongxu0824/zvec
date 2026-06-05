include(${PROJECT_ROOT_DIR}/cmake/bazel.cmake)

set(FTS_BENCH_ZVEC_LIB zvec_shared)
if(MSVC)
    set(FTS_BENCH_ZVEC_LIB zvec_db)
endif()

cc_binary(
    NAME fts_bench PACKED
    SRCS fts_bench_main.cc
    LIBS
         ${FTS_BENCH_ZVEC_LIB}
         gflags
         roaring
         rocksdb
    INCS . ${PROJECT_SOURCE_DIR}/src
    LDFLAGS ${APPLE_FRAMEWORK_LIBS}
)
