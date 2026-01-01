#include "network_server.hpp"
#include "config_parser.hpp"
#include "logger.hpp"
#include "ofs_types.hpp"
#include <iostream>
#include <thread>
#include <chrono>

int main(int, char**) {
    ofs::ConfigParser& config_parser = ofs::ConfigParser::getInstance();
    ofs::Config config;
    
    OFSErrorCodes result = config_parser.parseConfig("default.uconf", config);
    if (result != OFSErrorCodes::SUCCESS) {
        std::cout << "Failed to parse configuration, using defaults" << std::endl;
        config = ofs::ConfigParser::getInstance().getDefaultConfig();
    }

    std::cout << "\n╔════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║         OFS Server v1.0                                  ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════╝" << std::endl;
    std::cout << "\nConfiguration loaded from: default.uconf" << std::endl;
    std::cout << "  Port: " << config.port << std::endl;
    std::cout << "  Max Connections: " << config.max_connections << std::endl;
    std::cout << "\nStarting server..." << std::endl;

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

    std::cout << "✓ Server running on localhost:" << config.port << std::endl;
    std::cout << "  Press Ctrl+C to shutdown" << std::endl << std::endl;

    while (server.isRunning()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    server.stop();
    std::cout << "\nServer shutdown complete" << std::endl;
    return 0;
}
