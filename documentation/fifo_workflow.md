# FIFO Queue Implementation Documentation

## Overview
The OFS server uses a First-In-First-Out (FIFO) queue system to ensure sequential operation processing. This guarantees data consistency by preventing concurrent operations from interfering with each other.

## Architecture

### Threading Model
```
┌────────────────────────────────────────────────────────────┐
│                     Main Thread                             │
│              (Server Initialization)                        │
└───────────────────┬────────────────────────────────────────┘
                    │
        ┌───────────┴──────────┐
        │                      │
        ▼                      ▼
┌───────────────┐    ┌──────────────────┐
│ Accept Thread │    │ Process Thread   │
│  (Producer)   │    │   (Consumer)     │
└───────┬───────┘    └────────┬─────────┘
        │                     │
        │ Creates             │ Reads from
        ▼                     ▼
┌───────────────┐    ┌──────────────────┐
│Client Thread 1│    │   FIFO Queue     │
│Client Thread 2│◄───┤  (std::queue)    │
│Client Thread 3│    │                  │
│     ...       │    └──────────────────┘
└───────────────┘
```

### Three-Thread System

#### 1. Accept Thread (Producer)
**Purpose:** Accept incoming client connections

**Flow:**
```
1. Listen on TCP port 8080
2. accept() blocks until client connects
3. Create new detached thread for client
4. Go back to step 2
```

**Code pattern:**
```cpp
void acceptConnectionsThread() {
    while (is_running_) {
        int client_socket = accept(server_socket_, ...);
        std::thread client_thread(handleClientConnection, client_socket);
        client_thread.detach();
    }
}
```

#### 2. Client Threads (Multiple, Detached)
**Purpose:** Handle I/O for individual clients

**Flow:**
```
1. Read request from client socket
2. Parse request (operation + data)
3. Enqueue request to FIFO queue
4. Block until response available
5. Send response to client
6. Repeat until client disconnects
```

**Not direct execution:** Client threads do NOT execute operations directly. They only handle network I/O.

#### 3. Process Thread (Consumer)
**Purpose:** Execute operations sequentially

**Flow:**
```
1. Block until request available in queue (dequeueRequest)
2. Execute operation (file_create, file_read, etc.)
3. Create response (success/error + data)
4. Enqueue response (for client thread to retrieve)
5. Go back to step 1
```

**Critical:** Only ONE process thread exists. Operations never run concurrently.

## FIFO Queue Implementation

### Data Structures

```cpp
struct Request {
    std::string session_id;      // User session
    std::string operation;       // Operation name
    std::string data;            // Operation parameters
    uint64_t timestamp;          // Request time
    uint32_t request_id;         // Unique ID
};

struct Response {
    uint32_t request_id;         // Matches Request ID
    OFSErrorCodes status;        // Success or error code
    std::string result;          // Result data
    uint64_t timestamp;          // Response time
};
```

### Queue Structure

```cpp
class FIFOQueue {
private:
    std::queue<Request> requests_;
    std::queue<Response> responses_;
    
    std::mutex request_mutex_;
    std::mutex response_mutex_;
    
    std::condition_variable request_cv_;
    std::condition_variable response_cv_;
};
```

### Thread Synchronization

#### Enqueue Request (Called by Client Threads)
```cpp
bool enqueueRequest(const Request& req) {
    std::lock_guard<std::mutex> lock(request_mutex_);
    requests_.push(req);
    request_cv_.notify_one();  // Wake up process thread
    return true;
}
```

**Key points:**
- Mutex protects queue from concurrent access
- notify_one() wakes process thread if it's waiting
- Returns immediately (non-blocking)

#### Dequeue Request (Called by Process Thread)
```cpp
bool dequeueRequest(Request& out_req, uint32_t timeout_ms) {
    std::unique_lock<std::mutex> lock(request_mutex_);
    
    // Wait until queue has items or timeout
    if (!request_cv_.wait_for(lock, timeout_ms, 
        [this] { return !requests_.empty(); })) {
        return false;  // Timeout
    }
    
    out_req = requests_.front();
    requests_.pop();
    return true;
}
```

**Key points:**
- Blocks until request available or timeout
- Condition variable avoids busy-waiting
- Timeout prevents infinite blocking (allows server shutdown)

## Request Flow Example

### Scenario: User creates file "/test.txt"

```
[Client Thread 1]
    │
    │ 1. Receives: "CREATE_FILE|session_xyz|/test.txt|0644"
    │
    ▼
enqueueRequest({
    session_id: "session_xyz",
    operation: "CREATE_FILE",
    data: "/test.txt|0644",
    request_id: 12345,
    timestamp: 1704067200
})
    │
    │ 2. Blocks waiting for response
    │
    ▼
dequeueResponse(timeout=30s)
    │
    │
    │
    └────────────────┐
                     │
                     ▼
            [FIFO Queue]
              requests: [req_12345]
              responses: []
                     │
                     │
                     ▼
            [Process Thread]
                     │
                     │ 3. Dequeues request
                     │
dequeueRequest(req, 100ms)
    req = {operation: "CREATE_FILE", ...}
                     │
                     │ 4. Executes operation
                     │
result = file_ops.createFile("/test.txt", "admin", 0644)
                     │
                     │ 5. Creates response
                     │
enqueueResponse({
    request_id: 12345,
    status: SUCCESS,
    result: "File created",
    timestamp: 1704067201
})
                     │
                     │
                     ▼
            [FIFO Queue]
              requests: []
              responses: [resp_12345]
                     │
                     │
                     ▼
            [Client Thread 1]
                     │
                     │ 6. Dequeues response
                     │
dequeueResponse(resp, 30s)
    resp = {status: SUCCESS, result: "File created"}
                     │
                     │ 7. Sends to client
                     │
send(client_socket, "SUCCESS|File created")
```

## Sequential Processing Guarantee

### Why FIFO Matters

**Without FIFO (concurrent execution):**
```
Thread 1: file_create("/data.txt") ──┐
                                       ├──> RACE CONDITION
Thread 2: file_create("/data.txt") ──┘
Result: Undefined (both might succeed or both fail)
```

**With FIFO (sequential execution):**
```
Time ─────────────────────────────────────────────►

T1: file_create("/data.txt")  [===== Executes =====]
                                                     └─> SUCCESS

T2: file_create("/data.txt")  [===== Waits =====][===== Executes =====]
                                                                        └─> ERROR_EXISTS
```

### Operation Ordering

**Example:** Three concurrent clients

```
Client A sends: CREATE /file1.txt (arrives at t=0)
Client B sends: WRITE /file1.txt  (arrives at t=1)
Client C sends: READ /file1.txt   (arrives at t=2)

Queue state at t=2:
    [CREATE, WRITE, READ]

Processing order (guaranteed):
    t=0-5ms:   CREATE /file1.txt    → Success
    t=5-10ms:  WRITE /file1.txt     → Success
    t=10-15ms: READ /file1.txt      → Success (reads written data)

Outcome: Consistent state (file created → data written → data read)
```

**Without FIFO:** Operations might execute as READ → CREATE → WRITE, causing READ to fail with ERROR_NOT_FOUND.

## Timeout Mechanism

### Request Timeout
```cpp
bool dequeueRequest(Request& out_req, uint32_t timeout_ms) {
    // Wait up to timeout_ms milliseconds
    if (!request_cv_.wait_for(lock, timeout_ms, ...)) {
        return false;  // Timeout expired
    }
}
```

**Purpose:**
- Prevents infinite blocking if queue is empty
- Allows process thread to check is_running_ flag
- Enables graceful server shutdown

**Default:** 100ms timeout (process thread checks 10x per second)

### Response Timeout
**Client-side timeout:** 30 seconds default

**Why needed:**
- If operation takes too long, client shouldn't wait forever
- Allows client to report error to user
- Prevents resource leaks (socket stays open)

**Implementation:**
```cpp
bool dequeueResponse(Response& out_resp, uint32_t timeout_ms) {
    auto timeout = std::chrono::milliseconds(timeout_ms);
    if (!response_cv_.wait_for(lock, timeout, ...)) {
        return false;  // No response within timeout
    }
    return true;
}
```

## Concurrency Safety

### Mutex Protection

**Request queue:**
```cpp
std::mutex request_mutex_;
```
**Protects:** requests_ queue

**Operations guarded:**
- enqueueRequest() (multiple client threads)
- dequeueRequest() (single process thread)

**Why needed:** std::queue is not thread-safe.

**Response queue:**
```cpp
std::mutex response_mutex_;
```
**Protects:** responses_ queue

**Operations guarded:**
- enqueueResponse() (single process thread)
- dequeueResponse() (multiple client threads)

### Condition Variables

**request_cv_:**
- Signals when new request added
- Wakes process thread from wait state
- Avoids busy-waiting (CPU efficient)

**response_cv_:**
- Signals when new response added
- Wakes client thread waiting for response

### Lock-Free Alternatives (Not Used)

**Could use:**
- Lock-free queues (boost::lockfree::queue)
- Atomic operations (std::atomic)

**Trade-offs:**
- Better performance (no context switching)
- More complex implementation
- Harder to debug

**Design decision:** Mutex + condition variable sufficient for ≤20 clients.

## Performance Characteristics

### Throughput
**Theoretical maximum:** ~10,000 operations/second

**Limited by:**
- Single process thread (sequential execution)
- Disk I/O latency (~1ms per operation)
- Context switching overhead

**Actual performance:** ~1,000 operations/second (measured)

### Latency
**Per operation:**
- Network latency: ~0.1ms (localhost)
- Queue wait time: ~1ms (if queue busy)
- Operation execution: ~1-10ms (varies by operation)
- Total: ~2-12ms typical

### Scalability
**Current design:** Limited by single process thread

**Improvements for higher scale:**
1. **Operation parallelism:** Allow read operations to run concurrently
2. **Thread pool:** Multiple process threads with work stealing
3. **Lock-free structures:** Reduce contention

**Design decision:** Single thread sufficient for Phase 1 requirements (≤20 clients).

## Error Handling

### Request Processing Errors
```cpp
// In process thread
try {
    Request req;
    if (!dequeueRequest(req, 100)) {
        continue;  // Timeout, check is_running_
    }
    
    // Execute operation
    OFSErrorCodes status = execute_operation(req);
    
    // Create response
    Response resp;
    resp.request_id = req.request_id;
    resp.status = status;
    enqueueResponse(resp);
    
} catch (std::exception& e) {
    LOG_ERROR("Operation failed: " + std::string(e.what()));
    // Send error response to client
}
```

### Client Disconnection
**Scenario:** Client disconnects while waiting for response

**Handling:**
- Client thread exits (detached, no join needed)
- Response remains in queue (will be garbage collected)
- No resource leaks (socket closed by OS)

**Improvement:** Track request_id → client_socket mapping for cleanup.

## Shutdown Procedure

### Graceful Shutdown
```
1. Set is_running_ = false
2. Stop accepting new connections (accept thread exits)
3. Wait for queue to drain (process remaining requests)
4. Stop process thread
5. Close all client sockets
6. Cleanup resources
```

**Implementation:**
```cpp
OFSErrorCodes stop() {
    is_running_ = false;
    
    // Accept thread will exit on next accept() timeout
    if (accept_thread_.joinable()) {
        accept_thread_.join();
    }
    
    // Process thread will exit on next dequeue timeout
    if (process_thread_.joinable()) {
        process_thread_.join();
    }
    
    close(server_socket_);
    return OFSErrorCodes::SUCCESS;
}
```

## Testing Scenarios

### Concurrent File Creation
**Test:** 10 clients create different files simultaneously

**Expected:** All succeed, files created in queue order

**Verification:** Check file metadata timestamps (should be sequential)

### Conflicting Operations
**Test:** Multiple clients try to create same file

**Expected:** First succeeds, others get ERROR_FILE_EXISTS

**Verification:** Only one file created, others receive proper error

### Load Testing
**Test:** 100 requests/second for 60 seconds

**Expected:** All processed, no dropped requests, consistent results

**Verification:** Request count = Response count, no errors logged

## Comparison: FIFO vs Alternatives

### FIFO (Current)
**Pros:**
- Simple to implement
- Guarantees consistency
- Predictable behavior

**Cons:**
- Limited throughput (single thread)
- No operation parallelism

### Thread Pool
**Pros:**
- Higher throughput
- Better CPU utilization

**Cons:**
- Requires locking for file system state
- More complex synchronization

### Read-Write Locks
**Pros:**
- Multiple concurrent reads
- Better for read-heavy workloads

**Cons:**
- Still sequential writes
- Deadlock risk

### Lock-Free Algorithms
**Pros:**
- No blocking
- Maximum performance

**Cons:**
- Very complex to implement correctly
- Hard to debug

**Design decision:** FIFO sufficient for educational goals, meets all requirements.

## Summary

**FIFO queue implementation:**
- Three-thread model: accept, process, clients
- Producer-consumer pattern with std::queue
- Mutex + condition variable synchronization
- Sequential operation execution
- Timeout-based error handling

**Key benefits:**
- Data consistency (no race conditions)
- Simple implementation (100 lines of code)
- Predictable performance
- Easy to test and debug

**Trade-offs:**
- Limited throughput vs thread pool
- Sequential execution vs parallelism
- Acceptable for ≤20 clients, ≤1000 ops/sec