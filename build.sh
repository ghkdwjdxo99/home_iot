#!/bin/bash

set -e

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$PROJECT_DIR/build"
OBJ_DIR="$BUILD_DIR/obj"
TARGET="$BUILD_DIR/homecam"

CXX=g++
CXXFLAGS="-std=c++17 -Wall -Wextra"
INCLUDES="-I$PROJECT_DIR/cam/header -I$PROJECT_DIR/common/header"

JOBS=$(nproc)

echo "[BUILD] home_iot project"
echo "[BUILD] project dir: $PROJECT_DIR"
echo "[BUILD] jobs: $JOBS"

mkdir -p "$BUILD_DIR"
mkdir -p "$OBJ_DIR"

# Compile source files in parallel
$CXX $CXXFLAGS $INCLUDES -c "$PROJECT_DIR/cam/src/main.cpp" -o "$OBJ_DIR/main.o" &
$CXX $CXXFLAGS $INCLUDES -c "$PROJECT_DIR/cam/src/cam_init.cpp" -o "$OBJ_DIR/cam_init.o" &
$CXX $CXXFLAGS $INCLUDES -c "$PROJECT_DIR/common/src/log/log.cpp" -o "$OBJ_DIR/log.o" &

# Wait for all compile jobs
wait

# Link object files
$CXX \
    "$OBJ_DIR/main.o" \
    "$OBJ_DIR/cam_init.o" \
    "$OBJ_DIR/log.o" \
    -o "$TARGET"

echo "[DONE] build complete: $TARGET"