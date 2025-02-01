#include "trial_network.hpp"

int main (int argc, char** argv) {
     /*
    client.cpp takes two required arguments as input in the following 
    order
        - servername
        - port_number
    
    The maximum size of the command buffer is set to 200 bytes. This value
    is defined in "network.hpp" (#define MAXDATASIZE 200).
    */

    // Parse command line arguments
    const char *SERVER_ADDRESS = argv[1];
    const char *PORT_NUMBER = argv[2];

    /*
        sockfd   : host socket file descriptor (will listen for connections)
        status   : hold return value of getaddrinfo
        hints    : add socket description (like ipv4, tcp socket)
        servinfo : pointer to linked list of many ip's returned by getaddrinfo
        res      : contains the relevant 'addrinfo' from 'servinfo' for out ip
                   (this will be used for all the operations, servinfo will be
                   freed after we get this)
        p        : temporary struct to loop through servinfo
        s        : store the ip of the server
        ipstr    : temporary string to hold ip if needed
    */
    int sockfd, status;
    struct addrinfo hints, *servinfo, *res, *p;
    char s[INET_ADDRSTRLEN], ipstr[INET_ADDRSTRLEN];
    
    char buf[MAXDATASIZE] = {0};
    int numbytes;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    
    status = getaddrinfo(SERVER_ADDRESS, PORT_NUMBER, &hints, &servinfo);

    // Check if servinfo was populated without errors
    if (status != 0) {
        fprintf(stderr, "(client) getaddrinfo: %s\n", gai_strerror(status));
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
        cout << "(client) IP address not found in servinfo" << endl;
        exit(1);
    }

    // From this point onwards, all the info about connection is stored in 'res'

    // Create socket (and check for errors)
    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    
    if (sockfd == -1) {
        perror("(client) socket");
        exit(1);
    }

    // Connect to socket (and check for errors)
    if (connect(sockfd, res->ai_addr, res->ai_addrlen) == -1) {
        perror("(client) connect");
        exit(1);
    }

    if (res == NULL) {
        fprintf(stderr, "(client) failed to connect\n");
        exit(1);
    }

    inet_ntop(res->ai_family, get_ip_addr(res), s, sizeof s);
    cout << "(client) connection established with " << s << endl;

    freeaddrinfo(servinfo);

    // Handle client commands
    string user_input, command, command_args;
    int command_id, temp;

    while (1) {
        show_prompt();
        getline(cin, user_input);
        command = user_input;
        command_args = command_arg(command);
	command_id = COMMAND_ID[command];

        //even in the case of quit, the server needs to close the socket
	
	if (send(sockfd, user_input.c_str(), user_input.size(), 0) == -1)
            perror("(client) send:");
        
	switch (command_id) {
	    case 1:
		// get
		//command_args = command_arg(command);
		cout << "pre get " << endl;
		recv_file_stream(command_args, sockfd, 0);
		//break;
		//
		//during testing this should go straight to default
	    
	    case 2:
		// put
		command_args = command_arg(command);
		//(TODO)
		break;

	    case 8:
                // quit
                if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1)
            	    perror("(client) recv");
        	cout << "client received = " << buf << endl; 

		if (close(sockfd) == -1)
                    perror("(client) close socket");
                cout << "(client) connection closed with " << s << endl;
                exit(0);

	    default:
		if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1)
            	    perror("(client) recv");
        	cout << "client received = " << buf << endl;
        
       }
       memset(buf, '\0', numbytes); //wipe slate of buf clean
    }
}
