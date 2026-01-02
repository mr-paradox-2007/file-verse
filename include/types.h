#ifndef TYPES_H
#define TYPES_H

#include <string>
#include <cstring>

// Error codes
enum class Result {
    SUCCESS = 0,
    ERROR_NOT_FOUND = -1,
    ERROR_PERMISSION = -2,
    ERROR_FILE_EXISTS = -3,
    ERROR_INVALID = -4,
};

// User role
enum class Role {
    NORMAL = 0,
    ADMIN = 1
};

// User structure - stored in binary file
struct User {
    char username[32];           // Username
    char password_hash[128];     // Hashed password
    uint32_t role;               // 0=NORMAL, 1=ADMIN
    uint64_t created_at;         // Creation timestamp
    
    User() {
        memset(username, 0, sizeof(username));
        memset(password_hash, 0, sizeof(password_hash));
        role = 0;
        created_at = 0;
    }
};

#endif
