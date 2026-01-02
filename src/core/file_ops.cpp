#include "file_ops.hpp"
#include "logger.hpp"
#include "path_resolver.hpp"
#include <map>
#include <vector>
#include <mutex>

// In-memory file system structure
struct FileNode {
    FileEntry entry;
    std::vector<char> data;
    std::vector<uint32_t> children;  // For directories: child file indices
};

static std::vector<FileNode> file_tree;
static std::map<std::string, uint32_t> path_index;  // path -> file index
static std::mutex fs_mutex;

// ============================================================================
// FILE OPERATIONS
// ============================================================================

int file_create(OFS_Session session, const std::string& path,
                const void* data, size_t size) {
    
    int validation = PathResolver::validate_path(path);
    if (validation != static_cast<int>(OFSErrorCodes::SUCCESS)) {
        return validation;
    }
    
    std::lock_guard<std::mutex> lock(fs_mutex);
    
    // Check if file already exists
    if (path_index.find(path) != path_index.end()) {
        return static_cast<int>(OFSErrorCodes::ERROR_FILE_EXISTS);
    }
    
    // Create file entry
    FileNode node;
    node.entry.setType(EntryType::FILE);
    std::strncpy(node.entry.name, PathResolver::get_filename(path).c_str(), sizeof(node.entry.name) - 1);
    node.entry.size = size;
    node.entry.permissions = 0644;
    node.entry.created_time = std::time(nullptr);
    node.entry.modified_time = std::time(nullptr);
    node.entry.inode = file_tree.size();
    
    if (data && size > 0) {
        node.data.assign((char*)data, (char*)data + size);
    }
    
    uint32_t file_index = file_tree.size();
    file_tree.push_back(node);
    path_index[path] = file_index;
    
    Logger::log_file_op("CREATE_FILE", path, "system", true);
    return static_cast<int>(OFSErrorCodes::SUCCESS);
}

int file_read(OFS_Session session, const std::string& path,
              void** out_buffer, size_t* out_size) {
    
    int validation = PathResolver::validate_path(path);
    if (validation != static_cast<int>(OFSErrorCodes::SUCCESS)) {
        return validation;
    }
    
    std::lock_guard<std::mutex> lock(fs_mutex);
    
    if (path_index.find(path) == path_index.end()) {
        return static_cast<int>(OFSErrorCodes::ERROR_NOT_FOUND);
    }
    
    uint32_t file_index = path_index[path];
    FileNode& node = file_tree[file_index];
    
    if (node.entry.getType() != EntryType::FILE) {
        return static_cast<int>(OFSErrorCodes::ERROR_INVALID_OPERATION);
    }
    
    *out_size = node.data.size();
    if (*out_size == 0) {
        *out_buffer = nullptr;
    } else {
        *out_buffer = new char[*out_size];
        std::memcpy(*out_buffer, node.data.data(), *out_size);
    }
    
    Logger::log_file_op("READ_FILE", path, "system", true);
    return static_cast<int>(OFSErrorCodes::SUCCESS);
}

int file_edit(OFS_Session session, const std::string& path,
              const void* data, size_t size, uint64_t index) {
    
    int validation = PathResolver::validate_path(path);
    if (validation != static_cast<int>(OFSErrorCodes::SUCCESS)) {
        return validation;
    }
    
    std::lock_guard<std::mutex> lock(fs_mutex);
    
    if (path_index.find(path) == path_index.end()) {
        return static_cast<int>(OFSErrorCodes::ERROR_NOT_FOUND);
    }
    
    uint32_t file_index = path_index[path];
    FileNode& node = file_tree[file_index];
    
    if (node.entry.getType() != EntryType::FILE) {
        return static_cast<int>(OFSErrorCodes::ERROR_INVALID_OPERATION);
    }
    
    // Extend file if needed
    if (index + size > node.data.size()) {
        node.data.resize(index + size, 0);
    }
    
    std::memcpy(node.data.data() + index, data, size);
    node.entry.size = node.data.size();
    node.entry.modified_time = std::time(nullptr);
    
    Logger::log_file_op("EDIT_FILE", path, "system", true);
    return static_cast<int>(OFSErrorCodes::SUCCESS);
}

int file_delete(OFS_Session session, const std::string& path) {
    
    int validation = PathResolver::validate_path(path);
    if (validation != static_cast<int>(OFSErrorCodes::SUCCESS)) {
        return validation;
    }
    
    std::lock_guard<std::mutex> lock(fs_mutex);
    
    if (path_index.find(path) == path_index.end()) {
        return static_cast<int>(OFSErrorCodes::ERROR_NOT_FOUND);
    }
    
    uint32_t file_index = path_index[path];
    FileNode& node = file_tree[file_index];
    
    if (node.entry.getType() != EntryType::FILE) {
        return static_cast<int>(OFSErrorCodes::ERROR_INVALID_OPERATION);
    }
    
    node.data.clear();
    path_index.erase(path);
    
    Logger::log_file_op("DELETE_FILE", path, "system", true);
    return static_cast<int>(OFSErrorCodes::SUCCESS);
}

int file_truncate(OFS_Session session, const std::string& path) {
    
    int validation = PathResolver::validate_path(path);
    if (validation != static_cast<int>(OFSErrorCodes::SUCCESS)) {
        return validation;
    }
    
    std::lock_guard<std::mutex> lock(fs_mutex);
    
    if (path_index.find(path) == path_index.end()) {
        return static_cast<int>(OFSErrorCodes::ERROR_NOT_FOUND);
    }
    
    uint32_t file_index = path_index[path];
    FileNode& node = file_tree[file_index];
    
    node.data.clear();
    node.entry.size = 0;
    node.entry.modified_time = std::time(nullptr);
    
    Logger::log_file_op("TRUNCATE_FILE", path, "system", true);
    return static_cast<int>(OFSErrorCodes::SUCCESS);
}

int file_exists(OFS_Session session, const std::string& path) {
    
    int validation = PathResolver::validate_path(path);
    if (validation != static_cast<int>(OFSErrorCodes::SUCCESS)) {
        return validation;
    }
    
    std::lock_guard<std::mutex> lock(fs_mutex);
    
    if (path_index.find(path) != path_index.end()) {
        return static_cast<int>(OFSErrorCodes::SUCCESS);
    }
    
    return static_cast<int>(OFSErrorCodes::ERROR_NOT_FOUND);
}

int file_rename(OFS_Session session, const std::string& old_path,
                const std::string& new_path) {
    
    int validation1 = PathResolver::validate_path(old_path);
    int validation2 = PathResolver::validate_path(new_path);
    
    if (validation1 != static_cast<int>(OFSErrorCodes::SUCCESS) ||
        validation2 != static_cast<int>(OFSErrorCodes::SUCCESS)) {
        return static_cast<int>(OFSErrorCodes::ERROR_INVALID_PATH);
    }
    
    std::lock_guard<std::mutex> lock(fs_mutex);
    
    if (path_index.find(old_path) == path_index.end()) {
        return static_cast<int>(OFSErrorCodes::ERROR_NOT_FOUND);
    }
    
    if (path_index.find(new_path) != path_index.end()) {
        return static_cast<int>(OFSErrorCodes::ERROR_FILE_EXISTS);
    }
    
    uint32_t file_index = path_index[old_path];
    path_index[new_path] = file_index;
    path_index.erase(old_path);
    
    std::strncpy(file_tree[file_index].entry.name, 
                 PathResolver::get_filename(new_path).c_str(),
                 sizeof(file_tree[file_index].entry.name) - 1);
    
    Logger::log_file_op("RENAME_FILE", old_path + " -> " + new_path, "system", true);
    return static_cast<int>(OFSErrorCodes::SUCCESS);
}

int get_metadata(OFS_Session session, const std::string& path,
                 FileMetadata* metadata) {
    
    int validation = PathResolver::validate_path(path);
    if (validation != static_cast<int>(OFSErrorCodes::SUCCESS)) {
        return validation;
    }
    
    std::lock_guard<std::mutex> lock(fs_mutex);
    
    if (path_index.find(path) == path_index.end()) {
        return static_cast<int>(OFSErrorCodes::ERROR_NOT_FOUND);
    }
    
    uint32_t file_index = path_index[path];
    metadata->entry = file_tree[file_index].entry;
    metadata->blocks_used = (file_tree[file_index].data.size() + 4095) / 4096;
    metadata->actual_size = file_tree[file_index].data.size();
    
    std::strncpy(metadata->path, path.c_str(), sizeof(metadata->path) - 1);
    
    return static_cast<int>(OFSErrorCodes::SUCCESS);
}

int set_permissions(OFS_Session session, const std::string& path,
                   uint32_t permissions) {
    
    int validation = PathResolver::validate_path(path);
    if (validation != static_cast<int>(OFSErrorCodes::SUCCESS)) {
        return validation;
    }
    
    std::lock_guard<std::mutex> lock(fs_mutex);
    
    if (path_index.find(path) == path_index.end()) {
        return static_cast<int>(OFSErrorCodes::ERROR_NOT_FOUND);
    }
    
    uint32_t file_index = path_index[path];
    file_tree[file_index].entry.permissions = permissions;
    
    return static_cast<int>(OFSErrorCodes::SUCCESS);
}

// ============================================================================
// DIRECTORY OPERATIONS
// ============================================================================

int dir_create(OFS_Session session, const std::string& path) {
    
    int validation = PathResolver::validate_path(path);
    if (validation != static_cast<int>(OFSErrorCodes::SUCCESS)) {
        return validation;
    }
    
    std::lock_guard<std::mutex> lock(fs_mutex);
    
    if (path_index.find(path) != path_index.end()) {
        return static_cast<int>(OFSErrorCodes::ERROR_FILE_EXISTS);
    }
    
    // Create directory entry
    FileNode node;
    node.entry.setType(EntryType::DIRECTORY);
    std::strncpy(node.entry.name, PathResolver::get_filename(path).c_str(), sizeof(node.entry.name) - 1);
    node.entry.size = 0;
    node.entry.permissions = 0755;
    node.entry.created_time = std::time(nullptr);
    node.entry.modified_time = std::time(nullptr);
    node.entry.inode = file_tree.size();
    
    uint32_t dir_index = file_tree.size();
    file_tree.push_back(node);
    path_index[path] = dir_index;
    
    Logger::log_file_op("CREATE_DIR", path, "system", true);
    return static_cast<int>(OFSErrorCodes::SUCCESS);
}

int dir_list(OFS_Session session, const std::string& path,
             FileEntry** out_entries, int* out_count) {
    
    int validation = PathResolver::validate_path(path);
    if (validation != static_cast<int>(OFSErrorCodes::SUCCESS)) {
        return validation;
    }
    
    std::lock_guard<std::mutex> lock(fs_mutex);
    
    if (path_index.find(path) == path_index.end()) {
        return static_cast<int>(OFSErrorCodes::ERROR_NOT_FOUND);
    }
    
    uint32_t dir_index = path_index[path];
    FileNode& node = file_tree[dir_index];
    
    if (node.entry.getType() != EntryType::DIRECTORY) {
        return static_cast<int>(OFSErrorCodes::ERROR_INVALID_OPERATION);
    }
    
    // Count children in this directory
    *out_count = node.children.size();
    
    if (*out_count == 0) {
        *out_entries = nullptr;
        return static_cast<int>(OFSErrorCodes::SUCCESS);
    }
    
    *out_entries = new FileEntry[*out_count];
    for (int i = 0; i < *out_count; i++) {
        (*out_entries)[i] = file_tree[node.children[i]].entry;
    }
    
    Logger::log_file_op("LIST_DIR", path, "system", true);
    return static_cast<int>(OFSErrorCodes::SUCCESS);
}

int dir_delete(OFS_Session session, const std::string& path) {
    
    int validation = PathResolver::validate_path(path);
    if (validation != static_cast<int>(OFSErrorCodes::SUCCESS)) {
        return validation;
    }
    
    std::lock_guard<std::mutex> lock(fs_mutex);
    
    if (path_index.find(path) == path_index.end()) {
        return static_cast<int>(OFSErrorCodes::ERROR_NOT_FOUND);
    }
    
    uint32_t dir_index = path_index[path];
    FileNode& node = file_tree[dir_index];
    
    if (node.entry.getType() != EntryType::DIRECTORY) {
        return static_cast<int>(OFSErrorCodes::ERROR_INVALID_OPERATION);
    }
    
    if (!node.children.empty()) {
        return static_cast<int>(OFSErrorCodes::ERROR_DIRECTORY_NOT_EMPTY);
    }
    
    path_index.erase(path);
    
    Logger::log_file_op("DELETE_DIR", path, "system", true);
    return static_cast<int>(OFSErrorCodes::SUCCESS);
}

int dir_exists(OFS_Session session, const std::string& path) {
    
    int validation = PathResolver::validate_path(path);
    if (validation != static_cast<int>(OFSErrorCodes::SUCCESS)) {
        return validation;
    }
    
    std::lock_guard<std::mutex> lock(fs_mutex);
    
    if (path_index.find(path) != path_index.end()) {
        if (file_tree[path_index[path]].entry.getType() == EntryType::DIRECTORY) {
            return static_cast<int>(OFSErrorCodes::SUCCESS);
        }
    }
    
    return static_cast<int>(OFSErrorCodes::ERROR_NOT_FOUND);
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

void free_buffer(void* buffer) {
    delete[] reinterpret_cast<char*>(buffer);
}

int get_stats(OFS_Session session, FSStats* stats) {
    std::lock_guard<std::mutex> lock(fs_mutex);
    
    stats->total_files = 0;
    stats->total_directories = 0;
    stats->used_space = 0;
    
    for (const auto& node : file_tree) {
        if (node.entry.getType() == EntryType::FILE) {
            stats->total_files++;
            stats->used_space += node.data.size();
        } else {
            stats->total_directories++;
        }
    }
    
    stats->total_size = 104857600;  // Default 100MB
    stats->free_space = stats->total_size - stats->used_space;
    stats->fragmentation = 0.0;
    
    return static_cast<int>(OFSErrorCodes::SUCCESS);
}

const char* get_error_message(int error_code) {
    switch (static_cast<OFSErrorCodes>(error_code)) {
        case OFSErrorCodes::SUCCESS: return "Success";
        case OFSErrorCodes::ERROR_NOT_FOUND: return "File or resource not found";
        case OFSErrorCodes::ERROR_PERMISSION_DENIED: return "Permission denied";
        case OFSErrorCodes::ERROR_IO_ERROR: return "I/O error";
        case OFSErrorCodes::ERROR_INVALID_PATH: return "Invalid path";
        case OFSErrorCodes::ERROR_FILE_EXISTS: return "File already exists";
        case OFSErrorCodes::ERROR_NO_SPACE: return "No space available";
        case OFSErrorCodes::ERROR_INVALID_CONFIG: return "Invalid configuration";
        case OFSErrorCodes::ERROR_NOT_IMPLEMENTED: return "Feature not yet implemented";
        case OFSErrorCodes::ERROR_INVALID_SESSION: return "Invalid or expired session";
        case OFSErrorCodes::ERROR_DIRECTORY_NOT_EMPTY: return "Directory is not empty";
        case OFSErrorCodes::ERROR_INVALID_OPERATION: return "Invalid operation";
        default: return "Unknown error";
    }
}
