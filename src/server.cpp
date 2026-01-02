#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <fstream>
#include <sstream>
#include <map>
#include <vector>
#include <ctime>
#include <iomanip>
#include <openssl/sha.h>

#include "types.h"

// Global user database
std::map<std::string, User> users;
pthread_mutex_t users_mutex = PTHREAD_MUTEX_INITIALIZER;

const char* USERS_FILE = "data/users.bin";
const int PORT = 9000;

// Helper: SHA256 hash
std::string sha256(const std::string& str) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char*)str.c_str(), str.length(), hash);
    
    std::stringstream ss;
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

// Load users from file
void load_users() {
    pthread_mutex_lock(&users_mutex);
    
    std::ifstream file(USERS_FILE, std::ios::binary);
    if (!file.good()) {
        std::cout << "[LOAD] No users file, starting fresh\n";
        pthread_mutex_unlock(&users_mutex);
        return;
    }
    
    User u;
    while (file.read((char*)&u, sizeof(User))) {
        users[u.username] = u;
        std::cout << "[LOAD] Loaded user: " << u.username << "\n";
    }
    file.close();
    
    std::cout << "[LOAD] Total users: " << users.size() << "\n";
    pthread_mutex_unlock(&users_mutex);
}

// Save users to file
void save_users() {
    pthread_mutex_lock(&users_mutex);
    
    std::ofstream file(USERS_FILE, std::ios::binary | std::ios::trunc);
    for (auto& pair : users) {
        file.write((char*)&pair.second, sizeof(User));
    }
    file.close();
    
    std::cout << "[SAVE] Saved " << users.size() << " users\n";
    pthread_mutex_unlock(&users_mutex);
}

// Helper: Extract JSON field
std::string extract_json_field(const std::string& json, const std::string& field) {
    std::string search = "\"" + field + "\":\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return "";
    
    pos += search.length();
    size_t end = json.find("\"", pos);
    if (end == std::string::npos) return "";
    
    return json.substr(pos, end - pos);
}

// Helper: Create JSON response
std::string json_response(const std::string& status, const std::string& msg, const std::string& extra = "") {
    std::string resp = "{\"status\":\"" + status + "\",\"message\":\"" + msg + "\"";
    if (!extra.empty()) resp += "," + extra;
    resp += "}";
    return resp;
}

// Handle HTTP requests
std::string handle_request(const std::string& body, const std::string& path) {
    std::cout << "[REQ] " << path << "\n";
    
    // LOGIN
    if (path == "/user/login") {
        std::string username = extract_json_field(body, "username");
        std::string password = extract_json_field(body, "password");
        
        if (username.empty() || password.empty()) {
            return json_response("error", "Missing username or password");
        }
        
        pthread_mutex_lock(&users_mutex);
        if (users.find(username) == users.end()) {
            pthread_mutex_unlock(&users_mutex);
            return json_response("error", "User not found");
        }
        
        User& user = users[username];
        std::string hash = sha256(password);
        
        if (hash != user.password_hash) {
            pthread_mutex_unlock(&users_mutex);
            return json_response("error", "Invalid password");
        }
        
        pthread_mutex_unlock(&users_mutex);
        std::cout << "[LOGIN] User: " << username << "\n";
        return json_response("success", "Login OK", "\"username\":\"" + username + "\",\"role\":" + std::to_string(user.role));
    }
    
    // SIGNUP
    else if (path == "/user/signup") {
        std::string username = extract_json_field(body, "username");
        std::string password = extract_json_field(body, "password");
        
        if (username.empty() || password.empty()) {
            return json_response("error", "Missing username or password");
        }
        
        if (username.length() < 3 || username.length() > 31) {
            return json_response("error", "Username must be 3-31 chars");
        }
        
        if (password.length() < 4) {
            return json_response("error", "Password must be 4+ chars");
        }
        
        pthread_mutex_lock(&users_mutex);
        
        if (users.find(username) != users.end()) {
            pthread_mutex_unlock(&users_mutex);
            return json_response("error", "User already exists");
        }
        
        User u;
        strncpy(u.username, username.c_str(), sizeof(u.username) - 1);
        strncpy(u.password_hash, sha256(password).c_str(), sizeof(u.password_hash) - 1);
        u.role = 0;  // NORMAL
        u.created_at = time(nullptr);
        
        users[username] = u;
        pthread_mutex_unlock(&users_mutex);
        
        save_users();
        
        std::cout << "[SIGNUP] New user: " << username << "\n";
        return json_response("success", "User created", "\"username\":\"" + username + "\"");
    }
    
    // USERS LIST (for debugging)
    else if (path == "/debug/users") {
        pthread_mutex_lock(&users_mutex);
        std::string list = "\"users\":[";
        bool first = true;
        for (auto& pair : users) {
            if (!first) list += ",";
            list += "{\"username\":\"" + std::string(pair.second.username) + "\",\"role\":" + std::to_string(pair.second.role) + "}";
            first = false;
        }
        list += "]";
        pthread_mutex_unlock(&users_mutex);
        
        return json_response("success", "User count: " + std::to_string(users.size()), list);
    }
    
    return json_response("error", "Unknown endpoint");
}

// Handle client connection
void* handle_client(void* arg) {
    int client_fd = *(int*)arg;
    delete (int*)arg;
    
    char buffer[4096];
    memset(buffer, 0, sizeof(buffer));
    
    int bytes = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    if (bytes > 0) {
        buffer[bytes] = '\0';
        
        // Parse HTTP request
        std::string request(buffer);
        std::string path, body;
        
        // Extract path from request line
        size_t path_start = request.find(' ') + 1;
        size_t path_end = request.find(' ', path_start);
        path = request.substr(path_start, path_end - path_start);
        
        // Extract body (after headers)
        size_t body_start = request.find("\r\n\r\n");
        if (body_start != std::string::npos) {
            body = request.substr(body_start + 4);
        }
        
        std::cout << "[RECV] " << path << " (" << body.length() << " bytes)\n";
        
        // Handle request
        std::string response_body = handle_request(body, path);
        
        // Send HTTP response
        std::string response = "HTTP/1.1 200 OK\r\n";
        response += "Content-Type: application/json\r\n";
        response += "Access-Control-Allow-Origin: *\r\n";
        response += "Access-Control-Allow-Methods: POST, GET, OPTIONS\r\n";
        response += "Access-Control-Allow-Headers: Content-Type\r\n";
        response += "Content-Length: " + std::to_string(response_body.length()) + "\r\n";
        response += "Connection: close\r\n";
        response += "\r\n";
        response += response_body;
        
        send(client_fd, response.c_str(), response.length(), 0);
        std::cout << "[SEND] " << response_body.substr(0, 50) << "...\n";
    }
    
    close(client_fd);
    return nullptr;
}

int main() {
    std::cout << "\n=== OFS Server v2 (Fresh Start) ===\n";
    std::cout << "Port: " << PORT << "\n";
    
    // Load existing users
    load_users();
    
    // Create default admin if no users
    if (users.empty()) {
        std::cout << "[INIT] Creating admin user\n";
        User admin;
        strncpy(admin.username, "admin", sizeof(admin.username) - 1);
        strncpy(admin.password_hash, sha256("admin123").c_str(), sizeof(admin.password_hash) - 1);
        admin.role = 1;  // ADMIN
        admin.created_at = time(nullptr);
        
        users["admin"] = admin;
        save_users();
        std::cout << "[INIT] Admin created: admin / admin123\n";
    }
    
    // Create socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "Socket failed\n";
        return 1;
    }
    
    // Set socket options
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Setsockopt failed\n";
        return 1;
    }
    
    // Bind
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);
    
    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "Bind failed\n";
        return 1;
    }
    
    // Listen
    if (listen(server_fd, 10) < 0) {
        std::cerr << "Listen failed\n";
        return 1;
    }
    
    std::cout << "✓ Server listening on port " << PORT << "\n\n";
    
    // Accept connections
    while (true) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) continue;
        
        std::cout << "[ACCEPT] New connection\n";
        
        // Handle in thread
        int* arg = new int(client_fd);
        pthread_t tid;
        pthread_create(&tid, nullptr, handle_client, arg);
        pthread_detach(tid);
    }
    
    close(server_fd);
    return 0;
}
