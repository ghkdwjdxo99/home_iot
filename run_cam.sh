#!/bin/bash

set -e

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
EXEC="$PROJECT_DIR/build/homecam"

if [ ! -f "$EXEC" ]; then
    echo "[ERROR] homecam executable not found"
    echo "Run ./build.sh first"
    exit 1
fi

cd "$PROJECT_DIR"
"$EXEC"