#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>
#include "../source/include/network_server.hpp"
#include "../source/include/fifo_queue.hpp"
#include "../source/include/logger.hpp"

using namespace ofs;

void printTestHeader(const std::string& test_name) {
    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << "  " << test_name << std::endl;
    std::cout << std::string(70, '=') << std::endl;
}

void printResult(bool passed, const std::string& message) {
    std::cout << (passed ? "  ✓ " : "  ✗ ") << message << std::endl;
}

bool testFIFOQueue() {
    printTestHeader("Test 1: FIFO Queue");

    FIFOQueue& queue = FIFOQueue::getInstance();
    queue.clear();

    Request req1;
    req1.request_id = 1;
    req1.operation = "file_create";
    req1.data = "/test.txt";
    req1.timestamp = std::time(nullptr);

    bool enqueued = queue.enqueueRequest(req1);
    printResult(enqueued, "Enqueue request");

    uint32_t size = queue.getRequestQueueSize();
    printResult(size == 1, "Request queue size is 1");

    Request req_out;
    bool dequeued = queue.dequeueRequest(req_out, 100);
    printResult(dequeued && req_out.request_id == 1, "Dequeue request successfully");

    size = queue.getRequestQueueSize();
    printResult(size == 0, "Request queue empty after dequeue");

    return true;
}

bool testQueueTimeout() {
    printTestHeader("Test 2: Queue Timeout");

    FIFOQueue& queue = FIFOQueue::getInstance();
    queue.clear();

    Request req;
    bool result = queue.dequeueRequest(req, 50);
    printResult(!result, "Dequeue timeout on empty queue");

    return true;
}

bool testResponseQueue() {
    printTestHeader("Test 3: Response Queue");

    FIFOQueue& queue = FIFOQueue::getInstance();
    queue.clear();

    Response resp;
    resp.request_id = 100;
    resp.status = OFSErrorCodes::SUCCESS;
    resp.result = "File created successfully";
    resp.timestamp = std::time(nullptr);

    bool enqueued = queue.enqueueResponse(resp);
    printResult(enqueued, "Enqueue response");

    uint32_t size = queue.getResponseQueueSize();
    printResult(size == 1, "Response queue size is 1");

    Response resp_out;
    bool dequeued = queue.dequeueResponse(resp_out, 100);
    printResult(dequeued && resp_out.request_id == 100, "Dequeue response successfully");

    return true;
}

bool testConcurrentQueueing() {
    printTestHeader("Test 4: Concurrent Queue Operations");

    FIFOQueue& queue = FIFOQueue::getInstance();
    queue.clear();

    auto producer = [&]() {
        for (int i = 0; i < 10; i++) {
            Request req;
            req.request_id = i;
            req.operation = "test_op_" + std::to_string(i);
            req.timestamp = std::time(nullptr);
            queue.enqueueRequest(req);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    };

    auto consumer = [&]() {
        int count = 0;
        for (int i = 0; i < 10; i++) {
            Request req;
            if (queue.dequeueRequest(req, 500)) {
                count++;
            }
        }
        return count;
    };

    std::thread prod_thread(producer);
    std::thread cons_thread(consumer);

    prod_thread.join();
    cons_thread.join();

    uint32_t remaining = queue.getRequestQueueSize();
    printResult(remaining == 0, "All requests processed (concurrent)");

    return true;
}

bool testServerInitialization() {
    printTestHeader("Test 5: Server Initialization");

    NetworkServer& server = NetworkServer::getInstance();

    Config config;
    config.port = 8080;
    config.max_users = 50;
    config.total_size = 104857600;
    config.block_size = 4096;

    OFSErrorCodes result = server.initialize(config);
    printResult(result == OFSErrorCodes::SUCCESS, "Initialize network server");

    printResult(!server.isRunning(), "Server not running yet");

    return true;
}

bool testQueueClear() {
    printTestHeader("Test 6: Queue Clear");

    FIFOQueue& queue = FIFOQueue::getInstance();

    Request req1, req2;
    req1.request_id = 1;
    req2.request_id = 2;
    req1.timestamp = std::time(nullptr);
    req2.timestamp = std::time(nullptr);

    queue.enqueueRequest(req1);
    queue.enqueueRequest(req2);

    uint32_t before = queue.getRequestQueueSize();
    printResult(before == 2, "Queue has 2 items");

    queue.clear();

    uint32_t after = queue.getRequestQueueSize();
    printResult(after == 0, "Queue cleared successfully");

    return true;
}

bool testMixedOperations() {
    printTestHeader("Test 7: Mixed Queue Operations");

    FIFOQueue& queue = FIFOQueue::getInstance();
    queue.clear();

    Request req;
    req.request_id = 42;
    req.operation = "file_read";
    req.timestamp = std::time(nullptr);

    queue.enqueueRequest(req);

    Response resp;
    resp.request_id = 42;
    resp.status = OFSErrorCodes::SUCCESS;
    resp.result = "File contents";
    resp.timestamp = std::time(nullptr);

    queue.enqueueResponse(resp);

    printResult(queue.getRequestQueueSize() == 1, "Request queue size is 1");
    printResult(queue.getResponseQueueSize() == 1, "Response queue size is 1");

    Request req_out;
    queue.dequeueRequest(req_out);

    Response resp_out;
    queue.dequeueResponse(resp_out);

    printResult(queue.getRequestQueueSize() == 0 && 
                queue.getResponseQueueSize() == 0, 
                "Both queues empty after operations");

    return true;
}

int main() {
    std::cout << "\n╔════════════════════════════════════════════════════════════════╗"
              << std::endl;
    std::cout << "║         Network Server & FIFO Queue - Test Suite               ║"
              << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════════════╝"
              << std::endl;

    testFIFOQueue();
    testQueueTimeout();
    testResponseQueue();
    testConcurrentQueueing();
    testServerInitialization();
    testQueueClear();
    testMixedOperations();

    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << "  All Tests Completed" << std::endl;
    std::cout << std::string(70, '=') << std::endl << std::endl;

    LOG_INFO("TEST", 0, "Network server and queue test suite completed");

    return 0;
}
