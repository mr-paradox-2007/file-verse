#ifndef FS_FORMAT_HPP
#define FS_FORMAT_HPP

#include "ofs_types.hpp"
#include "config_parser.hpp"

namespace ofs
{

/**
 * Initialize and format a new .omni filesystem file
 * 
 * This function creates an empty, properly structured .omni container file with:
 * 1. OMNIHeader with magic number and filesystem parameters
 * 2. User table initialized with admin user
 * 3. Root directory entry (/)
 * 4. Empty metadata area (reserved for file entries)
 * 5. Free space tracking area
 * 6. Empty content block area
 * 
 * The resulting file is ready for immediate use by the filesystem core.
 * 
 * @param filename Path where the .omni file will be created
 * @param config Configuration object with all filesystem parameters
 * 
 * @return OFSErrorCodes::SUCCESS if format succeeded
 * @return OFSErrorCodes::ERROR_IO_ERROR if file cannot be written
 * @return OFSErrorCodes::ERROR_NO_SPACE if total_size is too small for metadata
 * @return OFSErrorCodes::ERROR_INVALID_CONFIG if config values are invalid
 * 
 * Example usage:
 * @code
 *   Config cfg = ConfigParser::getInstance().getDefaultConfig();
 *   OFSErrorCodes result = fs_format("filesystem.omni", cfg);
 *   if (result != OFSErrorCodes::SUCCESS) {
 *       // Handle error
 *   }
 * @endcode
 */
OFSErrorCodes fs_format(const std::string& filename, const Config& config);

}  // namespace ofs

#endif  // FS_FORMAT_HPP
