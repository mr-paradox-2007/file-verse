#ifndef FS_FORMAT_HPP
#define FS_FORMAT_HPP

#include "ofs_types.hpp"
#include <string>

/**
 * File system formatting and initialization
 * 
 * Responsible for:
 * - Creating new .omni files with proper structure
 * - Writing headers, user tables, metadata areas
 * - Initializing free space tracking
 * - Setting up root directory
 */

/**
 * Format a new .omni file with given configuration
 * Creates the file with all necessary structures initialized
 * 
 * Returns: SUCCESS if file created, error code otherwise
 */
int fs_format(const std::string& omni_path, const std::string& config_path);

/**
 * Validate an existing .omni file
 * Checks magic number, version, sizes, etc.
 * 
 * Returns: SUCCESS if valid, error code otherwise
 */
int fs_validate(const std::string& omni_path);

/**
 * Get size requirements for .omni file based on config
 */
uint64_t fs_calculate_size(const std::string& config_path);

#endif
