#!/bin/bash

# Quick Multi-Client Testing Guide
# Port: 8080
# Users: Use admin_cli to create test accounts

echo "╔════════════════════════════════════════╗"
echo "║     OFS Multi-Client Testing          ║"
echo "║            Port: 8080                  ║"
echo "╚════════════════════════════════════════╝"
echo ""

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
SERVER_URL="http://localhost:8080"

# Verify server is running
echo "Checking server at $SERVER_URL..."
if curl -s "$SERVER_URL" > /dev/null 2>&1; then
    echo "✓ Server is running"
else
    echo "✗ Server is NOT running"
    echo "Start it with: ./compiled/server"
    exit 1
fi

echo ""
echo "To test multiple clients:"
echo ""
echo "1. Open Browser Tab 1: $SERVER_URL"
echo "   Login as: alice / alice_pass"
echo ""
echo "2. Open Browser Tab 2: $SERVER_URL"  
echo "   Login as: bob / bob_pass"
echo ""
echo "3. Open Browser Tab 3: $SERVER_URL"
echo "   Try to login as: alice / alice_pass"
echo "   Result: Should be BLOCKED (already logged in)"
echo ""
echo "═════════════════════════════════════════"
echo ""

# Create test accounts if they don't exist
echo "Setting up test accounts..."
$SCRIPT_DIR/compiled/admin_cli create alice alice_pass 2>/dev/null || true
$SCRIPT_DIR/compiled/admin_cli create bob bob_pass 2>/dev/null || true
$SCRIPT_DIR/compiled/admin_cli create charlie charlie_pass 2>/dev/null || true

echo ""
echo "Test accounts ready:"
echo "  alice / alice_pass"
echo "  bob / bob_pass"
echo "  charlie / charlie_pass"
echo ""
echo "Opening browser... http://localhost:8080"
which xdg-open > /dev/null && xdg-open "$SERVER_URL" || true
