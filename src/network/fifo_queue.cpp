#include "fifo_queue.hpp"
#include <thread>
#include <chrono>

FIFOQueue::FIFOQueue(std::function<OperationResult(const Operation&)> worker_func)
    : worker_function(worker_func), running(true), total_ops(0), total_errors(0), total_time(0) {
    
    // Start worker thread
    std::thread worker(&FIFOQueue::worker_loop, this);
    worker.detach();  // Run in background
}

FIFOQueue::~FIFOQueue() {
    running = false;
    queue_cv.notify_all();
    
    // Wait briefly for pending operations
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

void FIFOQueue::enqueue(const Operation& op) {
    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        operations.push(op);
    }
    queue_cv.notify_one();
}

std::shared_ptr<OperationResult> FIFOQueue::dequeue_result(uint64_t timeout_ms) {
    std::unique_lock<std::mutex> lock(result_mutex);
    
    if (result_cv.wait_for(lock, std::chrono::milliseconds(timeout_ms),
                          [this] { return !results.empty(); })) {
        auto result = results.front();
        results.pop();
        return result;
    }
    
    return nullptr;
}

size_t FIFOQueue::size() {
    std::lock_guard<std::mutex> lock(queue_mutex);
    return operations.size();
}

bool FIFOQueue::is_running() {
    return running;
}

void FIFOQueue::wait_all(uint64_t timeout_ms) {
    auto deadline = std::chrono::system_clock::now() + 
                   std::chrono::milliseconds(timeout_ms);
    
    while (true) {
        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            if (operations.empty()) {
                break;
            }
        }
        
        if (std::chrono::system_clock::now() > deadline) {
            break;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

FIFOQueue::Stats FIFOQueue::get_stats() {
    Stats stats;
    
    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        stats.operations_queued = operations.size();
    }
    
    stats.total_operations = total_ops;
    stats.total_errors = total_errors;
    stats.avg_processing_time = (total_ops > 0) ? (total_time / total_ops) : 0;
    
    return stats;
}

void FIFOQueue::worker_loop() {
    while (running) {
        Operation op(0, "", "");
        
        // Wait for operation to be queued
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            
            if (queue_cv.wait_for(lock, std::chrono::seconds(1),
                                 [this] { return !operations.empty() || !running; })) {
                
                if (!operations.empty()) {
                    op = operations.front();
                    operations.pop();
                } else {
                    continue;
                }
            } else {
                continue;  // Timeout, check running flag
            }
        }
        
        if (!running) {
            break;
        }
        
        // Execute operation
        auto start_time = std::chrono::system_clock::now();
        OperationResult result = worker_function(op);
        auto end_time = std::chrono::system_clock::now();
        
        uint64_t elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                             end_time - start_time).count();
        
        result.processing_time_ms = elapsed_ms;
        
        // Update statistics
        total_ops++;
        if (result.status_code != 0) {
            total_errors++;
        }
        total_time += elapsed_ms;
        
        // Store result
        {
            std::lock_guard<std::mutex> lock(result_mutex);
            results.push(std::make_shared<OperationResult>(result));
        }
        result_cv.notify_one();
    }
}
