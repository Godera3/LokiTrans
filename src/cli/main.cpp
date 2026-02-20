#include <iostream>
#include <string>

void print_help() {
    std::cout << "LokiTrans CLI\n";
    std::cout << "Usage:\n";
    std::cout << "  loki help\n";
    std::cout << "  loki whoami\n";
    std::cout << "  loki peers\n";
}

int main(int argc, char** argv) {

    //  No command → show help
    if (argc < 2) {
        print_help();
        return 0;
    }

    std::string command = argv[1];

    //  help command
    if (command == "help" || command == "--help") {
        print_help();
        return 0;
    }

    //  whoami command
    if (command == "whoami") {
        std::cout << "DeviceName: Luka-PC\n";
        std::cout << "Hostname: Luka-PC\n";
        std::cout << "IPs:\n";
        std::cout << "  - 192.168.1.10\n";
        return 0;
    }

    //  peers command
    if (command == "peers") {
        std::cout << "No peers found\n";
        return 0;
    }

    //  unknown command
    std::cout << "Unknown command: " << command << "\n\n";
    print_help();

    return 1;
}