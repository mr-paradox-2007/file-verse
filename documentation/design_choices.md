# Design Choices Documentation

## Data Structure Decisions

### 1. User Indexing
**Choice:** `std::unordered_map<std::string, UserInfo>`

**Rationale:**
- O(1) average-case lookup by username during login operations
- Essential for fast authentication in multi-user scenarios
- Hash table provides constant-time access for user validation
- Supports dynamic addition/removal of users without restructuring

**Trade-offs:**
- Memory overhead: Additional ~32 bytes per entry for hash table metadata
- Not cache-friendly for iteration (when listing all users)
- Acceptable because user operations are authentication-heavy, not iteration-heavy

### 2. File System Structure
**Choice:** `std::vector<FileEntry>` with in-memory file table

**Rationale:**
- Simple sequential storage matching .omni file layout
- Fast loading at startup (single sequential read)
- Linear search for file path lookup (O(n) but acceptable for ≤1000 files)
- Direct correspondence to disk structure simplifies persistence

**Trade-offs:**
- O(n) file lookup time (could use hash map for O(1))
- Chose simplicity over speed for educational clarity
- With max_files=1000, linear search is acceptable (<1ms typically)

### 3. Free Space Management
**Choice:** `std::vector<bool>` bitmap

**Rationale:**
- Memory efficient: 1 bit per block = 25,000 blocks / 8 = 3KB memory
- O(n) block allocation but only once per file creation
- Simple implementation: mark bit 0=used, 1=free
- Fast batch operations (can use bitwise operations if needed)

**Trade-offs:**
- Not optimal for finding N consecutive free blocks
- Could use free list or buddy system for better allocation
- Chose simplicity - fragmentation minimal with small file count

### 4. File Data Storage
**Choice:** In-memory `std::unordered_map<std::string, std::string>` for file contents

**Rationale:**
- Fast read/write operations for demo purposes
- Simplifies implementation by avoiding complex block chaining
- Direct path-to-data mapping
- Acceptable for Phase 1 with limited file sizes

**Trade-offs:**
- Memory usage: All file contents in RAM
- Not scalable to large files (limited by max_files * avg_file_size)
- Acceptable for 100MB total filesystem size

## .omni File Structure

### Physical Layout
```
Offset 0:     OMNIHeader (512 bytes)
Offset 512:   User Table (max_users * 128 bytes)
Offset N:     File Metadata Table (max_files * 416 bytes)
Offset M:     Free Space Bitmap (65536 bytes reserved)
Offset P:     Content Blocks (remaining space, 4KB per block)
```

### Serialization Strategy
**Binary format:** All structures written as raw bytes
**No byte alignment:** Packed structures, no padding
**Fixed sizes:** All fields have predetermined sizes
**Endianness:** Native (assumes little-endian Linux)

## Memory Management

### Loading Strategy (fs_init)
1. Read entire OMNIHeader (512 bytes)
2. Load all UserInfo structures into hash map
3. Load all FileEntry structures into vector
4. Build free space bitmap from file metadata
5. Keep all in memory for fast access

**Memory footprint:**
- Users: 50 * 128 bytes = 6.4 KB
- Files: 1000 * 416 bytes = 416 KB
- Bitmap: 3 KB
- Total metadata: ~425 KB (acceptable)

### Write-through Strategy
- Changes written to .omni file immediately on operation
- No caching layer (simplicity over performance)
- Ensures crash consistency at operation boundaries

## Network Architecture

### FIFO Queue
**Choice:** Producer-consumer with mutex + condition variable

**Rationale:**
- Thread-safe without complex locking schemes
- Natural FIFO ordering with std::queue
- Blocking wait with timeout prevents busy-waiting
- One producer (accept thread), one consumer (process thread)

**Implementation:**
```cpp
std::queue<Request> requests;
std::mutex request_mutex;
std::condition_variable request_cv;
```

### Multi-threading Model
**Choice:** Thread-per-client for connections, single processor thread

**Rationale:**
- Accept thread: Handles new connections only
- Client threads: Read requests, write responses (I/O bound)
- Process thread: Single sequential executor (ensures FIFO)
- Detached client threads (fire-and-forget after accept)

**Trade-offs:**
- Thread creation overhead per client
- Better: thread pool (but more complex for Phase 1)
- Acceptable for ≤20 concurrent clients

## Security Implementation

### Password Storage
**Choice:** SHA-256 with per-user salt

**Format:** `salt:hash` (32-byte salt + 64-byte hash = 96 bytes)

**Rationale:**
- Industry standard (SHA-256)
- Per-user salt prevents rainbow table attacks
- Simple to implement with OpenSSL
- Sufficient for educational project

**Process:**
1. Generate random 32-byte salt
2. Concatenate password + salt
3. Compute SHA-256 hash
4. Store as "salt:hash" string

### Session Management
**Choice:** Random 64-character session ID

**Rationale:**
- Cryptographically random (sufficient entropy)
- Stored in-memory hash map (session_id → UserSession)
- Timeout-based expiration (1 hour default)
- No persistent sessions (cleared on server restart)

## Performance Characteristics

| Operation        | Time Complexity | Space Complexity |
| ---------------- | --------------- | ---------------- |
| User login       | O(1) average    | O(users)         |
| File create      | O(n) files      | O(1)             |
| File read        | O(n) files      | O(file_size)     |
| File write       | O(n) files      | O(file_size)     |
| Directory list   | O(n) files      | O(matching)      |
| Block allocation | O(blocks)       | O(blocks)        |

**Bottlenecks:**
- File path lookup: O(n) linear search
- Block allocation: O(n) bitmap scan

**Acceptable because:**
- n ≤ 1000 files (max_files config)
- Operations complete in <10ms typically
- Educational project, not production system

## Phase 2 Considerations

### Reserved Space
- OMNIHeader has reserved fields for future use
- 20% of blocks marked "reserved" in bitmap
- File metadata includes unused fields for history linking

### Extensibility
- Hash map structure easily extendable
- Can add file_state_storage without restructuring
- Block chaining strategy supports delta storage
- Logging provides audit trail for change tracking

## Alternative Approaches Considered

### 1. B-Tree for File Index
**Rejected:** Too complex for Phase 1, O(log n) vs O(n) not significant for n=1000

### 2. Buddy System for Block Allocation
**Rejected:** Overkill for simple allocation, adds complexity without clear benefit

### 3. Write-back Caching
**Rejected:** Crash consistency concerns, complexity not justified for demo

### 4. Thread Pool
**Rejected:** Thread-per-client simpler, acceptable overhead for ≤20 clients

## Conclusion

Design prioritizes:
1. **Simplicity:** Educational clarity over optimal performance
2. **Correctness:** Thread-safe, crash-consistent operations
3. **Scalability:** Within project constraints (100MB, 1000 files, 20 clients)
4. **Extensibility:** Prepared for Phase 2 delta vault requirements

All choices documented with rationale and trade-offs explicitly stated.