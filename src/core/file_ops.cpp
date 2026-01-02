#include "file_ops.hpp"
#include "omni_storage.hpp"
#include "logger.hpp"
#include "path_resolver.hpp"
#include <map>
#include <mutex>

static OmniStorage* g_storage = nullptr;
static std::mutex g_storage_mutex;
static std::map<std::string, uint32_t> g_user_id_map;
static uint32_t g_next_user_id = 1;

void set_storage_instance(OmniStorage* storage) {
    g_storage = storage;
}

uint32_t get_user_id(const std::string& username) {
    auto it = g_user_id_map.find(username);
    if (it != g_user_id_map.end()) return it->second;
    
    uint32_t id = g_next_user_id++;
    g_user_id_map[username] = id;
    return id;
}

uint32_t find_entry_by_path(const std::string& path, uint32_t user_id) {
    if (!g_storage) return 0xFFFFFFFF;
    
    if (path == "/") return 0;
    
    std::vector<std::string> parts = PathResolver::split(path);
    uint32_t current = 0;
    
    for (const auto& part : parts) {
        std::vector<uint32_t> children = g_storage->list_children(current);
        bool found = false;
        
        for (uint32_t child_idx : children) {
            MetadataEntry* entry = g_storage->get_entry(child_idx);
            if (entry && strcmp(entry->name, part.c_str()) == 0) {
                current = child_idx;
                found = true;
                break;
            }
        }
        
        if (!found) return 0xFFFFFFFF;
    }
    
    return current;
}

int file_create(OFS_Session session, const std::string& path, const void* data, size_t size) {
    if (!g_storage) return static_cast<int>(OFSErrorCodes::ERROR_IO_ERROR);
    
    int validation = PathResolver::validate_path(path);
    if (validation != static_cast<int>(OFSErrorCodes::SUCCESS)) return validation;
    
    std::lock_guard<std::mutex> lock(g_storage_mutex);
    
    std::string parent_path = PathResolver::get_parent(path);
    std::string filename = PathResolver::get_filename(path);
    
    uint32_t user_id = 1;
    uint32_t parent_idx = find_entry_by_path(parent_path, user_id);
    
    if (parent_idx == 0xFFFFFFFF) {
        return static_cast<int>(OFSErrorCodes::ERROR_NOT_FOUND);
    }
    
    if (find_entry_by_path(path, user_id) != 0xFFFFFFFF) {
        return static_cast<int>(OFSErrorCodes::ERROR_FILE_EXISTS);
    }
    
    uint32_t entry_idx = g_storage->allocate_entry(0, parent_idx, filename, user_id);
    if (entry_idx == 0xFFFFFFFF) {
        return static_cast<int>(OFSErrorCodes::ERROR_NO_SPACE);
    }
    
    if (data && size > 0) {
        if (!g_storage->write_file_data(entry_idx, data, size)) {
            g_storage->free_entry(entry_idx);
            return static_cast<int>(OFSErrorCodes::ERROR_IO_ERROR);
        }
    }
    
    Logger::log_file_op("CREATE", path, "user", true);
    return static_cast<int>(OFSErrorCodes::SUCCESS);
}

int file_read(OFS_Session session, const std::string& path, void** out_buffer, size_t* out_size) {
    if (!g_storage) return static_cast<int>(OFSErrorCodes::ERROR_IO_ERROR);
    
    int validation = PathResolver::validate_path(path);
    if (validation != static_cast<int>(OFSErrorCodes::SUCCESS)) return validation;
    
    std::lock_guard<std::mutex> lock(g_storage_mutex);
    
    uint32_t entry_idx = find_entry_by_path(path, 1);
    if (entry_idx == 0xFFFFFFFF) {
        return static_cast<int>(OFSErrorCodes::ERROR_NOT_FOUND);
    }
    
    MetadataEntry* entry = g_storage->get_entry(entry_idx);
    if (!entry || entry->type != 0) {
        return static_cast<int>(OFSErrorCodes::ERROR_INVALID_OPERATION);
    }
    
    *out_size = entry->total_size;
    if (*out_size == 0) {
        *out_buffer = nullptr;
        return static_cast<int>(OFSErrorCodes::SUCCESS);
    }
    
    *out_buffer = new char[*out_size];
    size_t read = g_storage->read_file_data(entry_idx, *out_buffer, *out_size);
    
    if (read != *out_size) {
        delete[] (char*)*out_buffer;
        *out_buffer = nullptr;
        return static_cast<int>(OFSErrorCodes::ERROR_IO_ERROR);
    }
    
    Logger::log_file_op("READ", path, "user", true);
    return static_cast<int>(OFSErrorCodes::SUCCESS);
}

int file_edit(OFS_Session session, const std::string& path, const void* data, size_t size, uint64_t index) {
    return file_delete(session, path) == 0 ? file_create(session, path, data, size) : static_cast<int>(OFSErrorCodes::ERROR_IO_ERROR);
}

int file_delete(OFS_Session session, const std::string& path) {
    if (!g_storage) return static_cast<int>(OFSErrorCodes::ERROR_IO_ERROR);
    
    int validation = PathResolver::validate_path(path);
    if (validation != static_cast<int>(OFSErrorCodes::SUCCESS)) return validation;
    
    std::lock_guard<std::mutex> lock(g_storage_mutex);
    
    uint32_t entry_idx = find_entry_by_path(path, 1);
    if (entry_idx == 0xFFFFFFFF) {
        return static_cast<int>(OFSErrorCodes::ERROR_NOT_FOUND);
    }
    
    if (!g_storage->free_entry(entry_idx)) {
        return static_cast<int>(OFSErrorCodes::ERROR_IO_ERROR);
    }
    
    Logger::log_file_op("DELETE", path, "user", true);
    return static_cast<int>(OFSErrorCodes::SUCCESS);
}

int file_truncate(OFS_Session session, const std::string& path) {
    if (!g_storage) return static_cast<int>(OFSErrorCodes::ERROR_IO_ERROR);
    
    std::lock_guard<std::mutex> lock(g_storage_mutex);
    
    uint32_t entry_idx = find_entry_by_path(path, 1);
    if (entry_idx == 0xFFFFFFFF) {
        return static_cast<int>(OFSErrorCodes::ERROR_NOT_FOUND);
    }
    
    return g_storage->write_file_data(entry_idx, nullptr, 0) ? 
           static_cast<int>(OFSErrorCodes::SUCCESS) : 
           static_cast<int>(OFSErrorCodes::ERROR_IO_ERROR);
}

int file_exists(OFS_Session session, const std::string& path) {
    if (!g_storage) return static_cast<int>(OFSErrorCodes::ERROR_IO_ERROR);
    
    std::lock_guard<std::mutex> lock(g_storage_mutex);
    
    return find_entry_by_path(path, 1) != 0xFFFFFFFF ? 
           static_cast<int>(OFSErrorCodes::SUCCESS) : 
           static_cast<int>(OFSErrorCodes::ERROR_NOT_FOUND);
}

int file_rename(OFS_Session session, const std::string& old_path, const std::string& new_path) {
    if (!g_storage) return static_cast<int>(OFSErrorCodes::ERROR_IO_ERROR);
    
    std::lock_guard<std::mutex> lock(g_storage_mutex);
    
    uint32_t old_idx = find_entry_by_path(old_path, 1);
    if (old_idx == 0xFFFFFFFF) {
        return static_cast<int>(OFSErrorCodes::ERROR_NOT_FOUND);
    }
    
    if (find_entry_by_path(new_path, 1) != 0xFFFFFFFF) {
        return static_cast<int>(OFSErrorCodes::ERROR_FILE_EXISTS);
    }
    
    MetadataEntry* entry = g_storage->get_entry(old_idx);
    std::string new_name = PathResolver::get_filename(new_path);
    strncpy(entry->name, new_name.c_str(), 31);
    entry->name[31] = '\0';
    
    return static_cast<int>(OFSErrorCodes::SUCCESS);
}

int dir_create(OFS_Session session, const std::string& path) {
    if (!g_storage) return static_cast<int>(OFSErrorCodes::ERROR_IO_ERROR);
    
    int validation = PathResolver::validate_path(path);
    if (validation != static_cast<int>(OFSErrorCodes::SUCCESS)) return validation;
    
    std::lock_guard<std::mutex> lock(g_storage_mutex);
    
    std::string parent_path = PathResolver::get_parent(path);
    std::string dirname = PathResolver::get_filename(path);
    
    uint32_t parent_idx = find_entry_by_path(parent_path, 1);
    if (parent_idx == 0xFFFFFFFF) {
        return static_cast<int>(OFSErrorCodes::ERROR_NOT_FOUND);
    }
    
    if (find_entry_by_path(path, 1) != 0xFFFFFFFF) {
        return static_cast<int>(OFSErrorCodes::ERROR_FILE_EXISTS);
    }
    
    uint32_t entry_idx = g_storage->allocate_entry(1, parent_idx, dirname, 1);
    if (entry_idx == 0xFFFFFFFF) {
        return static_cast<int>(OFSErrorCodes::ERROR_NO_SPACE);
    }
    
    Logger::log_file_op("MKDIR", path, "user", true);
    return static_cast<int>(OFSErrorCodes::SUCCESS);
}

int dir_list(OFS_Session session, const std::string& path, FileEntry** out_entries, int* out_count) {
    if (!g_storage) return static_cast<int>(OFSErrorCodes::ERROR_IO_ERROR);
    
    int validation = PathResolver::validate_path(path);
    if (validation != static_cast<int>(OFSErrorCodes::SUCCESS)) return validation;
    
    std::lock_guard<std::mutex> lock(g_storage_mutex);
    
    uint32_t dir_idx = find_entry_by_path(path, 1);
    if (dir_idx == 0xFFFFFFFF) {
        return static_cast<int>(OFSErrorCodes::ERROR_NOT_FOUND);
    }
    
    std::vector<uint32_t> children = g_storage->list_children(dir_idx);
    *out_count = children.size();
    
    if (*out_count == 0) {
        *out_entries = nullptr;
        return static_cast<int>(OFSErrorCodes::SUCCESS);
    }
    
    *out_entries = new FileEntry[*out_count];
    
    for (int i = 0; i < *out_count; i++) {
        MetadataEntry* entry = g_storage->get_entry(children[i]);
        if (entry) {
            strncpy((*out_entries)[i].name, entry->name, sizeof((*out_entries)[i].name));
            (*out_entries)[i].type = entry->type;
            (*out_entries)[i].size = entry->total_size;
            (*out_entries)[i].permissions = entry->permissions;
            (*out_entries)[i].created_time = entry->created_time;
            (*out_entries)[i].modified_time = entry->modified_time;
            (*out_entries)[i].inode = children[i];
        }
    }
    
    Logger::log_file_op("LISTDIR", path, "user", true);
    return static_cast<int>(OFSErrorCodes::SUCCESS);
}

int dir_delete(OFS_Session session, const std::string& path) {
    if (!g_storage) return static_cast<int>(OFSErrorCodes::ERROR_IO_ERROR);
    
    std::lock_guard<std::mutex> lock(g_storage_mutex);
    
    uint32_t dir_idx = find_entry_by_path(path, 1);
    if (dir_idx == 0xFFFFFFFF) {
        return static_cast<int>(OFSErrorCodes::ERROR_NOT_FOUND);
    }
    
    std::vector<uint32_t> children = g_storage->list_children(dir_idx);
    if (!children.empty()) {
        return static_cast<int>(OFSErrorCodes::ERROR_DIRECTORY_NOT_EMPTY);
    }
    
    return g_storage->free_entry(dir_idx) ? 
           static_cast<int>(OFSErrorCodes::SUCCESS) : 
           static_cast<int>(OFSErrorCodes::ERROR_IO_ERROR);
}

int dir_exists(OFS_Session session, const std::string& path) {
    return file_exists(session, path);
}

void free_buffer(void* buffer) {
    delete[] (char*)buffer;
}

int get_metadata(OFS_Session session, const std::string& path, FileMetadata* metadata) {
    if (!g_storage) return static_cast<int>(OFSErrorCodes::ERROR_IO_ERROR);
    
    std::lock_guard<std::mutex> lock(g_storage_mutex);
    
    uint32_t entry_idx = find_entry_by_path(path, 1);
    if (entry_idx == 0xFFFFFFFF) {
        return static_cast<int>(OFSErrorCodes::ERROR_NOT_FOUND);
    }
    
    MetadataEntry* entry = g_storage->get_entry(entry_idx);
    if (!entry) return static_cast<int>(OFSErrorCodes::ERROR_NOT_FOUND);
    
    strncpy(metadata->path, path.c_str(), sizeof(metadata->path));
    strncpy(metadata->entry.name, entry->name, sizeof(metadata->entry.name));
    metadata->entry.type = entry->type;
    metadata->entry.size = entry->total_size;
    metadata->entry.permissions = entry->permissions;
    metadata->entry.created_time = entry->created_time;
    metadata->entry.modified_time = entry->modified_time;
    
    return static_cast<int>(OFSErrorCodes::SUCCESS);
}

int set_permissions(OFS_Session session, const std::string& path, uint32_t permissions) {
    if (!g_storage) return static_cast<int>(OFSErrorCodes::ERROR_IO_ERROR);
    
    std::lock_guard<std::mutex> lock(g_storage_mutex);
    
    uint32_t entry_idx = find_entry_by_path(path, 1);
    if (entry_idx == 0xFFFFFFFF) {
        return static_cast<int>(OFSErrorCodes::ERROR_NOT_FOUND);
    }
    
    MetadataEntry* entry = g_storage->get_entry(entry_idx);
    if (entry) {
        entry->permissions = permissions;
        return static_cast<int>(OFSErrorCodes::SUCCESS);
    }
    
    return static_cast<int>(OFSErrorCodes::ERROR_NOT_FOUND);
}

int get_stats(OFS_Session session, FSStats* stats) {
    if (!g_storage) return static_cast<int>(OFSErrorCodes::ERROR_IO_ERROR);
    
    std::lock_guard<std::mutex> lock(g_storage_mutex);
    
    stats->free_space = g_storage->get_free_space();
    stats->used_space = g_storage->get_used_blocks() * 65536;
    stats->total_size = g_storage->get_total_blocks() * 65536;
    stats->total_files = 0;
    stats->total_directories = 0;
    
    return static_cast<int>(OFSErrorCodes::SUCCESS);
}

const char* get_error_message(int error_code) {
    switch (static_cast<OFSErrorCodes>(error_code)) {
        case OFSErrorCodes::SUCCESS: return "Success";
        case OFSErrorCodes::ERROR_NOT_FOUND: return "Not found";
        case OFSErrorCodes::ERROR_PERMISSION_DENIED: return "Permission denied";
        case OFSErrorCodes::ERROR_IO_ERROR: return "I/O error";
        case OFSErrorCodes::ERROR_INVALID_PATH: return "Invalid path";
        case OFSErrorCodes::ERROR_FILE_EXISTS: return "File exists";
        case OFSErrorCodes::ERROR_NO_SPACE: return "No space";
        case OFSErrorCodes::ERROR_INVALID_CONFIG: return "Invalid config";
        case OFSErrorCodes::ERROR_NOT_IMPLEMENTED: return "Not implemented";
        case OFSErrorCodes::ERROR_INVALID_SESSION: return "Invalid session";
        case OFSErrorCodes::ERROR_DIRECTORY_NOT_EMPTY: return "Directory not empty";
        case OFSErrorCodes::ERROR_INVALID_OPERATION: return "Invalid operation";
        default: return "Unknown error";
    }
}