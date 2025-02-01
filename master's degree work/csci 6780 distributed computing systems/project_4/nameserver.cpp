#include "network.hpp"

struct SERVER name_server;

int main (int argc, char **argv) {
    // Read input data
    const char *conf_path = argv[1];
    std::string command, reply;
    char buffer[MAXFILEBUFFER] = {0};

    while (1) {
        std::string start_index;
        const char *PORT;
        const char *server_ip, *server_port;

        std::ifstream conf;
        conf.open(conf_path, std::ios::in);
        if (conf.is_open()) {
            std::string temp;
            std::vector<std::string> data;
            
            while (getline(conf, temp))
                data.push_back(temp);
            start_index = data[0];
            PORT = data[1].c_str();
            int index = data[2].find(' ');
            server_ip = data[2].substr(0, index).c_str();
            server_port = data[2].substr(index+1).c_str();
        }
        else {
            std::cout << "(nameserver) Cannot open configuration file" << std::endl;
            exit(1);
        }

        std::cout << "> ";
        getline(std::cin, command);
        
        if (command != "enter" && command != "exit") {
            std::cout << "Invalid command" << std::endl;
            continue;
        }

        int fd = establish_connection_to_server(server_ip, server_port);
        send_msg(fd, command, false);
        reply = recv_msg(fd, buffer, false);

        if (command == "enter") {
            send_msg(fd, start_index);

            std::string key_range, predecessor, successor, ids_traversed;
            
            key_range = recv_msg(fd, buffer, false);
            send_msg(fd, "", false);
            predecessor = recv_msg(fd, buffer, false);
            send_msg(fd, "", false);
            successor = recv_msg(fd, buffer, false);
            send_msg(fd, "", false);
            ids_traversed = recv_msg(fd, buffer, false);
            
            std::cout << "successful entry" << std::endl;
            std::cout << "Range of keys = " << key_range << std::endl;
            std::cout << "Predecessor ID = " << predecessor << std::endl;
            std::cout << "Successor ID = " << successor << std::endl;
            std::cout << "Server IDs traversed = [" << ids_traversed << "]" << std::endl;
        }

        else if (command == "exit") {
            send_msg(fd, start_index, false);
            std::string successor, key_range;

            successor = recv_msg(fd, buffer, false);
            send_msg(fd, "", false);
            key_range = recv_msg(fd, buffer, false);
            
            std::cout << "successful exit" << std::endl;
            std::cout << "Successor ID = " << successor << std::endl;
            std::cout << "Key range = " << key_range << std::endl;
        }

        if (close(fd) == -1) {
            perror("(nameserver) Error closing connection");
            continue;
        }
    }
       
}