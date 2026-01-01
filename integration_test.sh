#!/bin/bash

# Integration test script for OFS CLI client and server

PROJ_DIR="/home/ammar-anas/Documents/ITU/BSAI24056-Semester-03/DSA/OFS/file-verse"
COMPILED_DIR="$PROJ_DIR/compiled"
LOGS_DIR="$PROJ_DIR/logs"

echo "╔════════════════════════════════════════════════════════╗"
echo "║       OFS Integration Test - Server & Client           ║"
echo "╚════════════════════════════════════════════════════════╝"
echo ""

# Start server in background
echo "[1/3] Starting server..."
"$COMPILED_DIR/ofs_server" > "$LOGS_DIR/server.log" 2>&1 &
SERVER_PID=$!
echo "  Server PID: $SERVER_PID"
sleep 2

# Run client test
echo ""
echo "[2/3] Running CLI client test..."
{
    echo "connect localhost 8080"
    sleep 1
    echo "status"
    sleep 1
    echo "login admin password123"
    sleep 1
    echo "status"
    sleep 1
    echo "create /test_file.txt 0644"
    sleep 1
    echo "write /test_file.txt Hello from CLI"
    sleep 1
    echo "read /test_file.txt"
    sleep 1
    echo "mkdir /documents 0755"
    sleep 1
    echo "ls /"
    sleep 1
    echo "logout"
    sleep 1
    echo "exit"
} | "$COMPILED_DIR/ofs_client" 2>&1 | tee "$LOGS_DIR/client_test.log"

# Cleanup
echo ""
echo "[3/3] Shutting down server..."
kill $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null

echo ""
echo "✓ Integration test completed"
echo "  Server log: $LOGS_DIR/server.log"
echo "  Client log: $LOGS_DIR/client_test.log"
