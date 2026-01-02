#ifndef OMNI_STORAGE_HPP
#define OMNI_STORAGE_HPP

#include "ofs_types.hpp"
#include <string>
#include <vector>
#include <fstream>
#include <map>

#define BLOCK_SIZE 65536
#define METADATA_ENTRY_SIZE 128
#define MAX_METADATA_ENTRIES 8192

struct MetadataEntry {
    uint8_t valid;
    uint8_t type;
    uint32_t parent_index;
    char name[32];
    uint32_t start_block;
    uint64_t total_size;
    uint32_t owner_id;
    uint32_t permissions;
    uint64_t created_time;
    uint64_t modified_time;
    uint8_t reserved[32];
};

struct BlockHeader {
    uint32_t next_block;
    uint32_t data_size;
    uint8_t reserved[8];
};

class OmniStorage {
public:
    OmniStorage();
    ~OmniStorage();
    
    bool create(const std::string& path, uint64_t total_size);
    bool open(const std::string& path);
    void close();
    
    uint32_t allocate_entry(uint8_t type, uint32_t parent, const std::string& name, uint32_t owner_id);
    bool free_entry(uint32_t entry_idx);
    MetadataEntry* get_entry(uint32_t entry_idx);
    std::vector<uint32_t> list_children(uint32_t parent_idx);
    
    uint32_t allocate_block();
    void free_block(uint32_t block_idx);
    void free_block_chain(uint32_t start_block);
    bool write_block(uint32_t block_idx, const void* data, size_t size, uint32_t next_block);
    size_t read_block(uint32_t block_idx, void* buffer, size_t buffer_size, uint32_t* next_block);
    
    bool write_file_data(uint32_t entry_idx, const void* data, size_t size);
    size_t read_file_data(uint32_t entry_idx, void* buffer, size_t buffer_size);
    
    void init_encryption_table();
    void encode_data(void* data, size_t size);
    void decode_data(void* data, size_t size);
    
    bool add_user(const UserInfo& user);
    bool get_user(const std::string& username, UserInfo* user);
    bool update_user(const UserInfo& user);
    std::vector<UserInfo> list_users();
    
    uint64_t get_free_space();
    uint32_t get_total_blocks();
    uint32_t get_used_blocks();
    
private:
    std::string file_path;
    std::fstream file;
    
    OMNIHeader header;
    std::vector<MetadataEntry> metadata_cache;
    std::vector<uint8_t> block_bitmap;
    std::map<std::string, UserInfo> user_cache;
    uint8_t encryption_table[256];
    uint8_t decryption_table[256];
    
    bool load_header();
    bool save_header();
    bool load_metadata();
    bool save_metadata();
    bool load_users();
    bool save_users();
    bool load_bitmap();
    bool save_bitmap();
    
    uint64_t get_metadata_offset();
    uint64_t get_bitmap_offset();
    uint64_t get_user_table_offset();
    uint64_t get_block_offset(uint32_t block_idx);
};

#endif