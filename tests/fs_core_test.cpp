#include <iostream>
#include <iomanip>
#include "../source/include/fs_init.hpp"
#include "../source/include/config_parser.hpp"
#include "../source/include/file_ops.hpp"
#include "../source/include/logger.hpp"

using namespace ofs;

void printTestHeader(const std::string& test_name) {
    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << "  " << test_name << std::endl;
    std::cout << std::string(70, '=') << std::endl;
}

void printResult(bool passed, const std::string& message) {
    std::cout << (passed ? "  ✓ " : "  ✗ ") << message << std::endl;
}

bool testFsInitialization() {
    printTestHeader("Test 1: File System Initialization");

    ConfigParser& config_mgr = ConfigParser::getInstance();
    
    Config config;
    OFSErrorCodes result = config_mgr.parseConfig("default.uconf", config);
    printResult(result == OFSErrorCodes::SUCCESS,
               "Parse configuration file");

    FileSystemManager& fs = FileSystemManager::getInstance();
    result = fs.initialize("data/test.omni", config);
    printResult(result == OFSErrorCodes::SUCCESS,
               "Initialize file system from .omni");

    if (result == OFSErrorCodes::SUCCESS) {
        std::cout << "    File system loaded successfully" << std::endl;
        std::cout << "    Users: " << fs.getUserTable().size() << std::endl;
        std::cout << "    Files: " << fs.getFileTable().size() << std::endl;
        std::cout << "    Free blocks: " << fs.getFreeBlockCount() << std::endl;
    }

    return result == OFSErrorCodes::SUCCESS;
}

bool testBlockAllocation() {
    printTestHeader("Test 2: Block Allocation");

    FileSystemManager& fs = FileSystemManager::getInstance();

    uint32_t initial_free = fs.getFreeBlockCount();
    printResult(initial_free > 0, "Initial free blocks available");

    uint32_t block = fs.allocateBlock();
    printResult(block != UINT32_MAX, "Allocate single block");

    uint32_t after_alloc = fs.getFreeBlockCount();
    printResult(after_alloc < initial_free,
               "Free block count decreased after allocation");

    fs.markBlockFree(block);
    uint32_t after_free = fs.getFreeBlockCount();
    printResult(after_free == initial_free,
               "Free block count restored after deallocation");

    return true;
}

bool testFileCreation() {
    printTestHeader("Test 3: File Operations");

    FileOperations& fops = FileOperations::getInstance();

    OFSErrorCodes result = fops.createFile("/test_file.txt", "admin", 0644);
    printResult(result == OFSErrorCodes::SUCCESS, "Create new file");

    result = fops.createFile("/test_file.txt", "admin", 0644);
    printResult(result == OFSErrorCodes::ERROR_FILE_EXISTS,
               "Reject duplicate file creation");

    result = fops.writeFile("/test_file.txt", "Hello, World!");
    printResult(result == OFSErrorCodes::SUCCESS, "Write to file");

    std::string data;
    result = fops.readFile("/test_file.txt", data);
    printResult(result == OFSErrorCodes::SUCCESS, "Read from file");

    result = fops.editFile("/test_file.txt", 0, "Hi");
    printResult(result == OFSErrorCodes::SUCCESS, "Edit file at offset");

    result = fops.renameFile("/test_file.txt", "/renamed_file.txt");
    printResult(result == OFSErrorCodes::SUCCESS, "Rename file");

    result = fops.deleteFile("/renamed_file.txt");
    printResult(result == OFSErrorCodes::SUCCESS, "Delete file");

    return true;
}

bool testDirectoryOperations() {
    printTestHeader("Test 4: Directory Operations");

    FileOperations& fops = FileOperations::getInstance();

    OFSErrorCodes result = fops.createDirectory("/mydir", "admin", 0755);
    printResult(result == OFSErrorCodes::SUCCESS, "Create directory");

    result = fops.createDirectory("/mydir", "admin", 0755);
    printResult(result == OFSErrorCodes::ERROR_FILE_EXISTS,
               "Reject duplicate directory");

    std::vector<std::string> entries;
    result = fops.listDirectory("/", entries);
    printResult(result == OFSErrorCodes::SUCCESS,
               "List directory contents");

    if (result == OFSErrorCodes::SUCCESS) {
        std::cout << "    Found " << entries.size() << " entries" << std::endl;
    }

    result = fops.deleteDirectory("/mydir");
    printResult(result == OFSErrorCodes::SUCCESS, "Delete directory");

    return true;
}

bool testMetadata() {
    printTestHeader("Test 5: Metadata Operations");

    FileOperations& fops = FileOperations::getInstance();

    fops.createFile("/metadata_test.txt", "admin", 0644);

    FileEntry entry;
    OFSErrorCodes result = fops.getMetadata("/metadata_test.txt", entry);
    printResult(result == OFSErrorCodes::SUCCESS, "Get file metadata");

    result = fops.setPermissions("/metadata_test.txt", 0600);
    printResult(result == OFSErrorCodes::SUCCESS, "Set file permissions");

    result = fops.truncateFile("/metadata_test.txt", 1024);
    printResult(result == OFSErrorCodes::SUCCESS, "Truncate file");

    fops.deleteFile("/metadata_test.txt");
    return true;
}

bool testErrorHandling() {
    printTestHeader("Test 6: Error Handling");

    FileOperations& fops = FileOperations::getInstance();

    FileEntry entry;
    OFSErrorCodes result = fops.getMetadata("/nonexistent.txt", entry);
    printResult(result == OFSErrorCodes::ERROR_NOT_FOUND,
               "Return NOT_FOUND for nonexistent file");

    result = fops.readFile("/nonexistent.txt", (std::string&)entry);
    printResult(result == OFSErrorCodes::ERROR_NOT_FOUND,
               "Handle read from nonexistent file");

    result = fops.deleteFile("/nonexistent.txt");
    printResult(result == OFSErrorCodes::ERROR_NOT_FOUND,
               "Handle delete of nonexistent file");

    return true;
}

int main() {
    std::cout << "\n╔════════════════════════════════════════════════════════════════╗"
              << std::endl;
    std::cout << "║         File System Core - Test Suite                           ║"
              << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════════════╝"
              << std::endl;

    if (!testFsInitialization() ||
        !testBlockAllocation() ||
        !testFileCreation() ||
        !testDirectoryOperations() ||
        !testMetadata() ||
        !testErrorHandling()) {
        
        std::cout << "\n" << std::string(70, '=') << std::endl;
        std::cout << "  Some Tests Failed" << std::endl;
        std::cout << std::string(70, '=') << std::endl << std::endl;
        return 1;
    }

    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << "  All Tests Completed" << std::endl;
    std::cout << std::string(70, '=') << std::endl << std::endl;

    LOG_INFO("TEST", 0, "File system core test suite completed");

    FileSystemManager& fs = FileSystemManager::getInstance();
    fs.shutdown();

    return 0;
}
