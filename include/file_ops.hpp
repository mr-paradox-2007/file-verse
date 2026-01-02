#ifndef FILE_OPS_HPP
#define FILE_OPS_HPP

#include "ofs_types.hpp"
#include <string>
#include <vector>
#include <memory>
#include <cstdint>

/**
 * Internal file system instance (opaque handle)
 * Holds all in-memory state and file handle
 */
typedef void* OFS_Instance;

/**
 * Session handle (represents authenticated user session)
 */
typedef void* OFS_Session;

// ============================================================================
// FILE OPERATIONS
// ============================================================================

/**
 * Create a new file with initial data
 * Path must not exist
 */
int file_create(OFS_Session session, const std::string& path, 
                const void* data, size_t size);

/**
 * Read entire file content
 * Allocates buffer internally (must be freed with free_buffer)
 */
int file_read(OFS_Session session, const std::string& path,
              void** out_buffer, size_t* out_size);

/**
 * Edit file content at specific index
 * Can write at any offset, extends file if needed
 */
int file_edit(OFS_Session session, const std::string& path,
              const void* data, size_t size, uint64_t index);

/**
 * Delete file completely
 * Path must exist and be a regular file
 */
int file_delete(OFS_Session session, const std::string& path);

/**
 * Truncate file to 0 bytes
 * Keeps file but clears content
 */
int file_truncate(OFS_Session session, const std::string& path);

/**
 * Check if file exists
 * Returns SUCCESS if exists, ERROR_NOT_FOUND if not
 */
int file_exists(OFS_Session session, const std::string& path);

/**
 * Rename or move file to new path
 * new_path must not exist
 */
int file_rename(OFS_Session session, const std::string& old_path,
                const std::string& new_path);

/**
 * Get file metadata
 */
int get_metadata(OFS_Session session, const std::string& path,
                 FileMetadata* metadata);

/**
 * Set file permissions (Unix-style: 0644, etc.)
 */
int set_permissions(OFS_Session session, const std::string& path,
                   uint32_t permissions);

// ============================================================================
// DIRECTORY OPERATIONS
// ============================================================================

/**
 * Create new directory
 * Parent must exist
 */
int dir_create(OFS_Session session, const std::string& path);

/**
 * List directory contents
 * Allocates array internally (must be freed with free_buffer)
 */
int dir_list(OFS_Session session, const std::string& path,
             FileEntry** out_entries, int* out_count);

/**
 * Delete directory
 * Must be empty
 */
int dir_delete(OFS_Session session, const std::string& path);

/**
 * Check if directory exists
 */
int dir_exists(OFS_Session session, const std::string& path);

// ============================================================================
// SESSION AND SYSTEM OPERATIONS
// ============================================================================

/**
 * Free memory allocated by file_read or dir_list
 */
void free_buffer(void* buffer);

/**
 * Get detailed statistics about file system
 */
int get_stats(OFS_Session session, FSStats* stats);

/**
 * Get error message for error code
 */
const char* get_error_message(int error_code);

#endif
