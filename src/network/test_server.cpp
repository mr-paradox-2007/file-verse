#include <iostream>
#include <string>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

const int PORT = 9000;

void* handle_client(void* arg) {
    int client_socket = *((int*)arg);
    delete (int*)arg;
    
    char buffer[4096];
    int bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        std::cout << "[*] Received request from client" << std::endl;
        
        std::string response = 
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: 41\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "\r\n"
            "{\"status\":\"success\",\"message\":\"OK\"}";
        
        send(client_socket, response.c_str(), response.length(), 0);
    }
    
    close(client_socket);
    return nullptr;
}

int main() {
    std::cout << "╔═════════════════════════════════╗" << std::endl;
    std::cout << "║  OFS HTTP Server (Test Mode)    ║" << std::endl;
    std::cout << "╚═════════════════════════════════╝" << std::endl << std::endl;
    
    // Create socket
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        std::cerr << "[!] Failed to create socket" << std::endl;
        return 1;
    }
    
    // Set socket options
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "[!] Failed to set socket options" << std::endl;
        return 1;
    }
    
    // Bind socket
    struct sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    
    std::cout << "[*] Binding to port " << PORT << "..." << std::endl;
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "[!] Failed to bind socket: " << strerror(errno) << std::endl;
        return 1;
    }
    std::cout << "[✓] Bound successfully" << std::endl;
    
    // Listen for connections
    if (listen(server_socket, 20) < 0) {
        std::cerr << "[!] Listen failed" << std::endl;
        return 1;
    }
    std::cout << "[✓] Listening for connections..." << std::endl << std::endl;
    
    // Accept connections
    while (true) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        
        int* client_socket = new int;
        *client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_len);
        
        if (*client_socket < 0) {
            std::cerr << "[!] Accept failed" << std::endl;
            delete client_socket;
            continue;
        }
        
        std::cout << "[+] Client connected" << std::endl;
        
        pthread_t thread_id;
        pthread_create(&thread_id, nullptr, handle_client, (void*)client_socket);
        pthread_detach(thread_id);
    }
    
    close(server_socket);
    return 0;
}
