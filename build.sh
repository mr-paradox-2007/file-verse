#!/bin/bash

# Build script for OFS Configuration & Format System Test

PROJ_DIR="/home/ammar-anas/Documents/ITU/BSAI24056-Semester-03/DSA/OFS/file-verse"
COMPILED_DIR="$PROJ_DIR/compiled"
SOURCE_DIR="$PROJ_DIR/source"
TESTS_DIR="$PROJ_DIR/tests"

echo "=========================================="
echo "  OFS Build System"
echo "=========================================="

# Create compiled directory if it doesn't exist
mkdir -p "$COMPILED_DIR"
mkdir -p "$PROJ_DIR/logs"

# Compile flags
CXX="g++"
CXXFLAGS="-std=c++17 -Wall -Wextra -g -O2"
INCLUDE_FLAGS="-I$SOURCE_DIR/include"

echo ""
echo "[1] Compiling logger.cpp..."
$CXX $CXXFLAGS $INCLUDE_FLAGS -c "$SOURCE_DIR/utils/logger.cpp" -o "$COMPILED_DIR/logger.o"
if [ $? -ne 0 ]; then
    echo "ERROR: Failed to compile logger.cpp"
    exit 1
fi
echo "✓ logger.o compiled"

echo ""
echo "[2] Compiling config_parser.cpp..."
$CXX $CXXFLAGS $INCLUDE_FLAGS -c "$SOURCE_DIR/utils/config_parser.cpp" -o "$COMPILED_DIR/config_parser.o"
if [ $? -ne 0 ]; then
    echo "ERROR: Failed to compile config_parser.cpp"
    exit 1
fi
echo "✓ config_parser.o compiled"

echo ""
echo "[3] Compiling fs_format.cpp..."
$CXX $CXXFLAGS $INCLUDE_FLAGS -c "$SOURCE_DIR/core/fs_format.cpp" -o "$COMPILED_DIR/fs_format.o"
if [ $? -ne 0 ]; then
    echo "ERROR: Failed to compile fs_format.cpp"
    exit 1
fi
echo "✓ fs_format.o compiled"

echo ""
echo "[4] Compiling user_manager.cpp..."
$CXX $CXXFLAGS $INCLUDE_FLAGS -c "$SOURCE_DIR/utils/user_manager.cpp" -o "$COMPILED_DIR/user_manager.o"
if [ $? -ne 0 ]; then
    echo "ERROR: Failed to compile user_manager.cpp"
    exit 1
fi
echo "✓ user_manager.o compiled"

echo ""
echo "[5] Compiling format_test.cpp..."
$CXX $CXXFLAGS $INCLUDE_FLAGS -c "$TESTS_DIR/format_test.cpp" -o "$COMPILED_DIR/format_test.o"
if [ $? -ne 0 ]; then
    echo "ERROR: Failed to compile format_test.cpp"
    exit 1
fi
echo "✓ format_test.o compiled"

echo ""
echo "[6] Compiling user_auth_test.cpp..."
$CXX $CXXFLAGS $INCLUDE_FLAGS -c "$TESTS_DIR/user_auth_test.cpp" -o "$COMPILED_DIR/user_auth_test.o"
if [ $? -ne 0 ]; then
    echo "ERROR: Failed to compile user_auth_test.cpp"
    exit 1
fi
echo "✓ user_auth_test.o compiled"

echo ""
echo "[7] Linking format_test executable..."
$CXX $CXXFLAGS "$COMPILED_DIR/logger.o" \
                "$COMPILED_DIR/config_parser.o" \
                "$COMPILED_DIR/fs_format.o" \
                "$COMPILED_DIR/format_test.o" \
                -o "$COMPILED_DIR/format_test"
if [ $? -ne 0 ]; then
    echo "ERROR: Failed to link format_test"
    exit 1
fi
echo "✓ format_test executable created"

echo ""
echo "[8] Linking user_auth_test executable..."
$CXX $CXXFLAGS "$COMPILED_DIR/logger.o" \
                "$COMPILED_DIR/user_manager.o" \
                "$COMPILED_DIR/user_auth_test.o" \
                -lssl -lcrypto \
                -o "$COMPILED_DIR/user_auth_test"
if [ $? -ne 0 ]; then
    echo "ERROR: Failed to link user_auth_test"
    exit 1
fi
echo "✓ user_auth_test executable created"

echo ""
echo "=========================================="
echo "  Build Complete - Running Tests"
echo "=========================================="
echo ""

cd "$PROJ_DIR"

echo "Running format test..."
"$COMPILED_DIR/format_test"
FORMAT_TEST_RESULT=$?

echo ""
echo "=========================================="
echo "  User Authentication Test"
echo "=========================================="
echo ""

echo "Running user authentication test..."
"$COMPILED_DIR/user_auth_test"
AUTH_TEST_RESULT=$?

echo ""
echo "=========================================="
if [ $FORMAT_TEST_RESULT -eq 0 ] && [ $AUTH_TEST_RESULT -eq 0 ]; then
    echo "  ✓ All tests passed!"
else
    echo "  ✗ Some tests failed"
fi
echo "=========================================="
