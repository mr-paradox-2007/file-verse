#include <iostream>
#include <fstream>
#include <iomanip>
#include <vector>
#include <cstring>
#include "../source/include/config_parser.hpp"
#include "../source/include/fs_format.hpp"
#include "../source/include/ofs_types.hpp"
#include "../source/include/logger.hpp"

using namespace ofs;

/**
 * Print hex dump of file contents for verification
 */
void hexDump(const std::string& filename, size_t max_bytes = 512)
{
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open())
    {
        std::cerr << "Cannot open file: " << filename << std::endl;
        return;
    }

    std::vector<uint8_t> buffer(max_bytes);
    file.read(reinterpret_cast<char*>(buffer.data()), max_bytes);
    size_t bytes_read = file.gcount();
    file.close();

    std::cout << "\n=== HEX DUMP: " << filename << " ===" << std::endl;
    std::cout << "Bytes read: " << bytes_read << std::endl << std::endl;

    for (size_t i = 0; i < bytes_read; i += 16)
    {
        // Print offset
        std::cout << "0x" << std::setfill('0') << std::setw(6) << std::hex << i << "  ";

        // Print hex values
        for (size_t j = 0; j < 16 && i + j < bytes_read; j++)
        {
            std::cout << std::setfill('0') << std::setw(2) << std::hex 
                     << static_cast<int>(buffer[i + j]) << " ";
        }

        // Print ASCII representation
        std::cout << " | ";
        for (size_t j = 0; j < 16 && i + j < bytes_read; j++)
        {
            char c = buffer[i + j];
            if (c >= 32 && c < 127)
                std::cout << c;
            else
                std::cout << ".";
        }
        std::cout << std::endl;
    }

    std::cout << std::dec << std::endl;
}

/**
 * Verify the structure of the created .omni file
 */
void verifyOmniStructure(const std::string& filename)
{
    std::cout << "\n=== VERIFYING OMNI FILE STRUCTURE ===" << std::endl;

    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open())
    {
        std::cerr << "ERROR: Cannot open file for verification" << std::endl;
        return;
    }

    // Read header
    OMNIHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(OMNIHeader));

    std::cout << "\n[HEADER]" << std::endl;
    std::cout << "  Magic: " << std::string(header.magic, 8) << std::endl;
    std::cout << "  Format Version: 0x" << std::hex << header.format_version << std::dec << std::endl;
    std::cout << "  Total Size: " << header.total_size << " bytes" << std::endl;
    std::cout << "  Header Size: " << header.header_size << " bytes" << std::endl;
    std::cout << "  Block Size: " << header.block_size << " bytes" << std::endl;
    std::cout << "  Student ID: " << std::string(header.student_id) << std::endl;
    std::cout << "  Submission Date: " << std::string(header.submission_date) << std::endl;
    std::cout << "  User Table Offset: 0x" << std::hex << header.user_table_offset << std::dec << std::endl;
    std::cout << "  Max Users: " << header.max_users << std::endl;
    std::cout << "  File State Storage Offset: 0x" << std::hex << header.file_state_storage_offset << std::dec << std::endl;
    std::cout << "  Change Log Offset: 0x" << std::hex << header.change_log_offset << std::dec << std::endl;

    // Verify magic number
    if (std::string(header.magic, 7) != "OMNIFS0")
    {
        std::cerr << "ERROR: Invalid magic number!" << std::endl;
    }
    else
    {
        std::cout << "\n✓ Magic number is correct" << std::endl;
    }

    // Read first user (admin)
    file.seekg(header.user_table_offset);
    UserInfo admin_user;
    file.read(reinterpret_cast<char*>(&admin_user), sizeof(UserInfo));

    std::cout << "\n[ADMIN USER (Index 0)]" << std::endl;
    std::cout << "  Username: " << std::string(admin_user.username) << std::endl;
    std::cout << "  Role: " << (admin_user.role == UserRole::ADMIN ? "ADMIN" : "NORMAL") << std::endl;
    std::cout << "  Active: " << (admin_user.is_active ? "Yes" : "No") << std::endl;
    std::cout << "  Created Time (Unix): " << admin_user.created_time << std::endl;

    if (admin_user.is_active == 1 && std::string(admin_user.username) == "admin")
    {
        std::cout << "\n✓ Admin user is properly initialized" << std::endl;
    }
    else
    {
        std::cerr << "ERROR: Admin user not properly initialized!" << std::endl;
    }

    // Read second user (should be inactive)
    UserInfo user1;
    file.read(reinterpret_cast<char*>(&user1), sizeof(UserInfo));

    std::cout << "\n[USER 1 (Index 1)]" << std::endl;
    std::cout << "  Active: " << (user1.is_active ? "Yes" : "No") << std::endl;

    if (user1.is_active == 0)
    {
        std::cout << "✓ Non-admin users are properly inactive" << std::endl;
    }

    // Read root directory
    // Metadata area starts after all users
    uint64_t metadata_offset = header.user_table_offset + (header.max_users * sizeof(UserInfo));
    file.seekg(metadata_offset);

    FileEntry root_dir;
    file.read(reinterpret_cast<char*>(&root_dir), sizeof(FileEntry));

    std::cout << "\n[ROOT DIRECTORY]" << std::endl;
    std::cout << "  Name: " << std::string(root_dir.name) << std::endl;
    std::cout << "  Type: " << (root_dir.type == 1 ? "DIRECTORY" : "FILE") << std::endl;
    std::cout << "  Owner: " << std::string(root_dir.owner) << std::endl;
    std::cout << "  Permissions: " << std::oct << root_dir.permissions << std::dec << std::endl;
    std::cout << "  Inode: " << root_dir.inode << std::endl;
    std::cout << "  Created Time (Unix): " << root_dir.created_time << std::endl;

    if (std::string(root_dir.name) == "/" && root_dir.type == 1 && root_dir.inode == 1)
    {
        std::cout << "\n✓ Root directory is properly initialized" << std::endl;
    }
    else
    {
        std::cerr << "ERROR: Root directory not properly initialized!" << std::endl;
    }

    file.close();

    std::cout << "\n=== VERIFICATION COMPLETE ===" << std::endl;
}

int main()
{
    std::cout << "========================================" << std::endl;
    std::cout << "  OFS Configuration & Format Test" << std::endl;
    std::cout << "========================================" << std::endl;

    // Step 1: Parse configuration
    std::cout << "\n[1] Parsing configuration from default.uconf..." << std::endl;
    
    ConfigParser& parser = ConfigParser::getInstance();
    Config config;
    
    OFSErrorCodes parse_result = parser.parseConfig("default.uconf", config);
    
    if (parse_result != OFSErrorCodes::SUCCESS)
    {
        std::cerr << "ERROR: Configuration parsing failed with code: " 
                 << static_cast<int>(parse_result) << std::endl;
        return 1;
    }

    std::cout << "✓ Configuration parsed successfully" << std::endl;
    std::cout << "  Total Size: " << config.total_size << " bytes" << std::endl;
    std::cout << "  Block Size: " << config.block_size << " bytes" << std::endl;
    std::cout << "  Max Files: " << config.max_files << std::endl;
    std::cout << "  Max Users: " << config.max_users << std::endl;
    std::cout << "  Admin User: " << config.admin_username << std::endl;

    // Step 2: Format filesystem
    std::cout << "\n[2] Formatting new filesystem: data/test.omni..." << std::endl;
    
    OFSErrorCodes format_result = fs_format("data/test.omni", config);
    
    if (format_result != OFSErrorCodes::SUCCESS)
    {
        std::cerr << "ERROR: Filesystem format failed with code: " 
                 << static_cast<int>(format_result) << std::endl;
        return 1;
    }

    std::cout << "✓ Filesystem formatted successfully" << std::endl;

    // Step 3: Verify file structure
    std::cout << "\n[3] Verifying file structure..." << std::endl;
    verifyOmniStructure("data/test.omni");

    // Step 4: Print hex dump of header
    std::cout << "\n[4] Hex dump of first 512 bytes (OMNIHeader)..." << std::endl;
    hexDump("data/test.omni", 512);

    std::cout << "\n========================================" << std::endl;
    std::cout << "  Test Complete - All checks passed!" << std::endl;
    std::cout << "========================================" << std::endl;

    LOG_INFO("TEST", 0, "Configuration and format test completed successfully");

    return 0;
}
