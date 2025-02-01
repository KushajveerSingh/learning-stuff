#include <iostream>
#include <vector>
#include <algorithm>
// using namespace std;

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

#define MAXDATASIZE 1024 // max size of user_input
#define MAXFILEBUFFER 4096 // max size of reply/message/error buffer
std::string end_of_file = "end_of_file_bits";

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
    std::cout << "mytftp> ";
    return;
}


void send_msg (int fd, std::string msg="", bool server = true) {
    /*
        Send message. If message if of 0-length, the the std::string "NO_MESSAGE" is
        sent. At the receiving end user has to compare against "NO_MESSAGE" to 
        handle the situation when received message is of 0-length.

        Also, the std::string is appended with "EOF" before sending it. At the receiving
        end, the user is required to do a linear scan till EOF to get the complete
        message.

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
    msg += EOF; 

    if (send(fd, msg.c_str(), msg.size(), 0) == -1) {
        if (server)
            perror("(server) send");
        else
            perror("(client) send");
    }
    return;
}


std::string recv_msg (int fd, char *msg, int msg_size, bool server = true) {
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
                           with std::string "(server)" in it and if False perror is
                           shown with std::string "(client)" in it.
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
    std::string reply = "";
    for (int i=0; i<msg_size; i++) {
        if (msg[i] == EOF)
            return reply;
        else
            reply += msg[i];
    }

    return "";
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


bool check_file_exist(std::string file_name) {
    /*
        Opens a file and returns "true" if file can be opened, "false" if the
        file cannot be opened.
    */
    FILE *file;
    if (fopen(file_name.c_str(), "rb") == NULL)
        return false;
    return true;
}


void send_file (int fd, std::string file_name, bool server = true) {
    /*
        Sends a file. It the file does not exist then it sends an error message
        "FILE_NOT_EXIST" to "fd". This function automatically allocates memory
        depending on the size of the file, so there is no limit on the size of
        file you can send (ofcourse, the size is limited by your RAM size).

        Understanding the string message being sent
        * "FILE_NOT_EXIST" = if file does not exist
        * file_name + EOF + file_contents = EOF characters is used to separate
                file_name from file_content

        Args:
            fd (int): file descriptor with info on from where to receive the
                      message
            file_name (string): name of the file to send
            server (bool): Used to show perror. If True then perror is shown
                           with std::string "(server)" in it and if False perror is
                           shown with std::string "(client)" in it.
    */
    bool file_exist = check_file_exist(file_name);
    if (!file_exist) {
        send_msg(fd, "FILE_NOT_EXIST");
        return;
    }
    
    // char buf[MAXFILEBUFFER] = {0};
    FILE *file;
    file = fopen(file_name.c_str(), "rb");
    if (file == NULL) {
        if (server)
            perror("(server) cannot open file");
        else
            perror("(client) cannot open file");
    }

    fseek(file, 0, SEEK_END);
    int numbytes = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *buffer;
    buffer = (char*)calloc(numbytes, sizeof(char));
    
    fread(buffer, sizeof(char), numbytes, file);
    fclose(file);

    std::string correct_buffer = "";
    for (int i=0; i<numbytes; i++)
        correct_buffer += buffer[i];

    std::string reply = file_name;
    reply += EOF;
    reply += correct_buffer;
    reply += end_of_file;
    free(buffer);
    send_msg(fd, reply, server);
}

void write_file (std::string file_name, std::string file_contents, bool server = true) {
    /*
        Write "file_contents" to "file_name" on disk.

        Args:
            file_name (string): name of the file to write to on disk
            file_contents (string): contents of file
            server (bool): Used to show perror. If True then perror is shown
                           with std::string "(server)" in it and if False perror is
                           shown with std::string "(client)" in it.
    */
    FILE *file;
    file = fopen(file_name.c_str(), "w");
    if (file == NULL) {
        if (server)
            perror("(server) cannot create file to write to");
        else
            perror("(client) cannot create file to write to");
    }

    std::string message = "";
    int eof_index = file_contents.find(end_of_file);
    for (int i=0; i<eof_index; i++)
        message += file_contents[i];

    fprintf(file, "%s", message.c_str());
    fclose(file);
}

void recv_and_write_file (int fd, bool print_msg = false, bool server = true) {
    /*
        Parses the output of "send_file" and send the following message
        * "File does not exist": If "send_file" sends "FILE_NOT_EXIST" err message
                then this string is sent to fd
        * "File transfer complete": If the file was sucessfully written on disk, then
                this string is sent to fd
        
        Internally calls "write_file" to write the file contents to disk.

        Args:
            fd (int): file descriptor with info on from where to receive the
                      message
            print_msg (bool): If "true", then prints the message to stdout. If "false"
                              sends the message to "fd"
            server (bool): Used to show perror. If True then perror is shown
                           with std::string "(server)" in it and if False perror is
                           shown with std::string "(client)" in it.
    */
    char buf[MAXFILEBUFFER] = {0};
    int numbytes;
    numbytes = recv(fd, buf, MAXFILEBUFFER, 0);
    
    if (numbytes == -1) {
        if (server)
            perror("(server) receive");
        else
            perror("(client) receive");
    }

    // There are two cases
    // Case 1: 1 EOF in string (in which case we get "FILE_NOT_EXIST" err message)
    // Case 2: 2 EOF in string (in which the first EOF is used to seperate file_name
    //         from file_content)
    
    std::string message="";
    // Handle Case 1
    for (int i=0; i<numbytes; i++) {
        if (buf[i] == EOF)
            break;
        message += buf[i];
    }

    if (message == "FILE_NOT_EXIST") {
        if (print_msg)
            std::cout << "File does not exist" << std::endl;
        else
            send_msg(fd, "File does not exist");
        return;
    }

    // Handle Case 2 (below 2 for loops can be combined into single for loop)
    // Get the string till last EOF
    std::string file_name="", file_contents="";
    bool skip_first_eof = true;
    for (int i=0; i<MAXFILEBUFFER; i++) {
        if (buf[i] == EOF) {
            if (skip_first_eof)
                skip_first_eof = false;
            else {
                numbytes = i;
                break;
            }
        }
    }
    // Find the position of first EOF and split data to get file_name, file_content
    bool seen_eof = false;
    for (int i=0; i<numbytes; i++) {
        if (seen_eof == true)
            file_contents += buf[i];
        else {
            if (buf[i] == EOF)
                seen_eof = true;
            else
                file_name += buf[i];
        }
    }

    write_file(file_name, file_contents, server);
    if (print_msg)
        std::cout << "File transfer complete" << std::endl;
    else
        send_msg(fd, "File transfer complete");
}