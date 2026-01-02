#!/bin/bash
set -e

echo "Building OFS System (Phase 1)..."

# Create folders
mkdir -p compiled logs data web

# List all source files
SOURCES=(
    src/network/server_main.cpp
    src/network/fifo_queue.cpp
    src/core/fs_format.cpp
    src/core/fs_init.cpp
    src/core/file_ops.cpp
    src/core/path_resolver.cpp
    src/core/user_manager.cpp
    src/utils/crypto.cpp
    src/utils/logger.cpp
    src/utils/config_parser.cpp
)

echo "Compiling OFS server..."
g++ -std=c++17 -Wall -Wextra -g -O2 \
    -I./include \
    ${SOURCES[@]} \
    -o compiled/server \
    -pthread

echo "✓ Built: ./compiled/server"
echo "✓ All components compiled successfully"
