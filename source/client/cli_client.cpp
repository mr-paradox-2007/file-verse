#include "cli_client.hpp"
#include "logger.hpp"
#include <iostream>
#include <sstream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
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
    socket_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (socket_ < 0) {
        std::cerr << "Error: Cannot create socket" << std::endl;
        return false;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_);

    if (inet_pton(AF_INET, host_.c_str(), &server_addr.sin_addr) <= 0) {
        std::cerr << "Error: Invalid address/address not supported" << std::endl;
        close(socket_);
        return false;
    }

    if (::connect(socket_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Error: Cannot connect to server at " << host_ << ":" << port_ << std::endl;
        close(socket_);
        return false;
    }

    connected_ = true;
    std::cout << "✓ Connected to server at " << host_ << ":" << port_ << std::endl;
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
    std::cout << "✓ Disconnected from server" << std::endl;
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
        std::cout << "✓ Logged in as " << username << std::endl;
        return true;
    }

    std::cerr << "✗ Login failed: " << response << std::endl;
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
        std::cout << "✓ Logged out successfully" << std::endl;
        return true;
    }

    std::cerr << "✗ Logout failed" << std::endl;
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
        std::cout << "✓ File created: " << path << std::endl;
        return true;
    }

    std::cerr << "✗ Failed to create file: " << response << std::endl;
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
        std::cout << "✓ File deleted: " << path << std::endl;
        return true;
    }

    std::cerr << "✗ Failed to delete file: " << response << std::endl;
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
        std::cout << "✓ File read: " << path << std::endl;
        return true;
    }

    std::cerr << "✗ Failed to read file: " << out_data << std::endl;
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
        std::cout << "✓ File written: " << path << std::endl;
        return true;
    }

    std::cerr << "✗ Failed to write file: " << response << std::endl;
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
        std::cout << "✓ Directory created: " << path << std::endl;
        return true;
    }

    std::cerr << "✗ Failed to create directory: " << response << std::endl;
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
        std::cout << "✓ Directory deleted: " << path << std::endl;
        return true;
    }

    std::cerr << "✗ Failed to delete directory: " << response << std::endl;
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
    std::cout << "\n╔════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║         OFS - CLI Client Help                            ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════╝" << std::endl;
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

void CliClient::run() {
    std::cout << "╔════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║         OFS - Command Line Client v1.0                 ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════╝" << std::endl;
    std::cout << "Type 'help' for commands, 'exit' to quit\n" << std::endl;

    std::string line;
    while (true) {
        std::cout << "> ";
        std::getline(std::cin, line);

        if (line.empty()) continue;

        std::istringstream iss(line);
        std::string cmd;
        iss >> cmd;

        if (cmd == "exit") {
            if (connected_) {
                disconnect();
            }
            std::cout << "Goodbye!" << std::endl;
            break;
        } else if (cmd == "help") {
            printHelp();
        } else if (cmd == "connect") {
            std::string host;
            uint16_t port;
            if (iss >> host >> port) {
                this->host_ = host;
                this->port_ = port;
                connect();
            } else {
                std::cerr << "Usage: connect <host> <port>" << std::endl;
            }
        } else if (cmd == "disconnect") {
            if (connected_) {
                disconnect();
            }
        } else if (cmd == "login") {
            std::string user, pass;
            iss >> user >> pass;
            if (!user.empty() && !pass.empty()) {
                login(user, pass);
            } else {
                std::cerr << "Usage: login <username> <password>" << std::endl;
            }
        } else if (cmd == "logout") {
            logout();
        } else if (cmd == "create") {
            std::string path;
            uint32_t perms = 0644;
            iss >> path;
            iss >> perms;
            if (!path.empty()) {
                createFile(path, perms);
            }
        } else if (cmd == "delete") {
            std::string path;
            iss >> path;
            if (!path.empty()) {
                deleteFile(path);
            }
        } else if (cmd == "read") {
            std::string path;
            iss >> path;
            if (!path.empty()) {
                std::string data;
                readFile(path, data);
            }
        } else if (cmd == "write") {
            std::string path, data;
            iss >> path;
            std::getline(iss, data);
            if (data[0] == ' ') data = data.substr(1);
            if (!path.empty() && !data.empty()) {
                writeFile(path, data);
            }
        } else if (cmd == "mkdir") {
            std::string path;
            uint32_t perms = 0755;
            iss >> path;
            iss >> perms;
            if (!path.empty()) {
                createDirectory(path, perms);
            }
        } else if (cmd == "rmdir") {
            std::string path;
            iss >> path;
            if (!path.empty()) {
                deleteDirectory(path);
            }
        } else if (cmd == "ls") {
            std::string path;
            iss >> path;
            if (!path.empty()) {
                listDirectory(path);
            }
        } else if (cmd == "info") {
            std::string path;
            iss >> path;
            if (!path.empty()) {
                getMetadata(path);
            }
        } else if (cmd == "status") {
            if (connected_) {
                std::cout << "✓ Connected to: " << host_ << ":" << port_ << std::endl;
                if (!current_user_.empty()) {
                    std::cout << "✓ Logged in as: " << current_user_ << std::endl;
                } else {
                    std::cout << "✗ Not logged in" << std::endl;
                }
            } else {
                std::cout << "✗ Not connected" << std::endl;
            }
        } else {
            std::cerr << "Unknown command: " << cmd << " (type 'help' for commands)" << std::endl;
        }
    }
}

}
