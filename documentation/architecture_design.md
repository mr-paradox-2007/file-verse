# OFS Project - Design Choices and Architecture

## Overview

OFS (OMNI File System) is a complete file system implementation in C++17 with networking support. It demonstrates core filesystem concepts including metadata management, block allocation, user authentication, and network communication.

## Core Architecture

### 1. Filesystem Format (OMNI Format)

**File Structure**: `.omni` binary file containing complete filesystem state

```
┌─────────────────┐
│   OMNIHeader    │ (512 bytes)
│  - magic: 0xA1  │
│  - version      │
│  - total_size   │
│  - block_size   │
├─────────────────┤
│   User Table    │ (config.max_users * sizeof(UserInfo))
│  [UserInfo...]  │
├─────────────────┤
│   File Table    │ (config.max_files * sizeof(FileEntry))
│  [FileEntry...] │
├─────────────────┤
│  Free Bitmap    │ (bits = total_size / block_size / 8)
│  (1 bit/block)  │
├─────────────────┤
│  Data Blocks    │ (block_size bytes each)
│  [Block 0...]   │
└─────────────────┘
```

**Design Rationale**:
- Simple sequential layout for fast boot
- Bitmap for O(1) block allocation
- Separate user and file tables for O(1) lookups
- 100MB default size with 4KB blocks = ~25K blocks

### 2. User Authentication System

**Components**:
- SHA-256 password hashing with per-user salt
- Session management with timeout
- Role-based access control (ADMIN, USER, GUEST)

**Password Storage Format**:
```
password_hash = "salt:hash"
where:
  salt = random 32-byte value (hex encoded)
  hash = SHA256(salt + plaintext_password) (hex encoded)
Total: 128 bytes per user
```

**Design Rationale**:
- Per-user salt prevents rainbow table attacks
- SHA-256 standard cryptographic hash
- Session tokens for stateful authentication
- Activity timestamps for timeout detection

### 3. File System Manager

**Initialization Process**:
1. Load OMNIHeader and validate magic number
2. Load user table into memory (std::unordered_map)
3. Load file table into memory (std::vector)
4. Build free space bitmap
5. Initialize metadata indexes

**Data Structures**:
```cpp
struct FileEntry {
    char name[256];          // Filename
    uint32_t size;           // File size
    uint32_t created;        // Timestamp
    uint32_t modified;       // Timestamp
    uint8_t type;            // FILE or DIRECTORY
    uint8_t permissions;     // Octal permissions
    char owner[64];          // Owner username
    uint32_t first_block;    // First block location
};
```

**Design Rationale**:
- In-memory loading for speed (accept RAM overhead)
- HashMap for O(1) user lookups
- Vector for sequential file access
- Bitmap for instant free space queries

### 4. File Operations

**Supported Operations**:
- `createFile(path, owner, permissions)` - Create new file
- `deleteFile(path)` - Remove file and free blocks
- `readFile(path, data)` - Load file contents
- `writeFile(path, data)` - Update file
- `createDirectory(path, permissions)` - Create directory
- `deleteDirectory(path)` - Remove empty directory
- `listDirectory(path)` - Get directory contents

**Design Pattern**: 
- Validator -> Operation -> Logger pattern
- Separate methods for metadata and data operations
- All operations logged for debugging

### 5. Network Server Architecture

**Threading Model**:
```
┌──────────────────┐
│   Main Thread    │
│  (Server Loop)   │
└────────┬─────────┘
         │
    ┌────┴────┐
    ▼         ▼
┌────────┐ ┌──────────────┐
│ Accept │ │   Process    │
│Thread  │ │   Thread     │
└────┬───┘ └──────────────┘
     │
  ┌──┴──┬──┬──┐
  ▼     ▼  ▼  ▼
[Client Handlers] (detached threads)
```

**FIFO Queue Pattern**:
```cpp
struct Request {
    uint32_t request_id;
    std::string session_id;
    std::string operation;
    std::string data;
    uint64_t timestamp;
};

struct Response {
    uint32_t request_id;
    OFSErrorCodes status;
    std::string result;
    uint64_t timestamp;
};
```

**Design Rationale**:
- Producer-consumer pattern ensures ordered processing
- Thread-safe with mutex + condition_variable
- Timeout mechanism prevents hanging requests
- Statistics tracking for monitoring

### 6. CLI Client Design

**Command Parser**:
- Simple line-based command parsing
- Argument extraction using istringstream
- Support for quoted strings in data

**Connection Protocol**:
```
Client -> Server: "OPERATION|SESSION_ID|DATA"
Server -> Client: "SUCCESS|result" or "ERROR|message"
```

**Design Rationale**:
- Simple pipe-delimited protocol
- Easy to debug and extend
- Works over TCP without special libraries
- Human-readable in network traces

## Design Decisions

### Why In-Memory File System?

**Pros**:
- Fast operations (no disk I/O per request)
- Predictable performance
- Simple implementation

**Cons**:
- Data loss on crash (acceptable for demo)
- RAM limited (100MB filesystem)

**Choice Rationale**: Educational project focused on architecture, not persistence.

### Why C++17?

**Reasons**:
- Modern language features (auto, std::optional, std::filesystem)
- Standard template library (unordered_map, vector, thread)
- Good balance of power and safety
- Cross-platform compilation

### Why Singleton Pattern?

**Used for**:
- ConfigParser - single configuration per app
- Logger - unified logging
- UserManager - single user database
- FileSystemManager - single filesystem state
- NetworkServer - single server instance

**Rationale**: These are inherently single-instance components.

### Why Block-Based Storage?

**Over byte-addressed**:
- Simpler allocation (bitmap)
- Reduced fragmentation
- Standard filesystem design
- Easier metadata management

**Block Size (4KB)**:
- Matches typical page size
- Good balance of overhead vs. efficiency
- Same as ext4 default
- Suitable for 100MB filesystem

## Error Handling Strategy

**Error Codes**: Enum with negative values
```cpp
enum class OFSErrorCodes : int32_t {
    SUCCESS = 0,
    ERROR_INVALID_OPERATION = -1,
    ERROR_NOT_FOUND = -2,
    ERROR_FILE_EXISTS = -3,
    ERROR_NO_SPACE = -4,
    // ... more codes
};
```

**Logging Strategy**:
- All errors logged with context
- Error codes included in logs
- Categorized by module (FILE_OPS, NET_SRV, etc.)
- Structured format for parsing

## Performance Characteristics

| Operation      | Time         | Space      |
| -------------- | ------------ | ---------- |
| User lookup    | O(1) avg     | O(users)   |
| File lookup    | O(files)     | O(files)   |
| Block alloc    | O(1) avg     | O(blocks)  |
| File create    | O(1)         | O(1)       |
| File read      | O(data_size) | O(1)       |
| Directory list | O(files)     | O(matches) |

## Security Considerations

1. **Authentication**: SHA-256 with salt per user
2. **Session Management**: Timeout-based invalidation
3. **Access Control**: Basic permission bits
4. **Input Validation**: Path traversal checks
5. **No SSL/TLS**: Acceptable for local network demo

## Future Enhancements

1. **Persistence**: Add journal for crash recovery
2. **Concurrency**: Multi-reader, single-writer locking
3. **Caching**: LRU cache for frequently accessed files
4. **Compression**: Optional block-level compression
5. **Encryption**: Per-user key-based file encryption
6. **GUI Client**: Raylib-based interface
7. **SSL/TLS**: Encrypted network communication
