#include "omni_storage.hpp"
#include <cstring>
#include <ctime>
#include <iostream>

OmniStorage::OmniStorage() {
    init_encryption_table();
}

OmniStorage::~OmniStorage() {
    close();
}

void OmniStorage::init_encryption_table() {
    // Simple substitution cipher
    for (int i = 0; i < 256; i++) {
        encryption_table[i] = (i + 73) % 256;
        decryption_table[encryption_table[i]] = i;
    }
}

bool OmniStorage::create(const std::string& path, uint64_t total_size) {
    file_path = path;
    
    file.open(path, std::ios::binary | std::ios::out | std::ios::trunc);
    if (!file.is_open()) return false;
    
    // Initialize header
    std::memset(&header, 0, sizeof(header));
    std::memcpy(header.magic, "OMNIFS01", 8);
    header.format_version = 0x00010000;
    header.total_size = total_size;
    header.header_size = 512;
    header.block_size = BLOCK_SIZE;
    header.max_users = 50;
    header.user_table_offset = 512;
    
    uint32_t num_blocks = (total_size - 512 - 6400 - MAX_METADATA_ENTRIES * METADATA_ENTRY_SIZE) / BLOCK_SIZE;
    
    // Write header
    file.write((char*)&header, sizeof(header));
    file.flush();
    
    // Initialize metadata
    metadata_cache.resize(MAX_METADATA_ENTRIES);
    for (auto& entry : metadata_cache) {
        entry.valid = 0;
    }
    
    // Create root directory (entry 0)
    metadata_cache[0].valid = 1;
    metadata_cache[0].type = 1;
    metadata_cache[0].parent_index = 0;
    strcpy(metadata_cache[0].name, "/");
    metadata_cache[0].start_block = 0;
    metadata_cache[0].owner_id = 0;
    metadata_cache[0].permissions = 0755;
    metadata_cache[0].created_time = time(nullptr);
    metadata_cache[0].modified_time = time(nullptr);
    
    // Initialize bitmap
    block_bitmap.resize(num_blocks, 0);
    
    // Save metadata and bitmap before closing
    if (!save_metadata()) {
        file.close();
        return false;
    }
    if (!save_bitmap()) {
        file.close();
        return false;
    }
    
    // Initialize user table
    for (int i = 0; i < header.max_users; i++) {
        UserInfo empty_user;
        memset(&empty_user, 0, sizeof(empty_user));
        file.seekp(header.user_table_offset + (i * sizeof(UserInfo)));
        file.write((char*)&empty_user, sizeof(UserInfo));
    }
    file.flush();
    
    file.close();
    return open(path);
}

bool OmniStorage::open(const std::string& path) {
    file_path = path;
    file.open(path, std::ios::binary | std::ios::in | std::ios::out);
    if (!file.is_open()) return false;
    
    if (!load_header()) return false;
    if (!load_metadata()) return false;
    if (!load_bitmap()) return false;
    if (!load_users()) return false;
    
    return true;
}

void OmniStorage::close() {
    if (file.is_open()) {
        save_metadata();
        save_bitmap();
        save_users();
        file.close();
    }
}

bool OmniStorage::load_header() {
    file.seekg(0);
    file.read((char*)&header, sizeof(header));
    return file.good() && memcmp(header.magic, "OMNIFS01", 8) == 0;
}

bool OmniStorage::save_header() {
    file.seekp(0);
    file.write((char*)&header, sizeof(header));
    file.flush();
    return file.good();
}

uint64_t OmniStorage::get_metadata_offset() {
    return header.user_table_offset + (header.max_users * sizeof(UserInfo));
}

uint64_t OmniStorage::get_bitmap_offset() {
    return get_metadata_offset() + (MAX_METADATA_ENTRIES * METADATA_ENTRY_SIZE);
}

uint64_t OmniStorage::get_user_table_offset() {
    return header.user_table_offset;
}

uint64_t OmniStorage::get_block_offset(uint32_t block_idx) {
    return get_bitmap_offset() + block_bitmap.size() + (block_idx * BLOCK_SIZE);
}

bool OmniStorage::load_metadata() {
    metadata_cache.resize(MAX_METADATA_ENTRIES);
    file.seekg(get_metadata_offset());
    
    for (auto& entry : metadata_cache) {
        file.read((char*)&entry, sizeof(MetadataEntry));
        if (!file.good()) return false;
    }
    return true;
}

bool OmniStorage::save_metadata() {
    file.seekp(get_metadata_offset());
    
    for (const auto& entry : metadata_cache) {
        file.write((char*)&entry, sizeof(MetadataEntry));
    }
    file.flush();
    return file.good();
}

bool OmniStorage::load_bitmap() {
    uint32_t num_blocks = (header.total_size - get_bitmap_offset()) / BLOCK_SIZE;
    block_bitmap.resize(num_blocks, 0);
    
    file.seekg(get_bitmap_offset());
    file.read((char*)block_bitmap.data(), block_bitmap.size());
    return file.good();
}

bool OmniStorage::save_bitmap() {
    file.seekp(get_bitmap_offset());
    file.write((char*)block_bitmap.data(), block_bitmap.size());
    file.flush();
    return file.good();
}

bool OmniStorage::load_users() {
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

bool OmniStorage::save_users() {
    file.seekp(get_user_table_offset());
    
    uint32_t i = 0;
    for (const auto& pair : user_cache) {
        if (i >= header.max_users) break;
        file.write((char*)&pair.second, sizeof(UserInfo));
        i++;
    }
    
    // Fill remaining with zeros
    UserInfo empty;
    memset(&empty, 0, sizeof(UserInfo));
    while (i < header.max_users) {
        file.write((char*)&empty, sizeof(UserInfo));
        i++;
    }
    
    file.flush();
    return file.good();
}

uint32_t OmniStorage::allocate_entry(uint8_t type, uint32_t parent, const std::string& name, uint32_t owner_id) {
    for (uint32_t i = 0; i < metadata_cache.size(); i++) {
        if (metadata_cache[i].valid == 0) {
            metadata_cache[i].valid = 1;
            metadata_cache[i].type = type;
            metadata_cache[i].parent_index = parent;
            strncpy(metadata_cache[i].name, name.c_str(), 31);
            metadata_cache[i].name[31] = '\0';
            metadata_cache[i].start_block = 0;
            metadata_cache[i].total_size = 0;
            metadata_cache[i].owner_id = owner_id;
            metadata_cache[i].permissions = (type == 1) ? 0755 : 0644;
            metadata_cache[i].created_time = time(nullptr);
            metadata_cache[i].modified_time = time(nullptr);
            save_metadata();
            return i;
        }
    }
    return 0xFFFFFFFF;
}

bool OmniStorage::free_entry(uint32_t entry_idx) {
    if (entry_idx >= metadata_cache.size()) return false;
    
    if (metadata_cache[entry_idx].start_block != 0) {
        free_block_chain(metadata_cache[entry_idx].start_block);
    }
    
    metadata_cache[entry_idx].valid = 0;
    save_metadata();
    return true;
}

MetadataEntry* OmniStorage::get_entry(uint32_t entry_idx) {
    if (entry_idx >= metadata_cache.size() || metadata_cache[entry_idx].valid == 0) {
        return nullptr;
    }
    return &metadata_cache[entry_idx];
}

std::vector<uint32_t> OmniStorage::list_children(uint32_t parent_idx) {
    std::vector<uint32_t> children;
    
    for (uint32_t i = 0; i < metadata_cache.size(); i++) {
        if (metadata_cache[i].valid && metadata_cache[i].parent_index == parent_idx) {
            children.push_back(i);
        }
    }
    
    return children;
}

uint32_t OmniStorage::allocate_block() {
    for (uint32_t i = 0; i < block_bitmap.size(); i++) {
        if (block_bitmap[i] == 0) {
            block_bitmap[i] = 1;
            save_bitmap();
            return i;
        }
    }
    return 0xFFFFFFFF;
}

void OmniStorage::free_block(uint32_t block_idx) {
    if (block_idx < block_bitmap.size()) {
        block_bitmap[block_idx] = 0;
        save_bitmap();
    }
}

void OmniStorage::free_block_chain(uint32_t start_block) {
    uint32_t current = start_block;
    
    while (current != 0 && current != 0xFFFFFFFF) {
        uint32_t next;
        read_block(current, nullptr, 0, &next);
        free_block(current);
        current = next;
    }
}

bool OmniStorage::write_block(uint32_t block_idx, const void* data, size_t size, uint32_t next_block) {
    if (block_idx >= block_bitmap.size()) return false;
    
    uint64_t offset = get_block_offset(block_idx);
    file.seekp(offset);
    
    BlockHeader hdr;
    hdr.next_block = next_block;
    hdr.data_size = size;
    file.write((char*)&hdr, sizeof(hdr));
    
    if (data && size > 0) {
        std::vector<uint8_t> encoded(size);
        memcpy(encoded.data(), data, size);
        encode_data(encoded.data(), size);
        file.write((char*)encoded.data(), size);
    }
    
    file.flush();
    return file.good();
}

size_t OmniStorage::read_block(uint32_t block_idx, void* buffer, size_t buffer_size, uint32_t* next_block) {
    if (block_idx >= block_bitmap.size()) return 0;
    
    uint64_t offset = get_block_offset(block_idx);
    file.seekg(offset);
    
    BlockHeader hdr;
    file.read((char*)&hdr, sizeof(hdr));
    if (!file.good()) return 0;
    
    if (next_block) *next_block = hdr.next_block;
    
    if (buffer && buffer_size > 0) {
        size_t to_read = std::min((size_t)hdr.data_size, buffer_size);
        file.read((char*)buffer, to_read);
        decode_data(buffer, to_read);
        return to_read;
    }
    
    return hdr.data_size;
}

bool OmniStorage::write_file_data(uint32_t entry_idx, const void* data, size_t size) {
    if (entry_idx >= metadata_cache.size()) return false;
    
    MetadataEntry* entry = &metadata_cache[entry_idx];
    
    if (entry->start_block != 0) {
        free_block_chain(entry->start_block);
    }
    
    if (size == 0) {
        entry->start_block = 0;
        entry->total_size = 0;
        save_metadata();
        return true;
    }
    
    const uint8_t* ptr = (const uint8_t*)data;
    size_t remaining = size;
    uint32_t prev_block = 0;
    uint32_t first_block = 0;
    
    while (remaining > 0) {
        uint32_t block_idx = allocate_block();
        if (block_idx == 0xFFFFFFFF) return false;
        
        if (first_block == 0) first_block = block_idx;
        
        size_t chunk_size = std::min(remaining, (size_t)(BLOCK_SIZE - sizeof(BlockHeader)));
        uint32_t next = (remaining > chunk_size) ? 0 : 0;
        
        write_block(block_idx, ptr, chunk_size, next);
        
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
    
    entry->start_block = first_block;
    entry->total_size = size;
    entry->modified_time = time(nullptr);
    save_metadata();
    
    return true;
}

size_t OmniStorage::read_file_data(uint32_t entry_idx, void* buffer, size_t buffer_size) {
    if (entry_idx >= metadata_cache.size()) return 0;
    
    MetadataEntry* entry = &metadata_cache[entry_idx];
    if (entry->start_block == 0) return 0;
    
    uint8_t* ptr = (uint8_t*)buffer;
    size_t total_read = 0;
    uint32_t current_block = entry->start_block;
    
    while (current_block != 0 && current_block != 0xFFFFFFFF && total_read < buffer_size) {
        uint32_t next_block;
        size_t read = read_block(current_block, ptr, buffer_size - total_read, &next_block);
        
        ptr += read;
        total_read += read;
        current_block = next_block;
    }
    
    return total_read;
}

void OmniStorage::encode_data(void* data, size_t size) {
    uint8_t* bytes = (uint8_t*)data;
    for (size_t i = 0; i < size; i++) {
        bytes[i] = encryption_table[bytes[i]];
    }
}

void OmniStorage::decode_data(void* data, size_t size) {
    uint8_t* bytes = (uint8_t*)data;
    for (size_t i = 0; i < size; i++) {
        bytes[i] = decryption_table[bytes[i]];
    }
}

bool OmniStorage::add_user(const UserInfo& user) {
    user_cache[user.username] = user;
    return save_users();
}

bool OmniStorage::get_user(const std::string& username, UserInfo* user) {
    auto it = user_cache.find(username);
    if (it != user_cache.end()) {
        *user = it->second;
        return true;
    }
    return false;
}

bool OmniStorage::update_user(const UserInfo& user) {
    return add_user(user);
}

std::vector<UserInfo> OmniStorage::list_users() {
    std::vector<UserInfo> users;
    for (const auto& pair : user_cache) {
        if (pair.second.is_active) {
            users.push_back(pair.second);
        }
    }
    return users;
}

uint64_t OmniStorage::get_free_space() {
    uint32_t free_blocks = 0;
    for (uint8_t b : block_bitmap) {
        if (b == 0) free_blocks++;
    }
    return free_blocks * BLOCK_SIZE;
}

uint32_t OmniStorage::get_total_blocks() {
    return block_bitmap.size();
}

uint32_t OmniStorage::get_used_blocks() {
    uint32_t used = 0;
    for (uint8_t b : block_bitmap) {
        if (b == 1) used++;
    }
    return used;
}