# Design Choices Document

## Overview

This document explains the data structures and design decisions made for the OFS (Omni File System) implementation.

## 1. User Management System

### Data Structure: Hash Map

**Choice**: `std::map<std::string, UserInfo>` for in-memory user storage

**Justification**:
- O(log n) lookup time for username-based authentication
- Automatic ordering by username for consistent iteration
- Built-in key-value association perfect for username to UserInfo mapping
- Thread-safe when combined with mutex locks

**Implementation Details**:
- Users loaded from disk at startup into memory map
- Hash computation for password verification using SHA-256
- Session tracking with separate map: `std::map<std::string, SessionData>`

**Trade-offs**:
- Memory: All users loaded into RAM (acceptable for 50 max users)
- Speed: Fast login/logout operations
- Persistence: Changes written back to disk on modification

### Session Management

**Choice**: Dual tracking system
1. Session ID to User mapping: `std::map<std::string, SessionData>`
2. Active users set: `std::set<std::string>`

**Justification**:
- Prevents duplicate logins (set ensures uniqueness)
- Fast session validation
- Tracks login time and last activity
- Thread-safe with mutex protection

**Session ID Generation**:
```
session_id = username + "_" + timestamp + "_" + random_number
```

## 2. File System Structure

### Metadata Index

**Choice**: Fixed-size array of MetadataEntry structures

```cpp
struct MetadataEntry {
    uint8_t valid;           // 0=free, 1=used
    uint8_t type;            // 0=file, 1=directory
    uint32_t parent_index;   // Parent directory entry
    char name[32];           // Short filename
    uint32_t start_block;    // First block index
    uint64_t total_size;     // File size in bytes
    uint32_t owner_id;       // Owner user ID
    uint32_t permissions;    // Unix-style permissions
    uint64_t created_time;   // Creation timestamp
    uint64_t modified_time;  // Modification timestamp
};
```

**Justification**:
- Fixed size enables direct indexing: O(1) access by entry index
- Array stored contiguously in .omni file for efficient disk I/O
- 8192 entries maximum provides ample capacity
- Parent index creates tree structure for directories

**Path Resolution Algorithm**:
```
1. Start at root (index 0)
2. Split path into components
3. For each component:
   - List children of current directory
   - Find child matching component name
   - Move to that child's index
4. Return final index or error if not found
```

Time Complexity: O(d * c) where d = depth, c = children per directory

### Directory Tree Representation

**Choice**: Parent-child relationship via parent_index field

**Justification**:
- Simple and efficient storage
- Natural tree traversal
- Easy to implement directory operations
- Minimal memory overhead

**Directory Listing**:
- Scan all metadata entries
- Filter by parent_index matching current directory
- Return matching entries

Time Complexity: O(n) where n = total entries (acceptable for max 8192 entries)

**Optimization Consideration**:
For larger systems, could add:
- Child index list stored in directory blocks
- Hash table for path to index mapping
- Directory entry caching

## 3. Block Storage System

### Block Allocation

**Choice**: Linked list with inline next-block pointers

```cpp
struct BlockHeader {
    uint32_t next_block;     // Index of next block (0=end)
    uint32_t data_size;      // Actual data in this block
};
```

**Justification**:
- Simple implementation
- No external allocation tables needed
- Follows file data naturally
- Efficient for sequential reads

**Block Size**: 64KB (65536 bytes)
- Large enough to minimize overhead
- Small enough for efficient memory usage
- Standard power-of-2 size

### Free Space Tracking

**Choice**: Bitmap (vector of uint8_t)

```cpp
std::vector<uint8_t> block_bitmap;
// 0 = free, 1 = used
```

**Justification**:
- Space efficient: 1 byte per block
- Fast allocation: linear scan for free blocks
- Simple deallocation: set bit to 0
- Easy to persist to disk

**Allocation Algorithm**:
```
1. Scan bitmap for first 0 bit
2. Set bit to 1
3. Return block index
```

Time Complexity: O(n) worst case, O(1) average case with free blocks available

**Optimization**:
- Track last allocated position to avoid re-scanning
- Could add free block counter for quick space check

## 4. File I/O Strategy

### Data Encoding

**Choice**: Simple byte substitution cipher

```cpp
uint8_t encryption_table[256];
uint8_t decryption_table[256];
```

**Justification**:
- Meets basic encryption requirement
- Symmetric: same operation for encode/decode
- Fast: O(n) time for n bytes
- Deterministic: no key management complexity

**Implementation**:
```
encode: byte = (byte + 73) % 256
decode: byte = (byte - 73 + 256) % 256
```

### Read/Write Operations

**File Write**:
1. Delete existing blocks if file exists
2. Split data into 64KB chunks
3. Allocate block for each chunk
4. Encode data before writing
5. Link blocks via next_block pointers
6. Update metadata entry

**File Read**:
1. Get start_block from metadata
2. Read first block
3. Decode data
4. Follow next_block chain
5. Accumulate data until next_block = 0

Time Complexity: O(b) where b = number of blocks

### Caching Strategy

**Current Implementation**: Metadata cached in memory

**Cache Invalidation**: Write-through cache
- Changes written to disk immediately
- Metadata cache updated in memory
- Bitmap persisted on each allocation/deallocation

**Future Enhancement**:
- Block cache for frequently accessed files
- LRU eviction policy
- Delayed write-back for better performance

## 5. Concurrency Control

### Threading Model

**Choice**: Thread-per-connection with mutex protection

**Critical Sections**:
```cpp
pthread_mutex_t session_mutex;  // Session operations
std::mutex g_storage_mutex;     // File system operations
```

**Justification**:
- Simple to implement
- Adequate for moderate concurrency
- Prevents race conditions
- Ensures data consistency

### FIFO Operation Processing

**Implementation**: Sequential processing within thread

**Justification**:
- No operation ordering issues
- Simple to debug
- Consistent state across operations
- Acceptable latency for file operations

**Future Enhancement**:
- Read-write locks for concurrent reads
- Per-directory locking for better concurrency
- Atomic operations for metadata updates

## 6. Memory Management

### Memory vs Disk Trade-offs

**In Memory**:
- All user accounts (max 50)
- All metadata entries (8192 entries = ~1MB)
- Bitmap (size varies with total blocks)
- Active sessions

**On Disk**:
- File content (stored in blocks)
- Persistent metadata copy
- Persistent bitmap copy

**Justification**:
- Fast operations on metadata
- Minimal memory footprint
- Persistent storage for reliability

**Memory Usage Estimation**:
```
Users: 50 * 256 bytes = ~13KB
Metadata: 8192 * 128 bytes = ~1MB
Bitmap: 1600 blocks = ~1.6KB (for 100MB system)
Sessions: negligible
Total: ~1MB + session overhead
```

## 7. Error Handling

### Error Code System

**Choice**: Enum-based error codes

```cpp
enum class OFSErrorCodes : int32_t {
    SUCCESS = 0,
    ERROR_NOT_FOUND = -1,
    ERROR_PERMISSION_DENIED = -2,
    // ... more error codes
};
```

**Justification**:
- Type-safe error handling
- Consistent across system
- Easy to extend
- Clear semantics

### Edge Cases Handled

1. **Duplicate Login**
   - Check active_users set before allowing login
   - Return error if user already logged in

2. **Invalid Session**
   - Verify session exists in session map
   - Return error if session expired or invalid

3. **File Already Exists**
   - Check path before creating file
   - Return ERROR_FILE_EXISTS

4. **Directory Not Empty**
   - Verify no children before deleting directory
   - Return ERROR_DIRECTORY_NOT_EMPTY

5. **No Free Space**
   - Check bitmap before allocation
   - Return ERROR_NO_SPACE if full

6. **Invalid Path**
   - Validate path format
   - Check for path traversal attempts
   - Return ERROR_INVALID_PATH

## 8. Phase 2 Preparation

### Reserved Fields

**OMNIHeader**:
```cpp
uint32_t file_state_storage_offset;
uint32_t change_log_offset;
```

**Design Consideration**:
- Space reserved for version control metadata
- Offset-based addressing for flexibility
- Can add delta storage without changing header size

**Future Delta Vault Implementation**:
- Forward deltas: store changes from base version
- Change log: linked list of modifications
- State reconstruction: apply deltas sequentially

## 9. Performance Characteristics

### Time Complexity Summary

| Operation       | Time Complexity | Notes                                |
| --------------- | --------------- | ------------------------------------ |
| User Login      | O(log n)        | Map lookup                           |
| File Create     | O(d + b)        | Path resolution + block allocation   |
| File Read       | O(d + b)        | Path resolution + block traversal    |
| File Delete     | O(d + b)        | Path resolution + block deallocation |
| Directory List  | O(n)            | Scan all metadata entries            |
| Path Resolution | O(d * c)        | Depth * children per level           |

### Space Complexity

| Component  | Space Usage | Notes               |
| ---------- | ----------- | ------------------- |
| Metadata   | O(m)        | m = max files       |
| Bitmap     | O(b)        | b = total blocks    |
| User Cache | O(u)        | u = max users       |
| Sessions   | O(s)        | s = active sessions |

## 10. Summary

The design prioritizes:
1. **Simplicity**: Easy to understand and maintain
2. **Correctness**: Proper error handling and validation
3. **Performance**: Adequate for target workload
4. **Extensibility**: Ready for Phase 2 enhancements

Key strengths:
- Fast user authentication
- Efficient metadata access
- Simple block management
- Thread-safe operations

Areas for enhancement:
- Directory listing performance (add indexing)
- Block allocation (track free list head)
- Caching (add block cache)
- Concurrency (read-write locks)