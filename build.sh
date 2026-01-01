#!/bin/bash

set -e

PROJ_DIR="$(pwd)"
COMPILED_DIR="$PROJ_DIR/compiled"
SOURCE_DIR="$PROJ_DIR/source"

echo "=========================================="
echo "  OFS Build System"
echo "=========================================="

mkdir -p "$COMPILED_DIR"
mkdir -p "$PROJ_DIR/logs"
mkdir -p "$PROJ_DIR/data"

CXX="g++"
CXXFLAGS="-std=c++17 -Wall -Wextra -g -O2"
INCLUDE_FLAGS="-I$SOURCE_DIR/include"
LINK_FLAGS="-lssl -lcrypto -pthread"

echo ""
echo "[1/11] Compiling logger.cpp..."
$CXX $CXXFLAGS $INCLUDE_FLAGS -c "$SOURCE_DIR/utils/logger.cpp" -o "$COMPILED_DIR/logger.o"
if [ $? -ne 0 ]; then echo "ERROR: Failed to compile logger.cpp"; exit 1; fi
echo "  logger.o compiled"

echo ""
echo "[2/11] Compiling config_parser.cpp..."
$CXX $CXXFLAGS $INCLUDE_FLAGS -c "$SOURCE_DIR/utils/config_parser.cpp" -o "$COMPILED_DIR/config_parser.o"
if [ $? -ne 0 ]; then echo "ERROR: Failed to compile config_parser.cpp"; exit 1; fi
echo "  config_parser.o compiled"

echo ""
echo "[3/11] Compiling fs_format.cpp..."
$CXX $CXXFLAGS $INCLUDE_FLAGS -c "$SOURCE_DIR/core/fs_format.cpp" -o "$COMPILED_DIR/fs_format.o"
if [ $? -ne 0 ]; then echo "ERROR: Failed to compile fs_format.cpp"; exit 1; fi
echo "  fs_format.o compiled"

echo ""
echo "[4/11] Compiling user_manager.cpp..."
$CXX $CXXFLAGS $INCLUDE_FLAGS -c "$SOURCE_DIR/utils/user_manager.cpp" -o "$COMPILED_DIR/user_manager.o"
if [ $? -ne 0 ]; then echo "ERROR: Failed to compile user_manager.cpp"; exit 1; fi
echo "  user_manager.o compiled"

echo ""
echo "[5/11] Compiling fs_init.cpp..."
$CXX $CXXFLAGS $INCLUDE_FLAGS -c "$SOURCE_DIR/core/fs_init.cpp" -o "$COMPILED_DIR/fs_init.o"
if [ $? -ne 0 ]; then echo "ERROR: Failed to compile fs_init.cpp"; exit 1; fi
echo "  fs_init.o compiled"

echo ""
echo "[6/11] Compiling file_ops.cpp..."
$CXX $CXXFLAGS $INCLUDE_FLAGS -c "$SOURCE_DIR/core/file_ops.cpp" -o "$COMPILED_DIR/file_ops.o"
if [ $? -ne 0 ]; then echo "ERROR: Failed to compile file_ops.cpp"; exit 1; fi
echo "  file_ops.o compiled"

echo ""
echo "[7/11] Compiling fifo_queue.cpp..."
$CXX $CXXFLAGS $INCLUDE_FLAGS -c "$SOURCE_DIR/network/fifo_queue.cpp" -o "$COMPILED_DIR/fifo_queue.o"
if [ $? -ne 0 ]; then echo "ERROR: Failed to compile fifo_queue.cpp"; exit 1; fi
echo "  fifo_queue.o compiled"

echo ""
echo "[8/11] Compiling server.cpp..."
$CXX $CXXFLAGS $INCLUDE_FLAGS -c "$SOURCE_DIR/network/server.cpp" -o "$COMPILED_DIR/server.o"
if [ $? -ne 0 ]; then echo "ERROR: Failed to compile server.cpp"; exit 1; fi
echo "  server.o compiled"

echo ""
echo "[9/11] Compiling cli_client.cpp..."
$CXX $CXXFLAGS $INCLUDE_FLAGS -c "$SOURCE_DIR/client/cli_client.cpp" -o "$COMPILED_DIR/cli_client.o"
if [ $? -ne 0 ]; then echo "ERROR: Failed to compile cli_client.cpp"; exit 1; fi
echo "  cli_client.o compiled"

echo ""
echo "[10/11] Linking ofs_server..."
$CXX $CXXFLAGS -c "$SOURCE_DIR/network/server_main.cpp" -o "$COMPILED_DIR/server_main.o" $INCLUDE_FLAGS
$CXX $CXXFLAGS \
    "$COMPILED_DIR/logger.o" \
    "$COMPILED_DIR/config_parser.o" \
    "$COMPILED_DIR/fs_format.o" \
    "$COMPILED_DIR/user_manager.o" \
    "$COMPILED_DIR/fs_init.o" \
    "$COMPILED_DIR/file_ops.o" \
    "$COMPILED_DIR/fifo_queue.o" \
    "$COMPILED_DIR/server.o" \
    "$COMPILED_DIR/server_main.o" \
    $LINK_FLAGS \
    -o "$COMPILED_DIR/ofs_server"
if [ $? -ne 0 ]; then echo "ERROR: Failed to link ofs_server"; exit 1; fi
echo "  ofs_server created"

echo ""
echo "[11/11] Linking ofs_client..."
$CXX $CXXFLAGS -c "$SOURCE_DIR/client/cli_main.cpp" -o "$COMPILED_DIR/cli_main.o" $INCLUDE_FLAGS
$CXX $CXXFLAGS \
    "$COMPILED_DIR/logger.o" \
    "$COMPILED_DIR/cli_client.o" \
    "$COMPILED_DIR/cli_main.o" \
    $LINK_FLAGS \
    -o "$COMPILED_DIR/ofs_client"
if [ $? -ne 0 ]; then echo "ERROR: Failed to link ofs_client"; exit 1; fi
echo "  ofs_client created"

echo ""
echo "=========================================="
echo "  Build Complete"
echo "=========================================="
echo ""
echo "Executables created:"
echo "  $COMPILED_DIR/ofs_server"
echo "  $COMPILED_DIR/ofs_client"
echo ""
echo "To run:"
echo "  Terminal 1: ./compiled/ofs_server"
echo "  Terminal 2: ./compiled/ofs_client"
echo ""