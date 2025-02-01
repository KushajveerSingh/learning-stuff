#include "network.hpp"

struct CLIENT_INFO {
    char status;
    std::string ip;
    std::string port;
    std::queue<std::pair<decltype(chrono_type), std::string> > queue;
    std::set<std::string> set;
};
std::map<std::string, CLIENT_INFO> client_info_map;
std::mutex client_info_map_mutex;
void participant_thread_a(int, std::string);

void print_client_info_map() {
    std::map<std::string, CLIENT_INFO>::iterator itr;

    for (itr=client_info_map.begin(); itr != client_info_map.end(); itr++) {
        std::string key = itr->first;
        struct CLIENT_INFO client_info = itr->second;
        std::cout << key << " = (" << client_info.ip << ", " << client_info.port << ", " << client_info.status << ", " << client_info.queue.size() << ", " << client_info.set.size() << ")" << std::endl;
    }
    std::cout << std::endl;
}

void modify_client_info_map(std::string id, std::string action, std::string port="", std::string ip="") {
    client_info_map_mutex.lock();

    std::map<std::string, CLIENT_INFO>::iterator itr;
    if (action != "register" && action != "msend") {
        itr = client_info_map.find(id);
        if (itr == client_info_map.end()) {
            std::cout << "(coordinator) CLIENT_ID not in map" << std::endl;
            client_info_map_mutex.unlock();
            return;
        }
    }

    if (action == "deregister") {
        client_info_map.erase(itr);
    }

    else if (action == "disconnect") {
        itr->second.status = 'O';
    }

    else if (action == "reconnect") {
        itr->second.status = 'A';
        itr->second.port = port;

        // Receive all the messages (in last t_d seconds)
        auto curr_time = std::chrono::system_clock::now();
        struct CLIENT_INFO client_info = itr->second;
        double TIME = std::stoi(ip);

        while (!itr->second.queue.empty()) {
            auto send_time = itr->second.queue.front().first;
            std::string message = itr->second.queue.front().second;
            itr->second.queue.pop();
            std::chrono::duration<double> elapsed_seconds = curr_time - send_time;
            
            int temp_fd = establish_connection_to_coordinator(itr->second.ip.c_str(), itr->second.port.c_str());
            
            // Check for time constraint and duplicate message
            if (elapsed_seconds.count() < TIME) {
                if (itr->second.set.find(message) == itr->second.set.end()) {
                    itr->second.set.insert(message);
                    send_msg(temp_fd, message);
                }
            }

            if (close(temp_fd) == -1)
                perror("(coordinator) Close temp_fd in reconnect");
        }
    }

    else if (action == "register") {
        struct CLIENT_INFO client_info;
        client_info.status = 'A';
        client_info.ip = ip;
        client_info.port = port;
        client_info.queue = std::queue<std::pair<decltype(chrono_type), std::string> >();
        client_info.set = std::set<std::string> ();
        client_info_map[id] = client_info;
    }

    else if (action == "stop_thread_b") {
        if (itr->second.status == 'A') {
            // Send stop message
            struct CLIENT_INFO client_info = itr->second;
            int temp_fd = establish_connection_to_coordinator(client_info.ip.c_str(), client_info.port.c_str());
            send_msg(temp_fd, "CLOSE_THREAD");

            if (close(temp_fd) == -1) {
                perror("(coordinator) Close temp_fd");
            }
        }
    }

    else if (action == "msend") {
        std::string message = port + "\n";
        auto curr_time = std::chrono::system_clock::now();
        std::pair<decltype(chrono_type), std::string> pair (curr_time, message);

        // Store message in queue of every ID
        std::map<std::string, CLIENT_INFO>::iterator itr;
        for (itr=client_info_map.begin(); itr != client_info_map.end(); itr++)
            itr->second.queue.push(pair);

        // Send message to all active participants
        for (itr=client_info_map.begin(); itr != client_info_map.end(); itr++) {
            struct CLIENT_INFO client_info = itr->second;

            if (client_info.status == 'A') {
                // Pop from queue and send message
                std::string message = client_info.queue.front().second;
                itr->second.queue.pop();

                // Check if it is duplicate message
                if (client_info.set.find(message) == client_info.set.end()) {
                    itr->second.set.insert(message);
               
                    int temp_fd = establish_connection_to_coordinator(client_info.ip.c_str(), client_info.port.c_str());
                    send_msg(temp_fd, message);
                    
                    if (close(temp_fd) == -1) {
                        perror("(coordinator) Close send_message_msend");
                    }
                }
           }
        }
    }

    else {
        std::cout << "Invalid action = " << action << std::endl;
    }

    client_info_map_mutex.unlock();
}

int main (int argc, char **argv) {
    // Read input data from coordinator.txt
    const char *conf_path = argv[1];
    const char *PORT;
    std::string TIME;

    std::ifstream conf;
    conf.open(conf_path, std::ios::in);
    if (conf.is_open()) {
        std::string temp;
        std::vector<std::string> data;
        while (getline(conf, temp))
            data.push_back(temp);
        PORT = data[0].c_str();
        TIME = data[1];
        conf.close();
    }
    else {
        std::cout << "(coordinator) Cannot open configuration file" << std::endl;
        exit(1);
    }
    
    // Open port for listening to connections
    int sockfd = open_port_for_connections(PORT);

    struct sigaction sa;
    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("(coordinator) sigaction");
        exit(1);
    }

    std::cout << "(coordinator) Started. Waiting for connections..." << std::endl;

    // Create loop to get values from participants. Close conn after getting info
    int new_fd, status;
    socklen_t sin_size;
    struct sockaddr_storage their_addr;
    sin_size = sizeof their_addr;
    char buffer[MAXFILEBUFFER] = {0};
    std::string reply;
    
    while (1) {
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("(coordinator) Error accepting connection");
            continue;
        }
        
        // Get client ip-address
        struct sockaddr_in* client_addr = (struct sockaddr_in*)&their_addr;
        struct in_addr ip_addr = client_addr->sin_addr;
        char str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &ip_addr, str, INET_ADDRSTRLEN);
        std::string client_ip = (std::string)str;

        // Send ip address of client (as a handshake message)
        std::cout << "(coordinator) Connection received from " << client_ip << std::endl;
        send_msg(new_fd, client_ip);

        std::thread handle_participant(participant_thread_a, new_fd, TIME);
        handle_participant.detach();
    }
}

void participant_thread_a(int new_fd, std::string TIME) {
    char buffer[MAXFILEBUFFER] = {0};
    std::string reply;

    while (1) {
        // Get user_input (and send acknolwedgement)
        reply = recv_msg(new_fd, buffer);
        std::string command="", args="", temp_input;
        temp_input = reply;
        bool seen_space = false;
        for (int i=0; i<temp_input.size(); i++) {
            if (seen_space == true)
                args += temp_input[i];
            else {
                if (temp_input[i] == ' ')
                    seen_space = true;
                else
                    command += temp_input[i];
            }
        }
        send_msg(new_fd, "MESSAGE_RECEIVED");

        // Take action as per every command
        if (command == "register") {
            reply = recv_msg(new_fd, buffer);
            if (reply != "CONTINUE") {
                // Received ID_CLIENTIP_PORT
                std::string ID, CLIENT_IP, CLIENT_PORT;
                int index1 = reply.find('_');
                int index2 = reply.find('_', index1+1);
                ID = reply.substr(0, index1);
                CLIENT_IP = reply.substr(index1+1, index2-index1-1);
                CLIENT_PORT = reply.substr(index2+1);

                // Add client info to map
                modify_client_info_map(ID, "register", CLIENT_PORT, CLIENT_IP);
            }
        }

        else if (command == "deregister") {
            reply = recv_msg(new_fd, buffer);

            // Stop thread_b of client
            modify_client_info_map(reply, "stop_thread_b");
            
            // Remove ID from client_info_map
            modify_client_info_map(reply, "deregister");
        }

        else if (command == "disconnect") {
            reply = recv_msg(new_fd, buffer);

            // Stop thread_b of client
            modify_client_info_map(reply, "stop_thread_b");

            // Change status in client_info_map
            modify_client_info_map(reply, "disconnect");
        }

        else if (command == "reconnect") {
            reply = recv_msg(new_fd, buffer);
            if (reply != "CONTINUE") {
                // Received ID_CLIENTIP_PORT
                std::string ID, CLIENT_IP, CLIENT_PORT;
                int index1 = reply.find('_');
                int index2 = reply.find('_', index1+1);
                ID = reply.substr(0, index1);
                CLIENT_IP = reply.substr(index1+1, index2-index1-1);
                CLIENT_PORT = reply.substr(index2+1);

                // Change status in client_info_map
                modify_client_info_map(ID, "reconnect", CLIENT_PORT, TIME);
            }
        }

        else if (command == "msend") {
            reply = recv_msg(new_fd, buffer);
            // Received ID_MESSAGE
            std::string ID, message;
            int index = reply.find('_');
            ID = reply.substr(0, index);
            message = reply.substr(index+1);

            modify_client_info_map(ID, "msend", message);
        }

        else {
            std::cout << "Invalid command entered by participant" << std::endl;
        }

        print_client_info_map();
    }

    if (close(new_fd) == -1) {
        perror("(coordinator) Error closing connection");
    }
    std::cout << "(coordinator) Connection closed with participant" << std::endl;
}