#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <sys/stat.h>
#include "omni_storage.hpp"
#include "user_manager.hpp"
#include "crypto.hpp"
#include "logger.hpp"

OmniStorage* g_storage = nullptr;

void print_usage() {
    std::cout << "\n╔════════════════════════════════════════╗\n";
    std::cout << "║     OFS Account Management CLI         ║\n";
    std::cout << "╚════════════════════════════════════════╝\n\n";
    std::cout << "Usage: ./compiled/admin_cli <command> [options]\n\n";
    std::cout << "Commands:\n";
    std::cout << "  create <username> <password>     Create new user account\n";
    std::cout << "  delete <username>                Delete user account\n";
    std::cout << "  list                             List all user accounts\n";
    std::cout << "  list-active                      List currently logged-in users\n";
    std::cout << "  enable <username>                Enable user account\n";
    std::cout << "  disable <username>               Disable user account\n";
    std::cout << "  change-pwd <username> <password> Change user password\n";
    std::cout << "  info <username>                  Show user information\n";
    std::cout << "  reset-admin                      Reset admin password to admin123\n";
    std::cout << "\nExamples:\n";
    std::cout << "  ./compiled/admin_cli create alice password123\n";
    std::cout << "  ./compiled/admin_cli create bob securepass --admin\n";
    std::cout << "  ./compiled/admin_cli list\n";
    std::cout << "  ./compiled/admin_cli delete alice\n";
    std::cout << "\n";
}

void print_user_info(const UserInfo& user) {
    std::string role_str = (user.role == UserRole::ADMIN) ? "ADMIN" : "USER";
    std::string status = user.is_active ? "ACTIVE" : "DISABLED";
    
    std::cout << "  Username:    " << user.username << "\n";
    std::cout << "  Role:        " << role_str << "\n";
    std::cout << "  Status:      " << status << "\n";
    std::cout << "  Created:     " << user.created_time << "\n";
    std::cout << "  Last Login:  " << (user.last_login > 0 ? std::to_string(user.last_login) : "Never") << "\n";
}

int cmd_create(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Error: Missing arguments\n";
        std::cerr << "Usage: admin_cli create <username> <password> [--admin]\n";
        return 1;
    }
    
    std::string username = argv[2];
    std::string password = argv[3];
    bool is_admin = (argc > 4 && std::string(argv[4]) == "--admin");
    
    if (username.length() < 3) {
        std::cerr << "Error: Username must be at least 3 characters\n";
        return 1;
    }
    
    if (password.length() < 4) {
        std::cerr << "Error: Password must be at least 4 characters\n";
        return 1;
    }
    
    UserInfo existing;
    if (g_storage->get_user(username, &existing)) {
        std::cerr << "Error: User '" << username << "' already exists\n";
        return 1;
    }
    
    UserInfo new_user;
    std::strncpy(new_user.username, username.c_str(), sizeof(new_user.username) - 1);
    std::string password_hash = Crypto::hash_password(password);
    std::strncpy(new_user.password_hash, password_hash.c_str(), sizeof(new_user.password_hash) - 1);
    new_user.role = is_admin ? UserRole::ADMIN : UserRole::NORMAL;
    new_user.created_time = std::time(nullptr);
    new_user.last_login = 0;
    new_user.is_active = 1;
    
    if (g_storage->add_user(new_user)) {
        std::cout << "✓ User '" << username << "' created successfully\n";
        print_user_info(new_user);
        return 0;
    }
    
    std::cerr << "Error: Failed to create user\n";
    return 1;
}

int cmd_delete(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Error: Missing username\n";
        std::cerr << "Usage: admin_cli delete <username>\n";
        return 1;
    }
    
    std::string username = argv[2];
    
    if (username == "admin") {
        std::cerr << "Error: Cannot delete admin account\n";
        return 1;
    }
    
    UserInfo user;
    if (!g_storage->get_user(username, &user)) {
        std::cerr << "Error: User '" << username << "' not found\n";
        return 1;
    }
    
    user.is_active = 0;
    g_storage->update_user(user);
    
    std::cout << "✓ User '" << username << "' deleted (disabled)\n";
    return 0;
}

int cmd_list(int argc, char* argv[]) {
    std::vector<UserInfo> users = g_storage->list_users();
    
    if (users.empty()) {
        std::cout << "No users found\n";
        return 0;
    }
    
    std::cout << "\n╔════════════════════════════════════════╗\n";
    std::cout << "║         Registered Users               ║\n";
    std::cout << "╚════════════════════════════════════════╝\n\n";
    
    int count = 0;
    for (const auto& user : users) {
        if (user.is_active) {
            count++;
            std::string role = (user.role == UserRole::ADMIN) ? "[ADMIN]" : "[USER]";
            std::cout << count << ". " << user.username << " " << role << "\n";
        }
    }
    
    std::cout << "\nTotal active users: " << count << "\n\n";
    return 0;
}

int cmd_list_active(int argc, char* argv[]) {
    std::cout << "\n╔════════════════════════════════════════╗\n";
    std::cout << "║     Currently Logged-in Users         ║\n";
    std::cout << "╚════════════════════════════════════════╝\n\n";
    
    std::cout << "Active session tracking requires server integration.\n";
    std::cout << "Check server logs for login/logout information.\n\n";
    return 0;
}

int cmd_enable(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Error: Missing username\n";
        return 1;
    }
    
    std::string username = argv[2];
    UserInfo user;
    
    if (!g_storage->get_user(username, &user)) {
        std::cerr << "Error: User '" << username << "' not found\n";
        return 1;
    }
    
    if (user.is_active) {
        std::cout << "User '" << username << "' is already enabled\n";
        return 0;
    }
    
    user.is_active = 1;
    g_storage->update_user(user);
    
    std::cout << "✓ User '" << username << "' enabled\n";
    return 0;
}

int cmd_disable(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Error: Missing username\n";
        return 1;
    }
    
    std::string username = argv[2];
    
    if (username == "admin") {
        std::cerr << "Error: Cannot disable admin account\n";
        return 1;
    }
    
    UserInfo user;
    if (!g_storage->get_user(username, &user)) {
        std::cerr << "Error: User '" << username << "' not found\n";
        return 1;
    }
    
    if (!user.is_active) {
        std::cout << "User '" << username << "' is already disabled\n";
        return 0;
    }
    
    user.is_active = 0;
    g_storage->update_user(user);
    
    std::cout << "✓ User '" << username << "' disabled\n";
    return 0;
}

int cmd_change_pwd(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "Error: Missing arguments\n";
        std::cerr << "Usage: admin_cli change-pwd <username> <new_password>\n";
        return 1;
    }
    
    std::string username = argv[2];
    std::string new_password = argv[3];
    
    if (new_password.length() < 4) {
        std::cerr << "Error: Password must be at least 4 characters\n";
        return 1;
    }
    
    UserInfo user;
    if (!g_storage->get_user(username, &user)) {
        std::cerr << "Error: User '" << username << "' not found\n";
        return 1;
    }
    
    std::string password_hash = Crypto::hash_password(new_password);
    std::strncpy(user.password_hash, password_hash.c_str(), sizeof(user.password_hash) - 1);
    g_storage->update_user(user);
    
    std::cout << "✓ Password changed for user '" << username << "'\n";
    return 0;
}

int cmd_info(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Error: Missing username\n";
        return 1;
    }
    
    std::string username = argv[2];
    UserInfo user;
    
    if (!g_storage->get_user(username, &user)) {
        std::cerr << "Error: User '" << username << "' not found\n";
        return 1;
    }
    
    std::cout << "\n╔════════════════════════════════════════╗\n";
    std::cout << "║         User Information               ║\n";
    std::cout << "╚════════════════════════════════════════╝\n\n";
    print_user_info(user);
    std::cout << "\n";
    return 0;
}

int cmd_reset_admin(int argc, char* argv[]) {
    UserInfo admin;
    if (!g_storage->get_user("admin", &admin)) {
        std::string password_hash = Crypto::hash_password("admin123");
        UserInfo new_admin;
        std::strncpy(new_admin.username, "admin", sizeof(new_admin.username) - 1);
        std::strncpy(new_admin.password_hash, password_hash.c_str(), sizeof(new_admin.password_hash) - 1);
        new_admin.role = UserRole::ADMIN;
        new_admin.created_time = std::time(nullptr);
        new_admin.last_login = 0;
        new_admin.is_active = 1;
        g_storage->add_user(new_admin);
        std::cout << "✓ Admin account created with password: admin123\n";
    } else {
        std::string password_hash = Crypto::hash_password("admin123");
        std::strncpy(admin.password_hash, password_hash.c_str(), sizeof(admin.password_hash) - 1);
        admin.is_active = 1;
        g_storage->update_user(admin);
        std::cout << "✓ Admin password reset to: admin123\n";
    }
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage();
        return 1;
    }
    
    g_storage = new OmniStorage();
    
    struct stat st;
    if (stat("data/system.omni", &st) != 0) {
        std::cout << "Initializing OFS storage..." << std::endl;
        if (!g_storage->create("data/system.omni", 104857600)) {
            std::cerr << "Error: Failed to initialize storage\n";
            delete g_storage;
            return 1;
        }
    } else {
        if (!g_storage->open("data/system.omni")) {
            std::cerr << "Error: Failed to open storage\n";
            delete g_storage;
            return 1;
        }
    }
    
    std::string command = argv[1];
    int result = 1;
    
    if (command == "create") {
        result = cmd_create(argc, argv);
    } else if (command == "delete") {
        result = cmd_delete(argc, argv);
    } else if (command == "list") {
        result = cmd_list(argc, argv);
    } else if (command == "list-active") {
        result = cmd_list_active(argc, argv);
    } else if (command == "enable") {
        result = cmd_enable(argc, argv);
    } else if (command == "disable") {
        result = cmd_disable(argc, argv);
    } else if (command == "change-pwd") {
        result = cmd_change_pwd(argc, argv);
    } else if (command == "info") {
        result = cmd_info(argc, argv);
    } else if (command == "reset-admin") {
        result = cmd_reset_admin(argc, argv);
    } else {
        std::cerr << "Error: Unknown command '" << command << "'\n";
        print_usage();
    }
    
    g_storage->close();
    delete g_storage;
    return result;
}
