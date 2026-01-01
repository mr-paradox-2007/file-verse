#include "ofs_types.hpp"
#include "config_parser.hpp"
#include "logger.hpp"
#include <fstream>
#include <cstring>
#include <ctime>
#include <vector>
#include <iomanip>
#include <sstream>

namespace ofs
{

/**
 * Simple SHA-256-like hash for password (for demo purposes)
 * In production, use a proper cryptographic library
 */
std::string computePasswordHash(const std::string& password)
{
    // For Phase 1, we'll use a simple XOR-based hash (not cryptographically secure)
    // This is just for demonstration. Replace with proper SHA-256 in production.
    std::string hash;
    hash.reserve(64);
    
    unsigned int seed = 5381;
    for (char c : password)
    {
        seed = ((seed << 5) + seed) + static_cast<unsigned char>(c);
    }
    
    // Convert to hex string (64 characters)
    char buffer[65];
    for (int i = 0; i < 8; i++)
    {
        snprintf(buffer + (i * 8), 9, "%08x", seed ^ (i * 0x12345678));
    }
    buffer[64] = '\0';
    hash = buffer;
    
    return hash;
}

/**
 * fs_format() - Initialize a new .omni filesystem file
 * 
 * Creates an empty .omni file with:
 * 1. OMNIHeader with magic number and configuration
 * 2. User table with admin user
 * 3. Root directory entry
 * 4. Empty content block area
 * 
 * @param filename Path to the .omni file to create
 * @param config Configuration object with filesystem parameters
 * @return OFSErrorCodes::SUCCESS if successful, error code otherwise
 */
OFSErrorCodes fs_format(const std::string& filename, const Config& config)
{
    LOG_INFO("FS_FORMAT", 0,
            "Starting filesystem format: " + filename);
    
    // ========================================================================
    // STEP 1: Create and initialize OMNIHeader
    // ========================================================================
    
    OMNIHeader header;
    
    // Set magic number
    std::strncpy(header.magic, "OMNIFS01", sizeof(header.magic) - 1);
    header.magic[sizeof(header.magic) - 1] = '\0';
    
    // Set version
    header.format_version = 0x00010000;  // v1.0
    
    // Set size parameters
    header.total_size = config.total_size;
    header.header_size = config.header_size;
    header.block_size = config.block_size;
    
    // Set user table information
    // User table starts right after the header
    header.user_table_offset = static_cast<uint32_t>(sizeof(OMNIHeader));
    header.max_users = config.max_users;
    
    // Set offsets for future use (Phase 2)
    // File state storage starts after user table
    uint64_t user_table_size = config.max_users * sizeof(UserInfo);
    header.file_state_storage_offset = static_cast<uint32_t>(header.user_table_offset + user_table_size);
    
    // Change log offset
    header.change_log_offset = static_cast<uint32_t>(header.file_state_storage_offset + 65536);
    
    // Set student ID and date
    std::strncpy(header.student_id, "BSAI24056-Semester-03", sizeof(header.student_id) - 1);
    header.student_id[sizeof(header.student_id) - 1] = '\0';
    
    // Set submission date
    time_t now = std::time(nullptr);
    struct tm* timeinfo = std::localtime(&now);
    std::strftime(header.submission_date, sizeof(header.submission_date), "%Y-%m-%d", timeinfo);
    
    LOG_DEBUG("FS_FORMAT", 1,
            "OMNIHeader initialized: magic=" + std::string(header.magic) +
            ", version=0x" + std::to_string(header.format_version) +
            ", total_size=" + std::to_string(header.total_size) +
            ", block_size=" + std::to_string(header.block_size));
    
    // ========================================================================
    // STEP 2: Create user table
    // ========================================================================
    
    std::vector<UserInfo> user_table(config.max_users);
    
    // Initialize all users as inactive
    for (uint32_t i = 0; i < config.max_users; i++)
    {
        user_table[i].is_active = 0;
        std::memset(user_table[i].username, 0, sizeof(user_table[i].username));
        std::memset(user_table[i].password_hash, 0, sizeof(user_table[i].password_hash));
        std::memset(user_table[i].reserved, 0, sizeof(user_table[i].reserved));
    }
    
    // Create admin user in slot 0
    UserInfo admin;
    std::strncpy(admin.username, config.admin_username.c_str(), sizeof(admin.username) - 1);
    admin.username[sizeof(admin.username) - 1] = '\0';
    
    std::string password_hash = computePasswordHash(config.admin_password);
    std::strncpy(admin.password_hash, password_hash.c_str(), sizeof(admin.password_hash) - 1);
    admin.password_hash[sizeof(admin.password_hash) - 1] = '\0';
    
    admin.role = UserRole::ADMIN;
    admin.created_time = static_cast<uint64_t>(now);
    admin.last_login = 0;
    admin.is_active = 1;
    
    user_table[0] = admin;
    
    LOG_DEBUG("FS_FORMAT", 2,
            "User table created with admin user: " + std::string(admin.username));
    
    // ========================================================================
    // STEP 3: Create root directory entry
    // ========================================================================
    
    // Root directory metadata will be stored at the beginning of metadata area
    FileEntry root_dir;
    std::strncpy(root_dir.name, "/", sizeof(root_dir.name) - 1);
    root_dir.name[sizeof(root_dir.name) - 1] = '\0';
    
    root_dir.type = static_cast<uint8_t>(EntryType::DIRECTORY);
    root_dir.size = 0;  // Directories don't have size
    root_dir.permissions = 0755;  // rwxr-xr-x
    root_dir.created_time = static_cast<uint64_t>(now);
    root_dir.modified_time = static_cast<uint64_t>(now);
    
    std::strncpy(root_dir.owner, config.admin_username.c_str(), sizeof(root_dir.owner) - 1);
    root_dir.owner[sizeof(root_dir.owner) - 1] = '\0';
    
    root_dir.inode = 1;  // Root directory has inode 1
    std::memset(root_dir.reserved, 0, sizeof(root_dir.reserved));
    
    LOG_DEBUG("FS_FORMAT", 3,
            "Root directory entry created with inode=1");
    
    // ========================================================================
    // STEP 4: Write everything to file
    // ========================================================================
    
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open())
    {
        LOG_ERROR("FS_FORMAT", 100,
                "Cannot open file for writing: " + filename);
        return OFSErrorCodes::ERROR_IO_ERROR;
    }
    
    LOG_DEBUG("FS_FORMAT", 4, "Writing OMNIHeader to file");
    
    // Write header
    file.write(reinterpret_cast<char*>(&header), sizeof(OMNIHeader));
    if (!file.good())
    {
        LOG_ERROR("FS_FORMAT", 101,
                "Failed to write OMNIHeader to file");
        file.close();
        return OFSErrorCodes::ERROR_IO_ERROR;
    }
    
    LOG_DEBUG("FS_FORMAT", 5, "Writing user table to file");
    
    // Write user table
    for (uint32_t i = 0; i < config.max_users; i++)
    {
        file.write(reinterpret_cast<char*>(&user_table[i]), sizeof(UserInfo));
        if (!file.good())
        {
            LOG_ERROR("FS_FORMAT", 102,
                    "Failed to write user table to file at index " + std::to_string(i));
            file.close();
            return OFSErrorCodes::ERROR_IO_ERROR;
        }
    }
    
    LOG_DEBUG("FS_FORMAT", 6, "Writing root directory entry to file");
    
    // Write root directory entry at the beginning of metadata area
    file.write(reinterpret_cast<char*>(&root_dir), sizeof(FileEntry));
    if (!file.good())
    {
        LOG_ERROR("FS_FORMAT", 103,
                "Failed to write root directory entry");
        file.close();
        return OFSErrorCodes::ERROR_IO_ERROR;
    }
    
    // Calculate size of metadata area
    // For now, reserve space for max_files entries
    uint64_t metadata_area_size = config.max_files * sizeof(FileEntry);
    uint64_t written_size = sizeof(OMNIHeader) + (config.max_users * sizeof(UserInfo)) + sizeof(FileEntry);
    uint64_t remaining_metadata = metadata_area_size - sizeof(FileEntry);
    
    LOG_DEBUG("FS_FORMAT", 7,
            "Writing metadata area padding: " + std::to_string(remaining_metadata) + " bytes");
    
    // Pad the rest of the metadata area with zeros
    std::vector<uint8_t> padding(remaining_metadata, 0);
    file.write(reinterpret_cast<char*>(padding.data()), remaining_metadata);
    if (!file.good())
    {
        LOG_ERROR("FS_FORMAT", 104,
                "Failed to write metadata area padding");
        file.close();
        return OFSErrorCodes::ERROR_IO_ERROR;
    }
    
    // Calculate remaining space for content blocks
    uint64_t metadata_end = sizeof(OMNIHeader) + 
                           (config.max_users * sizeof(UserInfo)) + 
                           metadata_area_size;
    
    // Reserve 64KB for free space tracking
    uint64_t free_space_tracking_size = 65536;
    uint64_t content_area_start = metadata_end + free_space_tracking_size;
    
    if (content_area_start > config.total_size)
    {
        LOG_ERROR("FS_FORMAT", 105,
                "Not enough space in filesystem: required=" + std::to_string(content_area_start) +
                ", total_size=" + std::to_string(config.total_size));
        file.close();
        return OFSErrorCodes::ERROR_NO_SPACE;
    }
    
    LOG_DEBUG("FS_FORMAT", 8,
            "Writing free space tracking area padding: " + std::to_string(free_space_tracking_size) + " bytes");
    
    // Pad free space tracking area
    std::vector<uint8_t> free_space_padding(free_space_tracking_size, 0xFF);  // Mark all as free initially
    file.write(reinterpret_cast<char*>(free_space_padding.data()), free_space_tracking_size);
    if (!file.good())
    {
        LOG_ERROR("FS_FORMAT", 106,
                "Failed to write free space tracking area");
        file.close();
        return OFSErrorCodes::ERROR_IO_ERROR;
    }
    
    LOG_DEBUG("FS_FORMAT", 9,
            "Writing content block area padding");
    
    // Fill remaining space with zeros (content blocks area)
    uint64_t content_area_size = config.total_size - content_area_start;
    uint64_t remaining = content_area_size;
    const uint64_t chunk_size = 1048576;  // 1MB chunks for efficiency
    
    std::vector<uint8_t> zero_chunk(std::min(chunk_size, remaining), 0);
    
    while (remaining > 0)
    {
        uint64_t to_write = std::min(chunk_size, remaining);
        file.write(reinterpret_cast<char*>(zero_chunk.data()), to_write);
        if (!file.good())
        {
            LOG_ERROR("FS_FORMAT", 107,
                    "Failed to write content area, remaining=" + std::to_string(remaining));
            file.close();
            return OFSErrorCodes::ERROR_IO_ERROR;
        }
        remaining -= to_write;
    }
    
    file.close();
    
    LOG_INFO("FS_FORMAT", 0,
            "Filesystem format completed successfully: " + filename +
            " (size=" + std::to_string(config.total_size) + " bytes)");
    
    // ========================================================================
    // STEP 5: Verification logging
    // ========================================================================
    
    LOG_INFO("FS_FORMAT", 10,
            "File system structure:\n" +
            std::string("  Header: 512 bytes (offset 0x0)\n") +
            std::string("  User Table: ") + std::to_string(config.max_users * sizeof(UserInfo)) +
            std::string(" bytes (offset 0x") + std::to_string(header.user_table_offset) + std::string(")\n") +
            std::string("  Metadata Area: ") + std::to_string(metadata_area_size) + std::string(" bytes\n") +
            std::string("  Free Space Tracking: ") + std::to_string(free_space_tracking_size) + std::string(" bytes\n") +
            std::string("  Content Blocks: ") + std::to_string(content_area_size) + std::string(" bytes\n") +
            std::string("  Total: ") + std::to_string(config.total_size) + std::string(" bytes"));
    
    return OFSErrorCodes::SUCCESS;
}

}  // namespace ofs
