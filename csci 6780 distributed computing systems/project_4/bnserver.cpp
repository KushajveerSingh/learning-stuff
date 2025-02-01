#include "network.hpp"

struct SERVER bn_server;
std::vector<int> bn;
std::mutex bn_mutex;
void bnthread_func (const char*);


int main (int argc, char **argv) {
    // Read input data
    bn_server.start_index = 0;
    bn_server.end_index = 1023;
    
    for (int i=0; i<1024; i++)
        bn_server.arr[i] = "EMPTY_KEY";

    const char *conf_path = argv[1];
    const char *PORT;

    std::ifstream conf;
    conf.open(conf_path, std::ios::in);
    if (conf.is_open()) {
        std::string temp;
        std::vector<std::string> data;
        int key;

        while (getline(conf, temp))
            data.push_back(temp);
        PORT = data[1].c_str();
        for (int i=2; i<data.size(); i++) {
            int index = data[i].find(' ');
            key = stoi(data[i].substr(0, index));
            bn_server.arr[key] = data[i].substr(index+1);
        }
        conf.close();
    }
    else {
        std::cout << "(bnserver) Cannot open configuration file" << std::endl;
        exit(1);
    }
    
    std::thread bnthread(bnthread_func, PORT);
    bnthread.detach();

    std::string user_input, temp_input, command, value="";
    int key;
    while (1) {
        // Get input
        std::cout << "> ";
        getline(std::cin, user_input);

        // Parse input
        int index = user_input.find(' ');
        command = user_input.substr(0, index);
        if (command == "insert") {
            int index1 = user_input.find(' ', index+1);
            key = stoi(user_input.substr(index+1, index1));
            value = user_input.substr(index1+1);
        }
        else {
            key = stoi(user_input.substr(index+1));
        }

        std::string server_id = "";
        std::string servers_traversed = "[0";
        std::string message = "";

        if (command == "lookup") {
            bn_mutex.lock();
            for (int i=1; i<bn.size(); i++) {
                if (key <= bn[i]) {
                    server_id = std::to_string(bn[i-1]);
                    if (bn_server.arr[key] == "EMPTY_KEY")
                        message = "Key not found";
                    else
                        message = bn_server.arr[key];
                    if (key == bn[i] && bn[i] != 1023) {
                        servers_traversed = servers_traversed + ", " + std::to_string(bn[i]);
                        server_id = std::to_string(bn[i]);
                    }
                    break;
                }
                servers_traversed = servers_traversed + ", " + std::to_string(bn[i]);
            }
            bn_mutex.unlock();
            servers_traversed = servers_traversed + "]";

            if (message == "Key not found")
                std::cout << message << std::endl;
            else
                std::cout << "Value = " << message << std::endl;
            std::cout << "Server IDs contacted = " << servers_traversed << std::endl;
            std::cout << "Response from server ID = " << server_id << std::endl;
        }

        else if (command == "insert") {
            bn_mutex.lock();
            for (int i=1; i<bn.size(); i++) {
                if (key <= bn[i]) {
                    server_id = std::to_string(bn[i-1]);
                    bn_server.arr[key] = value;
                    if (key == bn[i] && bn[i] != 1023) {
                        servers_traversed = servers_traversed + ", " + std::to_string(bn[i]);
                        server_id = std::to_string(bn[i]);
                    }
                    break;
                }
                servers_traversed = servers_traversed + ", " + std::to_string(bn[i]);
            }
            servers_traversed = servers_traversed + "]";
            bn_mutex.unlock();

            std::cout << "Server IDs contacted = " << servers_traversed << std::endl;
            std::cout << "Value inserted in server ID = " << server_id << std::endl;
        }

        else if (command == "delete") {
            bn_mutex.lock();
            for (int i=1; i<bn.size(); i++) {
                if (key <= bn[i]) {
                    server_id = std::to_string(bn[i-1]);
                    if (bn_server.arr[key] == "EMPTY_KEY")
                        message = "Key not found";
                    else {
                        message = "Successful deletion";
                        bn_server.arr[key] = "EMPTY_KEY";
                    }
                    if (key == bn[i] && bn[i] != 1023) {
                        servers_traversed = servers_traversed + ", " + std::to_string(bn[i]);
                        server_id = std::to_string(bn[i]);
                    }
                    break;
                }
                servers_traversed = servers_traversed + ", " + std::to_string(bn[i]);
            }
            bn_mutex.unlock();
            servers_traversed = servers_traversed + "]";

            std::cout << message << std::endl;
            std::cout << "Server IDs contacted = " << servers_traversed << std::endl;
            std::cout << "Deletion in server ID = " << server_id << std::endl;
        }

        else {
            std::cout << "Invalid command" << std::endl;
        }
    }
}

void bnthread_func (const char *PORT) {
    int fd = open_port_for_connections(PORT);

    struct sigaction sa;
    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("(coordinator) sigaction");
        exit(1);
    }

    bn.push_back(0);
    bn.push_back(1023);

    int new_fd;
    socklen_t sin_size;
    struct sockaddr_storage their_addr;
    sin_size = sizeof their_addr;
    char buffer[MAXFILEBUFFER] = {0};
    std::string reply;

    while (1) {
        new_fd = accept(fd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("(bnserver) Error accepting connection");
            continue;
        }

        reply = recv_msg(new_fd, buffer);
        send_msg(new_fd);

        if (reply == "enter") {
            reply = recv_msg(new_fd, buffer);
            int index = stoi(reply);
            
            std::string key_range = "";
            std::string predecessor = "";
            std::string successor = "";
            std::string ids_traversed = "";
            
            bn_mutex.lock();
            ids_traversed = ids_traversed + std::to_string(bn[0]);

            for (int i=1; i<bn.size(); i++) {
                if (bn[i] > index) {
                    int end_index;
                    if (bn[i] == 1023)
                        end_index = 1023;
                    else
                        end_index = bn[i]-1;
                    key_range = key_range + "[" + std::to_string(index) + "," + std::to_string(end_index) + "]";
                    predecessor = std::to_string(bn[i-1]);
                    successor = std::to_string(bn[i]);
                    if (successor == "1023")
                        successor = "0";
                    bn.insert(bn.begin()+i, index);
                    break;
                }
                ids_traversed = ids_traversed + ", " + std::to_string(bn[i]);
            }
            bn_mutex.unlock();

            send_msg(new_fd, key_range);
            reply = recv_msg(new_fd, buffer);
            send_msg(new_fd, predecessor);
            reply = recv_msg(new_fd, buffer);
            send_msg(new_fd, successor);
            reply = recv_msg(new_fd, buffer);
            send_msg(new_fd, ids_traversed);
        }

        else if (reply == "exit") {
            reply = recv_msg(new_fd, buffer);
            int index = stoi(reply);

            std::string successor = "";
            std::string key_range = "";

            bn_mutex.lock();
            for (int i=1; i<bn.size(); i++) {
                if (bn[i] == index) {
                    if (bn[i+1] == 1023)
                        successor = "0";
                    else
                        successor = std::to_string(bn[i+1]);

                    int end_index;
                    if (bn[i+1] == 1023)
                        end_index = 1023;
                    else
                        end_index = bn[i+1]-1;
                    key_range = key_range + "[" + std::to_string(bn[i]) + "," + std::to_string(end_index) + "]";
                    bn.erase(bn.begin()+i);
                    break;
                }
            }
            bn_mutex.unlock();

            send_msg(new_fd, successor);
            reply = recv_msg(new_fd, buffer);
            send_msg(new_fd, key_range);
        }

        if (close(new_fd) == -1) {
            perror("(bnserver) Error closing connection");
            continue;
        }
    }
}