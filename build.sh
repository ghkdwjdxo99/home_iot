#!/bin/bash

set -e

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$PROJECT_DIR/build"
OBJ_DIR="$BUILD_DIR/obj"
TARGET="$BUILD_DIR/homecam"

CXX=g++
CXXFLAGS="-std=c++17 -Wall -Wextra"
INCLUDES="-I$PROJECT_DIR/cam/header -I$PROJECT_DIR/cam/config -I$PROJECT_DIR/common/header"

JOBS=$(nproc)

echo "[BUILD] home_iot project"
echo "[BUILD] project dir: $PROJECT_DIR"
echo "[BUILD] jobs: $JOBS"

mkdir -p "$BUILD_DIR"
mkdir -p "$OBJ_DIR"

$CXX $CXXFLAGS $INCLUDES -c "$PROJECT_DIR/cam/src/main.cpp" -o "$OBJ_DIR/main.o" &
$CXX $CXXFLAGS $INCLUDES -c "$PROJECT_DIR/cam/src/cam_init.cpp" -o "$OBJ_DIR/cam_init.o" &
$CXX $CXXFLAGS $INCLUDES -c "$PROJECT_DIR/cam/src/buffer_manager.cpp" -o "$OBJ_DIR/buffer_manager.o" &
$CXX $CXXFLAGS $INCLUDES -c "$PROJECT_DIR/cam/src/recorder.cpp" -o "$OBJ_DIR/recorder.o" &
$CXX $CXXFLAGS $INCLUDES -c "$PROJECT_DIR/cam/src/motion_manager.cpp" -o "$OBJ_DIR/motion_manager.o" &
$CXX $CXXFLAGS $INCLUDES -c "$PROJECT_DIR/common/src/log/log.cpp" -o "$OBJ_DIR/log.o" &

wait

$CXX \
    "$OBJ_DIR/main.o" \
    "$OBJ_DIR/cam_init.o" \
    "$OBJ_DIR/buffer_manager.o" \
    "$OBJ_DIR/recorder.o" \
    "$OBJ_DIR/motion_manager.o" \
    "$OBJ_DIR/log.o" \
    -o "$TARGET"

echo "[DONE] build complete: $TARGET"