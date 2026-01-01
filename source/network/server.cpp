#include "network_server.hpp"
#include "logger.hpp"
#include "user_manager.hpp"
#include "fs_init.hpp"
#include "file_ops.hpp"
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
    std::string session_id;
    std::string current_user;
    bool authenticated = false;
    
    UserManager& user_mgr = UserManager::getInstance();
    FileOperations& file_ops = FileOperations::getInstance();
    
    while (is_running_) {
        char buffer[4096];
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
        std::string rest = request_str.substr(delim1 + 1);
        
        size_t delim2 = rest.find('|');
        std::string recv_session = "";
        std::string data = rest;
        
        if (delim2 != std::string::npos) {
            recv_session = rest.substr(0, delim2);
            data = rest.substr(delim2 + 1);
        }

        std::string response;

        LOG_DEBUG("NET_SRV", 0, "Operation: " + operation + ", User: " + current_user);

        if (operation == "LOGIN") {
            size_t delim = data.find('|');
            if (delim != std::string::npos) {
                std::string user = data.substr(0, delim);
                std::string pass = data.substr(delim + 1);
                
                UserSession user_session;
                OFSErrorCodes result = user_mgr.loginUser(user, pass, user_session);
                
                if (result == OFSErrorCodes::SUCCESS) {
                    authenticated = true;
                    current_user = user;
                    session_id = user_session.session_id;
                    response = "SUCCESS|Logged in as " + user;
                    LOG_INFO("NET_SRV", 0, "User logged in: " + user);
                } else {
                    response = "ERROR|Login failed";
                    LOG_WARN("NET_SRV", 0, "Login failed for user: " + user);
                }
            } else {
                response = "ERROR|Invalid login format";
            }
        } else if (operation == "LOGOUT") {
            if (authenticated) {
                user_mgr.logoutUser(session_id);
                authenticated = false;
                current_user.clear();
                session_id.clear();
                response = "SUCCESS|Logged out";
                LOG_INFO("NET_SRV", 0, "User logged out");
            } else {
                response = "ERROR|Not logged in";
            }
        } else if (operation == "CREATE_FILE") {
            if (!authenticated) {
                response = "ERROR|Not authenticated";
            } else {
                size_t delim = data.find('|');
                std::string path = data;
                uint32_t perms = 0644;
                
                if (delim != std::string::npos) {
                    path = data.substr(0, delim);
                    perms = std::stoul(data.substr(delim + 1));
                }
                
                OFSErrorCodes result = file_ops.createFile(path, current_user, perms);
                if (result == OFSErrorCodes::SUCCESS) {
                    response = "SUCCESS|File created: " + path;
                } else {
                    response = "ERROR|Failed to create file: " + std::to_string(static_cast<int>(result));
                }
            }
        } else if (operation == "DELETE_FILE") {
            if (!authenticated) {
                response = "ERROR|Not authenticated";
            } else {
                OFSErrorCodes result = file_ops.deleteFile(data);
                if (result == OFSErrorCodes::SUCCESS) {
                    response = "SUCCESS|File deleted: " + data;
                } else {
                    response = "ERROR|Failed to delete file: " + std::to_string(static_cast<int>(result));
                }
            }
        } else if (operation == "READ_FILE") {
            if (!authenticated) {
                response = "ERROR|Not authenticated";
            } else {
                std::string file_data;
                OFSErrorCodes result = file_ops.readFile(data, file_data);
                if (result == OFSErrorCodes::SUCCESS) {
                    response = "SUCCESS|" + file_data;
                } else {
                    response = "ERROR|Failed to read file: " + std::to_string(static_cast<int>(result));
                }
            }
        } else if (operation == "WRITE_FILE") {
            if (!authenticated) {
                response = "ERROR|Not authenticated";
            } else {
                size_t delim = data.find('|');
                if (delim != std::string::npos) {
                    std::string path = data.substr(0, delim);
                    std::string content = data.substr(delim + 1);
                    
                    OFSErrorCodes result = file_ops.writeFile(path, content);
                    if (result == OFSErrorCodes::SUCCESS) {
                        response = "SUCCESS|File written: " + path;
                    } else {
                        response = "ERROR|Failed to write file: " + std::to_string(static_cast<int>(result));
                    }
                } else {
                    response = "ERROR|Invalid write format";
                }
            }
        } else if (operation == "LIST_DIR") {
            if (!authenticated) {
                response = "ERROR|Not authenticated";
            } else {
                std::vector<std::string> entries;
                OFSErrorCodes result = file_ops.listDirectory(data, entries);
                if (result == OFSErrorCodes::SUCCESS) {
                    response = "SUCCESS|";
                    for (const auto& entry : entries) {
                        response += entry + "\n";
                    }
                } else {
                    response = "ERROR|Failed to list directory: " + std::to_string(static_cast<int>(result));
                }
            }
        } else if (operation == "CREATE_DIR") {
            if (!authenticated) {
                response = "ERROR|Not authenticated";
            } else {
                size_t delim = data.find('|');
                std::string path = data;
                uint32_t perms = 0755;
                
                if (delim != std::string::npos) {
                    path = data.substr(0, delim);
                    perms = std::stoul(data.substr(delim + 1));
                }
                
                OFSErrorCodes result = file_ops.createDirectory(path, current_user, perms);
                if (result == OFSErrorCodes::SUCCESS) {
                    response = "SUCCESS|Directory created: " + path;
                } else {
                    response = "ERROR|Failed to create directory: " + std::to_string(static_cast<int>(result));
                }
            }
        } else if (operation == "DELETE_DIR") {
            if (!authenticated) {
                response = "ERROR|Not authenticated";
            } else {
                OFSErrorCodes result = file_ops.deleteDirectory(data);
                if (result == OFSErrorCodes::SUCCESS) {
                    response = "SUCCESS|Directory deleted: " + data;
                } else {
                    response = "ERROR|Failed to delete directory: " + std::to_string(static_cast<int>(result));
                }
            }
        } else if (operation == "GET_META") {
            if (!authenticated) {
                response = "ERROR|Not authenticated";
            } else {
                FileEntry entry;
                OFSErrorCodes result = file_ops.getMetadata(data, entry);
                if (result == OFSErrorCodes::SUCCESS) {
                    response = "SUCCESS|size=" + std::to_string(entry.size) + 
                              "|owner=" + std::string(entry.owner) +
                              "|permissions=" + std::to_string(entry.permissions);
                } else {
                    response = "ERROR|Failed to get metadata: " + std::to_string(static_cast<int>(result));
                }
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
    LOG_DEBUG("NET_SRV", 0, "Client disconnected");
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