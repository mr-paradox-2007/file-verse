#include "cli_client.hpp"
#include "logger.hpp"
#include <iostream>
#include <sstream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <chrono>

namespace ofs {

CliClient::CliClient(const std::string& host, uint16_t port)
    : host_(host), port_(port), socket_(-1), connected_(false) {}

CliClient::~CliClient() {
    if (connected_) {
        disconnect();
    }
}

bool CliClient::connect() {
    std::cout << "Connecting to " << host_ << ":" << port_ << std::endl;
    
    socket_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (socket_ < 0) {
        std::cerr << "Error: Cannot create socket" << std::endl;
        return false;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_);

    // Try inet_pton first (for numeric IPs), then gethostbyname (for hostnames)
    if (inet_pton(AF_INET, host_.c_str(), &server_addr.sin_addr) > 0) {
        // Successfully parsed as numeric IP
    } else {
        // Try hostname resolution
        struct hostent* host_ent = gethostbyname(host_.c_str());
        if (!host_ent) {
            std::cerr << "Error: Could not resolve hostname: " << host_ << std::endl;
            close(socket_);
            return false;
        }
        std::memcpy(&server_addr.sin_addr, host_ent->h_addr, host_ent->h_length);
    }

    if (::connect(socket_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Error: Cannot connect to server at " << host_ << ":" << port_ << std::endl;
        close(socket_);
        return false;
    }

    connected_ = true;
    std::cout << "[OK] Connected to server at " << host_ << ":" << port_ << std::endl;
    return true;
}

void CliClient::disconnect() {
    if (socket_ >= 0) {
        close(socket_);
        socket_ = -1;
    }
    connected_ = false;
    session_id_.clear();
    current_user_.clear();
    std::cout << "[OK] Disconnected from server" << std::endl;
}

bool CliClient::isConnected() const {
    return connected_;
}

bool CliClient::sendRequest(const std::string& operation, const std::string& data) {
    if (!connected_) {
        std::cerr << "Error: Not connected to server" << std::endl;
        return false;
    }

    std::string request = operation + "|" + session_id_ + "|" + data;
    if (::send(socket_, request.c_str(), request.length(), 0) < 0) {
        std::cerr << "Error: Failed to send request" << std::endl;
        return false;
    }

    return true;
}

bool CliClient::receiveResponse(std::string& out_response) {
    if (!connected_) {
        std::cerr << "Error: Not connected to server" << std::endl;
        return false;
    }

    char buffer[4096];
    int bytes = ::recv(socket_, buffer, sizeof(buffer) - 1, 0);
    if (bytes < 0) {
        std::cerr << "Error: Failed to receive response" << std::endl;
        return false;
    }

    buffer[bytes] = '\0';
    out_response = std::string(buffer);
    return true;
}

bool CliClient::login(const std::string& username, const std::string& password) {
    if (!connected_) {
        std::cerr << "Error: Not connected to server" << std::endl;
        return false;
    }

    std::string data = username + "|" + password;
    if (!sendRequest("LOGIN", data)) {
        return false;
    }

    std::string response;
    if (!receiveResponse(response)) {
        return false;
    }

    if (response.find("SUCCESS") != std::string::npos) {
        current_user_ = username;
        session_id_ = "session_" + username + "_" + 
                     std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
        std::cout << "[OK] Logged in as " << username << std::endl;
        return true;
    }

    std::cerr << "[FAIL] Login failed: " << response << std::endl;
    return false;
}

bool CliClient::logout() {
    if (!connected_) {
        std::cerr << "Error: Not connected to server" << std::endl;
        return false;
    }

    if (!sendRequest("LOGOUT", "")) {
        return false;
    }

    std::string response;
    if (!receiveResponse(response)) {
        return false;
    }

    if (response.find("SUCCESS") != std::string::npos) {
        current_user_.clear();
        session_id_.clear();
        std::cout << "[OK] Logged out successfully" << std::endl;
        return true;
    }

    std::cerr << "[FAIL] Logout failed" << std::endl;
    return false;
}

bool CliClient::createFile(const std::string& path, uint32_t permissions) {
    if (!connected_ || session_id_.empty()) {
        std::cerr << "Error: Not logged in" << std::endl;
        return false;
    }

    std::string data = path + "|" + std::to_string(permissions);
    if (!sendRequest("CREATE_FILE", data)) {
        return false;
    }

    std::string response;
    if (!receiveResponse(response)) {
        return false;
    }

    if (response.find("SUCCESS") != std::string::npos) {
        std::cout << "[OK] File created: " << path << std::endl;
        return true;
    }

    std::cerr << "[FAIL] Failed to create file: " << response << std::endl;
    return false;
}

bool CliClient::deleteFile(const std::string& path) {
    if (!connected_ || session_id_.empty()) {
        std::cerr << "Error: Not logged in" << std::endl;
        return false;
    }

    if (!sendRequest("DELETE_FILE", path)) {
        return false;
    }

    std::string response;
    if (!receiveResponse(response)) {
        return false;
    }

    if (response.find("SUCCESS") != std::string::npos) {
        std::cout << "[OK] File deleted: " << path << std::endl;
        return true;
    }

    std::cerr << "[FAIL] Failed to delete file: " << response << std::endl;
    return false;
}

bool CliClient::readFile(const std::string& path, std::string& out_data) {
    if (!connected_ || session_id_.empty()) {
        std::cerr << "Error: Not logged in" << std::endl;
        return false;
    }

    if (!sendRequest("READ_FILE", path)) {
        return false;
    }

    if (!receiveResponse(out_data)) {
        return false;
    }

    if (out_data.find("SUCCESS") != std::string::npos) {
        std::cout << "[OK] File read: " << path << std::endl;
        return true;
    }

    std::cerr << "[FAIL] Failed to read file: " << out_data << std::endl;
    return false;
}

bool CliClient::writeFile(const std::string& path, const std::string& data) {
    if (!connected_ || session_id_.empty()) {
        std::cerr << "Error: Not logged in" << std::endl;
        return false;
    }

    std::string request_data = path + "|" + data;
    if (!sendRequest("WRITE_FILE", request_data)) {
        return false;
    }

    std::string response;
    if (!receiveResponse(response)) {
        return false;
    }

    if (response.find("SUCCESS") != std::string::npos) {
        std::cout << "[OK] File written: " << path << std::endl;
        return true;
    }

    std::cerr << "[FAIL] Failed to write file: " << response << std::endl;
    return false;
}

bool CliClient::listDirectory(const std::string& path) {
    if (!connected_ || session_id_.empty()) {
        std::cerr << "Error: Not logged in" << std::endl;
        return false;
    }

    if (!sendRequest("LIST_DIR", path)) {
        return false;
    }

    std::string response;
    if (!receiveResponse(response)) {
        return false;
    }

    std::cout << "Directory contents of " << path << ":" << std::endl;
    std::cout << response << std::endl;
    return true;
}

bool CliClient::createDirectory(const std::string& path, uint32_t permissions) {
    if (!connected_ || session_id_.empty()) {
        std::cerr << "Error: Not logged in" << std::endl;
        return false;
    }

    std::string data = path + "|" + std::to_string(permissions);
    if (!sendRequest("CREATE_DIR", data)) {
        return false;
    }

    std::string response;
    if (!receiveResponse(response)) {
        return false;
    }

    if (response.find("SUCCESS") != std::string::npos) {
        std::cout << "[OK] Directory created: " << path << std::endl;
        return true;
    }

    std::cerr << "[FAIL] Failed to create directory: " << response << std::endl;
    return false;
}

bool CliClient::deleteDirectory(const std::string& path) {
    if (!connected_ || session_id_.empty()) {
        std::cerr << "Error: Not logged in" << std::endl;
        return false;
    }

    if (!sendRequest("DELETE_DIR", path)) {
        return false;
    }

    std::string response;
    if (!receiveResponse(response)) {
        return false;
    }

    if (response.find("SUCCESS") != std::string::npos) {
        std::cout << "[OK] Directory deleted: " << path << std::endl;
        return true;
    }

    std::cerr << "[FAIL] Failed to delete directory: " << response << std::endl;
    return false;
}

bool CliClient::getMetadata(const std::string& path) {
    if (!connected_ || session_id_.empty()) {
        std::cerr << "Error: Not logged in" << std::endl;
        return false;
    }

    if (!sendRequest("GET_META", path)) {
        return false;
    }

    std::string response;
    if (!receiveResponse(response)) {
        return false;
    }

    std::cout << "Metadata for " << path << ":" << std::endl;
    std::cout << response << std::endl;
    return true;
}

void CliClient::printHelp() const {
    std::cout << "\n================================================================" << std::endl;
    std::cout << "                   OFS - CLI Client Help" << std::endl;
    std::cout << "================================================================" << std::endl;
    std::cout << "\nCommands:\n" << std::endl;
    std::cout << "  connect <host> [port]     - Connect to server" << std::endl;
    std::cout << "  disconnect                - Disconnect from server" << std::endl;
    std::cout << "  login <user> <pass>       - Login with credentials" << std::endl;
    std::cout << "  logout                    - Logout from server" << std::endl;
    std::cout << "  create <path> [perms]     - Create file" << std::endl;
    std::cout << "  delete <path>             - Delete file" << std::endl;
    std::cout << "  read <path>               - Read file" << std::endl;
    std::cout << "  write <path> <data>       - Write to file" << std::endl;
    std::cout << "  mkdir <path> [perms]      - Create directory" << std::endl;
    std::cout << "  rmdir <path>              - Delete directory" << std::endl;
    std::cout << "  ls <path>                 - List directory" << std::endl;
    std::cout << "  info <path>               - Get file metadata" << std::endl;
    std::cout << "  help                      - Show this help" << std::endl;
    std::cout << "  exit                      - Exit client" << std::endl;
    std::cout << "\nExample:\n" << std::endl;
    std::cout << "  > connect localhost 8080" << std::endl;
    std::cout << "  > login admin password123" << std::endl;
    std::cout << "  > create /test.txt 0644" << std::endl;
    std::cout << "  > write /test.txt \"Hello, World!\"" << std::endl;
    std::cout << "  > read /test.txt" << std::endl;
    std::cout << "  > logout" << std::endl;
    std::cout << "  > disconnect" << std::endl << std::endl;
}

void CliClient::displayMainMenu() const {
    std::cout << "\n================================================================" << std::endl;
    std::cout << "                   OFS - Client Main Menu" << std::endl;
    std::cout << "================================================================" << std::endl;
    std::cout << "\n[1] Connection Management" << std::endl;
    std::cout << "[2] Authentication" << std::endl;
    std::cout << "[3] File Operations" << std::endl;
    std::cout << "[4] Directory Operations" << std::endl;
    std::cout << "[5] View System Status" << std::endl;
    std::cout << "[6] Show Help" << std::endl;
    std::cout << "[0] Exit" << std::endl;
    std::cout << "\nSelect option: ";
}

void CliClient::displayConnectionMenu() {
    std::cout << "\n================================================================" << std::endl;
    std::cout << "              Connection Management Menu" << std::endl;
    std::cout << "================================================================" << std::endl;
    std::cout << "\n[1] Connect to Server" << std::endl;
    std::cout << "[2] Disconnect from Server" << std::endl;
    std::cout << "[3] Check Connection Status" << std::endl;
    std::cout << "[0] Back to Main Menu" << std::endl;
    std::cout << "\nSelect option: ";
    
    std::string choice;
    std::getline(std::cin, choice);
    
    if (choice == "1") {
        std::cout << "Enter server host (default: localhost): ";
        std::string host;
        std::getline(std::cin, host);
        if (host.empty()) host = "localhost";
        
        std::cout << "Enter server port (default: 8080): ";
        std::string port_str;
        std::getline(std::cin, port_str);
        uint16_t port = 8080;
        if (!port_str.empty()) {
            try {
                port = static_cast<uint16_t>(std::stoul(port_str));
            } catch (...) {
                port = 8080;
            }
        }
        
        host_ = host;
        port_ = port;
        connect();
    } else if (choice == "2") {
        if (connected_) {
            disconnect();
        } else {
            std::cout << "Not connected to any server" << std::endl;
        }
    } else if (choice == "3") {
        if (connected_) {
            std::cout << "Connected to: " << host_ << ":" << port_ << std::endl;
        } else {
            std::cout << "Not connected" << std::endl;
        }
    } else if (choice == "0") {
        return;
    } else {
        std::cout << "Invalid option" << std::endl;
    }
}

void CliClient::displayAuthMenu() {
    std::cout << "\n================================================================" << std::endl;
    std::cout << "                  Authentication Menu" << std::endl;
    std::cout << "================================================================" << std::endl;
    std::cout << "\n[1] Login" << std::endl;
    std::cout << "[2] Logout" << std::endl;
    std::cout << "[3] Current User Status" << std::endl;
    std::cout << "[0] Back to Main Menu" << std::endl;
    std::cout << "\nSelect option: ";
    
    std::string choice;
    std::getline(std::cin, choice);
    
    if (choice == "1") {
        if (!connected_) {
            std::cout << "Error: Not connected to server" << std::endl;
            return;
        }
        
        std::cout << "Enter username: ";
        std::string user;
        std::getline(std::cin, user);
        
        std::cout << "Enter password: ";
        std::string pass;
        std::getline(std::cin, pass);
        
        login(user, pass);
    } else if (choice == "2") {
        logout();
    } else if (choice == "3") {
        if (!current_user_.empty()) {
            std::cout << "Logged in as: " << current_user_ << std::endl;
        } else {
            std::cout << "Not logged in" << std::endl;
        }
    } else if (choice == "0") {
        return;
    } else {
        std::cout << "Invalid option" << std::endl;
    }
}

void CliClient::displayFileMenu() {
    std::cout << "\n================================================================" << std::endl;
    std::cout << "                  File Operations Menu" << std::endl;
    std::cout << "================================================================" << std::endl;
    std::cout << "\n[1] Create File" << std::endl;
    std::cout << "[2] Delete File" << std::endl;
    std::cout << "[3] Read File" << std::endl;
    std::cout << "[4] Write to File" << std::endl;
    std::cout << "[5] Get File Info" << std::endl;
    std::cout << "[0] Back to Main Menu" << std::endl;
    std::cout << "\nSelect option: ";
    
    std::string choice;
    std::getline(std::cin, choice);
    
    if (choice == "1") {
        if (!connected_ || session_id_.empty()) {
            std::cout << "Error: Not logged in" << std::endl;
            return;
        }
        
        std::cout << "Enter file path: ";
        std::string path;
        std::getline(std::cin, path);
        
        std::cout << "Enter permissions (default: 0644): ";
        std::string perms_str;
        std::getline(std::cin, perms_str);
        uint32_t perms = 0644;
        if (!perms_str.empty()) {
            try {
                perms = std::stoul(perms_str, nullptr, 8);
            } catch (...) {
                perms = 0644;
            }
        }
        
        createFile(path, perms);
    } else if (choice == "2") {
        if (!connected_ || session_id_.empty()) {
            std::cout << "Error: Not logged in" << std::endl;
            return;
        }
        
        std::cout << "Enter file path to delete: ";
        std::string path;
        std::getline(std::cin, path);
        
        deleteFile(path);
    } else if (choice == "3") {
        if (!connected_ || session_id_.empty()) {
            std::cout << "Error: Not logged in" << std::endl;
            return;
        }
        
        std::cout << "Enter file path to read: ";
        std::string path;
        std::getline(std::cin, path);
        
        std::string data;
        if (readFile(path, data)) {
            std::cout << "File contents:" << std::endl;
            std::cout << data << std::endl;
        }
    } else if (choice == "4") {
        if (!connected_ || session_id_.empty()) {
            std::cout << "Error: Not logged in" << std::endl;
            return;
        }
        
        std::cout << "Enter file path: ";
        std::string path;
        std::getline(std::cin, path);
        
        std::cout << "Enter content to write: ";
        std::string content;
        std::getline(std::cin, content);
        
        writeFile(path, content);
    } else if (choice == "5") {
        if (!connected_ || session_id_.empty()) {
            std::cout << "Error: Not logged in" << std::endl;
            return;
        }
        
        std::cout << "Enter file path: ";
        std::string path;
        std::getline(std::cin, path);
        
        getMetadata(path);
    } else if (choice == "0") {
        return;
    } else {
        std::cout << "Invalid option" << std::endl;
    }
}

void CliClient::displayDirectoryMenu() {
    std::cout << "\n================================================================" << std::endl;
    std::cout << "              Directory Operations Menu" << std::endl;
    std::cout << "================================================================" << std::endl;
    std::cout << "\n[1] Create Directory" << std::endl;
    std::cout << "[2] Delete Directory" << std::endl;
    std::cout << "[3] List Directory" << std::endl;
    std::cout << "[0] Back to Main Menu" << std::endl;
    std::cout << "\nSelect option: ";
    
    std::string choice;
    std::getline(std::cin, choice);
    
    if (choice == "1") {
        if (!connected_ || session_id_.empty()) {
            std::cout << "Error: Not logged in" << std::endl;
            return;
        }
        
        std::cout << "Enter directory path: ";
        std::string path;
        std::getline(std::cin, path);
        
        std::cout << "Enter permissions (default: 0755): ";
        std::string perms_str;
        std::getline(std::cin, perms_str);
        uint32_t perms = 0755;
        if (!perms_str.empty()) {
            try {
                perms = std::stoul(perms_str, nullptr, 8);
            } catch (...) {
                perms = 0755;
            }
        }
        
        createDirectory(path, perms);
    } else if (choice == "2") {
        if (!connected_ || session_id_.empty()) {
            std::cout << "Error: Not logged in" << std::endl;
            return;
        }
        
        std::cout << "Enter directory path to delete: ";
        std::string path;
        std::getline(std::cin, path);
        
        deleteDirectory(path);
    } else if (choice == "3") {
        if (!connected_ || session_id_.empty()) {
            std::cout << "Error: Not logged in" << std::endl;
            return;
        }
        
        std::cout << "Enter directory path: ";
        std::string path;
        std::getline(std::cin, path);
        
        listDirectory(path);
    } else if (choice == "0") {
        return;
    } else {
        std::cout << "Invalid option" << std::endl;
    }
}

void CliClient::run() {
    std::cout << "\n================================================================" << std::endl;
    std::cout << "           OFS - Command Line Client v1.0" << std::endl;
    std::cout << "================================================================" << std::endl;
    std::cout << "\nWelcome to OFS CLI Client" << std::endl;
    std::cout << "Type 'help' at any menu for more information\n" << std::endl;

    while (true) {
        displayMainMenu();
        
        std::string choice;
        std::getline(std::cin, choice);
        
        if (choice == "1") {
            displayConnectionMenu();
        } else if (choice == "2") {
            displayAuthMenu();
        } else if (choice == "3") {
            displayFileMenu();
        } else if (choice == "4") {
            displayDirectoryMenu();
        } else if (choice == "5") {
            std::cout << "\n================================================================" << std::endl;
            std::cout << "                      System Status" << std::endl;
            std::cout << "================================================================" << std::endl;
            if (connected_) {
                std::cout << "\nConnection Status: Connected" << std::endl;
                std::cout << "  Server: " << host_ << ":" << port_ << std::endl;
                if (!current_user_.empty()) {
                    std::cout << "\nAuthentication Status: Logged In" << std::endl;
                    std::cout << "  User: " << current_user_ << std::endl;
                } else {
                    std::cout << "\nAuthentication Status: Not Logged In" << std::endl;
                }
            } else {
                std::cout << "\nConnection Status: Not Connected" << std::endl;
                std::cout << "Authentication Status: Not Logged In" << std::endl;
            }
        } else if (choice == "6") {
            printHelp();
        } else if (choice == "0") {
            if (connected_) {
                disconnect();
            }
            std::cout << "\nThank you for using OFS Client. Goodbye!" << std::endl;
            break;
        } else {
            std::cout << "\nInvalid option. Please try again." << std::endl;
        }
    }
}

}
