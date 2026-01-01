#pragma once

#include <string>
#include <vector>
#include <memory>
#include <cstdint>

namespace ofs {

class CliClient {
public:
    CliClient(const std::string& host = "localhost", uint16_t port = 8080);
    ~CliClient();

    bool connect();
    void disconnect();
    bool isConnected() const;

    bool login(const std::string& username, const std::string& password);
    bool logout();
    bool createFile(const std::string& path, uint32_t permissions = 0644);
    bool deleteFile(const std::string& path);
    bool readFile(const std::string& path, std::string& out_data);
    bool writeFile(const std::string& path, const std::string& data);
    bool listDirectory(const std::string& path);
    bool createDirectory(const std::string& path, uint32_t permissions = 0755);
    bool deleteDirectory(const std::string& path);
    bool getMetadata(const std::string& path);

    void printHelp() const;
    void run();

private:
    std::string host_;
    uint16_t port_;
    int socket_;
    std::string session_id_;
    bool connected_;
    std::string current_user_;

    bool sendRequest(const std::string& operation, const std::string& data);
    bool receiveResponse(std::string& out_response);
    void processCommand(const std::string& cmd);
};

}
