set(ROCKSDB_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/rocksdb-8.1.1)
if (ANDROID)
    set(ROCKSDB_ANDROID_PATCH ${CMAKE_CURRENT_SOURCE_DIR}/rocksdb.android.patch)
    apply_patch_once("rocksdb_android_fix" "${ROCKSDB_SRC_DIR}" "${ROCKSDB_ANDROID_PATCH}")
elseif(IOS)
    # iOS-specific patch for endianness detection
    set(ROCKSDB_IOS_PATCH ${CMAKE_CURRENT_SOURCE_DIR}/rocksdb.ios.patch)
    apply_patch_once("rocksdb_ios_fix" "${ROCKSDB_SRC_DIR}" "${ROCKSDB_IOS_PATCH}")
    # iOS-specific options
    set(WITH_JNI OFF CACHE BOOL "Disable JNI for iOS" FORCE)
    set(WITH_GFLAGS OFF CACHE BOOL "Disable gflags for iOS" FORCE)
    # Define OS_MACOSX for iOS to disable O_DIRECT and enable Apple-specific code paths
    add_definitions(-DOS_MACOSX)
    # Apple Clang for arm64 always supports CRC32 intrinsics (__ARM_FEATURE_CRC32 is
    # always defined), but may reject the GCC-style -march=armv8-a+crc+crypto flag
    # (especially for the simulator target). Pre-set the cache variable so RocksDB's
    # CHECK_C_COMPILER_FLAG is skipped and crc32c_arm64.cc is included in the build.
    if(CMAKE_OSX_ARCHITECTURES MATCHES "arm64")
        set(HAS_ARMV8_CRC ON CACHE INTERNAL "Force ARM CRC for iOS arm64")
    endif()
endif()

set(ROCKSDB_BUILD_SHARED OFF CACHE BOOL "Disable install in rocksdb" FORCE)

set(WITH_TESTS OFF CACHE BOOL "Disable testing in rocksdb" FORCE)
set(WITH_ALL_TESTS OFF CACHE BOOL "Build all test, rather than a small subset" FORCE)
set(WITH_BENCHMARK_TOOLS OFF CACHE BOOL "Disable benchmarks in rocksdb" FORCE)
set(WITH_CORE_TOOLS OFF CACHE BOOL "build with ldb and sst_dump" FORCE)
set(WITH_TOOLS OFF CACHE BOOL "build with tools" FORCE)
set(WITH_LZ4 ON CACHE BOOL "build with lz4" FORCE)
set(USE_RTTI ON CACHE BOOL "build with RTTI" FORCE)
set(FAIL_ON_WARNINGS OFF CACHE BOOL "build with no Werror" FORCE)
set(PORTABLE ON CACHE BOOL "build a portable lib" FORCE)

set(_SAVED_CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${EXTERNAL_LIB_DIR})
add_subdirectory(rocksdb-8.1.1)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${_SAVED_CMAKE_ARCHIVE_OUTPUT_DIRECTORY})
unset(_SAVED_CMAKE_ARCHIVE_OUTPUT_DIRECTORY)

get_target_property(rocksdb_SOURCE_DIR rocksdb SOURCE_DIR)
set(ROCKSDB_INCLUDE_DIR ${rocksdb_SOURCE_DIR}/include)

target_include_directories(rocksdb PUBLIC $<BUILD_INTERFACE:${ROCKSDB_INCLUDE_DIR}>)
add_dependencies(rocksdb Lz4.BUILD)
