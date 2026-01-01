#ifndef USER_MANAGER_HPP
#define USER_MANAGER_HPP

#include "ofs_types.hpp"
#include <string>
#include <unordered_map>
#include <chrono>
#include <random>
#include <memory>
#include <mutex>

namespace ofs
{

/**
 * User Session information for authenticated users
 * Note: Different from SessionInfo in ofs_types.hpp which is for get_session_info()
 */
struct UserSession
{
    std::string session_id;              // Unique session identifier (64 bytes hex)
    UserRole user_role;                  // Role of authenticated user
    std::string username;                // Username of authenticated user
    uint64_t login_time;                 // Unix timestamp of login
    uint64_t last_activity;              // Unix timestamp of last activity
    uint64_t expiration_time;            // Unix timestamp when session expires
    uint32_t operations_count;           // Number of operations performed
    bool is_valid;                       // Whether session is still valid

    UserSession() 
        : user_role(UserRole::NORMAL), login_time(0), last_activity(0), 
          expiration_time(0), operations_count(0), is_valid(false)
    {
    }

    /**
     * Check if session is still valid (not expired)
     */
    bool isExpired(uint64_t current_time) const
    {
        return !is_valid || current_time > expiration_time;
    }

    /**
     * Update last activity timestamp
     */
    void updateActivity(uint64_t current_time)
    {
        last_activity = current_time;
        operations_count++;
    }
};

/**
 * User Authentication and Session Manager
 * Handles user creation, login, and session tracking
 */
class UserManager
{
public:
    /**
     * Get singleton instance
     */
    static UserManager& getInstance();

    // Delete copy constructor and assignment operator
    UserManager(const UserManager&) = delete;
    UserManager& operator=(const UserManager&) = delete;

    /**
     * Create a new user with hashed password
     * 
     * @param username Username (max 31 characters)
     * @param password Plain text password (will be hashed)
     * @param role User role (ADMIN or NORMAL)
     * @return OFSErrorCodes::SUCCESS if user created successfully
     * @return OFSErrorCodes::ERROR_FILE_EXISTS if user already exists
     * @return OFSErrorCodes::ERROR_INVALID_OPERATION if username is invalid
     */
    OFSErrorCodes createUser(const std::string& username, 
                            const std::string& password,
                            UserRole role = UserRole::NORMAL);

    /**
     * Authenticate user and create session
     * 
     * @param username Username to authenticate
     * @param password Plain text password to verify
     * @param session_duration_seconds How long session lasts (default: 3600 = 1 hour)
     * @param out_session Output parameter for created session
     * @return OFSErrorCodes::SUCCESS if login successful
     * @return OFSErrorCodes::ERROR_NOT_FOUND if user doesn't exist
     * @return OFSErrorCodes::ERROR_PERMISSION_DENIED if password incorrect
     */
    OFSErrorCodes loginUser(const std::string& username,
                           const std::string& password,
                           UserSession& out_session,
                           uint64_t session_duration_seconds = 3600);

    /**
     * Logout user and invalidate session
     * 
     * @param session_id Session ID to invalidate
     * @return OFSErrorCodes::SUCCESS if logout successful
     * @return OFSErrorCodes::ERROR_INVALID_SESSION if session not found
     */
    OFSErrorCodes logoutUser(const std::string& session_id);

    /**
     * Validate session and get user information
     * 
     * @param session_id Session ID to validate
     * @param out_session Output parameter for session info
     * @return OFSErrorCodes::SUCCESS if session is valid and active
     * @return OFSErrorCodes::ERROR_INVALID_SESSION if session expired or invalid
     */
    OFSErrorCodes validateSession(const std::string& session_id,
                                 UserSession& out_session);

    /**
     * Update session activity timestamp
     * 
     * @param session_id Session ID to update
     * @return OFSErrorCodes::SUCCESS if updated
     * @return OFSErrorCodes::ERROR_INVALID_SESSION if session not found
     */
    OFSErrorCodes updateSessionActivity(const std::string& session_id);

    /**
     * Check if user exists
     * 
     * @param username Username to check
     * @return true if user exists, false otherwise
     */
    bool userExists(const std::string& username) const;

    /**
     * Get session count (for statistics)
     */
    uint32_t getActiveSessionCount() const;

    /**
     * Get user count (for statistics)
     */
    uint32_t getUserCount() const;

private:
    UserManager() = default;

    // In-memory user database
    std::unordered_map<std::string, UserInfo> users_;      // username → UserInfo
    std::unordered_map<std::string, UserSession> sessions_; // session_id → UserSession
    mutable std::mutex users_mutex_;
    mutable std::mutex sessions_mutex_;

    /**
     * Generate random salt for password hashing
     * @return 16-byte salt as hex string
     */
    std::string generateSalt();

    /**
     * Hash password with salt using SHA-256
     * 
     * @param password Plain text password
     * @param salt Salt string (hex format)
     * @return SHA-256 hash as hex string (64 characters)
     */
    std::string hashPassword(const std::string& password, const std::string& salt);

    /**
     * Generate unique session ID
     * @return 64-character hex string session ID
     */
    std::string generateSessionId();

    /**
     * Get current Unix timestamp
     */
    uint64_t getCurrentTimestamp() const;

    /**
     * Verify password against stored hash
     * 
     * @param password Plain text password
     * @param stored_hash Hash from storage (includes salt prefix)
     * @return true if password matches
     */
    bool verifyPassword(const std::string& password, const std::string& stored_hash);
};

}  // namespace ofs

#endif  // USER_MANAGER_HPP
