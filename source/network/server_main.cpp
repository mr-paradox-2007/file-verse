#include "network_server.hpp"
#include "config_parser.hpp"
#include "logger.hpp"
#include "fs_init.hpp"
#include "fs_format.hpp"
#include "user_manager.hpp"
#include "ofs_types.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <fstream>

int main(int, char**) {
    ofs::ConfigParser& config_parser = ofs::ConfigParser::getInstance();
    ofs::Config config;
    
    OFSErrorCodes result = config_parser.parseConfig("default.uconf", config);
    if (result != OFSErrorCodes::SUCCESS) {
        std::cout << "Failed to parse configuration, using defaults" << std::endl;
        config = ofs::ConfigParser::getInstance().getDefaultConfig();
    }

    std::cout << "\n================================================================" << std::endl;
    std::cout << "           OFS Server v1.0                                      " << std::endl;
    std::cout << "================================================================" << std::endl;
    
    std::cout << "\nConfiguration:" << std::endl;
    std::cout << "  Port: " << config.port << std::endl;
    std::cout << "  Max Connections: " << config.max_connections << std::endl;
    std::cout << "  Filesystem: data/test.omni" << std::endl;
    
    std::ifstream test_file("data/test.omni");
    if (!test_file.good()) {
        std::cout << "\nFilesystem not found, creating new one..." << std::endl;
        result = ofs::fs_format("data/test.omni", config);
        if (result != OFSErrorCodes::SUCCESS) {
            std::cerr << "Failed to format filesystem" << std::endl;
            return 1;
        }
        std::cout << "Filesystem created successfully" << std::endl;
    }
    test_file.close();
    
    std::cout << "\nInitializing filesystem..." << std::endl;
    ofs::FileSystemManager& fs = ofs::FileSystemManager::getInstance();
    result = fs.initialize("data/test.omni", config);
    if (result != OFSErrorCodes::SUCCESS) {
        std::cerr << "Failed to initialize filesystem" << std::endl;
        return 1;
    }
    std::cout << "Filesystem initialized successfully" << std::endl;
    
    ofs::UserManager& user_mgr = ofs::UserManager::getInstance();
    if (!user_mgr.userExists(config.admin_username)) {
        std::cout << "\nCreating default admin user..." << std::endl;
        result = user_mgr.createUser(config.admin_username, config.admin_password, UserRole::ADMIN);
        if (result == OFSErrorCodes::SUCCESS) {
            std::cout << "Admin user created: " << config.admin_username << std::endl;
        }
    }

    std::cout << "\nStarting network server..." << std::endl;
    ofs::NetworkServer& server = ofs::NetworkServer::getInstance();
    result = server.initialize(config);
    if (result != OFSErrorCodes::SUCCESS) {
        std::cerr << "Failed to initialize server" << std::endl;
        return 1;
    }

    result = server.start();
    if (result != OFSErrorCodes::SUCCESS) {
        std::cerr << "Failed to start server" << std::endl;
        return 1;
    }

    std::cout << "\n================================================================" << std::endl;
    std::cout << "  Server running on localhost:" << config.port << std::endl;
    std::cout << "  Press Ctrl+C to shutdown" << std::endl;
    std::cout << "================================================================" << std::endl;
    std::cout << "\nDefault credentials:" << std::endl;
    std::cout << "  Username: " << config.admin_username << std::endl;
    std::cout << "  Password: " << config.admin_password << std::endl;
    std::cout << "\n" << std::endl;

    while (server.isRunning()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::cout << "\nShutting down..." << std::endl;
    server.stop();
    fs.shutdown();
    std::cout << "Server shutdown complete" << std::endl;
    return 0;
}