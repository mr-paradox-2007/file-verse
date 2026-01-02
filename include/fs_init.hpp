#ifndef FS_INIT_HPP
#define FS_INIT_HPP

#include "ofs_types.hpp"
#include <string>

/**
 * File system initialization and shutdown
 * 
 * Responsible for:
 * - Loading entire .omni file into memory
 * - Building efficient in-memory indices
 * - Preparing user tables
 * - Initializing free space structures
 */

// Opaque file system instance handle
typedef void* OFS_Instance;

/**
 * Initialize file system
 * Loads .omni file and prepares for operations
 * 
 * instance: Output pointer to file system handle
 * omni_path: Path to .omni file to load
 * config_path: Path to configuration file
 * private_key: Private key for security (stealth mode)
 * 
 * Returns: SUCCESS if initialized, error code otherwise
 */
int fs_init(OFS_Instance* instance, const std::string& omni_path, 
            const std::string& config_path, const std::string& private_key);

/**
 * Shutdown file system
 * Flushes all changes to disk and releases memory
 */
int fs_shutdown(OFS_Instance instance);

/**
 * Get current instance (for global access if needed)
 */
OFS_Instance fs_get_current_instance();

#endif
