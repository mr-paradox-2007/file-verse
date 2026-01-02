# File I/O Strategy Document

## Overview

This document describes how the OFS file system reads from and writes to the .omni container file, including serialization strategies, buffering, and data integrity approaches.

## 1. Physical File Structure

### .omni File Layout

```
+---------------------------+
| OMNIHeader (512 bytes)    |  Offset: 0
+---------------------------+
| User Table                |  Offset: 512
| (50 * 256 bytes = 12.8KB) |
+---------------------------+
| Metadata Index            |  Offset: 512 + 12.8KB
| (8192 * 128 bytes = 1MB)  |
+---------------------------+
| Free Space Bitmap         |  Offset: 512 + 12.8KB + 1MB
| (variable size)           |
+---------------------------+
| Content Block Area        |  Offset: calculated
| (64KB blocks)             |
+---------------------------+
```

### Offset Calculation

```cpp
uint64_t get_metadata_offset() {
    return header.user_table_offset + (header.max_users * sizeof(UserInfo));
}

uint64_t get_bitmap_offset() {
    return get_metadata_offset() + (MAX_METADATA_ENTRIES * METADATA_ENTRY_SIZE);
}

uint64_t get_block_offset(uint32_t block_idx) {
    return get_bitmap_offset() + block_bitmap.size() + (block_idx * BLOCK_SIZE);
}
```

## 2. Serialization/Deserialization

### Structure Serialization

All structures are written as binary data using direct memory copies:

```cpp
// Write header
file.seekp(0);
file.write((char*)&header, sizeof(OMNIHeader));

// Read header
file.seekg(0);
file.read((char*)&header, sizeof(OMNIHeader));
```

### Data Type Sizes

| Type     | Size    | Notes             |
| -------- | ------- | ----------------- |
| uint8_t  | 1 byte  | Flags, types      |
| uint32_t | 4 bytes | Indices, counts   |
| uint64_t | 8 bytes | Sizes, timestamps |
| char[N]  | N bytes | Fixed strings     |

All integers use native byte order (little-endian on x86).

### Portability Considerations

- Fixed-size types ensure consistent sizes across platforms
- No padding issues due to explicit structure layout
- Binary format is platform-dependent (same architecture required)

## 3. Read Operations

### Header Loading

```cpp
bool load_header() {
    file.seekg(0);
    file.read((char*)&header, sizeof(header));
    
    // Validate magic number
    if (memcmp(header.magic, "OMNIFS01", 8) != 0) {
        return false;
    }
    
    return file.good();
}
```

**Error Handling**:
- Check file.good() after each read
- Verify magic number for corruption detection
- Return false on any failure

### Metadata Loading

```cpp
bool load_metadata() {
    metadata_cache.resize(MAX_METADATA_ENTRIES);
    file.seekg(get_metadata_offset());
    
    for (auto& entry : metadata_cache) {
        file.read((char*)&entry, sizeof(MetadataEntry));
        if (!file.good()) return false;
    }
    
    return true;
}
```

**Strategy**:
- Load all metadata at startup
- Keep in memory for fast access
- Single sequential read for efficiency

### User Table Loading

```cpp
bool load_users() {
    file.seekg(get_user_table_offset());
    
    for (uint32_t i = 0; i < header.max_users; i++) {
        UserInfo user;
        file.read((char*)&user, sizeof(UserInfo));
        
        if (file.good() && user.is_active) {
            user_cache[user.username] = user;
        }
    }
    
    return true;
}
```

**Strategy**:
- Load only active users
- Store in map for fast lookup
- Skip inactive entries

### Bitmap Loading

```cpp
bool load_bitmap() {
    uint32_t num_blocks = calculate_num_blocks();
    block_bitmap.resize(num_blocks, 0);
    
    file.seekg(get_bitmap_offset());
    file.read((char*)block_bitmap.data(), block_bitmap.size());
    
    return file.good();
}
```

**Strategy**:
- Single read of entire bitmap
- Keep in memory for allocation decisions

### Block Data Reading

```cpp
size_t read_block(uint32_t block_idx, void* buffer, size_t buffer_size, uint32_t* next_block) {
    uint64_t offset = get_block_offset(block_idx);
    file.seekg(offset);
    
    // Read block header
    BlockHeader hdr;
    file.read((char*)&hdr, sizeof(hdr));
    
    if (next_block) *next_block = hdr.next_block;
    
    // Read data
    if (buffer && buffer_size > 0) {
        size_t to_read = min(hdr.data_size, buffer_size);
        file.read((char*)buffer, to_read);
        
        // Decrypt data
        decode_data(buffer, to_read);
        
        return to_read;
    }
    
    return hdr.data_size;
}
```

**Strategy**:
- Seek to block offset
- Read header first
- Read data into buffer
- Decode immediately after reading

### File Content Reading

```cpp
size_t read_file_data(uint32_t entry_idx, void* buffer, size_t buffer_size) {
    MetadataEntry* entry = &metadata_cache[entry_idx];
    
    uint8_t* ptr = (uint8_t*)buffer;
    size_t total_read = 0;
    uint32_t current_block = entry->start_block;
    
    // Follow block chain
    while (current_block != 0 && total_read < buffer_size) {
        uint32_t next_block;
        size_t read = read_block(current_block, ptr, buffer_size - total_read, &next_block);
        
        ptr += read;
        total_read += read;
        current_block = next_block;
    }
    
    return total_read;
}
```

**Strategy**:
- Follow linked list of blocks
- Accumulate data in buffer
- Stop at end of chain or buffer full

## 4. Write Operations

### Header Writing

```cpp
bool save_header() {
    file.seekp(0);
    file.write((char*)&header, sizeof(header));
    file.flush();
    
    return file.good();
}
```

**Strategy**:
- Seek to beginning
- Overwrite header
- Flush immediately for durability

### Metadata Writing

```cpp
bool save_metadata() {
    file.seekp(get_metadata_offset());
    
    for (const auto& entry : metadata_cache) {
        file.write((char*)&entry, sizeof(MetadataEntry));
    }
    
    file.flush();
    return file.good();
}
```

**Strategy**:
- Write entire metadata array
- Single sequential write
- Flush after completion

### Bitmap Writing

```cpp
bool save_bitmap() {
    file.seekp(get_bitmap_offset());
    file.write((char*)block_bitmap.data(), block_bitmap.size());
    file.flush();
    
    return file.good();
}
```

**Strategy**:
- Single write of entire bitmap
- Flush immediately
- Called after each allocation/deallocation

### Block Data Writing

```cpp
bool write_block(uint32_t block_idx, const void* data, size_t size, uint32_t next_block) {
    uint64_t offset = get_block_offset(block_idx);
    file.seekp(offset);
    
    // Write block header
    BlockHeader hdr;
    hdr.next_block = next_block;
    hdr.data_size = size;
    file.write((char*)&hdr, sizeof(hdr));
    
    // Encrypt and write data
    if (data && size > 0) {
        vector<uint8_t> encoded(size);
        memcpy(encoded.data(), data, size);
        encode_data(encoded.data(), size);
        
        file.write((char*)encoded.data(), size);
    }
    
    file.flush();
    return file.good();
}
```

**Strategy**:
- Seek to block offset
- Write header first
- Encode data before writing
- Flush after each block

### File Content Writing

```cpp
bool write_file_data(uint32_t entry_idx, const void* data, size_t size) {
    MetadataEntry* entry = &metadata_cache[entry_idx];
    
    // Free old blocks if exists
    if (entry->start_block != 0) {
        free_block_chain(entry->start_block);
    }
    
    // Allocate and write new blocks
    const uint8_t* ptr = (const uint8_t*)data;
    size_t remaining = size;
    uint32_t prev_block = 0;
    uint32_t first_block = 0;
    
    while (remaining > 0) {
        uint32_t block_idx = allocate_block();
        if (block_idx == INVALID) return false;
        
        if (first_block == 0) first_block = block_idx;
        
        size_t chunk_size = min(remaining, BLOCK_SIZE - sizeof(BlockHeader));
        uint32_t next = 0;  // Will be updated for prev_block
        
        write_block(block_idx, ptr, chunk_size, next);
        
        // Link previous block to this one
        if (prev_block != 0) {
            uint8_t temp[BLOCK_SIZE];
            uint32_t dummy;
            size_t prev_size = read_block(prev_block, temp, sizeof(temp), &dummy);
            write_block(prev_block, temp, prev_size, block_idx);
        }
        
        prev_block = block_idx;
        ptr += chunk_size;
        remaining -= chunk_size;
    }
    
    // Update metadata
    entry->start_block = first_block;
    entry->total_size = size;
    entry->modified_time = time(nullptr);
    save_metadata();
    
    return true;
}
```

**Strategy**:
- Free old blocks first
- Allocate new blocks
- Write in chunks
- Link blocks together
- Update metadata

## 5. Buffering Strategy

### Current Implementation

- **No explicit buffering**: Direct file I/O
- **OS-level buffering**: Relies on system page cache
- **Flush after writes**: Ensures durability

### Memory Buffers

```cpp
// Metadata: entire array in memory
vector<MetadataEntry> metadata_cache;

// Bitmap: entire array in memory
vector<uint8_t> block_bitmap;

// Users: hash map in memory
map<string, UserInfo> user_cache;
```

**Trade-off**:
- Memory: ~1MB for metadata + bitmap
- Speed: Fast access to metadata
- Consistency: Write-through to disk

### Future Enhancement

Could add block cache:
```cpp
struct BlockCache {
    map<uint32_t, vector<uint8_t>> cached_blocks;
    queue<uint32_t> lru_queue;
    size_t max_cache_size;
};
```

Benefits:
- Reduce disk I/O for frequently accessed blocks
- Better performance for repeated reads
- LRU eviction for memory management

## 6. File Growth Strategy

### Allocation on Write

Files grow dynamically by allocating new blocks:

```cpp
while (remaining_data > 0) {
    uint32_t new_block = allocate_block();
    // Link to chain
    // Write data
}
```

**Characteristics**:
- No pre-allocation
- Grows exactly as needed
- May cause fragmentation

### Fragmentation

**Current State**:
- Files may be fragmented across non-contiguous blocks
- Linked list provides logical contiguity
- Physical fragmentation acceptable for current workload

**Mitigation**:
- Could implement defragmentation routine
- Could prefer contiguous block allocation
- Could use extent-based allocation

## 7. Free Space Management

### Bitmap Operations

**Allocation**:
```cpp
uint32_t allocate_block() {
    for (uint32_t i = 0; i < block_bitmap.size(); i++) {
        if (block_bitmap[i] == 0) {
            block_bitmap[i] = 1;
            save_bitmap();
            return i;
        }
    }
    return INVALID;
}
```

Time: O(n) worst case

**Deallocation**:
```cpp
void free_block(uint32_t block_idx) {
    if (block_idx < block_bitmap.size()) {
        block_bitmap[block_idx] = 0;
        save_bitmap();
    }
}
```

Time: O(1)

**Chain Deallocation**:
```cpp
void free_block_chain(uint32_t start_block) {
    uint32_t current = start_block;
    
    while (current != 0) {
        uint32_t next;
        read_block(current, nullptr, 0, &next);
        free_block(current);
        current = next;
    }
}
```

Time: O(b) where b = blocks in chain

### Space Tracking

```cpp
uint64_t get_free_space() {
    uint32_t free_blocks = 0;
    for (uint8_t b : block_bitmap) {
        if (b == 0) free_blocks++;
    }
    return free_blocks * BLOCK_SIZE;
}
```

**Optimization**:
- Could maintain free_block_count variable
- Update on each allocation/deallocation
- O(1) query time vs O(n) scan

## 8. Data Integrity

### Consistency Measures

1. **Atomic Operations**: Each metadata write is atomic
2. **Flush After Write**: Ensures data on disk
3. **Magic Number**: Validates file format
4. **Valid Flag**: Marks entries as used/free

### Error Detection

```cpp
// Check magic number
if (memcmp(header.magic, "OMNIFS01", 8) != 0) {
    return ERROR_INVALID_CONFIG;
}

// Check file operations
if (!file.good()) {
    return ERROR_IO_ERROR;
}
```

### Recovery

**Current State**:
- No crash recovery mechanism
- Partial writes may leave inconsistent state

**Future Enhancement**:
- Write-ahead logging
- Transaction support
- Checkpoint/recovery system

## 9. Encryption Implementation

### Byte Substitution

```cpp
void init_encryption_table() {
    for (int i = 0; i < 256; i++) {
        encryption_table[i] = (i + 73) % 256;
        decryption_table[encryption_table[i]] = i;
    }
}

void encode_data(void* data, size_t size) {
    uint8_t* bytes = (uint8_t*)data;
    for (size_t i = 0; i < size; i++) {
        bytes[i] = encryption_table[bytes[i]];
    }
}

void decode_data(void* data, size_t size) {
    uint8_t* bytes = (uint8_t*)data;
    for (size_t i = 0; i < size; i++) {
        bytes[i] = decryption_table[bytes[i]];
    }
}
```

**Properties**:
- One-to-one mapping (bijection)
- Deterministic: same input → same output
- Symmetric: encode/decode use same table
- Fast: O(n) for n bytes

**Security Level**:
- Basic obfuscation
- Not cryptographically secure
- Meets project requirements
- Easy to implement and debug

## 10. Memory vs Disk Trade-offs

### In-Memory Data

**Advantages**:
- Fast access (no disk I/O)
- O(1) lookup for most operations
- Consistent view of state

**Disadvantages**:
- Memory usage (~1MB)
- Requires loading at startup
- Changes must be written back

### On-Disk Data

**Advantages**:
- Persistent storage
- No memory limit
- Survives crashes

**Disadvantages**:
- Slower access
- Requires I/O for each operation
- Potential for inconsistency

### Hybrid Approach

**Current Strategy**:
- Metadata in memory, persisted on change
- Bitmap in memory, persisted on allocation
- File content on disk, read on demand

**Rationale**:
- Balances speed and memory usage
- Adequate for target workload
- Simple to implement and maintain

## 11. Performance Characteristics

### Read Performance

| Operation      | Disk I/O | Time                   |
| -------------- | -------- | ---------------------- |
| Open file      | 0        | O(1) metadata access   |
| Read metadata  | 0        | O(1) memory access     |
| Read file      | O(b)     | Sequential block reads |
| List directory | 0        | O(n) metadata scan     |

### Write Performance

| Operation       | Disk I/O | Time                    |
| --------------- | -------- | ----------------------- |
| Create file     | O(b)     | Block writes + metadata |
| Write data      | O(b)     | Block writes            |
| Delete file     | O(b)     | Block frees + metadata  |
| Update metadata | 1        | Single metadata write   |

### Bottlenecks

1. **Directory Listing**: O(n) scan of all metadata
2. **Block Allocation**: O(n) scan of bitmap
3. **Disk I/O**: Sequential reads/writes

### Optimization Opportunities

1. Add directory index for O(1) child lookup
2. Track first free block for faster allocation
3. Implement block cache for repeated reads
4. Use memory-mapped I/O for better performance

## 12. Summary

The file I/O strategy emphasizes:
- **Simplicity**: Direct read/write with clear structure
- **Reliability**: Flush after writes, validate on reads
- **Performance**: Metadata in memory, efficient block I/O
- **Maintainability**: Clear separation of concerns

Trade-offs made:
- Memory usage for speed (metadata cache)
- Simple encryption for ease of implementation
- Sequential block allocation for simplicity
- No advanced features (caching, transactions) for minimal complexity