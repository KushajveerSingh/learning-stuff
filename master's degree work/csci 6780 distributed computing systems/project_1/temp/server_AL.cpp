#include "server.hpp"
#include <fstream>

int main (int argc, char** argv) {
     /*
    server.cpp takes one required arguments as input
        - port_number
    
    The maximum size of the command buffer is set to 200 bytes. This value
    is defined in "network.hpp" (#define MAXDATASIZE 200).
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
        ipstr    : temporary string to hold ip if needed

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

    char buf[MAXDATASIZE] = {0};
    char file_buf[MAXFILEBUFFER] = {0};
    string full_command;
    string command;
    int command_id;
    int numbytes;
    
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    
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
        cout << "(server) IP address not found in servinfo" << endl;
        exit(1);
    }

    // From this point onwards, all the info about connection is stored in 'res'

    // Create socket (and check for errors)
    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    
    if (sockfd == -1) {
        perror("(server) socket");
        exit(1);
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        perror("(server) setsockopt");
        exit(1);
    }

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

    cout << "(server) Started. Waiting for connections..." << endl;

    while (1) { // keep listening for connections
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("(server) accept");
            continue;
        }

        cout << "(server) got connection" << endl;

        while (1) {

            numbytes = recv_msg(new_fd, buf);
	    
            full_command = buf;
	    command = full_command.substr(0, full_command.find(delim));
	    cout << "(server) received = " << command << endl;

	    string reply = "reply from server";
	    	    
            command_id = COMMAND_ID[command];
	    switch (command_id) {
		
		case 4:
                    // ls
		    send_system_msg(new_fd, buf, file_buf);
		    break;
	        
	        case 5:
		    // cd <remote_directory_name> or cd ..
		    //string secondary = command_arg(command);
		    if (command.length() == full_command.length()){
		        send_msg(new_fd, "improper use of cd, specify directory");
		    }
		    else {
			reply += ": " + full_command;
			chdir(command_arg(full_command).c_str());
			send_msg(new_fd, reply);
		    }
		    break;

		case 6:
		    // mkdir <remote_directory_name>
		    if (command.length() == full_command.length()){
		        send_msg(new_fd, "improper use of cd, specify directory");
		    }
		    else {
			reply += ": " + full_command;
			mkdir(command_arg(full_command).c_str(), write_access);
			send_msg(new_fd, reply);
		    }
		    break;
                
                case 7:
		    // pwd
		    send_system_msg(new_fd, buf, file_buf);
		    break;

		case 8:
		    // quit
		    send_msg(new_fd, reply);
		    if (close(sockfd) == -1)
		        perror("(server) close socket");
		    cout<< "(server) closing connection" << endl;
		    exit(0);

		default:
		    string nocase = reply + ": no program for entered string";
		    send_msg(new_fd, nocase);
            }
            memset(buf, '\0', numbytes); //wipe slate of buf clean

        }

	cout<< "while finished"<<endl;

    }
    // Stop server
    if (close(sockfd) == -1)
        perror("(server) close socket");
    cout << "(server) Server stopped" << endl;
    exit(0);
}

