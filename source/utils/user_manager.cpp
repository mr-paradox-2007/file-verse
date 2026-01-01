#include "user_manager.hpp"
#include "logger.hpp"
#include <openssl/sha.h>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <cstring>

namespace ofs
{

// ============================================================================
// SINGLETON INSTANCE
// ============================================================================

UserManager& UserManager::getInstance()
{
    static UserManager instance;
    return instance;
}

// ============================================================================
// PUBLIC API
// ============================================================================

OFSErrorCodes UserManager::createUser(const std::string& username,
                                     const std::string& password,
                                     UserRole role)
{
    std::lock_guard<std::mutex> lock(users_mutex_);

    LOG_INFO("USER_MGR", 0, "Creating new user: " + username);

    // Validate username
    if (username.empty() || username.length() > 31)
    {
        LOG_ERROR("USER_MGR", 401,
                 "Invalid username: must be 1-31 characters");
        return OFSErrorCodes::ERROR_INVALID_OPERATION;
    }

    // Check if user already exists
    if (users_.find(username) != users_.end())
    {
        LOG_WARN("USER_MGR", 402,
                "User already exists: " + username);
        return OFSErrorCodes::ERROR_FILE_EXISTS;
    }

    // Validate password
    if (password.empty() || password.length() < 4)
    {
        LOG_ERROR("USER_MGR", 403,
                 "Invalid password: must be at least 4 characters");
        return OFSErrorCodes::ERROR_INVALID_OPERATION;
    }

    // Generate salt
    std::string salt = generateSalt();
    LOG_DEBUG("USER_MGR", 0, "Generated salt for user: " + username);

    // Hash password with salt
    std::string password_hash = hashPassword(password, salt);

    // Create UserInfo
    UserInfo user;
    std::strncpy(user.username, username.c_str(), sizeof(user.username) - 1);
    user.username[sizeof(user.username) - 1] = '\0';

    // Store hash with salt prefix (salt:hash format)
    std::string full_hash = salt + ":" + password_hash;
    std::strncpy(user.password_hash, full_hash.c_str(), sizeof(user.password_hash) - 1);
    user.password_hash[sizeof(user.password_hash) - 1] = '\0';

    user.role = role;
    user.created_time = getCurrentTimestamp();
    user.last_login = 0;
    user.is_active = 1;

    // Store in database
    users_[username] = user;

    LOG_INFO("USER_MGR", 0,
            "User created successfully: " + username +
            " (role=" + (role == UserRole::ADMIN ? "ADMIN" : "NORMAL") + ")");

    return OFSErrorCodes::SUCCESS;
}

OFSErrorCodes UserManager::loginUser(const std::string& username,
                                    const std::string& password,
                                    UserSession& out_session,
                                    uint64_t session_duration_seconds)
{
    std::lock_guard<std::mutex> lock(users_mutex_);

    LOG_INFO("USER_MGR", 0, "Login attempt for user: " + username);

    // Check if user exists
    auto user_it = users_.find(username);
    if (user_it == users_.end())
    {
        LOG_WARN("USER_MGR", 404,
                "Login failed: user not found: " + username);
        return OFSErrorCodes::ERROR_NOT_FOUND;
    }

    UserInfo& user = user_it->second;

    // Check if user is active
    if (user.is_active == 0)
    {
        LOG_WARN("USER_MGR", 405,
                "Login failed: user account disabled: " + username);
        return OFSErrorCodes::ERROR_PERMISSION_DENIED;
    }

    // Verify password
    if (!verifyPassword(password, std::string(user.password_hash)))
    {
        LOG_WARN("USER_MGR", 406,
                "Login failed: incorrect password for user: " + username);
        return OFSErrorCodes::ERROR_PERMISSION_DENIED;
    }

    // Generate session
    std::string session_id = generateSessionId();
    uint64_t now = getCurrentTimestamp();

    out_session.session_id = session_id;
    out_session.username = username;
    out_session.user_role = user.role;
    out_session.login_time = now;
    out_session.last_activity = now;
    out_session.expiration_time = now + session_duration_seconds;
    out_session.operations_count = 0;
    out_session.is_valid = true;

    // Store session
    {
        std::lock_guard<std::mutex> session_lock(sessions_mutex_);
        sessions_[session_id] = out_session;
    }

    // Update last login timestamp
    user.last_login = now;

    LOG_INFO("USER_MGR", 0,
            "User logged in successfully: " + username +
            " (session=" + session_id.substr(0, 16) + "...)");

    return OFSErrorCodes::SUCCESS;
}

OFSErrorCodes UserManager::logoutUser(const std::string& session_id)
{
    std::lock_guard<std::mutex> lock(sessions_mutex_);

    LOG_INFO("USER_MGR", 0, "Logout for session: " + session_id.substr(0, 16) + "...");

    auto session_it = sessions_.find(session_id);
    if (session_it == sessions_.end())
    {
        LOG_WARN("USER_MGR", 407,
                "Logout failed: session not found");
        return OFSErrorCodes::ERROR_INVALID_SESSION;
    }

    session_it->second.is_valid = false;
    sessions_.erase(session_it);

    LOG_INFO("USER_MGR", 0, "User logged out: " + session_id.substr(0, 16) + "...");

    return OFSErrorCodes::SUCCESS;
}

OFSErrorCodes UserManager::validateSession(const std::string& session_id,
                                          UserSession& out_session)
{
    std::lock_guard<std::mutex> lock(sessions_mutex_);

    auto session_it = sessions_.find(session_id);
    if (session_it == sessions_.end())
    {
        LOG_WARN("USER_MGR", 408,
                "Session validation failed: session not found");
        return OFSErrorCodes::ERROR_INVALID_SESSION;
    }

    UserSession& session = session_it->second;
    uint64_t now = getCurrentTimestamp();

    // Check if session is expired
    if (session.isExpired(now))
    {
        LOG_WARN("USER_MGR", 409,
                "Session validation failed: session expired");
        session.is_valid = false;
        sessions_.erase(session_it);
        return OFSErrorCodes::ERROR_INVALID_SESSION;
    }

    out_session = session;
    return OFSErrorCodes::SUCCESS;
}

OFSErrorCodes UserManager::updateSessionActivity(const std::string& session_id)
{
    std::lock_guard<std::mutex> lock(sessions_mutex_);

    auto session_it = sessions_.find(session_id);
    if (session_it == sessions_.end())
    {
        return OFSErrorCodes::ERROR_INVALID_SESSION;
    }

    uint64_t now = getCurrentTimestamp();
    session_it->second.updateActivity(now);

    return OFSErrorCodes::SUCCESS;
}

bool UserManager::userExists(const std::string& username) const
{
    std::lock_guard<std::mutex> lock(users_mutex_);
    return users_.find(username) != users_.end();
}

uint32_t UserManager::getActiveSessionCount() const
{
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    return static_cast<uint32_t>(sessions_.size());
}

uint32_t UserManager::getUserCount() const
{
    std::lock_guard<std::mutex> lock(users_mutex_);
    return static_cast<uint32_t>(users_.size());
}

// ============================================================================
// PRIVATE IMPLEMENTATION
// ============================================================================

std::string UserManager::generateSalt()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);

    std::stringstream ss;
    for (int i = 0; i < 16; i++)
    {
        ss << std::hex << std::setfill('0') << std::setw(2) << dis(gen);
    }

    return ss.str();
}

std::string UserManager::hashPassword(const std::string& password,
                                     const std::string& salt)
{
    // Combine password and salt
    std::string combined = password + salt;

    // Compute SHA-256
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<unsigned char*>(const_cast<char*>(combined.c_str())),
           combined.length(),
           hash);

    // Convert to hex string
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
    {
        ss << std::hex << std::setfill('0') << std::setw(2) << (int)hash[i];
    }

    return ss.str();
}

std::string UserManager::generateSessionId()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);

    std::stringstream ss;
    for (int i = 0; i < 32; i++)
    {
        ss << std::hex << std::setfill('0') << std::setw(2) << dis(gen);
    }

    return ss.str();
}

uint64_t UserManager::getCurrentTimestamp() const
{
    return static_cast<uint64_t>(std::time(nullptr));
}

bool UserManager::verifyPassword(const std::string& password,
                                const std::string& stored_hash)
{
    // Extract salt and hash from stored format: "salt:hash"
    size_t colon_pos = stored_hash.find(':');
    if (colon_pos == std::string::npos || colon_pos == 0)
    {
        LOG_ERROR("USER_MGR", 410,
                 "Invalid stored hash format");
        return false;
    }

    std::string salt = stored_hash.substr(0, colon_pos);
    std::string expected_hash = stored_hash.substr(colon_pos + 1);

    // Hash the provided password with the extracted salt
    std::string computed_hash = hashPassword(password, salt);

    // Compare hashes
    return computed_hash == expected_hash;
}

}  // namespace ofs
