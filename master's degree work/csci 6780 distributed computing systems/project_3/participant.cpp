#include "network.hpp"

bool thread_b_active = false;
std::mutex thread_b_mutex;

void thread_b_func(std::string, std::string);

void empty_log_file(std::string path) {
    std::ofstream file;
    file.open(path);
    if (file.is_open())
        file << "";
    else
        std::cout << "Error creating log file" << std::endl;
    file.close();
}

int main (int argc, char **argv) {
    // Read input data from participant.txt
    const char *conf_path = argv[1];
    std::string ID;
    std::string LOG_FILE;
    std::string SERVER_IP;
    std::string SERVER_PORT;

    std::ifstream conf;
    conf.open(conf_path, std::ios::in);
    if (conf.is_open()) {
        std::string temp;
        std::vector<std::string> data;
        while (getline(conf, temp))
            data.push_back(temp);
        ID = data[0];
        LOG_FILE = data[1];
        int index = data[2].find(':');
        SERVER_IP = data[2].substr(0, index);
        SERVER_PORT = data[2].substr(index+1);
    }
    else {
        std::cout << "(participant) Cannot open configuration file" << std::endl;
    }
    
    std::string reply, client_ip;
    char buffer[MAXFILEBUFFER] = {0};

    // Establish connection with coordinator (receive client ip as handshake message)
    int sockfd = establish_connection_to_coordinator(SERVER_IP.c_str(), SERVER_PORT.c_str());
    client_ip = recv_msg(sockfd, buffer, false);
    
    // Main thread that is used to receive user commands
    while (1) {
        // Get user input and store in command, args
        std::string command="", args="", user_input, temp_input;
        std::cout << "> ";
        getline(std::cin, user_input);
        temp_input = user_input;
        
        // Parse input
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

        // Send input message to server (and receive acknowledgment)
        send_msg(sockfd, user_input, false);
        reply = recv_msg(sockfd, buffer, false);

        // Take action as per every command
        // register (need to have thread open for receiving multicast messages)
        if (command == "register") {
            empty_log_file(LOG_FILE);
            thread_b_mutex.lock();
            if (thread_b_active == true) {
                std::cout << "(participant) You already have a port open for multicast messages." << std::endl;
                send_msg(sockfd, "CONTINUE", false);
                thread_b_mutex.unlock();
                continue;
            }
            else {
                thread_b_mutex.unlock();
                std::thread thread_b(thread_b_func, args, LOG_FILE);
                thread_b.detach();
                std::string message = ID + "_" + client_ip + "_" + args;
                send_msg(sockfd, message, false);
            }
        }

        else if (command == "deregister") {
            send_msg(sockfd, ID, false);
            empty_log_file(LOG_FILE);
        }

        else if (command == "disconnect") {
            send_msg(sockfd, ID, false);
        }

        else if (command == "reconnect") {
            thread_b_mutex.lock();
            if (thread_b_active == true) {
                std::cout << "(participant) You already have a port open for multicast messages. Disconnect first." << std::endl;
                send_msg(sockfd, "CONTINUE", false);
                thread_b_mutex.unlock();
                continue;
            }
            else {
                thread_b_mutex.unlock();
                std::thread thread_b(thread_b_func, args, LOG_FILE);
                thread_b.detach();
                std::this_thread::sleep_for(sleep_dura);
                std::this_thread::sleep_for(sleep_dura);
                std::string message = ID + "_" + client_ip + "_" + args;
                send_msg(sockfd, message, false);
            }
        }

        else if (command == "msend") {
            std::string message = ID + "_" + args;
            send_msg(sockfd, message, false);
        }

        else {
            std::cout << '"' << command << '"' << " is not a valid command" << std::endl;
        }

        std::this_thread::sleep_for(sleep_dura);
    }
}

void thread_b_func (std::string port, std::string log_file) {
    // Open port for connections
    int fd = open_port_for_connections(port.c_str());

    struct sigaction sa;
    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("(participant) sigaction");
        exit(1);
    }

    // Specify that port is open for connections
    thread_b_mutex.lock();
    thread_b_active = true;
    thread_b_mutex.unlock();

    int new_fd, status;
    socklen_t sin_size;
    struct sockaddr_storage their_addr;
    sin_size = sizeof their_addr;
    char buffer[MAXFILEBUFFER] = {0};
    std::string reply;

    while (1) {
        new_fd = accept(fd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("(participant) Error accepting connection");
            continue;
        }

        // Check if thread should close
        reply = recv_msg(new_fd, buffer, false);
        if (reply == "CLOSE_THREAD") {
            if (close(fd) == -1) {
                perror("(participant) Error closing main connection");
                continue;
            }
            if (close(new_fd) == -1) {
                perror("(participant) Error closing connection");
                continue;
            }
            thread_b_mutex.lock();
            thread_b_active = false;
            thread_b_mutex.unlock();
            break;
        }
        else {
            // Write the message to log file
            std::ofstream outfile;

            outfile.open(log_file, std::ios_base::app);
            if (outfile.is_open()) {
                outfile << reply;
                outfile.close();
            }
            else {
                std::cout << "(participant) Could not open log file for writing" << std::endl;
            }
        }

        if (close(new_fd) == -1) {
            perror("(participant) Error closing connection");
            continue;
        }

    }
}