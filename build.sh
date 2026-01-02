#!/bin/bash
set -e

echo "╔════════════════════════════════════════╗"
echo "║  Building OFS - Complete System        ║"
echo "╚════════════════════════════════════════╝"
echo ""

mkdir -p compiled logs data web

echo "[1/4] Compiling storage engine..."
g++ -c -std=c++17 -O2 -I./include src/core/omni_storage.cpp -o compiled/omni_storage.o

echo "[2/4] Compiling core modules..."
g++ -c -std=c++17 -O2 -I./include src/core/file_ops.cpp -o compiled/file_ops.o
g++ -c -std=c++17 -O2 -I./include src/core/user_manager.cpp -o compiled/user_manager.o
g++ -c -std=c++17 -O2 -I./include src/core/path_resolver.cpp -o compiled/path_resolver.o

echo "[3/4] Compiling utilities..."
g++ -c -std=c++17 -O2 -I./include src/utils/crypto.cpp -o compiled/crypto.o
g++ -c -std=c++17 -O2 -I./include src/utils/logger.cpp -o compiled/logger.o
g++ -c -std=c++17 -O2 -I./include src/utils/config_parser.cpp -o compiled/config_parser.o

echo "[4/4] Linking server..."
g++ -std=c++17 -O2 -I./include \
    src/network/server_main.cpp \
    compiled/omni_storage.o \
    compiled/file_ops.o \
    compiled/user_manager.o \
    compiled/path_resolver.o \
    compiled/crypto.o \
    compiled/logger.o \
    compiled/config_parser.o \
    -o compiled/server \
    -pthread

echo ""
echo "╔════════════════════════════════════════╗"
echo "║  Build Complete!                       ║"
echo "╚════════════════════════════════════════╝"
echo ""
echo "To run the server:"
echo "  ./compiled/server"
echo ""
echo "Then open: http://localhost:9000"
echo "Login: admin / admin123"
echo ""