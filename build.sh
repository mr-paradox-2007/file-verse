#!/bin/bash
set -e

echo "====================================="
echo "  Building OFS Multi-User System    "
echo "====================================="
echo ""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

# Create necessary directories
echo "[*] Creating directories..."
mkdir -p compiled logs data web

# Check for required tools
echo "[*] Checking dependencies..."
if ! command -v g++ &> /dev/null; then
    echo -e "${RED}[ERROR]${NC} g++ not found. Install with: sudo apt-get install build-essential"
    exit 1
fi

if ! ldconfig -p | grep -q libcrypto; then
    echo -e "${YELLOW}[WARNING]${NC} OpenSSL not found. Install with: sudo apt-get install libssl-dev"
    echo -e "${YELLOW}[WARNING]${NC} Continuing anyway..."
fi

echo -e "${GREEN}[OK]${NC} Dependencies verified"
echo ""

# Compile core modules
echo "[1/5] Compiling storage engine..."
g++ -c -std=c++17 -O2 -Wall -I./include src/core/omni_storage.cpp -o compiled/omni_storage.o

echo "[2/5] Compiling file operations..."
g++ -c -std=c++17 -O2 -Wall -I./include src/core/file_ops.cpp -o compiled/file_ops.o
g++ -c -std=c++17 -O2 -Wall -I./include src/core/user_manager.cpp -o compiled/user_manager.o
g++ -c -std=c++17 -O2 -Wall -I./include src/core/path_resolver.cpp -o compiled/path_resolver.o

echo "[3/5] Compiling utilities..."
g++ -c -std=c++17 -O2 -Wall -I./include src/utils/crypto.cpp -o compiled/crypto.o
g++ -c -std=c++17 -O2 -Wall -I./include src/utils/logger.cpp -o compiled/logger.o
g++ -c -std=c++17 -O2 -Wall -I./include src/utils/config_parser.cpp -o compiled/config_parser.o

echo "[4/5] Compiling initialization..."
if [ -f "src/core/fs_init.cpp" ]; then
    g++ -c -std=c++17 -O2 -Wall -I./include src/core/fs_init.cpp -o compiled/fs_init.o
    echo "    - fs_init.cpp compiled"
else
    echo "    - fs_init.cpp not found (skipping)"
fi

if [ -f "src/core/fs_format.cpp" ]; then
    g++ -c -std=c++17 -O2 -Wall -I./include src/core/fs_format.cpp -o compiled/fs_format.o
    echo "    - fs_format.cpp compiled"
else
    echo "    - fs_format.cpp not found (skipping)"
fi

if [ -f "src/network/fifo_queue.cpp" ]; then
    g++ -c -std=c++17 -O2 -Wall -I./include src/network/fifo_queue.cpp -o compiled/fifo_queue.o
    echo "    - fifo_queue.cpp compiled"
else
    echo "    - fifo_queue.cpp not found (skipping)"
fi

echo "[5/5] Linking server..."
g++ -std=c++17 -O2 -Wall -I./include \
    src/network/server_main.cpp \
    compiled/omni_storage.o \
    compiled/file_ops.o \
    compiled/user_manager.o \
    compiled/path_resolver.o \
    compiled/crypto.o \
    compiled/logger.o \
    compiled/config_parser.o \
    $([ -f "compiled/fs_init.o" ] && echo "compiled/fs_init.o") \
    $([ -f "compiled/fs_format.o" ] && echo "compiled/fs_format.o") \
    $([ -f "compiled/fifo_queue.o" ] && echo "compiled/fifo_queue.o") \
    -o compiled/server \
    -pthread \
    -lcrypto 2>/dev/null || \
g++ -std=c++17 -O2 -Wall -I./include \
    src/network/server_main.cpp \
    compiled/omni_storage.o \
    compiled/file_ops.o \
    compiled/user_manager.o \
    compiled/path_resolver.o \
    compiled/crypto.o \
    compiled/logger.o \
    compiled/config_parser.o \
    $([ -f "compiled/fs_init.o" ] && echo "compiled/fs_init.o") \
    $([ -f "compiled/fs_format.o" ] && echo "compiled/fs_format.o") \
    $([ -f "compiled/fifo_queue.o" ] && echo "compiled/fifo_queue.o") \
    -o compiled/server \
    -pthread

echo ""
echo "====================================="
echo "  Build Complete!                   "
echo "====================================="
echo ""
echo "To run the server:"
echo -e "  ${GREEN}./compiled/server${NC}"
echo ""
echo "Then open your browser:"
echo -e "  ${GREEN}http://localhost:9000${NC}"
echo ""
echo "Default login:"
echo "  Username: admin"
echo "  Password: admin123"
echo ""
echo "Server will:"
echo "  - Create filesystem on first run"
echo "  - Listen on port 9000"
echo "  - Support multiple concurrent users"
echo "  - Prevent duplicate logins"
echo ""