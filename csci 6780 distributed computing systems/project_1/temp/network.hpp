#include <iostream>
#include <vector>
#include <algorithm>
using namespace std;

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

#define MAXDATASIZE 1024 // max size of user_input
#define MAXFILEBUFFER 4096 // max size of reply/message/error buffer

/*
List of useful helper functions
    * send_msg -> send message
    * recv_msg -> receive message
    * send_err_msg -> send strerror message
*/


void *get_ip_addr (struct addrinfo *sa) {
    /*
        Usage:
            void *addr = get_ip_addr(p);
            inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
    */
    struct sockaddr_in *ip_addr_info = (struct sockaddr_in *)sa->ai_addr;
    return &(ip_addr_info->sin_addr);
}


void show_prompt () {
    /*
        Helper function to show "myftp>" prompt as said in the
        project instructions
    */
    cout << "mytftp> ";
    return;
}


void send_msg (int fd, string msg="", bool server = true) {
    /*
        Send message. If message if of 0-length, the the string "NO_MESSAGE" is
        sent. At the receiving end user has to compare against "NO_MESSAGE" to 
        handle the situation when received message is of 0-length.

        Also, the string is appended with "EOF" before sending it. At the receiving
        end, the user is required to do a linear scan till EOF to get the complete
        message.

        Args:
            fd (int): file descriptor which holds the info where the message
                      is sent
            msg (string): the message to be sent
            server (bool): Used to show perror. If True then perror is shown
                           with string "(server)" in it and if False perror is
                           shown with string "(client)" in it.
    */
    if (msg.size() == 0)
        msg = "NO_MESSAGE";

    // Attach EOF for linear scan
    msg += EOF; 

    if (send(fd, msg.c_str(), msg.size(), 0) == -1) {
        if (server)
            perror("(server) send");
        else
            perror("(client) send");
    }
    return;
}


int recv_msg (int fd, char *msg, int msg_size, bool server = true) {
    /*
        Receive message. It first does a linear scan till EOF to get the
        message. The message is returned in the *msg buffer given as argument
        to the function.

        Args:
            fd (int): file descriptor with info on from where to receive the
                      message
            *msg (char *): message buffer where the message will be stored
            msg_size (int): The size of the msg buffer.
            server (bool): Used to show perror. If True then perror is shown
                           with string "(server)" in it and if False perror is
                           shown with string "(client)" in it.
    */
    int numbytes;
    numbytes = recv(fd, msg, msg_size, 0);
    
    if (numbytes == -1) {
        if (server)
            perror("(server) receive");
        else
            perror("(client) receive");
    }

    // Get message till EOF
    for (int i=0; i<msg_size; i++) {
        if (msg[i] == EOF) {
            numbytes = i;
            break;
        }
    }

    return numbytes;
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
                           with string "(server)" in it and if False perror is
                           shown with string "(client)" in it.
    */
    // snprintf(msg, msg_size, "%s", strerror(errno));
    string err = strerror(errno);
    send_msg(fd, err, server);
}


void send_file (int fd, string file_name, bool server = true) {
    char buf[MAXFILEBUFFER] = {0};
    FILE *file;
    file = fopen(file_name.c_str(), "r");
    if (file == NULL) {
        if (server)
            perror("(server) cannot open file");
        else
            perror("(client) cannot open file");
    }
    
    while(fgets(buf, MAXFILEBUFFER, file) != NULL) {}
    fclose(file);

    string reply = file_name;
    reply += EOF;
    reply += (string)buf;
    reply = reply.substr(0, reply.size()-1); // \n is added by fgets

    send_msg(fd, reply, server);
}

int recv_file (int fd, char *msg, int msg_size, bool server = true) {
    int numbytes;
    numbytes = recv(fd, msg, msg_size, 0);
    
    if (numbytes == -1) {
        if (server)
            perror("(server) receive");
        else
            perror("(client) receive");
    }

    // Get message till EOF
    bool skip_first_eof = true;
    for (int i=0; i<msg_size; i++) {
        if (msg[i] == EOF) {
            if (skip_first_eof)
                skip_first_eof = false;
            else {
                numbytes = i;
                break;
            }
        }
    }

    return numbytes;
}

void write_file (string msg, bool server = true) {
    FILE *file;
    string file_name, file_contents;
    bool seen_eof = false;

    for (int i=0; i<msg.size(); i++) {
        if (seen_eof == true)
            file_contents += msg[i];
        else {
            if (msg[i] == EOF)
                seen_eof = true;
            else
                file_name += msg[i];
        }
    }

    file = fopen(file_name.c_str(), "w");
    if (file == NULL) {
        if (server)
            perror("(server) cannot create file to write to");
        else
            perror("(client) cannot create file to write to");
    }

    fprintf(file, "%s", file_contents.c_str());
    fclose(file);
}