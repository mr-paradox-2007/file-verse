#!/bin/bash

PROJ_DIR="$(pwd)"
COMPILED_DIR="$PROJ_DIR/compiled"
SOURCE_DIR="$PROJ_DIR/source"
TESTS_DIR="$PROJ_DIR/tests"

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
echo "[1/15] Compiling logger.cpp..."
$CXX $CXXFLAGS $INCLUDE_FLAGS -c "$SOURCE_DIR/utils/logger.cpp" -o "$COMPILED_DIR/logger.o"
if [ $? -ne 0 ]; then echo "ERROR: Failed to compile logger.cpp"; exit 1; fi
echo "  logger.o compiled"

echo ""
echo "[2/15] Compiling config_parser.cpp..."
$CXX $CXXFLAGS $INCLUDE_FLAGS -c "$SOURCE_DIR/utils/config_parser.cpp" -o "$COMPILED_DIR/config_parser.o"
if [ $? -ne 0 ]; then echo "ERROR: Failed to compile config_parser.cpp"; exit 1; fi
echo "  config_parser.o compiled"

echo ""
echo "[3/15] Compiling fs_format.cpp..."
$CXX $CXXFLAGS $INCLUDE_FLAGS -c "$SOURCE_DIR/core/fs_format.cpp" -o "$COMPILED_DIR/fs_format.o"
if [ $? -ne 0 ]; then echo "ERROR: Failed to compile fs_format.cpp"; exit 1; fi
echo "  fs_format.o compiled"

echo ""
echo "[4/15] Compiling user_manager.cpp..."
$CXX $CXXFLAGS $INCLUDE_FLAGS -c "$SOURCE_DIR/utils/user_manager.cpp" -o "$COMPILED_DIR/user_manager.o"
if [ $? -ne 0 ]; then echo "ERROR: Failed to compile user_manager.cpp"; exit 1; fi
echo "  user_manager.o compiled"

echo ""
echo "[5/15] Compiling fs_init.cpp..."
$CXX $CXXFLAGS $INCLUDE_FLAGS -c "$SOURCE_DIR/core/fs_init.cpp" -o "$COMPILED_DIR/fs_init.o"
if [ $? -ne 0 ]; then echo "ERROR: Failed to compile fs_init.cpp"; exit 1; fi
echo "  fs_init.o compiled"

echo ""
echo "[6/15] Compiling file_ops.cpp..."
$CXX $CXXFLAGS $INCLUDE_FLAGS -c "$SOURCE_DIR/core/file_ops.cpp" -o "$COMPILED_DIR/file_ops.o"
if [ $? -ne 0 ]; then echo "ERROR: Failed to compile file_ops.cpp"; exit 1; fi
echo "  file_ops.o compiled"

echo ""
echo "[7/15] Compiling fifo_queue.cpp..."
$CXX $CXXFLAGS $INCLUDE_FLAGS -c "$SOURCE_DIR/network/fifo_queue.cpp" -o "$COMPILED_DIR/fifo_queue.o"
if [ $? -ne 0 ]; then echo "ERROR: Failed to compile fifo_queue.cpp"; exit 1; fi
echo "  fifo_queue.o compiled"

echo ""
echo "[8/15] Compiling server.cpp..."
$CXX $CXXFLAGS $INCLUDE_FLAGS -c "$SOURCE_DIR/network/server.cpp" -o "$COMPILED_DIR/server.o"
if [ $? -ne 0 ]; then echo "ERROR: Failed to compile server.cpp"; exit 1; fi
echo "  server.o compiled"

echo ""
echo "[9/15] Compiling cli_client.cpp..."
$CXX $CXXFLAGS $INCLUDE_FLAGS -c "$SOURCE_DIR/client/cli_client.cpp" -o "$COMPILED_DIR/cli_client.o"
if [ $? -ne 0 ]; then echo "ERROR: Failed to compile cli_client.cpp"; exit 1; fi
echo "  cli_client.o compiled"

echo ""
echo "[10/15] Compiling cli_main.cpp..."
$CXX $CXXFLAGS $INCLUDE_FLAGS -c "$SOURCE_DIR/client/cli_main.cpp" -o "$COMPILED_DIR/cli_main.o"
if [ $? -ne 0 ]; then echo "ERROR: Failed to compile cli_main.cpp"; exit 1; fi
echo "  cli_main.o compiled"

echo ""
echo "[11/15] Compiling server_main.cpp..."
$CXX $CXXFLAGS $INCLUDE_FLAGS -c "$SOURCE_DIR/network/server_main.cpp" -o "$COMPILED_DIR/server_main.o"
if [ $? -ne 0 ]; then echo "ERROR: Failed to compile server_main.cpp"; exit 1; fi
echo "  server_main.o compiled"

echo ""
echo "[12/15] Linking ofs_server executable..."
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
echo "  ofs_server executable created"

echo ""
echo "[13/15] Linking ofs_client executable..."
$CXX $CXXFLAGS \
    "$COMPILED_DIR/logger.o" \
    "$COMPILED_DIR/cli_client.o" \
    "$COMPILED_DIR/cli_main.o" \
    $LINK_FLAGS \
    -o "$COMPILED_DIR/ofs_client"
if [ $? -ne 0 ]; then echo "ERROR: Failed to link ofs_client"; exit 1; fi
echo "  ofs_client executable created"

echo ""
echo "[14/15] Compiling format_test..."
$CXX $CXXFLAGS $INCLUDE_FLAGS -c "$TESTS_DIR/format_test.cpp" -o "$COMPILED_DIR/format_test.o"
if [ $? -ne 0 ]; then echo "ERROR: Failed to compile format_test.cpp"; exit 1; fi

$CXX $CXXFLAGS \
    "$COMPILED_DIR/logger.o" \
    "$COMPILED_DIR/config_parser.o" \
    "$COMPILED_DIR/fs_format.o" \
    "$COMPILED_DIR/format_test.o" \
    $LINK_FLAGS \
    -o "$COMPILED_DIR/format_test"
if [ $? -ne 0 ]; then echo "ERROR: Failed to link format_test"; exit 1; fi
echo "  format_test executable created"

echo ""
echo "[15/15] Compiling user_auth_test..."
$CXX $CXXFLAGS $INCLUDE_FLAGS -c "$TESTS_DIR/user_auth_test.cpp" -o "$COMPILED_DIR/user_auth_test.o"
if [ $? -ne 0 ]; then echo "ERROR: Failed to compile user_auth_test.cpp"; exit 1; fi

$CXX $CXXFLAGS \
    "$COMPILED_DIR/logger.o" \
    "$COMPILED_DIR/config_parser.o" \
    "$COMPILED_DIR/user_manager.o" \
    "$COMPILED_DIR/user_auth_test.o" \
    $LINK_FLAGS \
    -o "$COMPILED_DIR/user_auth_test"
if [ $? -ne 0 ]; then echo "ERROR: Failed to link user_auth_test"; exit 1; fi
echo "  user_auth_test executable created"

echo ""
echo "=========================================="
echo "  Build Complete"
echo "=========================================="
echo ""
echo "Executables created:"
echo "  $COMPILED_DIR/ofs_server"
echo "  $COMPILED_DIR/ofs_client"
echo "  $COMPILED_DIR/format_test"
echo "  $COMPILED_DIR/user_auth_test"
echo ""
echo "To run:"
echo "  ./compiled/ofs_server      (in one terminal)"
echo "  ./compiled/ofs_client      (in another terminal)"
echo ""