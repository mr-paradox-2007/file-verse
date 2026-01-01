#pragma once

#include "../include/ofs_types.hpp"
#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <cstdint>

namespace ofs {

struct Request {
    std::string session_id;
    std::string operation;
    std::string data;
    uint64_t timestamp;
    uint32_t request_id;
};

struct Response {
    uint32_t request_id;
    OFSErrorCodes status;
    std::string result;
    uint64_t timestamp;
};

class FIFOQueue {
public:
    static FIFOQueue& getInstance();

    bool enqueueRequest(const Request& req);
    bool dequeueRequest(Request& out_req, uint32_t timeout_ms = 1000);
    
    bool enqueueResponse(const Response& resp);
    bool dequeueResponse(Response& out_resp, uint32_t timeout_ms = 1000);

    uint32_t getRequestQueueSize() const;
    uint32_t getResponseQueueSize() const;

    void clear();

private:
    FIFOQueue() = default;
    ~FIFOQueue() = default;

    mutable std::mutex request_mutex_;
    mutable std::mutex response_mutex_;
    std::condition_variable request_cv_;
    std::condition_variable response_cv_;

    std::queue<Request> requests_;
    std::queue<Response> responses_;
};

}
