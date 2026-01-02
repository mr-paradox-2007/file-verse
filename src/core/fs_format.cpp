#include "fs_format.hpp"
#include "config_parser.hpp"
#include "logger.hpp"
#include <fstream>
#include <cstring>
#include <iostream>

int fs_format(const std::string& omni_path, const std::string& config_path) {
    Logger::info("Formatting OFS file: " + omni_path);
    
    if (!ConfigParser::load(config_path)) {
        Logger::error("Failed to load configuration from: " + config_path);
        return static_cast<int>(OFSErrorCodes::ERROR_INVALID_CONFIG);
    }
    
    if (!ConfigParser::validate()) {
        Logger::error("Configuration validation failed");
        return static_cast<int>(OFSErrorCodes::ERROR_INVALID_CONFIG);
    }
    
    uint64_t total_size = ConfigParser::get_uint("filesystem", "total_size", 104857600);
    uint64_t block_size = ConfigParser::get_uint("filesystem", "block_size", 4096);
    uint32_t max_users = ConfigParser::get_uint("filesystem", "max_users", 50);
    
    std::ofstream file(omni_path, std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
        Logger::error("Failed to create omni file: " + omni_path);
        return static_cast<int>(OFSErrorCodes::ERROR_IO_ERROR);
    }
    
    OMNIHeader header;
    std::memcpy(header.magic, "OMNIFS01", 8);
    header.format_version = 0x00010000;
    header.total_size = total_size;
    header.header_size = 512;
    header.block_size = block_size;
    header.max_users = max_users;
    header.user_table_offset = 512;
    
    file.write(reinterpret_cast<const char*>(&header), sizeof(header));
    
    if (file.fail()) {
        Logger::error("Failed to write header to omni file");
        file.close();
        return static_cast<int>(OFSErrorCodes::ERROR_IO_ERROR);
    }
    
    uint64_t user_table_size = max_users * sizeof(UserInfo);
    char* user_table = new char[user_table_size];
    std::memset(user_table, 0, user_table_size);
    
    file.write(user_table, user_table_size);
    delete[] user_table;
    
    if (file.fail()) {
        Logger::error("Failed to write user table");
        file.close();
        return static_cast<int>(OFSErrorCodes::ERROR_IO_ERROR);
    }
    
    uint64_t metadata_area_size = 1024 * 1024;
    uint64_t num_blocks = (total_size - header.header_size - user_table_size - metadata_area_size) / block_size;
    
    char* bitmap = new char[num_blocks];
    std::memset(bitmap, 0, num_blocks);
    
    file.write(bitmap, num_blocks);
    delete[] bitmap;
    
    if (file.fail()) {
        Logger::error("Failed to write bitmap");
        file.close();
        return static_cast<int>(OFSErrorCodes::ERROR_IO_ERROR);
    }
    
    char* metadata = new char[metadata_area_size];
    std::memset(metadata, 0, metadata_area_size);
    
    file.write(metadata, metadata_area_size);
    delete[] metadata;
    
    if (file.fail()) {
        Logger::error("Failed to write metadata area");
        file.close();
        return static_cast<int>(OFSErrorCodes::ERROR_IO_ERROR);
    }
    
    char* padding = new char[65536];
    std::memset(padding, 0, 65536);
    
    uint64_t written = header.header_size + user_table_size + num_blocks + metadata_area_size;
    while (written < total_size && file.good()) {
        uint64_t to_write = std::min(static_cast<uint64_t>(65536), total_size - written);
        file.write(padding, to_write);
        written += to_write;
    }
    
    delete[] padding;
    file.close();
    
    Logger::info("OFS file formatted successfully");
    return static_cast<int>(OFSErrorCodes::SUCCESS);
}

int fs_validate(const std::string& omni_path) {
    std::ifstream file(omni_path, std::ios::binary);
    if (!file.is_open()) {
        return static_cast<int>(OFSErrorCodes::ERROR_IO_ERROR);
    }
    
    OMNIHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(header));
    
    if (file.fail()) {
        return static_cast<int>(OFSErrorCodes::ERROR_IO_ERROR);
    }
    
    if (std::memcmp(header.magic, "OMNIFS01", 8) != 0) {
        return static_cast<int>(OFSErrorCodes::ERROR_INVALID_CONFIG);
    }
    
    if (header.format_version != 0x00010000) {
        return static_cast<int>(OFSErrorCodes::ERROR_INVALID_CONFIG);
    }
    
    file.close();
    return static_cast<int>(OFSErrorCodes::SUCCESS);
}

uint64_t fs_calculate_size(const std::string& config_path) {
    ConfigParser::load(config_path);
    return ConfigParser::get_uint("filesystem", "total_size", 104857600);
}
