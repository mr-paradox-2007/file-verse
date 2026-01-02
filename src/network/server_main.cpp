#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <pthread.h>
#include <map>
#include <set>
#include <ctime>
#include "omni_storage.hpp"
#include "file_ops.hpp"
#include "user_manager.hpp"
#include "logger.hpp"

OmniStorage* g_storage = nullptr;

struct SessionData {
    std::string username;
    uint64_t login_time;
    uint64_t last_activity;
};

std::map<std::string, SessionData> active_sessions;
std::set<std::string> logged_in_users;
pthread_mutex_t session_mutex = PTHREAD_MUTEX_INITIALIZER;

std::string generate_session_id(const std::string& username) {
    return username + "_" + std::to_string(time(nullptr)) + "_" + std::to_string(rand());
}

bool is_user_logged_in(const std::string& username) {
    pthread_mutex_lock(&session_mutex);
    bool logged_in = logged_in_users.count(username) > 0;
    pthread_mutex_unlock(&session_mutex);
    return logged_in;
}

void add_session(const std::string& session_id, const std::string& username) {
    pthread_mutex_lock(&session_mutex);
    SessionData data;
    data.username = username;
    data.login_time = time(nullptr);
    data.last_activity = time(nullptr);
    active_sessions[session_id] = data;
    logged_in_users.insert(username);
    pthread_mutex_unlock(&session_mutex);
}

void remove_session(const std::string& session_id) {
    pthread_mutex_lock(&session_mutex);
    auto it = active_sessions.find(session_id);
    if (it != active_sessions.end()) {
        logged_in_users.erase(it->second.username);
        active_sessions.erase(it);
    }
    pthread_mutex_unlock(&session_mutex);
}

std::string get_username_from_session(const std::string& session_id) {
    pthread_mutex_lock(&session_mutex);
    auto it = active_sessions.find(session_id);
    std::string username = (it != active_sessions.end()) ? it->second.username : "";
    pthread_mutex_unlock(&session_mutex);
    return username;
}

std::string extract_json_string(const std::string& json_str, const std::string& key) {
    size_t pos = json_str.find("\"" + key + "\"");
    if (pos == std::string::npos) return "";
    pos = json_str.find(":", pos);
    pos = json_str.find("\"", pos);
    size_t end_pos = json_str.find("\"", pos + 1);
    if (pos != std::string::npos && end_pos != std::string::npos) {
        return json_str.substr(pos + 1, end_pos - pos - 1);
    }
    return "";
}

std::string escape_json_string(const std::string& str) {
    std::string result;
    for (char c : str) {
        if (c == '"') result += "\\\"";
        else if (c == '\\') result += "\\\\";
        else if (c == '\n') result += "\\n";
        else if (c == '\r') result += "\\r";
        else if (c == '\t') result += "\\t";
        else result += c;
    }
    return result;
}

std::string json_response(bool success, const std::string& message) {
    return "{\"success\":" + std::string(success ? "true" : "false") + 
           ",\"message\":\"" + escape_json_string(message) + "\"}";
}

std::string get_mime_type(const std::string& filename) {
    if (filename.find(".html") != std::string::npos) return "text/html";
    if (filename.find(".css") != std::string::npos) return "text/css";
    if (filename.find(".js") != std::string::npos) return "application/javascript";
    return "text/plain";
}

std::string read_file(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) return "";
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::string serve_static_file(const std::string& path) {
    std::string filename = "." + path;
    
    if (filename.find("..") != std::string::npos) {
        return "HTTP/1.1 403 Forbidden\r\n\r\n403 Forbidden";
    }
    
    struct stat buffer;
    if (stat(filename.c_str(), &buffer) != 0) {
        return "HTTP/1.1 404 Not Found\r\n\r\n404 Not Found";
    }
    
    std::string content = read_file(filename);
    std::string mime = get_mime_type(filename);
    
    std::string response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: " + mime + "\r\n";
    response += "Content-Length: " + std::to_string(content.length()) + "\r\n";
    response += "Access-Control-Allow-Origin: *\r\n";
    response += "Cache-Control: no-cache\r\n";
    response += "\r\n";
    response += content;
    
    return response;
}

std::string handle_login(const std::string& body) {
    std::string username = extract_json_string(body, "username");
    std::string password = extract_json_string(body, "password");
    
    if (username.empty() || password.empty()) {
        return json_response(false, "Missing credentials");
    }
    
    if (is_user_logged_in(username)) {
        return json_response(false, "User already logged in");
    }
    
    OFS_Session session;
    int result = user_login(&session, username, password);
    
    if (result == 0) {
        std::string sid = generate_session_id(username);
        add_session(sid, username);
        Logger::info("[LOGIN] " + username);
        return "{\"success\":true,\"message\":\"Login successful\",\"session_id\":\"" + sid + "\",\"username\":\"" + username + "\"}";
    }
    
    return json_response(false, "Invalid username or password");
}

std::string handle_logout(const std::string& body) {
    std::string session_id = extract_json_string(body, "session_id");
    
    if (session_id.empty()) {
        return json_response(false, "No session");
    }
    
    std::string username = get_username_from_session(session_id);
    if (!username.empty()) {
        remove_session(session_id);
        Logger::info("[LOGOUT] " + username);
        return json_response(true, "Logged out");
    }
    
    return json_response(false, "Invalid session");
}

std::string handle_signup(const std::string& body) {
    std::string username = extract_json_string(body, "username");
    std::string password = extract_json_string(body, "password");
    
    if (username.length() < 3 || username.length() > 31) {
        return json_response(false, "Username must be 3-31 characters");
    }
    
    if (password.length() < 4) {
        return json_response(false, "Password must be at least 4 characters");
    }
    
    int result = user_create(username, password);
    
    if (result == 0) {
        Logger::info("[SIGNUP] " + username);
        return json_response(true, "Account created successfully");
    }
    
    return json_response(false, "Username already exists");
}

std::string handle_file_create(const std::string& body) {
    std::string session_id = extract_json_string(body, "session_id");
    std::string path = extract_json_string(body, "path");
    std::string content = extract_json_string(body, "content");
    
    if (path.empty()) return json_response(false, "No path specified");
    
    std::string username = get_username_from_session(session_id);
    if (username.empty()) {
        return json_response(false, "Invalid session");
    }
    
    int result = file_create(nullptr, path, content.c_str(), content.length());
    
    if (result == 0) {
        Logger::info("[FILE] Create: " + path, username);
        return json_response(true, "File created");
    }
    
    return json_response(false, get_error_message(result));
}

std::string handle_file_read(const std::string& body) {
    std::string session_id = extract_json_string(body, "session_id");
    std::string path = extract_json_string(body, "path");
    
    if (path.empty()) return json_response(false, "No path specified");
    
    std::string username = get_username_from_session(session_id);
    if (username.empty()) {
        return json_response(false, "Invalid session");
    }
    
    void* buffer;
    size_t size;
    int result = file_read(nullptr, path, &buffer, &size);
    
    if (result == 0) {
        std::string content((char*)buffer, size);
        free_buffer(buffer);
        Logger::info("[FILE] Read: " + path, username);
        return "{\"success\":true,\"content\":\"" + escape_json_string(content) + "\"}";
    }
    
    return json_response(false, get_error_message(result));
}

std::string handle_file_edit(const std::string& body) {
    std::string session_id = extract_json_string(body, "session_id");
    std::string path = extract_json_string(body, "path");
    std::string content = extract_json_string(body, "content");
    
    if (path.empty()) return json_response(false, "No path specified");
    
    std::string username = get_username_from_session(session_id);
    if (username.empty()) {
        return json_response(false, "Invalid session");
    }
    
    file_delete(nullptr, path);
    int result = file_create(nullptr, path, content.c_str(), content.length());
    
    if (result == 0) {
        Logger::info("[FILE] Edit: " + path, username);
        return json_response(true, "File updated");
    }
    
    return json_response(false, get_error_message(result));
}

std::string handle_file_delete(const std::string& body) {
    std::string session_id = extract_json_string(body, "session_id");
    std::string path = extract_json_string(body, "path");
    
    if (path.empty()) return json_response(false, "No path specified");
    
    std::string username = get_username_from_session(session_id);
    if (username.empty()) {
        return json_response(false, "Invalid session");
    }
    
    int result = file_delete(nullptr, path);
    
    if (result == 0) {
        Logger::info("[FILE] Delete: " + path, username);
        return json_response(true, "File deleted");
    }
    
    return json_response(false, get_error_message(result));
}

std::string handle_file_list(const std::string& body) {
    std::string session_id = extract_json_string(body, "session_id");
    std::string path = extract_json_string(body, "path");
    if (path.empty()) path = "/";
    
    std::string username = get_username_from_session(session_id);
    if (username.empty()) {
        return json_response(false, "Invalid session");
    }
    
    FileEntry* entries;
    int count;
    int result = dir_list(nullptr, path, &entries, &count);
    
    std::stringstream json;
    json << "{\"success\":true,\"files\":[";
    
    if (result == 0 && count > 0) {
        for (int i = 0; i < count; i++) {
            if (i > 0) json << ",";
            json << "{";
            json << "\"name\":\"" << escape_json_string(entries[i].name) << "\",";
            json << "\"type\":\"" << (entries[i].type == 1 ? "directory" : "file") << "\",";
            json << "\"size\":" << entries[i].size << ",";
            
            std::string full_path = (path == "/") ? "/" + std::string(entries[i].name) : path + "/" + entries[i].name;
            json << "\"path\":\"" << escape_json_string(full_path) << "\"";
            json << "}";
        }
        
        free_buffer(entries);
    }
    
    json << "]}";
    Logger::info("[DIR] List: " + path, username);
    return json.str();
}

std::string handle_directory_create(const std::string& body) {
    std::string session_id = extract_json_string(body, "session_id");
    std::string path = extract_json_string(body, "path");
    
    if (path.empty()) return json_response(false, "No path specified");
    
    std::string username = get_username_from_session(session_id);
    if (username.empty()) {
        return json_response(false, "Invalid session");
    }
    
    int result = dir_create(nullptr, path);
    
    if (result == 0) {
        Logger::info("[DIR] Create: " + path, username);
        return json_response(true, "Directory created");
    }
    
    return json_response(false, get_error_message(result));
}

std::string handle_session_info(const std::string& body) {
    std::string session_id = extract_json_string(body, "session_id");
    
    std::string username = get_username_from_session(session_id);
    if (username.empty()) {
        return json_response(false, "Invalid session");
    }
    
    return "{\"success\":true,\"username\":\"" + username + "\"}";
}

std::string handle_http_request(const std::string& http_request) {
    std::stringstream ss(http_request);
    std::string method, path, protocol;
    ss >> method >> path >> protocol;
    
    size_t header_end = http_request.find("\r\n\r\n");
    if (header_end == std::string::npos) header_end = http_request.find("\n\n");
    
    std::string body = header_end != std::string::npos ? http_request.substr(header_end + 4) : "";
    
    if (method == "GET") {
        if (path == "/" || path == "/index.html") {
            return serve_static_file("/web/index.html");
        } else if (path.find("/web/") == 0) {
            return serve_static_file(path);
        }
        return serve_static_file("/web/index.html");
    }
    
    if (method == "POST") {
        std::string response;
        
        if (path == "/user/login") response = handle_login(body);
        else if (path == "/user/logout") response = handle_logout(body);
        else if (path == "/user/signup") response = handle_signup(body);
        else if (path == "/user/session") response = handle_session_info(body);
        else if (path == "/file/list") response = handle_file_list(body);
        else if (path == "/file/create") response = handle_file_create(body);
        else if (path == "/file/read") response = handle_file_read(body);
        else if (path == "/file/edit") response = handle_file_edit(body);
        else if (path == "/file/delete") response = handle_file_delete(body);
        else if (path == "/directory/create") response = handle_directory_create(body);
        else response = json_response(false, "Unknown endpoint");
        
        std::string http_response = "HTTP/1.1 200 OK\r\n";
        http_response += "Content-Type: application/json\r\n";
        http_response += "Content-Length: " + std::to_string(response.length()) + "\r\n";
        http_response += "Access-Control-Allow-Origin: *\r\n";
        http_response += "Connection: close\r\n\r\n";
        http_response += response;
        
        return http_response;
    }
    
    if (method == "OPTIONS") {
        return "HTTP/1.1 200 OK\r\nAccess-Control-Allow-Origin: *\r\nAccess-Control-Allow-Methods: GET, POST, OPTIONS\r\nAccess-Control-Allow-Headers: Content-Type\r\nContent-Length: 0\r\n\r\n";
    }
    
    return "HTTP/1.1 405 Method Not Allowed\r\n\r\n";
}

void* handle_client(void* arg) {
    int client_socket = *(int*)arg;
    delete (int*)arg;
    
    char buffer[65536];
    int bytes_received = recv(client_socket, buffer, 65535, 0);
    
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
        std::string response = handle_http_request(std::string(buffer));
        send(client_socket, response.c_str(), response.length(), 0);
    }
    
    close(client_socket);
    return nullptr;
}

int main() {
    std::cout << "=====================================" << std::endl;
    std::cout << "  OFS Multi-User File System        " << std::endl;
    std::cout << "=====================================" << std::endl;
    
    system("mkdir -p logs data web");
    
    Logger::init();
    
    std::cout << "[*] Initializing storage..." << std::endl;
    g_storage = new OmniStorage();
    
    struct stat st;
    if (stat("data/system.omni", &st) != 0) {
        std::cout << "[*] Creating new filesystem..." << std::endl;
        if (!g_storage->create("data/system.omni", 104857600)) {
            std::cerr << "[ERROR] Failed to create filesystem" << std::endl;
            return 1;
        }
    } else {
        std::cout << "[*] Opening existing filesystem..." << std::endl;
        if (!g_storage->open("data/system.omni")) {
            std::cerr << "[ERROR] Failed to open filesystem" << std::endl;
            return 1;
        }
    }
    
    set_storage_instance(g_storage);
    
    std::cout << "[*] Loading users..." << std::endl;
    load_users();
    
    std::cout << "[*] Creating socket..." << std::endl;
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        std::cerr << "[ERROR] Socket creation failed" << std::endl;
        return 1;
    }
    
    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(8080);
    
    std::cout << "[*] Binding to port 8080..." << std::endl;
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "[!] Bind failed" << std::endl;
        return 1;
    }
    
    listen(server_socket, 20);
    std::cout << "[✓] Server running on http://localhost:8080" << std::endl;
    std::cout << "[✓] Open http://localhost:8080 in your browser" << std::endl;
    std::cout << "[INFO] Press Ctrl+C to shutdown" << std::endl;
    std::cout << std::endl;
    
    while (true) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket < 0) continue;
        
        pthread_t thread_id;
        int* socket_ptr = new int(client_socket);
        pthread_create(&thread_id, nullptr, handle_client, socket_ptr);
        pthread_detach(thread_id);
    }
    
    close(server_socket);
    g_storage->close();
    delete g_storage;
    
    return 0;
}