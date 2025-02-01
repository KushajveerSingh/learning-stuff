#include <iostream>
#include <vector>
#include <queue>
#include <algorithm>
#include <string>
#include <map>
#include <fstream>
#include <ctime>
#include <set>

#include <thread>
#include <mutex>
#include <chrono>

#include <math.h>
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>

const int MAXINPUTSIZE = 1024;
const int MAXFILEBUFFER = 4096;
const std::string ENDOFMESSAGE = "ENDOFMESSAGE";
const char *SERVER_ADDRESS = "0.0.0.0";
std::chrono::milliseconds sleep_dura(500);
auto chrono_type = std::chrono::system_clock::now();

void *get_ip_addr (struct addrinfo *sa) {
    /*
        Usage:
            void *addr = get_ip_addr(p);
            inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
    */
    struct sockaddr_in *ip_addr_info = (struct sockaddr_in *)sa->ai_addr;
    return &(ip_addr_info->sin_addr);
}


void send_msg (int fd, std::string msg="", bool server = true) {
    /*
        Format
            NO_MESSAGE + ENDOFMESSAGE
            msg + ENDOFMESSAGE

        Send message. If message if of 0-length, the the std::string "NO_MESSAGE" is
        sent. At the receiving end user has to compare against "NO_MESSAGE" to 
        handle the situation when received message is of 0-length.

        Also, the `msg` is appended with "ENDOFMESSAGE" before sending it. At the 
        receiving end, the user is required to do a linear scan till "ENDOFMESSAGE" 
        to get the correct message.

        Args:
            fd (int): file descriptor which holds the info where the message
                      is sent
            msg (std::string): the message to be sent
            server (bool): Used to show perror. If True then perror is shown
                           with std::string "(server)" in it and if False perror is
                           shown with std::string "(client)" in it.
    */
    if (msg.size() == 0)
        msg = "NO_MESSAGE";

    // Attach EOF for linear scan
    msg += ENDOFMESSAGE; 

    if (send(fd, msg.c_str(), msg.size(), 0) == -1) {
        if (server)
            perror("(coordinator) send");
        else
            perror("(participant) send");
    }
    return;
}


std::string recv_msg (int fd, char *msg, bool server = true) {
    /*
        Output
            msg - ENDOFMESSAGE

        Receive message. It find the message before "ENDOFMESSAGE" and return the
        message as a string.

        Args:
            fd (int): file descriptor with info on from where to receive the
                      message
            *msg (char *): message buffer where the message will be stored
            server (bool): Used to show perror. If True then perror is shown
                           with std::string "(server)" in it and if False perror is
                           shown with std::string "(client)" in it.
    */
    if (recv(fd, msg, MAXFILEBUFFER, 0) == -1) {
        if (server)
            perror("(coordinator) receive");
        else
            perror("(participant) receive");
    }

    // Get message till ENDOFFILE
    std::string reply = (std::string)msg;
    return reply.substr(0, reply.find(ENDOFMESSAGE));
}


void send_err_msg (int fd, bool server = true) {
    /*
        Send the error message to "fd". The error is retreived using
        "strerror(errno)".

        Args:
            fd (int): file descriptor with info on from where to receive the
                      message
            *msg (char *): message buffer where the error message will be stored
            msg_size (int): The size of the msg buffer.
            server (bool): Used to show perror. If True then perror is shown
                           with std::string "(server)" in it and if False perror is
                           shown with std::string "(client)" in it.
    */
    // snprintf(msg, msg_size, "%s", strerror(errno));
    std::string err = strerror(errno);
    send_msg(fd, err, server);
}


int establish_connection_to_coordinator (const char *server_ip, const char *port_number) {
    
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
        ipstr    : temporary std::string to hold ip if needed
    */
    int status, sockfd;
    struct addrinfo hints, *servinfo, *res, *p;
    char ipstr[INET_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    status = getaddrinfo(server_ip, port_number, &hints, &servinfo);

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
        
        if (!strcmp(ipstr, server_ip))
            res = p;
        else
            ip_miss += 1;
    }

    if (ip_miss == len_list) {
        std::cout << "(client) IP address not found in servinfo" << std::endl;
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

    freeaddrinfo(servinfo);

    return sockfd;
}


void sigchld_handler (int s) {
    // Handles the zombie processes
    int saved_errno = errno;
    while(waitpid(-1, NULL, WNOHANG) > 0);
    errno = saved_errno;
}


void *get_ip_addr(struct sockaddr *sa) {
    // Overload the function from "network.hpp"
    struct sockaddr_in *ip_addr_info = (struct sockaddr_in *)sa;
    return &(ip_addr_info->sin_addr);
}


int open_port_for_connections (const char *port_number) {

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
    int sockfd, status;
    struct addrinfo hints, *servinfo, *res, *p;
    char ipstr[INET_ADDRSTRLEN];
    
    memset(&hints, 0, sizeof hints);    
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    status = getaddrinfo(SERVER_ADDRESS, port_number, &hints, &servinfo);

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
    if (listen(sockfd, 10) == -1) {
        perror("(server) listen");
        exit(1);
    }

    return sockfd;
}