#include "network_server.hpp"
#include "logger.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <sstream>
#include <ctime>

namespace ofs {

NetworkServer& NetworkServer::getInstance() {
    static NetworkServer instance;
    return instance;
}

OFSErrorCodes NetworkServer::initialize(const Config& config) {
    port_ = config.port;
    is_running_ = false;
    connected_clients_ = 0;
    total_requests_ = 0;
    total_responses_ = 0;

    LOG_INFO("NET_SRV", 0, "Network server initialized on port " + 
                          std::to_string(port_));

    return OFSErrorCodes::SUCCESS;
}

OFSErrorCodes NetworkServer::start() {
    if (is_running_) {
        LOG_WARN("NET_SRV", 701, "Server already running");
        return OFSErrorCodes::ERROR_INVALID_OPERATION;
    }

    server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_ < 0) {
        LOG_ERROR("NET_SRV", 702, "Failed to create socket");
        return OFSErrorCodes::ERROR_IO_ERROR;
    }

    int opt = 1;
    if (setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        LOG_ERROR("NET_SRV", 703, "Failed to set socket options");
        close(server_socket_);
        return OFSErrorCodes::ERROR_IO_ERROR;
    }

    struct sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port_);

    if (bind(server_socket_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        LOG_ERROR("NET_SRV", 704, "Failed to bind socket to port " + 
                                  std::to_string(port_));
        close(server_socket_);
        return OFSErrorCodes::ERROR_IO_ERROR;
    }

    if (listen(server_socket_, 5) < 0) {
        LOG_ERROR("NET_SRV", 705, "Failed to listen on socket");
        close(server_socket_);
        return OFSErrorCodes::ERROR_IO_ERROR;
    }

    is_running_ = true;

    accept_thread_ = std::thread(&NetworkServer::acceptConnectionsThread, this);
    process_thread_ = std::thread(&NetworkServer::processRequestsThread, this);

    LOG_INFO("NET_SRV", 0, "Server started successfully");
    return OFSErrorCodes::SUCCESS;
}

OFSErrorCodes NetworkServer::stop() {
    if (!is_running_) {
        LOG_WARN("NET_SRV", 706, "Server not running");
        return OFSErrorCodes::ERROR_INVALID_OPERATION;
    }

    is_running_ = false;

    if (accept_thread_.joinable()) {
        accept_thread_.join();
    }

    if (process_thread_.joinable()) {
        process_thread_.join();
    }

    close(server_socket_);

    LOG_INFO("NET_SRV", 0, "Server stopped");
    return OFSErrorCodes::SUCCESS;
}

uint32_t NetworkServer::getConnectedClients() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return connected_clients_;
}

uint64_t NetworkServer::getTotalRequests() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return total_requests_;
}

uint64_t NetworkServer::getTotalResponses() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return total_responses_;
}

void NetworkServer::acceptConnectionsThread() {
    LOG_INFO("NET_SRV", 0, "Accept connections thread started");

    while (is_running_) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);

        int client_socket = accept(server_socket_, 
                                   (struct sockaddr*)&client_addr,
                                   &client_addr_len);

        if (client_socket < 0) {
            if (is_running_) {
                LOG_WARN("NET_SRV", 707, "Accept failed");
            }
            continue;
        }

        {
            std::lock_guard<std::mutex> lock(stats_mutex_);
            connected_clients_++;
        }

        std::thread client_thread(&NetworkServer::handleClientConnection, this, client_socket);
        client_thread.detach();
    }

    LOG_INFO("NET_SRV", 0, "Accept connections thread stopped");
}

void NetworkServer::processRequestsThread() {
    LOG_INFO("NET_SRV", 0, "Process requests thread started");

    FIFOQueue& queue = FIFOQueue::getInstance();

    while (is_running_) {
        Request req;
        if (!queue.dequeueRequest(req, 100)) {
            continue;
        }

        {
            std::lock_guard<std::mutex> lock(stats_mutex_);
            total_requests_++;
        }

        LOG_DEBUG("NET_SRV", 0, "Processing request: " + std::to_string(req.request_id));

        Response resp;
        resp.request_id = req.request_id;
        resp.status = OFSErrorCodes::SUCCESS;
        resp.result = "OK";
        resp.timestamp = std::time(nullptr);

        {
            std::lock_guard<std::mutex> lock(stats_mutex_);
            total_responses_++;
        }

        queue.enqueueResponse(resp);
    }

    LOG_INFO("NET_SRV", 0, "Process requests thread stopped");
}

void NetworkServer::handleClientConnection(int socket_fd) {
    char buffer[4096];
    std::string session_id;
    std::string current_user;
    bool authenticated = false;
    
    while (is_running_) {
        int bytes_read = recv(socket_fd, buffer, sizeof(buffer) - 1, 0);

        if (bytes_read < 0) {
            usleep(10000);
            continue;
        }

        if (bytes_read == 0) {
            break;
        }

        buffer[bytes_read] = '\0';
        std::string request_str(buffer);

        size_t delim1 = request_str.find('|');
        if (delim1 == std::string::npos) {
            std::string error_resp = "ERROR|Invalid request format";
            send(socket_fd, error_resp.c_str(), error_resp.length(), 0);
            continue;
        }

        std::string operation = request_str.substr(0, delim1);
        std::string data = request_str.substr(delim1 + 1);

        std::string response;

        if (operation == "LOGIN") {
            size_t delim = data.find('|');
            if (delim != std::string::npos) {
                std::string user = data.substr(0, delim);
                std::string pass = data.substr(delim + 1);
                
                authenticated = true;
                current_user = user;
                session_id = "session_" + user + "_" + std::to_string(std::time(nullptr));
                response = "SUCCESS|Logged in as " + user;
            } else {
                response = "ERROR|Invalid login format";
            }
        } else if (operation == "LOGOUT") {
            authenticated = false;
            current_user.clear();
            response = "SUCCESS|Logged out";
        } else if (operation == "CREATE_FILE") {
            if (!authenticated) {
                response = "ERROR|Not authenticated";
            } else {
                response = "SUCCESS|File created";
            }
        } else if (operation == "DELETE_FILE") {
            if (!authenticated) {
                response = "ERROR|Not authenticated";
            } else {
                response = "SUCCESS|File deleted";
            }
        } else if (operation == "READ_FILE") {
            if (!authenticated) {
                response = "ERROR|Not authenticated";
            } else {
                response = "SUCCESS|File contents";
            }
        } else if (operation == "WRITE_FILE") {
            if (!authenticated) {
                response = "ERROR|Not authenticated";
            } else {
                response = "SUCCESS|File written";
            }
        } else if (operation == "LIST_DIR") {
            if (!authenticated) {
                response = "ERROR|Not authenticated";
            } else {
                response = "SUCCESS|/\n  ..";
            }
        } else if (operation == "CREATE_DIR") {
            if (!authenticated) {
                response = "ERROR|Not authenticated";
            } else {
                response = "SUCCESS|Directory created";
            }
        } else if (operation == "DELETE_DIR") {
            if (!authenticated) {
                response = "ERROR|Not authenticated";
            } else {
                response = "SUCCESS|Directory deleted";
            }
        } else if (operation == "GET_META") {
            if (!authenticated) {
                response = "ERROR|Not authenticated";
            } else {
                response = "SUCCESS|size=0|owner=" + current_user;
            }
        } else {
            response = "ERROR|Unknown operation: " + operation;
        }

        send(socket_fd, response.c_str(), response.length(), 0);
    }

    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        connected_clients_--;
    }

    close(socket_fd);
}

OFSErrorCodes NetworkServer::parseJsonRequest(const std::string& json, Request& out_req) {
    LOG_DEBUG("NET_SRV", 0, "Parsing JSON request");
    return OFSErrorCodes::SUCCESS;
}

std::string NetworkServer::createJsonResponse(const Response& resp) {
    std::stringstream ss;
    ss << "{\"request_id\":" << resp.request_id
       << ",\"status\":" << static_cast<int>(resp.status)
       << ",\"result\":\"" << resp.result << "\"}";
    return ss.str();
}

}
