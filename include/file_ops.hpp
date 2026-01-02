#ifndef FILE_OPS_HPP
#define FILE_OPS_HPP

#include "ofs_types.hpp"
#include <string>

class OmniStorage;
typedef void* OFS_Instance;
typedef void* OFS_Session;

// Initialize storage backend
void set_storage_instance(OmniStorage* storage);

// File operations
int file_create(OFS_Session session, const std::string& path, const void* data, size_t size);
int file_read(OFS_Session session, const std::string& path, void** out_buffer, size_t* out_size);
int file_edit(OFS_Session session, const std::string& path, const void* data, size_t size, uint64_t index);
int file_delete(OFS_Session session, const std::string& path);
int file_truncate(OFS_Session session, const std::string& path);
int file_exists(OFS_Session session, const std::string& path);
int file_rename(OFS_Session session, const std::string& old_path, const std::string& new_path);

// Directory operations
int dir_create(OFS_Session session, const std::string& path);
int dir_list(OFS_Session session, const std::string& path, FileEntry** out_entries, int* out_count);
int dir_delete(OFS_Session session, const std::string& path);
int dir_exists(OFS_Session session, const std::string& path);

// Metadata and stats
int get_metadata(OFS_Session session, const std::string& path, FileMetadata* metadata);
int set_permissions(OFS_Session session, const std::string& path, uint32_t permissions);
int get_stats(OFS_Session session, FSStats* stats);

// Utilities
void free_buffer(void* buffer);
const char* get_error_message(int error_code);

#endif