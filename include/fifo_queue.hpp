#ifndef FIFO_QUEUE_HPP
#define FIFO_QUEUE_HPP

#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <functional>
#include <string>
#include <cstdint>

/**
 * Request/Operation structure for FIFO queue
 * Represents a single file system operation to be processed
 */
struct Operation {
    uint64_t request_id;           // Unique request ID
    std::string operation_type;    // "file_create", "file_read", "user_login", etc.
    std::string path;              // File path (empty for non-file operations)
    std::string username;          // User performing operation
    std::string data;              // Operation data (file content, parameters, etc.)
    uint32_t priority;             // Priority (0 = normal, higher = more urgent)
    
    Operation(uint64_t id, const std::string& op, const std::string& usr)
        : request_id(id), operation_type(op), username(usr), priority(0) {}
};

/**
 * Result structure returned after operation completes
 */
struct OperationResult {
    uint64_t request_id;           // Request ID of completed operation
    int status_code;               // 0 for success, negative for error
    std::string result_data;       // Result data or error message
    uint64_t processing_time_ms;   // How long operation took
};

/**
 * Thread-safe FIFO queue for sequential operation processing
 * 
 * All file system operations are added to this queue and processed
 * one-at-a-time in FIFO order. This ensures data consistency without
 * complex locking mechanisms.
 */
class FIFOQueue {
public:
    /**
     * Initialize queue and start worker thread
     * worker_func: Function that processes each operation
     */
    FIFOQueue(std::function<OperationResult(const Operation&)> worker_func);
    
    /**
     * Shutdown queue and wait for current operation to complete
     */
    ~FIFOQueue();
    
    /**
     * Add operation to queue (thread-safe)
     * Returns immediately, operation processed asynchronously
     */
    void enqueue(const Operation& op);
    
    /**
     * Get result of completed operation (blocking)
     * Waits up to timeout_ms for result
     * Returns nullptr if timeout or queue shutdown
     */
    std::shared_ptr<OperationResult> dequeue_result(uint64_t timeout_ms = 5000);
    
    /**
     * Get current queue size
     */
    size_t size();
    
    /**
     * Check if queue is running
     */
    bool is_running();
    
    /**
     * Wait for all operations to complete
     */
    void wait_all(uint64_t timeout_ms = 30000);
    
    /**
     * Get statistics
     */
    struct Stats {
        uint64_t total_operations;     // Total operations processed
        uint64_t operations_queued;    // Current items in queue
        uint64_t total_errors;         // Total failed operations
        uint64_t avg_processing_time;  // Average processing time in ms
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
    
    // Statistics
    uint64_t total_ops;
    uint64_t total_errors;
    uint64_t total_time;
    
    /**
     * Worker thread main loop
     */
    void worker_loop();
};

#endif
