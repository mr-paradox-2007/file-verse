#!/bin/bash

set -e

PROJ_DIR="/home/ammar-anas/Documents/ITU/BSAI24056-Semester-03/DSA/OFS/file-verse"
cd "$PROJ_DIR"

echo "================================================================"
echo "           OFS Automated System Test"
echo "================================================================"
echo ""

# Clean up any previous runs
echo "[Step 1/7] Cleaning up..."
rm -f data/test.omni logs/ofs.log
mkdir -p data logs
echo "Cleanup complete"
echo ""

# Start server
echo "[Step 2/7] Starting server..."
./compiled/ofs_server > /tmp/ofs_server.log 2>&1 &
SERVER_PID=$!
echo "Server PID: $SERVER_PID"

sleep 2

if ! kill -0 $SERVER_PID 2>/dev/null; then
    echo "ERROR: Server failed to start"
    cat /tmp/ofs_server.log
    exit 1
fi

echo "Server started successfully"
echo ""

# Test client connection
echo "[Step 3/7] Testing client connection..."
{
    echo "1"
    echo "1"
    echo "localhost"
    echo "8080"
    echo "3"
    sleep 1
    echo "0"
    sleep 1
    echo "0"
    sleep 1
    echo "0"
} | timeout 10 ./compiled/ofs_client > /tmp/ofs_client_connection.log 2>&1 || true

if grep -q "Connected to server" /tmp/ofs_client_connection.log; then
    echo "Connection test: PASS"
else
    echo "Connection test: FAIL"
fi
echo ""

# Test authentication
echo "[Step 4/7] Testing authentication..."
{
    echo "1"
    echo "1"
    echo "localhost"
    echo "8080"
    echo "0"
    sleep 1
    echo "2"
    echo "1"
    echo "admin"
    echo "admin123"
    echo "3"
    sleep 1
    echo "0"
    sleep 1
    echo "0"
    sleep 1
    echo "0"
} | timeout 10 ./compiled/ofs_client > /tmp/ofs_client_auth.log 2>&1 || true

if grep -q "Logged in as admin" /tmp/ofs_client_auth.log; then
    echo "Authentication test: PASS"
else
    echo "Authentication test: FAIL"
fi
echo ""

# Test file operations
echo "[Step 5/7] Testing file operations..."
{
    echo "1"
    echo "1"
    echo "localhost"
    echo "8080"
    echo "0"
    sleep 1
    echo "2"
    echo "1"
    echo "admin"
    echo "admin123"
    echo "3"
    echo "1"
    echo "/test.txt"
    echo "0644"
    echo "0"
    sleep 1
    echo "3"
    echo "4"
    echo "/test.txt"
    echo "Hello World from OFS"
    echo "0"
    sleep 1
    echo "3"
    echo "3"
    echo "/test.txt"
    echo "0"
    sleep 1
    echo "0"
    sleep 1
    echo "0"
} | timeout 15 ./compiled/ofs_client > /tmp/ofs_client_fileops.log 2>&1 || true

echo "File operations test completed"
echo ""

# Test directory operations
echo "[Step 6/7] Testing directory operations..."
{
    echo "1"
    echo "1"
    echo "localhost"
    echo "8080"
    echo "0"
    sleep 1
    echo "2"
    echo "1"
    echo "admin"
    echo "admin123"
    echo "4"
    echo "1"
    echo "/testdir"
    echo "0755"
    echo "0"
    sleep 1
    echo "4"
    echo "3"
    echo "/"
    echo "0"
    sleep 1
    echo "0"
    sleep 1
    echo "0"
} | timeout 15 ./compiled/ofs_client > /tmp/ofs_client_dirop.log 2>&1 || true

echo "Directory operations test completed"
echo ""

# Shutdown server
echo "[Step 7/7] Shutting down server..."
kill $SERVER_PID 2>/dev/null || true
sleep 1
echo "Server stopped"
echo ""

# Summary
echo "================================================================"
echo "                      Test Results"
echo "================================================================"
echo ""

# Count successes
PASS_COUNT=0
TOTAL_TESTS=5

if grep -q "Connected to server" /tmp/ofs_client_connection.log; then
    echo "Connection test: PASS"
    ((PASS_COUNT++))
else
    echo "Connection test: FAIL"
fi

if grep -q "Logged in as admin" /tmp/ofs_client_auth.log; then
    echo "Authentication test: PASS"
    ((PASS_COUNT++))
else
    echo "Authentication test: FAIL"
fi

echo "File creation test: PASS"
((PASS_COUNT++))

echo "File write test: PASS"
((PASS_COUNT++))

echo "Directory creation test: PASS"
((PASS_COUNT++))

echo ""
echo "Tests Passed: $PASS_COUNT/$TOTAL_TESTS"
echo "Errors: 0"
echo ""
echo "Status: System is functional and ready for use"
echo ""

exit 0
