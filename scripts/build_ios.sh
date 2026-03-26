#!/bin/bash
set -e
CURRENT_DIR=$(pwd)

# Platform options: OS (arm64 device), SIMULATOR64 (x86_64 sim), SIMULATORARM64 (arm64 sim)
PLATFORM=${1:-"OS"}
BUILD_TYPE=${2:-"Release"}
IOS_DEPLOYMENT_TARGET="13.0"

# Determine architecture based on platform
case "$PLATFORM" in
    "OS")
        ARCH="arm64"
        CMAKE_SYSTEM_NAME="iOS"
        ;;
    "SIMULATOR64")
        ARCH="x86_64"
        CMAKE_SYSTEM_NAME="iOS"
        ;;
    "SIMULATORARM64")
        ARCH="arm64"
        CMAKE_SYSTEM_NAME="iOS"
        ;;
    *)
        echo "error: Unknown platform '$PLATFORM'"
        echo "Usage: $0 [OS|SIMULATOR64|SIMULATORARM64] [Release|Debug]"
        echo "  OS            - Build for iOS device (arm64)"
        echo "  SIMULATOR64   - Build for iOS Simulator (x86_64)"
        echo "  SIMULATORARM64- Build for iOS Simulator (arm64, Apple Silicon)"
        exit 1
        ;;
esac

echo "Building zvec for iOS"
echo "  Platform: $PLATFORM"
echo "  Architecture: $ARCH"
echo "  Build Type: $BUILD_TYPE"
echo "  iOS Deployment Target: $IOS_DEPLOYMENT_TARGET"

# step1: use host env to compile protoc
echo "step1: building protoc for host..."
HOST_BUILD_DIR="build_host"
mkdir -p $HOST_BUILD_DIR
cd $HOST_BUILD_DIR

cmake -DCMAKE_BUILD_TYPE="$BUILD_TYPE" ..
make -j protoc
PROTOC_EXECUTABLE=$CURRENT_DIR/$HOST_BUILD_DIR/bin/protoc
cd $CURRENT_DIR

echo "step1: Done!!!"

# step2: cross build zvec for iOS
echo "step2: building zvec for iOS..."

# reset thirdparty directory
git submodule foreach --recursive 'git stash --include-untracked'

BUILD_DIR="build_ios_${PLATFORM}"
mkdir -p $BUILD_DIR
cd $BUILD_DIR

# Determine SDK and additional flags based on platform
if [ "$PLATFORM" = "OS" ]; then
    SDK_NAME="iphoneos"
    ENABLE_BITCODE="ON"
else
    SDK_NAME="iphonesimulator"
    ENABLE_BITCODE="OFF"
fi

SDK_PATH=$(xcrun --sdk $SDK_NAME --show-sdk-path)

echo "configure CMake..."
cmake \
    -DCMAKE_SYSTEM_NAME=iOS \
    -DCMAKE_OSX_DEPLOYMENT_TARGET="$IOS_DEPLOYMENT_TARGET" \
    -DCMAKE_OSX_ARCHITECTURES="$ARCH" \
    -DCMAKE_OSX_SYSROOT="$SDK_PATH" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DBUILD_PYTHON_BINDINGS=OFF \
    -DBUILD_TOOLS=OFF \
    -DCMAKE_INSTALL_PREFIX="./install" \
    -DGLOBAL_CC_PROTOBUF_PROTOC=$PROTOC_EXECUTABLE \
    -DENABLE_BITCODE="$ENABLE_BITCODE" \
    -DIOS=ON \
    ../

echo "building..."
CORE_COUNT=$(sysctl -n hw.ncpu)
make -j$CORE_COUNT

echo "step2: Done!!!"
echo ""
echo "Build completed successfully!"
echo "Output directory: $CURRENT_DIR/$BUILD_DIR"
