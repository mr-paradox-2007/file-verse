#include "user_manager.hpp"
#include "logger.hpp"
#include "crypto.hpp"
#include <map>
#include <mutex>

static std::map<std::string, UserInfo> users;
static std::map<void*, std::string> sessions;  // Session -> username mapping
static std::map<void*, uint64_t> session_times;
static std::mutex users_mutex;
static std::mutex sessions_mutex;
static uint64_t session_counter = 0;

int user_login(OFS_Session* session, const std::string& username, const std::string& password) {
    std::lock_guard<std::mutex> lock(users_mutex);
    
    if (users.find(username) == users.end()) {
        Logger::warn("Login failed: user not found", username);
        return static_cast<int>(OFSErrorCodes::ERROR_NOT_FOUND);
    }
    
    UserInfo& user = users[username];
    
    // Verify password
    if (!Crypto::verify_password(password, user.password_hash)) {
        Logger::warn("Login failed: incorrect password", username);
        return static_cast<int>(OFSErrorCodes::ERROR_INVALID_OPERATION);
    }
    
    // Create session
    {
        std::lock_guard<std::mutex> session_lock(sessions_mutex);
        void* session_id = reinterpret_cast<void*>(++session_counter);
        sessions[session_id] = username;
        session_times[session_id] = std::time(nullptr);
        *session = session_id;
    }
    
    // Update last login
    user.last_login = std::time(nullptr);
    
    Logger::info("User login successful", username);
    return static_cast<int>(OFSErrorCodes::SUCCESS);
}

int user_logout(OFS_Session session) {
    std::lock_guard<std::mutex> lock(sessions_mutex);
    
    auto it = sessions.find(session);
    if (it != sessions.end()) {
        std::string username = it->second;
        sessions.erase(it);
        session_times.erase(session);
        Logger::info("User logout", username);
        return static_cast<int>(OFSErrorCodes::SUCCESS);
    }
    
    return static_cast<int>(OFSErrorCodes::ERROR_INVALID_SESSION);
}

int user_create(OFS_Session admin_session, const std::string& username,
                const std::string& password, UserRole role) {
    
    // Verify admin session
    std::string admin_user;
    UserRole admin_role;
    if (verify_session(admin_session, admin_user, admin_role) != static_cast<int>(OFSErrorCodes::SUCCESS)) {
        Logger::warn("User creation failed: invalid session", admin_user);
        return static_cast<int>(OFSErrorCodes::ERROR_INVALID_SESSION);
    }
    
    if (admin_role != UserRole::ADMIN) {
        Logger::warn("User creation failed: permission denied", admin_user);
        return static_cast<int>(OFSErrorCodes::ERROR_PERMISSION_DENIED);
    }
    
    std::lock_guard<std::mutex> lock(users_mutex);
    
    if (users.find(username) != users.end()) {
        Logger::warn("User creation failed: user exists", admin_user);
        return static_cast<int>(OFSErrorCodes::ERROR_FILE_EXISTS);
    }
    
    // Create new user
    try {
        std::string password_hash = Crypto::hash_password(password);
        UserInfo new_user;
        std::strncpy(new_user.username, username.c_str(), sizeof(new_user.username) - 1);
        std::strncpy(new_user.password_hash, password_hash.c_str(), sizeof(new_user.password_hash) - 1);
        new_user.role = role;
        new_user.created_time = std::time(nullptr);
        new_user.last_login = 0;
        new_user.is_active = 1;
        
        users[username] = new_user;
        
        Logger::log_user_op("CREATE_USER", username, admin_user, true);
        return static_cast<int>(OFSErrorCodes::SUCCESS);
    } catch (const std::exception& e) {
        Logger::log_user_op("CREATE_USER", username, admin_user, false, e.what());
        return static_cast<int>(OFSErrorCodes::ERROR_IO_ERROR);
    }
}

int user_delete(OFS_Session admin_session, const std::string& username) {
    // Verify admin session
    std::string admin_user;
    UserRole admin_role;
    if (verify_session(admin_session, admin_user, admin_role) != static_cast<int>(OFSErrorCodes::SUCCESS)) {
        return static_cast<int>(OFSErrorCodes::ERROR_INVALID_SESSION);
    }
    
    if (admin_role != UserRole::ADMIN) {
        Logger::warn("User deletion failed: permission denied", admin_user);
        return static_cast<int>(OFSErrorCodes::ERROR_PERMISSION_DENIED);
    }
    
    std::lock_guard<std::mutex> lock(users_mutex);
    
    if (users.find(username) == users.end()) {
        Logger::warn("User deletion failed: user not found", admin_user);
        return static_cast<int>(OFSErrorCodes::ERROR_NOT_FOUND);
    }
    
    users[username].is_active = 0;
    Logger::log_user_op("DELETE_USER", username, admin_user, true);
    return static_cast<int>(OFSErrorCodes::SUCCESS);
}

int user_list(OFS_Session admin_session, UserInfo** out_users, int* out_count) {
    // Verify admin session
    std::string admin_user;
    UserRole admin_role;
    if (verify_session(admin_session, admin_user, admin_role) != static_cast<int>(OFSErrorCodes::SUCCESS)) {
        return static_cast<int>(OFSErrorCodes::ERROR_INVALID_SESSION);
    }
    
    if (admin_role != UserRole::ADMIN) {
        return static_cast<int>(OFSErrorCodes::ERROR_PERMISSION_DENIED);
    }
    
    std::lock_guard<std::mutex> lock(users_mutex);
    
    *out_count = 0;
    for (const auto& u : users) {
        if (u.second.is_active) {
            (*out_count)++;
        }
    }
    
    if (*out_count == 0) {
        *out_users = nullptr;
        return static_cast<int>(OFSErrorCodes::SUCCESS);
    }
    
    *out_users = new UserInfo[*out_count];
    int index = 0;
    for (const auto& u : users) {
        if (u.second.is_active) {
            (*out_users)[index++] = u.second;
        }
    }
    
    return static_cast<int>(OFSErrorCodes::SUCCESS);
}

int get_session_info(OFS_Session session, SessionInfo* info) {
    std::lock_guard<std::mutex> lock(sessions_mutex);
    
    auto it = sessions.find(session);
    if (it == sessions.end()) {
        return static_cast<int>(OFSErrorCodes::ERROR_INVALID_SESSION);
    }
    
    std::string username = it->second;
    
    std::lock_guard<std::mutex> users_lock(users_mutex);
    if (users.find(username) == users.end()) {
        return static_cast<int>(OFSErrorCodes::ERROR_NOT_FOUND);
    }
    
    info->user = users[username];
    info->login_time = session_times[session];
    info->last_activity = std::time(nullptr);
    
    return static_cast<int>(OFSErrorCodes::SUCCESS);
}

int verify_session(OFS_Session session, std::string& out_username, UserRole& out_role) {
    std::lock_guard<std::mutex> lock(sessions_mutex);
    
    auto it = sessions.find(session);
    if (it == sessions.end()) {
        return static_cast<int>(OFSErrorCodes::ERROR_INVALID_SESSION);
    }
    
    out_username = it->second;
    
    std::lock_guard<std::mutex> users_lock(users_mutex);
    if (users.find(out_username) == users.end()) {
        return static_cast<int>(OFSErrorCodes::ERROR_NOT_FOUND);
    }
    
    out_role = users[out_username].role;
    return static_cast<int>(OFSErrorCodes::SUCCESS);
}

int save_users() {
    // TODO: Implement persistence
    Logger::info("Users saved to disk");
    return static_cast<int>(OFSErrorCodes::SUCCESS);
}

int load_users() {
    // TODO: Load from disk and initialize admin user if needed
    Logger::info("Users loaded from disk");
    
    // Create default admin user if no users exist
    {
        std::lock_guard<std::mutex> lock(users_mutex);
        if (users.empty()) {
            try {
                std::string password_hash = Crypto::hash_password("admin123");
                UserInfo admin;
                std::strncpy(admin.username, "admin", sizeof(admin.username) - 1);
                std::strncpy(admin.password_hash, password_hash.c_str(), sizeof(admin.password_hash) - 1);
                admin.role = UserRole::ADMIN;
                admin.created_time = std::time(nullptr);
                admin.last_login = 0;
                admin.is_active = 1;
                
                users["admin"] = admin;
                Logger::info("Default admin user created");
            } catch (const std::exception& e) {
                Logger::error("Failed to create admin user: " + std::string(e.what()));
            }
        }
    }
    
    return static_cast<int>(OFSErrorCodes::SUCCESS);
}
