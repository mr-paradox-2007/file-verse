# File I/O Strategy Documentation

## Overview
OFS uses a single binary .omni file for all data storage. This document describes the physical layout, serialization approach, and I/O operations.

## Physical File Layout

### Complete Structure
```
┌─────────────────────────────────────────────────────────────┐
│ Offset 0x0000: OMNIHeader (512 bytes)                      │
│   - Magic: "OMNIFS01" (8 bytes)                            │
│   - Format version: 0x00010000 (4 bytes)                   │
│   - Total size, block size, offsets (remaining)            │
├─────────────────────────────────────────────────────────────┤
│ Offset 0x0200: User Table (N * 128 bytes)                  │
│   - N = max_users from config (default 50)                 │
│   - Each UserInfo: 128 bytes fixed size                    │
│   - Unused slots: zeroed out (is_active = 0)               │
├─────────────────────────────────────────────────────────────┤
│ Offset X: File Metadata Table (M * 416 bytes)              │
│   - M = max_files from config (default 1000)               │
│   - Each FileEntry: 416 bytes fixed size                   │
│   - Empty entries: name[0] = '\0'                          │
├─────────────────────────────────────────────────────────────┤
│ Offset Y: Free Space Bitmap (65536 bytes)                  │
│   - Bit array: 1 bit per block                             │
│   - 0 = block used, 1 = block free                         │
│   - 65536 bytes = 524,288 bits = 524,288 blocks max        │
├─────────────────────────────────────────────────────────────┤
│ Offset Z: Content Blocks (remaining space)                 │
│   - Block size: 4096 bytes each (from config)              │
│   - Number of blocks: (total_size - Z) / 4096              │
│   - Each block contains raw file data                      │
└─────────────────────────────────────────────────────────────┘
```

## Serialization Strategy

### Structure Packing
**Approach:** Fixed-size structures with no padding

```cpp
struct OMNIHeader {
    char magic[8];              // 8 bytes
    uint32_t format_version;    // 4 bytes
    uint64_t total_size;        // 8 bytes
    // ... remaining fields
} __attribute__((packed));
```

**Key decisions:**
- No C struct padding (use __attribute__((packed)) if needed)
- Fixed-size arrays for strings (char[N])
- All integers in native byte order (little-endian on x86-64)
- Binary write: `write(fd, &struct, sizeof(struct))`

### Standard Structure Sizes
- OMNIHeader: 512 bytes (validated at compile time)
- UserInfo: 128 bytes (username[32] + password_hash[128] + metadata)
- FileEntry: 416 bytes (name[256] + size + timestamps + metadata)

## File System Initialization (fs_init)

### Loading Sequence
```cpp
1. Open .omni file in binary read mode
2. Read OMNIHeader (512 bytes from offset 0)
3. Validate magic number ("OMNIFS01")
4. Seek to user_table_offset
5. Read max_users * sizeof(UserInfo) bytes
6. Parse each UserInfo into std::unordered_map
7. Seek to file metadata offset
8. Read max_files * sizeof(FileEntry) bytes
9. Parse each FileEntry into std::vector
10. Load free space bitmap (65536 bytes)
11. Build in-memory bitmap (std::vector<bool>)
12. Close file (reopen per operation for simplicity)
```

### Why Load Everything at Startup?
**Advantages:**
- Fast operation execution (no disk I/O during requests)
- Simple locking (only at operation boundaries)
- Entire file system state in memory

**Disadvantages:**
- ~425KB memory for metadata
- Slower startup time (~100ms for 100MB file)
- Not suitable for very large file systems

**Design decision:** Acceptable for 100MB filesystem with ≤1000 files.

## File Operations

### File Creation (file_create)
```
1. Validate path and permissions
2. Check if file already exists (O(n) search in file_table)
3. Allocate inode number (max_inode + 1)
4. Create FileEntry structure
5. Set name, size, owner, timestamps
6. Add to in-memory file_table vector
7. Open .omni file in r+b mode
8. Seek to file metadata offset + (file_count * sizeof(FileEntry))
9. Write FileEntry structure
10. Close file
11. Store file data in memory (file_data_ map)
```

### File Write (file_write)
```
1. Find FileEntry by path (O(n) search)
2. Update size and modified_time
3. Store data in file_data_ map (path → content)
4. Open .omni file in r+b mode
5. Find FileEntry offset in metadata area
6. Write updated FileEntry structure
7. Close file
```

**Note:** Actual content stored in-memory only for Phase 1 simplicity.

### File Read (file_read)
```
1. Find FileEntry by path (O(n) search)
2. Retrieve data from file_data_ map
3. Return content to caller
```

### File Delete (file_delete)
```
1. Find FileEntry by path
2. Remove from file_table vector
3. Remove from file_data_ map
4. Rewrite entire file metadata table to disk
5. Mark blocks as free in bitmap
```

## Write Strategies

### Write-Through (Current Implementation)
**Approach:** Every change immediately written to disk

**Advantages:**
- Crash consistency (changes never lost)
- Simple to implement
- No complex cache invalidation

**Disadvantages:**
- High I/O overhead
- Slower performance (multiple disk seeks)

**When to use:** Small file systems, infrequent writes

### Write-Back (Not Implemented)
**Alternative approach:** Buffer changes in memory, flush periodically

**Would improve:**
- Batch multiple changes into single disk write
- Reduce disk seeks
- Better performance for write-heavy workloads

**Complexity:** Requires dirty tracking, periodic flushing, crash recovery

**Design decision:** Write-through sufficient for Phase 1 educational goals.

## Buffering Strategy

### Current Approach
**No buffering:** Direct I/O to .omni file

```cpp
std::fstream file(omni_path, std::ios::in | std::ios::out | std::ios::binary);
file.seekp(offset);
file.write(data, size);
file.close();
```

**Rationale:**
- Simple implementation
- OS handles disk caching
- File system size (100MB) fits in OS cache
- No manual buffer management needed

### Memory-Mapped I/O (Not Used)
**Alternative:** mmap() entire .omni file

**Advantages:**
- OS handles paging automatically
- Simpler code (treat file as memory)
- Potentially better performance

**Disadvantages:**
- Platform-specific (Linux mmap)
- Fixed file size (can't grow easily)
- More complex error handling

**Design decision:** Standard I/O sufficient, more portable.

## Data Integrity

### Consistency Guarantees
1. **Operation atomicity:** Each operation completes fully or not at all
2. **FIFO ordering:** Operations never interleave
3. **Immediate flush:** Changes written to disk before success response

### Error Handling
```cpp
// Write operation pattern
std::fstream file(path, std::ios::out | std::ios::binary);
if (!file.is_open()) {
    LOG_ERROR("Cannot open file");
    return ERROR_IO_ERROR;
}

file.write(data, size);
if (!file.good()) {
    LOG_ERROR("Write failed");
    return ERROR_IO_ERROR;
}

file.close();
```

### No Journaling
**Current:** No transaction log or journal

**Implications:**
- Crash during operation may corrupt file
- No recovery mechanism
- Acceptable for Phase 1 demo

**Phase 2 consideration:** Add write-ahead log for delta vault.

## Performance Characteristics

### Startup Time
- OMNIHeader: 1 disk read (512 bytes)
- User table: 1 disk read (6.4 KB)
- File metadata: 1 disk read (416 KB)
- Total: ~425 KB sequential read = ~5ms on SSD

### Operation Performance
| Operation   | Disk Reads | Disk Writes    | Time |
| ----------- | ---------- | -------------- | ---- |
| file_create | 0          | 1 (416 bytes)  | ~1ms |
| file_read   | 0          | 0              | <1ms |
| file_write  | 0          | 1 (416 bytes)  | ~1ms |
| file_delete | 0          | 1 (full table) | ~5ms |
| dir_list    | 0          | 0              | <1ms |

**Bottleneck:** file_delete rewrites entire metadata table.

### Optimization Opportunities
1. **Lazy deletion:** Mark entries invalid, compact periodically
2. **Block caching:** Cache frequently accessed blocks
3. **Batch writes:** Accumulate changes, flush together

**Design decision:** Acceptable performance for ≤20 clients, simple implementation prioritized.

## Free Space Management

### Bitmap Structure
```
Bit array: [1, 1, 1, 0, 0, 1, 1, 0, ...]
           block 0 free
           block 1 free
           block 2 free
           block 3 used
           block 4 used
           ...
```

### Allocation Algorithm
```cpp
uint32_t allocate_block() {
    for (size_t i = 0; i < free_blocks.size(); i++) {
        if (free_blocks[i]) {
            free_blocks[i] = false;
            return i;
        }
    }
    return UINT32_MAX; // No free blocks
}
```

**Complexity:** O(n) where n = number of blocks

**Improvement:** Keep free block counter, start search from last allocated position.

## Platform Considerations

### Endianness
**Assumption:** Little-endian (x86-64 Linux)

**Portability:** Would need byte swapping for big-endian systems

**Design decision:** No byte swapping for Phase 1 (Linux only).

### File System Limits
**Linux:** No special considerations, standard file I/O

**Maximum file size:** Determined by total_size config parameter

**Block alignment:** No special alignment required

## Memory vs. Disk Trade-offs

### In-Memory (Current)
- Metadata: ~425 KB
- File contents: Up to 100 MB
- Total: ~100 MB RAM

**Acceptable for:** Small file systems, fast operations

### Hybrid Approach (Alternative)
- Metadata: In memory (~425 KB)
- File contents: On disk (only cache recent)
- Total: ~1 MB RAM typical

**Better for:** Large file systems, many files

**Design decision:** Full in-memory for Phase 1 simplicity.

## Summary

**File I/O approach:**
- Fixed-size binary structures
- Write-through consistency
- Full metadata in memory
- Direct I/O, no manual buffering
- O(n) searches acceptable for small scale

**Key trade-offs:**
- Simplicity over performance
- Memory over disk I/O
- Crash consistency over speed

**Suitable for:** Educational file system with ≤1000 files, 100MB total size, ≤20 concurrent users.