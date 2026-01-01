#pragma once

#include "ofs_types.hpp"
#include "config_parser.hpp"
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <mutex>

namespace ofs {

class FileSystemManager {
public:
    static FileSystemManager& getInstance();

    OFSErrorCodes initialize(const std::string& omni_path, const Config& config);
    OFSErrorCodes shutdown();

    bool isInitialized() const { return is_initialized_; }

    const std::unordered_map<std::string, UserInfo>& getUserTable() const {
        return users_;
    }

    const std::vector<FileEntry>& getFileTable() const {
        return files_;
    }

    uint32_t getFreeBlockCount() const;
    bool isBlockFree(uint32_t block_num) const;
    void markBlockUsed(uint32_t block_num);
    void markBlockFree(uint32_t block_num);

    const FileEntry* getFileByPath(const std::string& path) const;
    FileEntry* getFileByPath(const std::string& path);
    const FileEntry* getFileByInode(uint32_t inode) const;

    void addFileEntry(const FileEntry& entry);
    void removeFileEntry(const std::string& path);

    uint32_t allocateBlock();
    void releaseBlock(uint32_t block_num);
    
    void saveFileTable();
    void saveUserTable();
    
    std::string readFileData(const std::string& path);
    void writeFileData(const std::string& path, const std::string& data);

private:
    FileSystemManager() = default;
    ~FileSystemManager() = default;

    OFSErrorCodes loadHeader();
    OFSErrorCodes loadUserTable();
    OFSErrorCodes loadFileTable();
    OFSErrorCodes initializeFreeSpaceBitmap();

    std::string omni_path_;
    Config config_;

    OMNIHeader header_;
    std::unordered_map<std::string, UserInfo> users_;
    std::vector<FileEntry> files_;
    std::unordered_map<std::string, std::string> file_data_;
    std::vector<bool> free_blocks_;

    bool is_initialized_ = false;
    mutable std::mutex fs_mutex_;
};

}