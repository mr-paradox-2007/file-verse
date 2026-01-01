#include "cli_client.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    ofs::CliClient client("localhost", 8080);
    
    if (argc > 1 && std::string(argv[1]) == "--help") {
        client.printHelp();
        return 0;
    }

    client.run();
    return 0;
}
