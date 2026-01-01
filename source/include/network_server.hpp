#pragma once

#include "../include/ofs_types.hpp"
#include "../include/config_parser.hpp"
#include "fifo_queue.hpp"
#include <string>
#include <thread>
#include <vector>
#include <cstdint>

namespace ofs {

class NetworkServer {
public:
    static NetworkServer& getInstance();

    OFSErrorCodes initialize(const Config& config);
    OFSErrorCodes start();
    OFSErrorCodes stop();

    bool isRunning() const { return is_running_; }

    uint32_t getConnectedClients() const;
    uint64_t getTotalRequests() const;
    uint64_t getTotalResponses() const;

private:
    NetworkServer() = default;
    ~NetworkServer() = default;

    void acceptConnectionsThread();
    void processRequestsThread();
    
    void handleClientConnection(int socket_fd);
    OFSErrorCodes parseJsonRequest(const std::string& json, Request& out_req);
    std::string createJsonResponse(const Response& resp);

    int server_socket_;
    uint16_t port_;
    bool is_running_;

    std::thread accept_thread_;
    std::thread process_thread_;

    uint32_t connected_clients_;
    uint64_t total_requests_;
    uint64_t total_responses_;

    mutable std::mutex stats_mutex_;
};

}
