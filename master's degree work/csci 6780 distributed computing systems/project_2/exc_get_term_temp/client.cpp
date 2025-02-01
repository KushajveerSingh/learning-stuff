#include "network.hpp"

void file_operation_function (int, std::string, std::string, std::string, bool, char*);
void remove_file_object (std::string);
char *SERVER_ADDRESS;

std::vector<std::string> file_table;
std::mutex file_table_mutex;

int main (int argc, char** argv) {
     /*
        client.cpp takes three required arguments as input in the following 
        order
            - servername
            - nport_number
            - tport_number
    */

    // Parse command line arguments
    SERVER_ADDRESS = argv[1];
    const char *NPORT_NUMBER = argv[2];
    const char *TPORT_NUMBER = argv[3];

    char buffer[MAXFILEBUFFER] = {0};
    std::string reply, user_input, temp_input;

    // Establish connection with server
    int n_sockfd = establish_connection_to_server(SERVER_ADDRESS, NPORT_NUMBER);
    
    reply = recv_msg(n_sockfd, buffer, false);
    if (reply == "CONNECTION_REFUSED") {
        std::cout << "(client) Maximum connections reached on server. Try after some time." << std::endl;
        if (close(n_sockfd) == -1)
            perror("(client) close nport socket on connection refuse");
        return 0;
    }
    else {
        std::cout << "(client) Connection established with server" << std::endl;
    }
    
    while (1) {
        std::string command="", args="";
        bool terminate = false;

        // Get user_input from cmd
        show_prompt();
        getline(std::cin, user_input);
        temp_input = user_input;

        // Process the input
        // Handle terminate command
        if (user_input[user_input.size()-1] == '&') {
            terminate = true;
            user_input = user_input.substr(0, user_input.size()-2);
        }
        // Get command, args
        bool seen_space = false;
        for (int i=0; i<user_input.size(); i++) {
            if (seen_space == true)
                args += user_input[i];
            else {
                if (user_input[i] == ' ')
                    seen_space = true;
                else
                    command += user_input[i];
            }
        }
        
        // Check valid input for get,put
        if (command == "get" or command == "put" or command == "delete" or command == "mkdir") {
            if (args.size() == 0) {
                std::cout << "did not provide file name" << std::endl;
                continue;
            }
        }

        if (command == "terminate") {
            int t_sockfd = establish_connection_to_server(SERVER_ADDRESS, TPORT_NUMBER);
            send_msg(t_sockfd, args, false);
            if (close(t_sockfd) == -1)
                perror("(client) terminate socket close");
            continue;
        }

        // Send message to server
        send_msg(n_sockfd, temp_input, false);

        // Get reply from server
        reply = recv_msg(n_sockfd, buffer, false);
        
        // Take action as per command
        if (command == "quit") {
            if (reply != "NO_MESSAGE")
                std::cout << reply << std::endl;
            else {
                if (close(n_sockfd) == -1)
                    perror("(client) close socket");
                std::cout << "(client) Connection closed with server" << std::endl;
                exit(0);
            }
        }
        
        else if (command == "pwd") {
            std::cout << reply << std::endl;
        }

        else if (command == "mkdir") {
            if (reply != "NO_MESSAGE")
                std::cout << reply << std::endl;
        }

        else if (command == "cd") {
            if (reply != "NO_MESSAGE")
                std::cout << reply << std::endl;
        }

        else if (command == "ls") {
            std::cout << reply << std::endl;
        }

        else if (command == "delete") {
            if (reply != "NO_MESSAGE")
                std::cout << reply << std::endl;
        }

        else if (command == "put") {
            if (terminate) {
                int index = reply.find('_');
                std::string id = reply.substr(0, index);
                std::string client_fd = reply.substr(index+1, reply.size());

                std::cout << "Id: " << id << std::endl;
                
                char *temp;
                std::thread handle(file_operation_function, -1, client_fd, args, command, terminate, temp);
                handle.detach();
            }
            else
                file_operation_function(n_sockfd, "", args, command, false, buffer);
        }

        else if (command == "get") {
            if (reply == "FILE_NOT_EXIST") {
                std::cout << "File does not exist" << std::endl;
                continue;
            }

            if (terminate) {
                int index = reply.find('_');
                std::string id = reply.substr(0, index);
                std::string client_fd = reply.substr(index+1, reply.size());

                std::cout << "Id: " << id << std::endl;
                
                char *temp;
                std::thread handle(file_operation_function, -1, client_fd, args, command, terminate, temp);
                handle.detach();
            }
            else
                file_operation_function(n_sockfd, "", args, command, false, buffer);
        }
    }
}

void file_operation_function (int fd, std::string client_fd, std::string args, std::string command, bool terminate, char *buffer) {
    while (1) {
        file_table_mutex.lock();
        bool exists = false;
        for (int i=0; i<file_table.size(); i++) {
            if (file_table[i] == args) {
                exists = true;
                break;
            }
        }

        if (exists) {
            file_table_mutex.unlock();
            std::this_thread::sleep_for(dura);
        }
        else {
            file_table.push_back(args);
            file_table_mutex.unlock();
            break;
        }
    }
    
    if (command == "put") {
        if (terminate) {
            char buffer[MAXFILEBUFFER] = {0};
            int fd = establish_connection_to_server(SERVER_ADDRESS, EXTRA_PORT);
            send_msg(fd, client_fd);

            std::string reply = recv_msg(fd, buffer, false);
            send_file(fd, args, buffer, false);
            reply = recv_msg(fd, buffer, false);
            
            std::cout << "(background thread)" << reply << std::endl;
            if (close(fd) == -1)
                perror("(client) close eport");
            remove_file_object(args);
        }
        else {
            send_file(fd, args, buffer, false);
            std::string reply = recv_msg(fd, buffer, false);
            std::cout << reply << std::endl;
            remove_file_object(args);
        }
    }
    
    if (command == "get") {
        if (terminate) {
            char buffer[MAXFILEBUFFER] = {0};
            int fd = establish_connection_to_server(SERVER_ADDRESS, EXTRA_PORT);
            send_msg(fd, client_fd);

            recv_and_write_file(fd, args, buffer, true, false);

            if (close(fd) == -1)
                perror("(client) close eport");
            remove_file_object(args);
        }
        else {
            send_msg(fd);
            recv_and_write_file(fd, args, buffer, true, false);
            remove_file_object(args);
        }
    }
}

void remove_file_object (std::string path) {
    file_table_mutex.lock();
    int index;
    for (int i=0; i<file_table.size(); i++) {
        if (file_table[i] == path) {
            index = i;
            break;
        }
    }
    file_table.erase(file_table.begin()+index);
    file_table_mutex.unlock();
}

char file_check(std::string path){
    //function is defined in network.hpp
    //necessary for server, unnecessary for client
    return 'A'; // A is practically continue for this function
}
