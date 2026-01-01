#include "fifo_queue.hpp"
#include "logger.hpp"
#include <chrono>
#include <ctime>

namespace ofs {

FIFOQueue& FIFOQueue::getInstance() {
    static FIFOQueue instance;
    return instance;
}

bool FIFOQueue::enqueueRequest(const Request& req) {
    std::lock_guard<std::mutex> lock(request_mutex_);
    requests_.push(req);
    request_cv_.notify_one();

    LOG_DEBUG("FIFO_Q", 0, "Request enqueued: id=" + std::to_string(req.request_id) +
                          ", op=" + req.operation);
    return true;
}

bool FIFOQueue::dequeueRequest(Request& out_req, uint32_t timeout_ms) {
    std::unique_lock<std::mutex> lock(request_mutex_);

    auto timeout = std::chrono::milliseconds(timeout_ms);
    if (!request_cv_.wait_for(lock, timeout, [this] { return !requests_.empty(); })) {
        return false;
    }

    if (requests_.empty()) {
        return false;
    }

    out_req = requests_.front();
    requests_.pop();

    LOG_DEBUG("FIFO_Q", 0, "Request dequeued: id=" + std::to_string(out_req.request_id));
    return true;
}

bool FIFOQueue::enqueueResponse(const Response& resp) {
    std::lock_guard<std::mutex> lock(response_mutex_);
    responses_.push(resp);
    response_cv_.notify_one();

    LOG_DEBUG("FIFO_Q", 0, "Response enqueued: id=" + std::to_string(resp.request_id));
    return true;
}

bool FIFOQueue::dequeueResponse(Response& out_resp, uint32_t timeout_ms) {
    std::unique_lock<std::mutex> lock(response_mutex_);

    auto timeout = std::chrono::milliseconds(timeout_ms);
    if (!response_cv_.wait_for(lock, timeout, [this] { return !responses_.empty(); })) {
        return false;
    }

    if (responses_.empty()) {
        return false;
    }

    out_resp = responses_.front();
    responses_.pop();

    LOG_DEBUG("FIFO_Q", 0, "Response dequeued: id=" + std::to_string(out_resp.request_id));
    return true;
}

uint32_t FIFOQueue::getRequestQueueSize() const {
    std::lock_guard<std::mutex> lock(request_mutex_);
    return requests_.size();
}

uint32_t FIFOQueue::getResponseQueueSize() const {
    std::lock_guard<std::mutex> lock(response_mutex_);
    return responses_.size();
}

void FIFOQueue::clear() {
    {
        std::lock_guard<std::mutex> lock(request_mutex_);
        while (!requests_.empty()) {
            requests_.pop();
        }
    }

    {
        std::lock_guard<std::mutex> lock(response_mutex_);
        while (!responses_.empty()) {
            responses_.pop();
        }
    }

    LOG_INFO("FIFO_Q", 0, "FIFO queues cleared");
}

}
