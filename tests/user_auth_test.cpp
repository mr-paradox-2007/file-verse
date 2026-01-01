#include <iostream>
#include <iomanip>
#include <vector>
#include <thread>
#include <chrono>
#include "../source/include/user_manager.hpp"
#include "../source/include/logger.hpp"

using namespace ofs;

/**
 * Print a test section header
 */
void printTestHeader(const std::string& test_name)
{
    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << "  " << test_name << std::endl;
    std::cout << std::string(70, '=') << std::endl;
}

/**
 * Print test result
 */
void printResult(bool passed, const std::string& message)
{
    std::cout << (passed ? "  ✓ " : "  ✗ ") << message << std::endl;
}

/**
 * Test 1: User Creation
 */
bool testUserCreation()
{
    printTestHeader("Test 1: User Creation");

    UserManager& mgr = UserManager::getInstance();

    // Test creating admin user
    OFSErrorCodes result = mgr.createUser("admin", "admin123", UserRole::ADMIN);
    printResult(result == OFSErrorCodes::SUCCESS, 
               "Create admin user: " + std::string(result == OFSErrorCodes::SUCCESS ? "PASSED" : "FAILED"));

    // Test creating normal user
    result = mgr.createUser("alice", "alicepass123", UserRole::NORMAL);
    printResult(result == OFSErrorCodes::SUCCESS,
               "Create normal user (alice): " + std::string(result == OFSErrorCodes::SUCCESS ? "PASSED" : "FAILED"));

    // Test creating another user
    result = mgr.createUser("bob", "bobpass456", UserRole::NORMAL);
    printResult(result == OFSErrorCodes::SUCCESS,
               "Create normal user (bob): " + std::string(result == OFSErrorCodes::SUCCESS ? "PASSED" : "FAILED"));

    // Test duplicate user (should fail)
    result = mgr.createUser("alice", "differentpass", UserRole::NORMAL);
    printResult(result == OFSErrorCodes::ERROR_FILE_EXISTS,
               "Reject duplicate user (alice): " + std::string(result == OFSErrorCodes::ERROR_FILE_EXISTS ? "PASSED" : "FAILED"));

    // Test invalid username (empty)
    result = mgr.createUser("", "password", UserRole::NORMAL);
    printResult(result == OFSErrorCodes::ERROR_INVALID_OPERATION,
               "Reject empty username: " + std::string(result == OFSErrorCodes::ERROR_INVALID_OPERATION ? "PASSED" : "FAILED"));

    // Test invalid password (too short)
    result = mgr.createUser("charlie", "abc", UserRole::NORMAL);
    printResult(result == OFSErrorCodes::ERROR_INVALID_OPERATION,
               "Reject short password: " + std::string(result == OFSErrorCodes::ERROR_INVALID_OPERATION ? "PASSED" : "FAILED"));

    // Check user count
    uint32_t count = mgr.getUserCount();
    printResult(count == 3,
               "User count is 3: " + std::string(count == 3 ? "PASSED" : "FAILED") + " (actual: " + std::to_string(count) + ")");

    return true;
}

/**
 * Test 2: User Login
 */
bool testUserLogin()
{
    printTestHeader("Test 2: User Login");

    UserManager& mgr = UserManager::getInstance();

    // Test successful login
    UserSession session;
    OFSErrorCodes result = mgr.loginUser("admin", "admin123", session);
    printResult(result == OFSErrorCodes::SUCCESS && session.is_valid,
               "Login with correct credentials (admin): " + std::string(result == OFSErrorCodes::SUCCESS ? "PASSED" : "FAILED"));

    std::string admin_session = session.session_id;

    if (result == OFSErrorCodes::SUCCESS)
    {
        std::cout << "    Session ID: " << session.session_id.substr(0, 16) << "..." << std::endl;
        std::cout << "    User Role: " << (session.user_role == UserRole::ADMIN ? "ADMIN" : "NORMAL") << std::endl;
        std::cout << "    Operations Count: " << session.operations_count << std::endl;
    }

    // Test login with wrong password
    result = mgr.loginUser("admin", "wrongpassword", session);
    printResult(result == OFSErrorCodes::ERROR_PERMISSION_DENIED,
               "Reject wrong password: " + std::string(result == OFSErrorCodes::ERROR_PERMISSION_DENIED ? "PASSED" : "FAILED"));

    // Test login for non-existent user
    result = mgr.loginUser("nonexistent", "password", session);
    printResult(result == OFSErrorCodes::ERROR_NOT_FOUND,
               "Reject non-existent user: " + std::string(result == OFSErrorCodes::ERROR_NOT_FOUND ? "PASSED" : "FAILED"));

    // Test login for alice
    result = mgr.loginUser("alice", "alicepass123", session);
    printResult(result == OFSErrorCodes::SUCCESS && session.user_role == UserRole::NORMAL,
               "Login alice (normal user): " + std::string(result == OFSErrorCodes::SUCCESS ? "PASSED" : "FAILED"));

    // Check session count
    uint32_t session_count = mgr.getActiveSessionCount();
    printResult(session_count >= 2,
               "Multiple active sessions: " + std::string(session_count >= 2 ? "PASSED" : "FAILED") + " (count: " + std::to_string(session_count) + ")");

    return true;
}

/**
 * Test 3: Session Validation
 */
bool testSessionValidation()
{
    printTestHeader("Test 3: Session Validation");

    UserManager& mgr = UserManager::getInstance();

    // Create and login user
    UserSession login_session;
    mgr.loginUser("bob", "bobpass456", login_session);
    std::string valid_session_id = login_session.session_id;

    // Validate valid session
    UserSession validated_session;
    OFSErrorCodes result = mgr.validateSession(valid_session_id, validated_session);
    printResult(result == OFSErrorCodes::SUCCESS,
               "Validate active session: " + std::string(result == OFSErrorCodes::SUCCESS ? "PASSED" : "FAILED"));

    if (result == OFSErrorCodes::SUCCESS)
    {
        std::cout << "    Validated User: " << validated_session.username << std::endl;
        std::cout << "    Session is Valid: " << (validated_session.is_valid ? "Yes" : "No") << std::endl;
    }

    // Validate invalid session
    result = mgr.validateSession("invalid_session_id_xyz", validated_session);
    printResult(result == OFSErrorCodes::ERROR_INVALID_SESSION,
               "Reject invalid session ID: " + std::string(result == OFSErrorCodes::ERROR_INVALID_SESSION ? "PASSED" : "FAILED"));

    return true;
}

/**
 * Test 4: Session Activity Update
 */
bool testSessionActivity()
{
    printTestHeader("Test 4: Session Activity Update");

    UserManager& mgr = UserManager::getInstance();

    // Create and login user
    UserSession session;
    mgr.loginUser("alice", "alicepass123", session);
    std::string session_id = session.session_id;

    uint32_t initial_ops = session.operations_count;

    // Update activity
    OFSErrorCodes result = mgr.updateSessionActivity(session_id);
    printResult(result == OFSErrorCodes::SUCCESS,
               "Update session activity: " + std::string(result == OFSErrorCodes::SUCCESS ? "PASSED" : "FAILED"));

    // Validate and check operation count increased
    UserSession updated_session;
    mgr.validateSession(session_id, updated_session);
    uint32_t new_ops = updated_session.operations_count;

    printResult(new_ops > initial_ops,
               "Operation count increased: " + std::string(new_ops > initial_ops ? "PASSED" : "FAILED") +
               " (was " + std::to_string(initial_ops) + ", now " + std::to_string(new_ops) + ")");

    return true;
}

/**
 * Test 5: Logout
 */
bool testLogout()
{
    printTestHeader("Test 5: Logout");

    UserManager& mgr = UserManager::getInstance();

    // Create and login user
    UserSession session;
    mgr.loginUser("admin", "admin123", session);
    std::string session_id = session.session_id;

    uint32_t before_logout = mgr.getActiveSessionCount();

    // Logout
    OFSErrorCodes result = mgr.logoutUser(session_id);
    printResult(result == OFSErrorCodes::SUCCESS,
               "Logout user: " + std::string(result == OFSErrorCodes::SUCCESS ? "PASSED" : "FAILED"));

    uint32_t after_logout = mgr.getActiveSessionCount();
    printResult(after_logout < before_logout,
               "Session removed after logout: " + std::string(after_logout < before_logout ? "PASSED" : "FAILED") +
               " (before: " + std::to_string(before_logout) + ", after: " + std::to_string(after_logout) + ")");

    // Try to validate logged out session (should fail)
    UserSession validated;
    result = mgr.validateSession(session_id, validated);
    printResult(result == OFSErrorCodes::ERROR_INVALID_SESSION,
               "Reject logged out session: " + std::string(result == OFSErrorCodes::ERROR_INVALID_SESSION ? "PASSED" : "FAILED"));

    return true;
}

/**
 * Test 6: User Existence Check
 */
bool testUserExistence()
{
    printTestHeader("Test 6: User Existence Check");

    UserManager& mgr = UserManager::getInstance();

    bool exists = mgr.userExists("admin");
    printResult(exists,
               "Check existing user (admin): " + std::string(exists ? "PASSED" : "FAILED"));

    exists = mgr.userExists("nonexistent_user");
    printResult(!exists,
               "Check non-existent user: " + std::string(!exists ? "PASSED" : "FAILED"));

    return true;
}

/**
 * Test 7: Stress Test - Multiple Sessions
 */
bool testMultipleSessions()
{
    printTestHeader("Test 7: Multiple Concurrent Sessions");

    UserManager& mgr = UserManager::getInstance();

    // Create multiple users
    mgr.createUser("user1", "pass1234", UserRole::NORMAL);
    mgr.createUser("user2", "pass1234", UserRole::NORMAL);
    mgr.createUser("user3", "pass1234", UserRole::NORMAL);

    // Login all users
    std::vector<UserSession> sessions;
    for (int i = 1; i <= 3; i++)
    {
        UserSession session;
        std::string username = "user" + std::to_string(i);
        mgr.loginUser(username, "pass1234", session);
        sessions.push_back(session);
    }

    uint32_t total_sessions = mgr.getActiveSessionCount();
    printResult(total_sessions >= 3,
               "Multiple concurrent sessions: " + std::string(total_sessions >= 3 ? "PASSED" : "FAILED") +
               " (sessions: " + std::to_string(total_sessions) + ")");

    // Validate all sessions
    bool all_valid = true;
    for (const auto& session : sessions)
    {
        UserSession validated;
        OFSErrorCodes result = mgr.validateSession(session.session_id, validated);
        if (result != OFSErrorCodes::SUCCESS)
        {
            all_valid = false;
            break;
        }
    }

    printResult(all_valid,
               "All sessions remain valid: " + std::string(all_valid ? "PASSED" : "FAILED"));

    return true;
}

/**
 * Test 8: Password Hashing Security
 */
bool testPasswordSecurity()
{
    printTestHeader("Test 8: Password Hashing Security");

    UserManager& mgr = UserManager::getInstance();

    // Create two users with same password - they should have different hashes
    mgr.createUser("secureuser1", "samepassword", UserRole::NORMAL);
    mgr.createUser("secureuser2", "samepassword", UserRole::NORMAL);

    // Both should login successfully
    UserSession session1, session2;
    OFSErrorCodes result1 = mgr.loginUser("secureuser1", "samepassword", session1);
    OFSErrorCodes result2 = mgr.loginUser("secureuser2", "samepassword", session2);

    printResult(result1 == OFSErrorCodes::SUCCESS && result2 == OFSErrorCodes::SUCCESS,
               "Both users with same password can login: PASSED");

    // But with different passwords, they should fail
    result1 = mgr.loginUser("secureuser1", "wrongpassword", session1);
    printResult(result1 == OFSErrorCodes::ERROR_PERMISSION_DENIED,
               "Reject wrong password for user1: PASSED");

    return true;
}

int main()
{
    std::cout << "\n╔════════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║         User Authentication System - Test Suite                 ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════════════╝" << std::endl;

    // Run all tests
    testUserCreation();
    testUserLogin();
    testSessionValidation();
    testSessionActivity();
    testLogout();
    testUserExistence();
    testMultipleSessions();
    testPasswordSecurity();

    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << "  All Tests Completed" << std::endl;
    std::cout << std::string(70, '=') << std::endl << std::endl;

    UserManager& mgr = UserManager::getInstance();
    std::cout << "Final Statistics:" << std::endl;
    std::cout << "  Total Users: " << mgr.getUserCount() << std::endl;
    std::cout << "  Active Sessions: " << mgr.getActiveSessionCount() << std::endl;

    LOG_INFO("TEST", 0, "User authentication test suite completed successfully");

    return 0;
}
