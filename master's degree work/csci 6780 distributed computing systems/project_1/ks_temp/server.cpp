#include "server.hpp"

int main (int argc, char** argv) {
     /*
    server.cpp takes one required arguments as input
        - port_number
    
    Note:
        Check network.hpp for the maximum allowed buffer sizes. The defaults
        are shown below
        * MAXDATASIZE 1024 -> The maximum size of user input in bytes
        * MAXFILEBUFFER 4096 -> The maximum size of messages (including error
                                messages) that will be shared by client and 
                                server
    */

    // Parse command line arguments
    const char *PORT_NUMBER = argv[1];

    /*
        sockfd   : host socket file descriptor (will listen for connections)
        new_fd   : file descriptor that will connect to client (one per client)
        status   : hold return value of getaddrinfo
        yes      : value of 'optval' in setsockopt
        hints    : add socket description (like ipv4, tcp socket)
        servinfo : pointer to linked list of many ip's returned by getaddrinfo
        res      : contains the relevant 'addrinfo' from 'servinfo' for out ip
                   (this will be used for all the operations, servinfo will be
                   freed after we get this)
        p        : temporary struct to loop through servinfo
        s        : store the ip of the server
        ipstr    : temporary std::string to hold ip if needed

        their_addr : socket file descriptor that will communicate with client
        sin_size   : size of their_addr
        sa         : used to handle zombie processes
    */

    int sockfd, new_fd, status, yes=1;
    struct addrinfo hints, *servinfo, *res, *p;
    char s[INET_ADDRSTRLEN], ipstr[INET_ADDRSTRLEN];

    socklen_t sin_size;
    struct sockaddr_storage their_addr;
    struct sigaction sa;
    
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    
    hints.ai_flags = AI_PASSIVE;
    
    status = getaddrinfo(SERVER_ADDRESS, PORT_NUMBER, &hints, &servinfo);

    // Check if servinfo was populated without errors
    if (status != 0) {
        fprintf(stderr, "(server) getaddrinfo: %s\n", gai_strerror(status));
        exit(1);
    }

    // There can be multiple entires in servinfo, ensure we are using correct one
    int len_list = 0, ip_miss = 0;
    for (p = servinfo; p != NULL; p = p->ai_next) {
        len_list += 1;
        void *addr = get_ip_addr(p);
        inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
        
        if (!strcmp(ipstr, SERVER_ADDRESS))
            res = p;
        else
            ip_miss += 1;
    }
    
    if (ip_miss == len_list) {
        std::cout << "(server) IP address not found in servinfo" << std::endl;
        exit(1);
    }

    // From this point onwards, all the info about connection is stored in 'res'

    // Create socket (and check for errors)
    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    
    if (sockfd == -1) {
        perror("(server) socket");
        exit(1);
    }

    // if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
    //     perror("(server) setsockopt");
    //     exit(1);
    // }

    // Bind socket to a port (and check for errors)
    if (bind(sockfd, res->ai_addr, res->ai_addrlen) == -1) {
        close(sockfd);
        perror("(server) bind");
        exit(1);
    }

    if (res == NULL) {
        fprintf(stderr, "(server) failed to bind\n");
        exit(1);
    }

    freeaddrinfo(servinfo);

    // listen to "MAX_CONNECTIONS"
    if (listen(sockfd, MAX_CONNECTIONS) == -1) {
        perror("(server) listen");
        exit(1);
    }

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
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("(server) accept");
            continue;
        }
        send_msg(new_fd); // Initial message to accept connection
        std::cout << "(server) connection established with client" << std::endl;
        
        // Create buffers for client
        char user_input[MAXDATASIZE] = {0};
        char file_buf[MAXFILEBUFFER] = {0};
        std::string reply;

        while (1) {
            std::string command="", args="";
            int input_bytes;
            bool seen_space = false;
            
            // Receive input from client (command and args)
            reply = recv_msg(new_fd, user_input, MAXFILEBUFFER);
            
            // Parse input to get "command" and "args"
            for (int i=0; i<reply.size(); i++) {
                if (seen_space == true) {
                    args += user_input[i];
                }
                else {
                    if (user_input[i] == ' ')
                        seen_space = true;
                    else
                        command += user_input[i];
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
                if (getcwd(file_buf, MAXFILEBUFFER) != NULL)
                    send_msg(new_fd, (std::string)file_buf);
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
                recv_and_write_file(new_fd);
            }

            // get
            else if (command == "get") {
                bool file_exist = check_file_exist(args);
                if (!file_exist) {
                    send_msg(new_fd, "NO_FILE"); // sends to common reply
                    continue;
                }
                else {
                    send_msg(new_fd);
                    reply = recv_msg(new_fd, file_buf, MAXFILEBUFFER);
                    send_file(new_fd, args);
                }
            }

            else {
                send_msg(new_fd, "invalid command");
            }
        }
    }

    // Stop server
    if (close(sockfd) == -1)
        perror("(server) close socket");
    std::cout << "(server) Server stopped" << std::endl;
    exit(0);
}