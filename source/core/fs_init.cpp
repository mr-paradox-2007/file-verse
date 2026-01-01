#include "fs_init.hpp"
#include "logger.hpp"
#include <fstream>
#include <cstring>
#include <algorithm>

namespace ofs {

FileSystemManager& FileSystemManager::getInstance() {
    static FileSystemManager instance;
    return instance;
}

OFSErrorCodes FileSystemManager::initialize(const std::string& omni_path,
                                            const Config& config) {
    if (is_initialized_) {
        LOG_WARN("FS_INIT", 501, "File system already initialized");
        return OFSErrorCodes::ERROR_INVALID_OPERATION;
    }

    omni_path_ = omni_path;
    config_ = config;

    LOG_INFO("FS_INIT", 0, "Initializing file system from: " + omni_path);

    if (loadHeader() != OFSErrorCodes::SUCCESS) {
        return OFSErrorCodes::ERROR_NOT_FOUND;
    }

    if (loadUserTable() != OFSErrorCodes::SUCCESS) {
        return OFSErrorCodes::ERROR_INVALID_OPERATION;
    }

    if (loadFileTable() != OFSErrorCodes::SUCCESS) {
        return OFSErrorCodes::ERROR_INVALID_OPERATION;
    }

    if (initializeFreeSpaceBitmap() != OFSErrorCodes::SUCCESS) {
        return OFSErrorCodes::ERROR_INVALID_OPERATION;
    }

    is_initialized_ = true;
    LOG_INFO("FS_INIT", 0, "File system initialized successfully");
    LOG_DEBUG("FS_INIT", 0, "Users: " + std::to_string(users_.size()) +
                            ", Files: " + std::to_string(files_.size()) +
                            ", Free blocks: " + std::to_string(getFreeBlockCount()));

    return OFSErrorCodes::SUCCESS;
}

OFSErrorCodes FileSystemManager::shutdown() {
    if (!is_initialized_) {
        LOG_WARN("FS_INIT", 502, "File system not initialized");
        return OFSErrorCodes::ERROR_INVALID_OPERATION;
    }

    LOG_INFO("FS_INIT", 0, "Shutting down file system");

    users_.clear();
    files_.clear();
    free_blocks_.clear();

    is_initialized_ = false;
    return OFSErrorCodes::SUCCESS;
}

OFSErrorCodes FileSystemManager::loadHeader() {
    std::ifstream file(omni_path_, std::ios::binary);
    if (!file.is_open()) {
        LOG_ERROR("FS_INIT", 503, "Cannot open .omni file: " + omni_path_);
        return OFSErrorCodes::ERROR_NOT_FOUND;
    }

    file.read(reinterpret_cast<char*>(&header_), sizeof(OMNIHeader));
    file.close();

    if (std::string(header_.magic) != "OMNIFS0") {
        LOG_ERROR("FS_INIT", 504, "Invalid magic number in header");
        return OFSErrorCodes::ERROR_INVALID_OPERATION;
    }

    LOG_DEBUG("FS_INIT", 0, "Header loaded: version=" + 
                            std::to_string(header_.format_version) +
                            ", total_size=" + std::to_string(header_.total_size) +
                            ", block_size=" + std::to_string(header_.block_size));

    return OFSErrorCodes::SUCCESS;
}

OFSErrorCodes FileSystemManager::loadUserTable() {
    std::ifstream file(omni_path_, std::ios::binary);
    if (!file.is_open()) {
        LOG_ERROR("FS_INIT", 505, "Cannot open .omni file for user table load");
        return OFSErrorCodes::ERROR_NOT_FOUND;
    }

    file.seekg(header_.user_table_offset);

    users_.clear();
    for (uint32_t i = 0; i < header_.max_users; i++) {
        UserInfo user;
        file.read(reinterpret_cast<char*>(&user), sizeof(UserInfo));

        if (user.username[0] != '\0' && user.is_active) {
            std::string username(user.username);
            users_[username] = user;
            LOG_DEBUG("FS_INIT", 0, "Loaded user: " + username);
        }
    }

    file.close();
    LOG_INFO("FS_INIT", 0, "User table loaded: " + std::to_string(users_.size()) + 
                          " active users");

    return OFSErrorCodes::SUCCESS;
}

OFSErrorCodes FileSystemManager::loadFileTable() {
    std::ifstream file(omni_path_, std::ios::binary);
    if (!file.is_open()) {
        LOG_ERROR("FS_INIT", 506, "Cannot open .omni file for file table load");
        return OFSErrorCodes::ERROR_NOT_FOUND;
    }

    uint32_t metadata_offset = header_.user_table_offset + 
                               (header_.max_users * sizeof(UserInfo));
    file.seekg(metadata_offset);

    files_.clear();
    uint32_t file_count = 0;
    char buffer[sizeof(FileEntry)];

    while (file.read(buffer, sizeof(FileEntry)) && file_count < 10000) {
        FileEntry entry;
        std::memcpy(&entry, buffer, sizeof(FileEntry));

        if (entry.name[0] != '\0') {
            files_.push_back(entry);
            file_count++;
        }
    }

    file.close();
    LOG_INFO("FS_INIT", 0, "File table loaded: " + std::to_string(files_.size()) + 
                          " entries");

    return OFSErrorCodes::SUCCESS;
}

OFSErrorCodes FileSystemManager::initializeFreeSpaceBitmap() {
    uint32_t total_blocks = header_.total_size / header_.block_size;
    free_blocks_.resize(total_blocks, true);

    uint32_t metadata_blocks = (header_.total_size / 10) / header_.block_size;
    for (uint32_t i = 0; i < metadata_blocks; i++) {
        free_blocks_[i] = false;
    }

    for (const auto& file : files_) {
        uint32_t file_blocks = (file.size + header_.block_size - 1) / header_.block_size;
        uint32_t start_block = file.inode % (header_.total_size / header_.block_size);

        for (uint32_t i = 0; i < file_blocks && start_block < total_blocks; i++) {
            if (start_block < free_blocks_.size()) {
                free_blocks_[start_block] = false;
            }
            start_block++;
        }
    }

    LOG_DEBUG("FS_INIT", 0, "Free space bitmap initialized: " +
                            std::to_string(getFreeBlockCount()) + " free blocks");

    return OFSErrorCodes::SUCCESS;
}

uint32_t FileSystemManager::getFreeBlockCount() const {
    return std::count(free_blocks_.begin(), free_blocks_.end(), true);
}

bool FileSystemManager::isBlockFree(uint32_t block_num) const {
    if (block_num >= free_blocks_.size()) {
        return false;
    }
    return free_blocks_[block_num];
}

void FileSystemManager::markBlockUsed(uint32_t block_num) {
    if (block_num < free_blocks_.size()) {
        free_blocks_[block_num] = false;
    }
}

void FileSystemManager::markBlockFree(uint32_t block_num) {
    if (block_num < free_blocks_.size()) {
        free_blocks_[block_num] = true;
    }
}

const FileEntry* FileSystemManager::getFileByPath(const std::string& path) const {
    for (const auto& file : files_) {
        if (std::string(file.name) == path) {
            return &file;
        }
    }
    return nullptr;
}

FileEntry* FileSystemManager::getFileByPath(const std::string& path) {
    for (auto& file : files_) {
        if (std::string(file.name) == path) {
            return &file;
        }
    }
    return nullptr;
}

const FileEntry* FileSystemManager::getFileByInode(uint32_t inode) const {
    for (const auto& file : files_) {
        if (file.inode == inode) {
            return &file;
        }
    }
    return nullptr;
}

uint32_t FileSystemManager::allocateBlock() {
    for (uint32_t i = 0; i < free_blocks_.size(); i++) {
        if (free_blocks_[i]) {
            free_blocks_[i] = false;
            return i;
        }
    }
    return UINT32_MAX;
}

void FileSystemManager::releaseBlock(uint32_t block_num) {
    markBlockFree(block_num);
}

void FileSystemManager::addFileEntry(const FileEntry& entry) {
    files_.push_back(entry);
}

void FileSystemManager::removeFileEntry(const std::string& path) {
    files_.erase(std::remove_if(files_.begin(), files_.end(),
                                [&](const FileEntry& entry) {
                                    return std::string(entry.name) == path;
                                }),
                 files_.end());
}

}
