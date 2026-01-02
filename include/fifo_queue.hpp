#ifndef FIFO_QUEUE_HPP
#define FIFO_QUEUE_HPP

#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <functional>
#include <string>
#include <cstdint>

struct Operation {
    uint64_t request_id;
    std::string operation_type;
    std::string path;
    std::string username;
    std::string data;
    uint32_t priority;
    
    Operation(uint64_t id, const std::string& op, const std::string& usr)
        : request_id(id), operation_type(op), username(usr), priority(0) {}
};

struct OperationResult {
    uint64_t request_id;
    int status_code;
    std::string result_data;
    uint64_t processing_time_ms;
};

class FIFOQueue {
public:
    FIFOQueue(std::function<OperationResult(const Operation&)> worker_func);
    
    ~FIFOQueue();
    
    void enqueue(const Operation& op);
    
    std::shared_ptr<OperationResult> dequeue_result(uint64_t timeout_ms = 5000);
    
    size_t size();
    
    bool is_running();
    
    void wait_all(uint64_t timeout_ms = 30000);
    
    struct Stats {
        uint64_t total_operations;
        uint64_t operations_queued;
        uint64_t total_errors;
        uint64_t avg_processing_time;
    };
    
    Stats get_stats();

private:
    std::queue<Operation> operations;
    std::queue<std::shared_ptr<OperationResult>> results;
    std::mutex queue_mutex;
    std::mutex result_mutex;
    std::condition_variable queue_cv;
    std::condition_variable result_cv;
    std::function<OperationResult(const Operation&)> worker_function;
    bool running;
    
    uint64_t total_ops;
    uint64_t total_errors;
    uint64_t total_time;
    
    void worker_loop();
};

#endif
