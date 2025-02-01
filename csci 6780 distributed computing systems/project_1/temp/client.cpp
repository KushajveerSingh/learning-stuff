#include "network.hpp"

int main (int argc, char** argv) {
     /*
    client.cpp takes two required arguments as input in the following 
    order
        - servername
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

    char recv_buf[MAXFILEBUFFER] = {0};
    int recv_bytes;

    while (1) {
        string user_input, reply, command="", args="";
        bool seen_space = false;

        // Get user input from cmd
        show_prompt();
        getline(cin, user_input);

        // Parse user_input to get "command" and "args"
        for (int i=0; i<user_input.size(); i++) {
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

        // Send message to server
        send_msg(sockfd, user_input, false);

        // Get reply from server
        recv_bytes = recv_msg(sockfd, recv_buf, MAXFILEBUFFER, false);

        // Parse reply
        for (int i=0; i<recv_bytes; i++)
            reply += recv_buf[i];

        // Take action as per command
        if (command == "quit") {
            if (close(sockfd) == -1)
                perror("(client) close socket");
            cout << "(client) connection closed with " << s << endl;
            exit(0);
        }
        else if (command == "put") {
            send_file(sockfd, args, false);
            int bytes = recv_msg(sockfd, recv_buf, MAXFILEBUFFER, false);
            string reply = "";
            for (int i=0; i<bytes; i++)
                reply += recv_buf[i];
            cout << reply << endl;
        }
        else {
            if (reply == "NO_MESSAGE")
                continue;
            else
                cout << reply << endl;
        }
    }
}