#include "fs_init.hpp"
#include "logger.hpp"
#include "crypto.hpp"
#include <iostream>

static OFS_Instance global_instance = nullptr;

int fs_init(OFS_Instance* instance, const std::string& omni_path,
            const std::string& config_path, const std::string& private_key) {
    
    std::cout << "  [*] Logger init..." << std::endl;
    Logger::init();
    Logger::info("Initializing OFS system");
    
    std::cout << "  [*] Crypto init..." << std::endl;
    try {
        Crypto::init();
        Logger::info("Crypto initialized");
    } catch (const std::exception& e) {
        Logger::error("Failed to initialize crypto: " + std::string(e.what()));
        return static_cast<int>(OFSErrorCodes::ERROR_IO_ERROR);
    }
    
    std::cout << "  [*] Creating instance..." << std::endl;
    *instance = new void*;
    global_instance = *instance;
    
    Logger::info("OFS system initialized successfully");
    return static_cast<int>(OFSErrorCodes::SUCCESS);
}

int fs_shutdown(OFS_Instance instance) {
    Logger::info("Shutting down OFS system");
    
    if (instance) {
        delete reinterpret_cast<void**>(instance);
    }
    
    Logger::shutdown();
    return static_cast<int>(OFSErrorCodes::SUCCESS);
}

OFS_Instance fs_get_current_instance() {
    return global_instance;
}
