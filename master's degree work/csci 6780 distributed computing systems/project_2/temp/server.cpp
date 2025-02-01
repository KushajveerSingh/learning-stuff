#include "server.hpp"

int main (int argc, char** argv) {
     /*
        server.cpp takes two required arguments as input
            - nport_number
            - tport_number
        
        Note:
            Check network.hpp for the maximum allowed buffer sizes. The defaults
            are shown below
            * MAXDATASIZE 1024 -> The maximum size of user input in bytes
            * MAXFILEBUFFER 4096 -> The maximum size of messages (including error
                                    messages) that will be shared by client and 
                                    server
    */

    // Parse command line arguments
    const char *NPORT_NUMBER = argv[1];
    const char *TPORT_NUMBER = argv[2];
    
    // Need to create two threads for these
    int n_sockfd, t_sockfd;
    n_sockfd = open_port_for_connections(NPORT_NUMBER);
    t_sockfd = open_port_for_connections(TPORT_NUMBER);
    
    int new_fd, status, yes=1;
    socklen_t sin_size;
    struct sockaddr_storage their_addr;
    struct sigaction sa;
    
    // handle zombie processes
    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("(server) sigaction");
        exit(1);
    }

    std::cout << "(server) Started. Waiting for connections..." << std::endl;

    while (1) { // keep listening for connections
        sin_size = sizeof their_addr;
        new_fd = accept(n_sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("(server) accept");
            continue;
        }
        send_msg(new_fd); // Initial message to accept connection
        std::cout << "(server) connection established with client" << std::endl;
        
        // Create buffers for client
        char user_input[MAXINPUTSIZE] = {0};
        char buffer[MAXFILEBUFFER] = {0};
        std::string reply;

        while (1) {
            std::string command="", args="";
            bool terminate_cmd = false, seen_space = false;
            
            // Receive input from client
            reply = recv_msg(new_fd, user_input, MAXFILEBUFFER);
            
            // Check for terminate command
            if (reply[reply.size()-1] == '&') {
                terminate_cmd = true;
                reply = reply.substr(0, reply.size()-2);
            }

            // Parse input to get "command" and "args"
            for (int i=0; i<reply.size(); i++) {
                if (seen_space == true)
                    args += reply[i];
                else {
                    if (user_input[i] == ' ')
                        seen_space = true;
                    else
                        command += reply[i];
                }
            }

            // quit
            if (command == "quit") {
                send_msg(new_fd);
                if (close(new_fd) == -1)
                    perror("(server) close child");
                std::cout << "(server) connection terminated with client" << std::endl;
                break;
            }

            // pwd
            else if (command == "pwd") {
                if (getcwd(buffer, MAXFILEBUFFER) != NULL)
                    send_msg(new_fd, (std::string)buffer);
                else 
                    send_err_msg(new_fd);
            }

            // mkdir
            else if (command == "mkdir") {
                if (args.size() == 0)
                    send_msg(new_fd, "did not specify dir name");
                else {
                    if (mkdir(args.c_str(), 0777) == -1)
                        send_err_msg(new_fd);
                    else
                        send_msg(new_fd);
                }
            }

            // cd
            else if (command == "cd") {
                if (args.size() == 0)
                    send_msg(new_fd, "did not specify dir name");
                else {
                    if (chdir(args.c_str()) == -1)
                        send_msg(new_fd, "incorrect file name");
                    else
                        send_msg(new_fd);
                }
            }

            // ls
            else if (command == "ls") {
                std::vector <std::string> files;
                DIR *d;
                std::string ls = "";
                struct dirent *dir;
                d = opendir(".");
                
                if (d) {
                    while ((dir = readdir(d)) != NULL) {
                        files.push_back(dir->d_name);
                    }
                    closedir(d);
                    std::sort(files.begin(), files.end());
                    for (int i=0; i<files.size(); i++)
                        ls = ls + files[i] + " ";
                    send_msg(new_fd, ls);
                }
                else
                    send_msg(new_fd, "not able to open dir");
            }

            // delete
            else if (command == "delete") {
                if (args.size() == 0)
                    send_msg(new_fd, "did not specify file name");
                else {
                    if (remove(args.c_str()) == -1)
                        send_err_msg(new_fd);
                    else
                        send_msg(new_fd);
                }
            }

            // put
            else if (command == "put") {
                send_msg(new_fd);
                recv_and_write_file(new_fd, buffer);
            }

            // get
            else if (command == "get") {
                bool file_exist = check_file_exists(args);
                if (!file_exist) {
                    send_msg(new_fd, "NO_FILE"); // sends to common reply
                    continue;
                }
                else {
                    send_msg(new_fd);
                    reply = recv_msg(new_fd, buffer, MAXFILEBUFFER);
                    send_file(new_fd, args, buffer);
                }
            }

            else {
                send_msg(new_fd, "invalid command");
            }
        }
    }

    // Stop server
    if (close(n_sockfd) == -1)
        perror("(server) close socket");
    std::cout << "(server) Server stopped" << std::endl;
    exit(0);
}