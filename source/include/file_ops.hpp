#pragma once

#include "ofs_types.hpp"
#include "fs_init.hpp"
#include <string>

namespace ofs {

class FileOperations {
public:
    static FileOperations& getInstance();

    OFSErrorCodes createFile(const std::string& path,
                             const std::string& owner,
                             uint32_t permissions);

    OFSErrorCodes deleteFile(const std::string& path);

    OFSErrorCodes readFile(const std::string& path,
                          std::string& out_data);

    OFSErrorCodes writeFile(const std::string& path,
                           const std::string& data);

    OFSErrorCodes editFile(const std::string& path,
                          uint32_t offset,
                          const std::string& data);

    OFSErrorCodes renameFile(const std::string& old_path,
                            const std::string& new_path);

    OFSErrorCodes truncateFile(const std::string& path,
                              uint32_t new_size);

    OFSErrorCodes createDirectory(const std::string& path,
                                 const std::string& owner,
                                 uint32_t permissions);

    OFSErrorCodes deleteDirectory(const std::string& path);

    OFSErrorCodes listDirectory(const std::string& path,
                               std::vector<std::string>& out_entries);

    OFSErrorCodes getMetadata(const std::string& path,
                             FileEntry& out_entry);

    OFSErrorCodes setPermissions(const std::string& path,
                                uint32_t permissions);

private:
    FileOperations() = default;
    ~FileOperations() = default;

    std::string encodeName(const std::string& name);
    std::string decodeName(const std::string& encoded);

    uint32_t getNextInode();
};

}
