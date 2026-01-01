#!/bin/bash

set -e

PROJ_DIR="$(pwd)"
cd "$PROJ_DIR"

echo "================================================================"
echo "           OFS System Test Suite"
echo "================================================================"
echo ""

rm -f data/test.omni logs/ofs.log
mkdir -p data logs

echo "[1/5] Starting server..."
./compiled/ofs_server > /tmp/ofs_server.log 2>&1 &
SERVER_PID=$!
echo "  Server PID: $SERVER_PID"

sleep 2

if ! kill -0 $SERVER_PID 2>/dev/null; then
    echo "ERROR: Server failed to start"
    cat /tmp/ofs_server.log
    exit 1
fi

echo "  Server started successfully"
echo ""

echo "[2/5] Testing client connection..."
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
} | timeout 10 ./compiled/ofs_client > /tmp/test_connection.log 2>&1 || true

if grep -q "Connected to server" /tmp/test_connection.log; then
    echo "  Connection test: PASS"
else
    echo "  Connection test: FAIL"
fi
echo ""

echo "[3/5] Testing authentication..."
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
} | timeout 10 ./compiled/ofs_client > /tmp/test_auth.log 2>&1 || true

if grep -q "Logged in as admin" /tmp/test_auth.log; then
    echo "  Authentication test: PASS"
else
    echo "  Authentication test: FAIL"
fi
echo ""

echo "[4/5] Testing file operations..."
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
    echo "0"
    sleep 1
    echo "3"
    echo "1"
    echo "/test.txt"
    echo "0644"
    sleep 1
    echo "4"
    echo "/test.txt"
    echo "Test content"
    sleep 1
    echo "3"
    echo "/test.txt"
    sleep 1
    echo "0"
    sleep 1
    echo "0"
    sleep 1
    echo "0"
} | timeout 15 ./compiled/ofs_client > /tmp/test_fileops.log 2>&1 || true

if grep -q "File created" /tmp/test_fileops.log && grep -q "File written" /tmp/test_fileops.log; then
    echo "  File operations test: PASS"
else
    echo "  File operations test: FAIL"
fi
echo ""

echo "[5/5] Shutting down server..."
kill $SERVER_PID 2>/dev/null || true
sleep 1
echo "  Server stopped"
echo ""

echo "================================================================"
echo "                      Test Summary"
echo "================================================================"
echo ""

PASS_COUNT=0
TOTAL_TESTS=3

if grep -q "Connected to server" /tmp/test_connection.log; then
    echo "Connection test: PASS"
    ((PASS_COUNT++))
else
    echo "Connection test: FAIL"
fi

if grep -q "Logged in as admin" /tmp/test_auth.log; then
    echo "Authentication test: PASS"
    ((PASS_COUNT++))
else
    echo "Authentication test: FAIL"
fi

if grep -q "File created" /tmp/test_fileops.log; then
    echo "File operations test: PASS"
    ((PASS_COUNT++))
else
    echo "File operations test: FAIL"
fi

echo ""
echo "Tests Passed: $PASS_COUNT/$TOTAL_TESTS"
echo ""

if [ $PASS_COUNT -eq $TOTAL_TESTS ]; then
    echo "Status: ALL TESTS PASSED"
    exit 0
else
    echo "Status: SOME TESTS FAILED"
    exit 1
fi