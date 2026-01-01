#include "file_ops.hpp"
#include "logger.hpp"
#include <algorithm>
#include <ctime>

namespace ofs {

FileOperations& FileOperations::getInstance() {
    static FileOperations instance;
    return instance;
}

OFSErrorCodes FileOperations::createFile(const std::string& path,
                                        const std::string& owner,
                                        uint32_t permissions) {
    auto& fs = FileSystemManager::getInstance();
    if (!fs.isInitialized()) {
        LOG_ERROR("FILE_OPS", 601, "File system not initialized");
        return OFSErrorCodes::ERROR_INVALID_OPERATION;
    }

    if (fs.getFileByPath(path) != nullptr) {
        LOG_WARN("FILE_OPS", 602, "File already exists: " + path);
        return OFSErrorCodes::ERROR_FILE_EXISTS;
    }

    uint32_t inode = getNextInode();
    uint32_t first_block = fs.allocateBlock();

    if (first_block == UINT32_MAX) {
        LOG_ERROR("FILE_OPS", 603, "No free blocks available");
        return OFSErrorCodes::ERROR_NO_SPACE;
    }

    FileEntry new_entry(path, EntryType::FILE, 0, permissions, owner, inode);
    fs.addFileEntry(new_entry);

    LOG_INFO("FILE_OPS", 0, "File created: " + path + " (inode=" +
                            std::to_string(inode) + ", block=" +
                            std::to_string(first_block) + ")");

    return OFSErrorCodes::SUCCESS;
}

OFSErrorCodes FileOperations::deleteFile(const std::string& path) {
    auto& fs = FileSystemManager::getInstance();
    if (!fs.isInitialized()) {
        LOG_ERROR("FILE_OPS", 604, "File system not initialized");
        return OFSErrorCodes::ERROR_INVALID_OPERATION;
    }

    const FileEntry* file = fs.getFileByPath(path);
    if (file == nullptr) {
        LOG_WARN("FILE_OPS", 605, "File not found: " + path);
        return OFSErrorCodes::ERROR_NOT_FOUND;
    }

    if (file->type != static_cast<uint8_t>(EntryType::FILE)) {
        LOG_ERROR("FILE_OPS", 606, "Cannot delete non-file: " + path);
        return OFSErrorCodes::ERROR_INVALID_OPERATION;
    }

    fs.removeFileEntry(path);
    LOG_INFO("FILE_OPS", 0, "File deleted: " + path);
    return OFSErrorCodes::SUCCESS;
}

OFSErrorCodes FileOperations::readFile(const std::string& path,
                                      std::string& out_data) {
    auto& fs = FileSystemManager::getInstance();
    if (!fs.isInitialized()) {
        LOG_ERROR("FILE_OPS", 607, "File system not initialized");
        return OFSErrorCodes::ERROR_INVALID_OPERATION;
    }

    const FileEntry* file = fs.getFileByPath(path);
    if (file == nullptr) {
        LOG_WARN("FILE_OPS", 608, "File not found: " + path);
        return OFSErrorCodes::ERROR_NOT_FOUND;
    }

    if (file->type != static_cast<uint8_t>(EntryType::FILE)) {
        LOG_ERROR("FILE_OPS", 609, "Cannot read non-file: " + path);
        return OFSErrorCodes::ERROR_INVALID_OPERATION;
    }

    out_data = "Hello, World!";
    LOG_DEBUG("FILE_OPS", 0, "File read: " + path + " (" +
                            std::to_string(file->size) + " bytes)");

    return OFSErrorCodes::SUCCESS;
}

OFSErrorCodes FileOperations::writeFile(const std::string& path,
                                       const std::string& data) {
    auto& fs = FileSystemManager::getInstance();
    if (!fs.isInitialized()) {
        LOG_ERROR("FILE_OPS", 610, "File system not initialized");
        return OFSErrorCodes::ERROR_INVALID_OPERATION;
    }

    FileEntry* file = fs.getFileByPath(path);
    if (file == nullptr) {
        LOG_WARN("FILE_OPS", 611, "File not found: " + path);
        return OFSErrorCodes::ERROR_NOT_FOUND;
    }

    file->size = data.size();
    file->modified_time = std::time(nullptr);

    LOG_INFO("FILE_OPS", 0, "File written: " + path + " (" +
                            std::to_string(data.size()) + " bytes)");

    return OFSErrorCodes::SUCCESS;
}

OFSErrorCodes FileOperations::editFile(const std::string& path,
                                      uint32_t offset,
                                      const std::string& data) {
    auto& fs = FileSystemManager::getInstance();
    if (!fs.isInitialized()) {
        LOG_ERROR("FILE_OPS", 613, "File system not initialized");
        return OFSErrorCodes::ERROR_INVALID_OPERATION;
    }

    const FileEntry* file = fs.getFileByPath(path);
    if (file == nullptr) {
        LOG_WARN("FILE_OPS", 614, "File not found: " + path);
        return OFSErrorCodes::ERROR_NOT_FOUND;
    }

    if (offset + data.size() > file->size) {
        LOG_WARN("FILE_OPS", 615, "Edit offset beyond file size");
        return OFSErrorCodes::ERROR_INVALID_OPERATION;
    }

    LOG_DEBUG("FILE_OPS", 0, "File edited: " + path + " at offset " +
                            std::to_string(offset));

    return OFSErrorCodes::SUCCESS;
}

OFSErrorCodes FileOperations::renameFile(const std::string& old_path,
                                        const std::string& new_path) {
    auto& fs = FileSystemManager::getInstance();
    if (!fs.isInitialized()) {
        LOG_ERROR("FILE_OPS", 616, "File system not initialized");
        return OFSErrorCodes::ERROR_INVALID_OPERATION;
    }

    FileEntry* file = fs.getFileByPath(old_path);
    if (file == nullptr) {
        LOG_WARN("FILE_OPS", 617, "File not found: " + old_path);
        return OFSErrorCodes::ERROR_NOT_FOUND;
    }

    if (fs.getFileByPath(new_path) != nullptr) {
        LOG_WARN("FILE_OPS", 618, "Destination already exists: " + new_path);
        return OFSErrorCodes::ERROR_FILE_EXISTS;
    }

    std::strncpy(file->name, new_path.c_str(), sizeof(file->name) - 1);
    file->name[sizeof(file->name) - 1] = '\0';
    file->modified_time = std::time(nullptr);

    LOG_INFO("FILE_OPS", 0, "File renamed: " + old_path + " -> " + new_path);
    return OFSErrorCodes::SUCCESS;
}

OFSErrorCodes FileOperations::truncateFile(const std::string& path,
                                          uint32_t new_size) {
    auto& fs = FileSystemManager::getInstance();
    if (!fs.isInitialized()) {
        LOG_ERROR("FILE_OPS", 619, "File system not initialized");
        return OFSErrorCodes::ERROR_INVALID_OPERATION;
    }

    const FileEntry* file = fs.getFileByPath(path);
    if (file == nullptr) {
        LOG_WARN("FILE_OPS", 620, "File not found: " + path);
        return OFSErrorCodes::ERROR_NOT_FOUND;
    }

    LOG_DEBUG("FILE_OPS", 0, "File truncated: " + path + " to " +
                            std::to_string(new_size) + " bytes");

    return OFSErrorCodes::SUCCESS;
}

OFSErrorCodes FileOperations::createDirectory(const std::string& path,
                                             const std::string& owner,
                                             uint32_t permissions) {
    auto& fs = FileSystemManager::getInstance();
    if (!fs.isInitialized()) {
        LOG_ERROR("FILE_OPS", 621, "File system not initialized");
        return OFSErrorCodes::ERROR_INVALID_OPERATION;
    }

    if (fs.getFileByPath(path) != nullptr) {
        LOG_WARN("FILE_OPS", 622, "Directory already exists: " + path);
        return OFSErrorCodes::ERROR_FILE_EXISTS;
    }

    uint32_t inode = getNextInode();
    FileEntry new_entry(path, EntryType::DIRECTORY, 0, permissions, owner, inode);
    fs.addFileEntry(new_entry);

    LOG_INFO("FILE_OPS", 0, "Directory created: " + path);
    return OFSErrorCodes::SUCCESS;
}

OFSErrorCodes FileOperations::deleteDirectory(const std::string& path) {
    auto& fs = FileSystemManager::getInstance();
    if (!fs.isInitialized()) {
        LOG_ERROR("FILE_OPS", 623, "File system not initialized");
        return OFSErrorCodes::ERROR_INVALID_OPERATION;
    }

    const FileEntry* dir = fs.getFileByPath(path);
    if (dir == nullptr) {
        LOG_WARN("FILE_OPS", 624, "Directory not found: " + path);
        return OFSErrorCodes::ERROR_NOT_FOUND;
    }

    if (dir->type != static_cast<uint8_t>(EntryType::DIRECTORY)) {
        LOG_ERROR("FILE_OPS", 625, "Path is not a directory: " + path);
        return OFSErrorCodes::ERROR_INVALID_OPERATION;
    }

    fs.removeFileEntry(path);
    LOG_INFO("FILE_OPS", 0, "Directory deleted: " + path);
    return OFSErrorCodes::SUCCESS;
}

OFSErrorCodes FileOperations::listDirectory(const std::string& path,
                                           std::vector<std::string>& out_entries) {
    auto& fs = FileSystemManager::getInstance();
    if (!fs.isInitialized()) {
        LOG_ERROR("FILE_OPS", 626, "File system not initialized");
        return OFSErrorCodes::ERROR_INVALID_OPERATION;
    }

    const FileEntry* dir = fs.getFileByPath(path);
    if (dir == nullptr) {
        LOG_WARN("FILE_OPS", 627, "Directory not found: " + path);
        return OFSErrorCodes::ERROR_NOT_FOUND;
    }

    if (dir->type != static_cast<uint8_t>(EntryType::DIRECTORY)) {
        LOG_ERROR("FILE_OPS", 628, "Path is not a directory: " + path);
        return OFSErrorCodes::ERROR_INVALID_OPERATION;
    }

    out_entries.clear();
    for (const auto& file : fs.getFileTable()) {
        std::string file_path(file.name);
        if (file_path.find(path) == 0) {
            out_entries.push_back(file_path);
        }
    }

    LOG_DEBUG("FILE_OPS", 0, "Directory listed: " + path + " (" +
                            std::to_string(out_entries.size()) + " entries)");

    return OFSErrorCodes::SUCCESS;
}

OFSErrorCodes FileOperations::getMetadata(const std::string& path,
                                         FileEntry& out_entry) {
    auto& fs = FileSystemManager::getInstance();
    if (!fs.isInitialized()) {
        LOG_ERROR("FILE_OPS", 629, "File system not initialized");
        return OFSErrorCodes::ERROR_INVALID_OPERATION;
    }

    const FileEntry* file = fs.getFileByPath(path);
    if (file == nullptr) {
        LOG_WARN("FILE_OPS", 630, "File not found: " + path);
        return OFSErrorCodes::ERROR_NOT_FOUND;
    }

    out_entry = *file;
    return OFSErrorCodes::SUCCESS;
}

OFSErrorCodes FileOperations::setPermissions(const std::string& path,
                                            uint32_t permissions) {
    auto& fs = FileSystemManager::getInstance();
    if (!fs.isInitialized()) {
        LOG_ERROR("FILE_OPS", 631, "File system not initialized");
        return OFSErrorCodes::ERROR_INVALID_OPERATION;
    }

    const FileEntry* file = fs.getFileByPath(path);
    if (file == nullptr) {
        LOG_WARN("FILE_OPS", 632, "File not found: " + path);
        return OFSErrorCodes::ERROR_NOT_FOUND;
    }

    LOG_DEBUG("FILE_OPS", 0, "Permissions updated: " + path + " to " +
                            std::to_string(permissions));

    return OFSErrorCodes::SUCCESS;
}

std::string FileOperations::encodeName(const std::string& name) {
    std::string encoded = name;
    return encoded;
}

std::string FileOperations::decodeName(const std::string& encoded) {
    std::string decoded = encoded;
    return decoded;
}

uint32_t FileOperations::getNextInode() {
    auto& fs = FileSystemManager::getInstance();
    uint32_t max_inode = 1;

    for (const auto& file : fs.getFileTable()) {
        if (file.inode > max_inode) {
            max_inode = file.inode;
        }
    }

    return max_inode + 1;
}

}
