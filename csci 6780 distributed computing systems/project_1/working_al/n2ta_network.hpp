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
#include <sys/sendfile.h>
#include <fcntl.h>
#include <fstream>

#define MAXDATASIZE 1024 // max size of user_input
#define MAXFILEBUFFER 4096 // max size of reply/message/error buffer
const int MAX = 20;

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

void send_err_msg (int fd, char *msg, int msg_size, bool server = true) {
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
    snprintf(msg, msg_size, "%s", strerror(errno));
    send_msg(fd, (string)msg, server);
}

int send_file_stream(string file_str, int out_fd, bool server){
    // (TODO) handle and understand errors
    //currently this does cut the document from one side and paste it at the other
    //the for loop is untested and hopefully works
    cout << "sending:" << file_str << endl;
    
    // opens file stream for reading
    FILE* file = fopen(file_str.c_str(), "r");
    
    if (file == NULL){
	int err_size = -1;
	if (send(out_fd, &err_size, sizeof err_size, 0) == -1){
	    cout << "error sending error int" << endl;
	}
	return err_size;
    }

    // acquire file size
    struct stat file_stat;
    int file_fd = fileno(file);
    fstat(file_fd, &file_stat);
    int file_size = file_stat.st_size;

    // send file size to receiving end
    cout << "file size: " << file_size << endl;
    if (send(out_fd, &file_size, sizeof file_size, 0) == -1){
	    cout << "error sending file size" << endl;
	    return -4;
    }
    
    char buf[MAXDATASIZE] = {0};
    int offset = 0;
    int size_sent = 0;
    int pack_size = min(file_size - offset, MAXDATASIZE);
    int count = 0; //while this should be really be a timeout, this is ok for now
    int temp_sent;

    // send file parts until file size has been sent
    while (size_sent < file_size or count > MAX){
        //this may require ntohl or equivalent later
	
	pack_size = min(file_size - size_sent, MAXDATASIZE);
	temp_sent = sendfile(out_fd, file_fd, 0, pack_size);
	if (temp_sent == -1){
	    cout << "-1 returned on file sent" << endl;
	    return -4;
	}
	else{
	    if (send(out_fd, &temp_sent, sizeof temp_sent, 0) == -1){
	        cout << "error sending temp size" << endl;
	        return -4;
            }
	    size_sent += temp_sent;
	}
	++count;
    }    
    
    fclose(file);
    string reply = "file " + file_str + " sent";
    cout << reply << endl;
    return 0;

}

int recv_file_stream(string file_str, int in_fd, bool server){
    // (TODO) handle and understand errors
    
    //to try: htonl formats to change files for expected zip
    
    //acquire the file size to be sent
    int *file_size_ptr = new int();
    int file_size;
    recv(in_fd, file_size_ptr, sizeof file_size, 0); 
    
    file_size = *file_size_ptr;
    if (file_size < 0){
        return -1;
    }

    cout << "receiving file: " << file_str << endl;
    cout << "size of file is: " << file_size << endl;
    ofstream create_file; //create the file if needed before using open()
    // as noted below this is necessary
    create_file.open(file_str);
    create_file.close();

    int file_fd = open(file_str.c_str(), O_RDWR | O_CREAT); //without creating the file, there is nothing written
    //best guess is issues with write protection, but this method works, even with O_CREAT flag nothing is written into the file
    
    

    char buf[MAXDATASIZE] = {0};
    
    int offset = 0;
    int *buf_size_ptr = new int;
    int buf_size;
    int cur_size = 0;
    int count = 0; // as noted in send, this should ideally be a timeout
    
    cout << "entering stream" << endl;

    while (cur_size < file_size or count > MAX){
	// find and attempt to read the expected file size
	int accept_size = min(file_size - offset, MAXDATASIZE);
	read(in_fd, buf, accept_size);
        write(file_fd, buf, accept_size);
	
	// receive the actual size sent (I was unable to find a way to do this on the receiving side automatically)
        recv(in_fd, buf_size_ptr, sizeof buf_size, 0); 
        buf_size = *buf_size_ptr;
        cur_size += buf_size;
	++count;
	cout << "received " << cur_size << endl;
    }
    close(file_fd);


    cout << "file: " << file_str << " received" << endl;
    delete file_size_ptr;
    delete buf_size_ptr;
    return 0;

}

// void send_file (int fd, char *filename) {
//         FILE *fp = fopen(filename, "r");
//         if (fp == NULL) {
//             perror("Error in reading file");
//             exit(1);
//         }
//         char buf[MAXFILEBUFFER] = {0};
//         while(fgets(buf, MAXFILEBUFFER, fp) != NULL) {
//             string data = (string)buf;
//             data += EOF;
//             send_msg(fd, data);
//             bzero(buf, MAXFILEBUFFER);
//         }
//     }
// }

// void write_file (int fd) {
//     FILE *fp;
//     char *file
// }

//        // get
//	else if (command == "get") {
//	    if (args.size() == 0) {
//                string err = "did not specify dir name";
//                send_msg(sockfd, err);
//            }
//	    else{
//		if (remove(args.c_str()) == -1) {
//		    string err = strerror(errno);
//		    send_msg(sockfd, err);
//		}
//		else{
//		    //perform command
//		    recv_file_stream(args[1], sockfd, 0);
//		    //cout << "unimplemented put" << endl;
//		}
//	    }
//	}
//
//	// put
//	else if (command == "put") {
//	    if (remove(args.c_str()) == -1) {
//	        string err  = strerror(errno);
//		send_msg(new_fd, err);
//	    }
//	    else {
//		//perform command
//		send_file_stream(args[1], sockfd, 0);
//		//cout << "unimplemented put" << endl;
//	    }
//	
//	}




