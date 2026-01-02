#include "user_manager.hpp"
#include "omni_storage.hpp"
#include "logger.hpp"
#include "crypto.hpp"
#include <map>
#include <mutex>

extern OmniStorage* g_storage;

static std::map<void*, std::string> sessions;
static std::map<void*, uint64_t> session_times;
static std::mutex sessions_mutex;
static uint64_t session_counter = 0;

int user_login(OFS_Session* session, const std::string& username, const std::string& password) {
    if (!g_storage) return static_cast<int>(OFSErrorCodes::ERROR_IO_ERROR);
    
    UserInfo user;
    if (!g_storage->get_user(username, &user)) {
        Logger::warn("Login failed: user not found", username);
        return static_cast<int>(OFSErrorCodes::ERROR_NOT_FOUND);
    }
    
    if (!Crypto::verify_password(password, user.password_hash)) {
        Logger::warn("Login failed: incorrect password", username);
        return static_cast<int>(OFSErrorCodes::ERROR_INVALID_OPERATION);
    }
    
    std::lock_guard<std::mutex> lock(sessions_mutex);
    void* session_id = reinterpret_cast<void*>(++session_counter);
    sessions[session_id] = username;
    session_times[session_id] = std::time(nullptr);
    *session = session_id;
    
    user.last_login = std::time(nullptr);
    g_storage->update_user(user);
    
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

int user_create(OFS_Session admin_session, const std::string& username, const std::string& password, UserRole role) {
    if (!g_storage) return static_cast<int>(OFSErrorCodes::ERROR_IO_ERROR);
    
    UserInfo existing;
    if (g_storage->get_user(username, &existing)) {
        return static_cast<int>(OFSErrorCodes::ERROR_FILE_EXISTS);
    }
    
    UserInfo new_user;
    std::strncpy(new_user.username, username.c_str(), sizeof(new_user.username) - 1);
    std::string password_hash = Crypto::hash_password(password);
    std::strncpy(new_user.password_hash, password_hash.c_str(), sizeof(new_user.password_hash) - 1);
    new_user.role = role;
    new_user.created_time = std::time(nullptr);
    new_user.last_login = 0;
    new_user.is_active = 1;
    
    if (g_storage->add_user(new_user)) {
        Logger::info("User created: " + username);
        return static_cast<int>(OFSErrorCodes::SUCCESS);
    }
    
    return static_cast<int>(OFSErrorCodes::ERROR_IO_ERROR);
}

int user_create(const std::string& username, const std::string& password) {
    return user_create(nullptr, username, password, UserRole::NORMAL);
}

int user_delete(OFS_Session admin_session, const std::string& username) {
    if (!g_storage) return static_cast<int>(OFSErrorCodes::ERROR_IO_ERROR);
    
    UserInfo user;
    if (!g_storage->get_user(username, &user)) {
        return static_cast<int>(OFSErrorCodes::ERROR_NOT_FOUND);
    }
    
    user.is_active = 0;
    g_storage->update_user(user);
    
    Logger::info("User deleted: " + username);
    return static_cast<int>(OFSErrorCodes::SUCCESS);
}

int user_list(OFS_Session admin_session, UserInfo** out_users, int* out_count) {
    if (!g_storage) return static_cast<int>(OFSErrorCodes::ERROR_IO_ERROR);
    
    std::vector<UserInfo> users = g_storage->list_users();
    *out_count = users.size();
    
    if (*out_count == 0) {
        *out_users = nullptr;
        return static_cast<int>(OFSErrorCodes::SUCCESS);
    }
    
    *out_users = new UserInfo[*out_count];
    for (int i = 0; i < *out_count; i++) {
        (*out_users)[i] = users[i];
    }
    
    return static_cast<int>(OFSErrorCodes::SUCCESS);
}

int get_session_info(OFS_Session session, SessionInfo* info) {
    std::lock_guard<std::mutex> lock(sessions_mutex);
    
    auto it = sessions.find(session);
    if (it == sessions.end()) {
        return static_cast<int>(OFSErrorCodes::ERROR_INVALID_SESSION);
    }
    
    if (!g_storage) return static_cast<int>(OFSErrorCodes::ERROR_IO_ERROR);
    
    UserInfo user;
    if (!g_storage->get_user(it->second, &user)) {
        return static_cast<int>(OFSErrorCodes::ERROR_NOT_FOUND);
    }
    
    info->user = user;
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
    
    if (!g_storage) return static_cast<int>(OFSErrorCodes::ERROR_IO_ERROR);
    
    UserInfo user;
    if (!g_storage->get_user(it->second, &user)) {
        return static_cast<int>(OFSErrorCodes::ERROR_NOT_FOUND);
    }
    
    out_username = it->second;
    out_role = user.role;
    
    return static_cast<int>(OFSErrorCodes::SUCCESS);
}

int save_users() {
    Logger::info("Users saved to disk");
    return static_cast<int>(OFSErrorCodes::SUCCESS);
}

int load_users() {
    if (!g_storage) return static_cast<int>(OFSErrorCodes::ERROR_IO_ERROR);
    
    std::vector<UserInfo> users = g_storage->list_users();
    
    if (users.empty()) {
        std::string password_hash = Crypto::hash_password("admin123");
        UserInfo admin;
        std::strncpy(admin.username, "admin", sizeof(admin.username) - 1);
        std::strncpy(admin.password_hash, password_hash.c_str(), sizeof(admin.password_hash) - 1);
        admin.role = UserRole::ADMIN;
        admin.created_time = std::time(nullptr);
        admin.last_login = 0;
        admin.is_active = 1;
        
        g_storage->add_user(admin);
        Logger::info("Default admin user created");
    }
    
    Logger::info("Users loaded from disk");
    return static_cast<int>(OFSErrorCodes::SUCCESS);
}