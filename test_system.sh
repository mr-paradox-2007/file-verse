#!/bin/bash

BASE_URL="http://localhost:9000"
GREEN='\033[0;32m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m'

echo "╔════════════════════════════════════════╗"
echo "║  OFS System Test Suite                ║"
echo "╚════════════════════════════════════════╝"
echo ""

test_count=0
pass_count=0

run_test() {
    test_count=$((test_count + 1))
    echo -n "[$test_count] $1 ... "
}

pass() {
    pass_count=$((pass_count + 1))
    echo -e "${GREEN}PASS${NC}"
}

fail() {
    echo -e "${RED}FAIL${NC}"
    if [ ! -z "$1" ]; then
        echo "    Error: $1"
    fi
}

# Test 1: Admin Login
run_test "Admin login"
response=$(curl -s -X POST $BASE_URL/user/login \
    -H "Content-Type: application/json" \
    -d '{"username":"admin","password":"admin123"}')

if echo "$response" | grep -q '"success":true'; then
    pass
else
    fail "$response"
fi

# Test 2: Create User
run_test "Create user 'testuser'"
response=$(curl -s -X POST $BASE_URL/user/signup \
    -H "Content-Type: application/json" \
    -d '{"username":"testuser","password":"test1234"}')

if echo "$response" | grep -q '"success":true'; then
    pass
else
    fail "$response"
fi

# Test 3: User Login
run_test "Login as testuser"
response=$(curl -s -X POST $BASE_URL/user/login \
    -H "Content-Type: application/json" \
    -d '{"username":"testuser","password":"test1234"}')

if echo "$response" | grep -q '"success":true'; then
    pass
else
    fail "$response"
fi

# Test 4: Create File
run_test "Create file /test.txt"
response=$(curl -s -X POST $BASE_URL/file/create \
    -H "Content-Type: application/json" \
    -d '{"path":"/test.txt","content":"Hello World"}')

if echo "$response" | grep -q '"success":true'; then
    pass
else
    fail "$response"
fi

# Test 5: Read File
run_test "Read file /test.txt"
response=$(curl -s -X POST $BASE_URL/file/read \
    -H "Content-Type: application/json" \
    -d '{"path":"/test.txt"}')

if echo "$response" | grep -q '"content":"Hello World"'; then
    pass
else
    fail "$response"
fi

# Test 6: Create Directory
run_test "Create directory /docs"
response=$(curl -s -X POST $BASE_URL/directory/create \
    -H "Content-Type: application/json" \
    -d '{"path":"/docs"}')

if echo "$response" | grep -q '"success":true'; then
    pass
else
    fail "$response"
fi

# Test 7: Create File in Directory
run_test "Create file /docs/readme.txt"
response=$(curl -s -X POST $BASE_URL/file/create \
    -H "Content-Type: application/json" \
    -d '{"path":"/docs/readme.txt","content":"Documentation"}')

if echo "$response" | grep -q '"success":true'; then
    pass
else
    fail "$response"
fi

# Test 8: List Directory
run_test "List directory /"
response=$(curl -s -X POST $BASE_URL/file/list \
    -H "Content-Type: application/json" \
    -d '{"path":"/"}')

if echo "$response" | grep -q '"name":"test.txt"' && \
   echo "$response" | grep -q '"name":"docs"'; then
    pass
else
    fail "$response"
fi

# Test 9: List Subdirectory
run_test "List directory /docs"
response=$(curl -s -X POST $BASE_URL/file/list \
    -H "Content-Type: application/json" \
    -d '{"path":"/docs"}')

if echo "$response" | grep -q '"name":"readme.txt"'; then
    pass
else
    fail "$response"
fi

# Test 10: Edit File
run_test "Edit file /test.txt"
response=$(curl -s -X POST $BASE_URL/file/edit \
    -H "Content-Type: application/json" \
    -d '{"path":"/test.txt","content":"Updated Content"}')

if echo "$response" | grep -q '"success":true'; then
    pass
else
    fail "$response"
fi

# Test 11: Read Edited File
run_test "Read edited file /test.txt"
response=$(curl -s -X POST $BASE_URL/file/read \
    -H "Content-Type: application/json" \
    -d '{"path":"/test.txt"}')

if echo "$response" | grep -q '"content":"Updated Content"'; then
    pass
else
    fail "$response"
fi

# Test 12: Delete File
run_test "Delete file /test.txt"
response=$(curl -s -X POST $BASE_URL/file/delete \
    -H "Content-Type: application/json" \
    -d '{"path":"/test.txt"}')

if echo "$response" | grep -q '"success":true'; then
    pass
else
    fail "$response"
fi

# Test 13: Verify Deletion
run_test "Verify /test.txt is deleted"
response=$(curl -s -X POST $BASE_URL/file/read \
    -H "Content-Type: application/json" \
    -d '{"path":"/test.txt"}')

if echo "$response" | grep -q '"success":false'; then
    pass
else
    fail "File still exists!"
fi

# Test 14: Large File
run_test "Create large file (10KB)"
large_content=$(python3 -c "print('X' * 10000)")
response=$(curl -s -X POST $BASE_URL/file/create \
    -H "Content-Type: application/json" \
    -d "{\"path\":\"/large.txt\",\"content\":\"$large_content\"}")

if echo "$response" | grep -q '"success":true'; then
    pass
else
    fail "$response"
fi

# Test 15: Read Large File
run_test "Read large file"
response=$(curl -s -X POST $BASE_URL/file/read \
    -H "Content-Type: application/json" \
    -d '{"path":"/large.txt"}')

content_length=$(echo "$response" | grep -o '"content":"X*"' | wc -c)
if [ $content_length -gt 10000 ]; then
    pass
else
    fail "Content too short: $content_length bytes"
fi

echo ""
echo "╔════════════════════════════════════════╗"
echo "║  Test Results                          ║"
echo "╚════════════════════════════════════════╝"
echo ""
echo "Total Tests: $test_count"
echo -e "Passed: ${GREEN}$pass_count${NC}"
echo -e "Failed: ${RED}$((test_count - pass_count))${NC}"
echo ""

if [ $pass_count -eq $test_count ]; then
    echo -e "${GREEN}✓ All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}✗ Some tests failed!${NC}"
    exit 1
fi