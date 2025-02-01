#include <iostream>
#include <vector>
#include <algorithm>
#include <string>

#include <thread>
#include <mutex>

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

#include <chrono>
std::chrono::seconds dura(1);
const int MAXINPUTSIZE = 1024;//set to 1024
const int MAXFILEBUFFER = 4096;
const std::string ENDOFMESSAGE = "ENDOFMESSAGE";
const std::string ENDOFFILENAME = "ENDOFFILENAME";

char file_check(std::string);


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
    
    //std::cout << "verbose send: " << msg << std::endl; //VERBOSE send

    if (send(fd, msg.c_str(), msg.size(), 0) == -1) {
        if (server)
            perror("(server) send");
        else
            perror("(client) send");
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
            perror("(server) receive");
        else
            perror("(client) receive");
    }

    // Get message till ENDOFFILE
    std::string reply = (std::string)msg;
    //std::cout << "received message: " << reply << std::endl;//VERBOSE receive 
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


bool check_file_exists(std::string file_name) {
    /*
        Opens a file and returns "true" if file can be opened, "false" if the
        file cannot be opened.
    */
    FILE *file;
    if (fopen(file_name.c_str(), "rb") == NULL)
        return false;
    return true;
}

//SEND_FILE MODIFY START2
void send_file (int fd, std::string path, char *msg, bool server = true) {
//parameters modified from also containing std::string file_name to none
//SEND_FILE MODIFY END2
    /*
        Format
            FILE_NOT_EXIST + send_msg
            file_name + ENDOFFILENAME + file_contents + send_msg
        
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
            *msg (char *): message buffer where the error message will be stored
            server (bool): Used to show perror. If True then perror is shown
                           with std::string "(server)" in it and if False perror is
                           shown with std::string "(client)" in it.
    */
    // std::this_thread::sleep_for(dura);
    std::cout << "checking path: " << path << std::endl;
    bool file_exist = check_file_exists(path);//MODIFIED FROM file_name
    if (!file_exist) {
        send_msg(fd, "FILE_NOT_EXIST");
        return;
    }
    
    FILE *file;
    file = fopen(path.c_str(), "rb");//MODIFIED FROM file_name
    if (file == NULL) {
        if (server)
            perror("(server) cannot open file");
        else
            perror("(client) cannot open file");
    }

    fseek(file, 0, SEEK_END);
    int numbytes = ftell(file);
    fseek(file, 0, SEEK_SET);

    //MODIFY START SEND_FILE
    send_msg(fd, std::to_string(numbytes), server);//send the number of bytes of the file
    
    int curr_bytes = std::min(numbytes, MAXINPUTSIZE);
    int bytes_pos = 0;
    std::cout << "bytes pos: " << bytes_pos << " of: " << numbytes << std::endl;

    std::string reply = "";
    char status = 'A';
    while (bytes_pos < numbytes and status == 'A'){
	curr_bytes = std::min(numbytes - bytes_pos, MAXINPUTSIZE);
        fread(msg, sizeof(char), curr_bytes, file);
        reply = "";
        for (int i=0; i<curr_bytes; i++)
            reply += msg[i];
	std::cout << "sending: " << reply << std::endl;

        send_msg(fd, reply, server);
	bytes_pos += curr_bytes;
	memset(msg, '\0', strlen(msg));
	std::cout << "bytes pos: " << bytes_pos << " of: " << numbytes << std::endl;

	if (not server){
	    //all terminate checks are handled by sender in send_file
	    reply = recv_msg(fd, msg, server);
	    status = reply[0];
	    std::cout << "status: " << status << " of reply: " << reply << std::endl;
	}
	else{
	    status = file_check(path);
	    reply = recv_msg(fd, msg, server);
	    std::cout << "likely null reply" << std::endl;
	}
	memset(msg, '\0', strlen(msg));
    }
    std::cout << "finished sending file" << std::endl;
    if (status != 'A'){
	if (status == 'T'){
	    reply = "TERMINATE";
	}
	else{
	    reply = "UNKNOWN_STATUS" + status;
	}
	send_msg(fd, reply, server);
    }
    fclose(file);
    //MODIFY END SEND_FILE

}

//MODIFY START WRITE_FILE
void write_file (std::string file_name, std::string file_contents, bool append = false, bool server = true) {
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
    if (append){
	file = fopen(file_name.c_str(), "a+");
    }
    else{
        file = fopen(file_name.c_str(), "w");
    }
    if (file == NULL) {
        if (server)
            perror("(server) cannot create file to write to");
        else
            perror("(client) cannot create file to write to");
    }

    fprintf(file, "%s", file_contents.c_str());
    fclose(file);
}
//MODIFY END WRITE_FILE
//MODIFY 2 START RECV_WRITE 
int recv_and_write_loop(int fd, std::string file_contents, std::string path, int curbytes, int numbytes, bool append, bool server){
    //
    write_file(path, file_contents, append, server);//MODIFIED FROM file_name
    curbytes += file_contents.length();
    std::cout << "writing: " << file_contents << std::endl;
    std::cout << "at bytes: " << curbytes << " of: " << numbytes << std::endl;
    
    if (server){
        //
        char status = file_check(path);
        send_msg(fd, std::string(1, status));
        //terminate command will be sent back, created to make decision tree easier: send_file always initiates termination
        std::cout << "status to send: " << status << std::endl;
    }
    else{
	send_msg(fd);
    }
    return curbytes;

}

void recv_and_write_file (int fd, std::string path, char *msg, bool print_msg = false, bool server = true) {
//parameters modified from also containing std::string file_name to none
//MODIFY 2 END RECV_WRITE
    /*
        Input
            FILE_NOT_EXIST + send_msg
            file_name + ENDOFFILENAME + file_contents + send_msg

        Parses the output of "send_file" and send the following message
        * "File does not exist": If "send_file" sends "FILE_NOT_EXIST" err message
                then this string is sent to fd
        * "File transfer complete": If the file was sucessfully written on disk, then
                this string is sent to fd
        
        Internally calls "write_file" to write the file contents to disk.

        Args:
            fd (int): file descriptor with info on from where to receive the
                      message
            *msg (char *): message buffer where the error message will be stored
            print_msg (bool): If "true", then prints the message to stdout. If "false"
                              sends the message to "fd"
            server (bool): Used to show perror. If True then perror is shown
                           with std::string "(server)" in it and if False perror is
                           shown with std::string "(client)" in it.
    */
    // std::this_thread::sleep_for(dura);
    //MODIFY RECV START
    std::string file_contents = recv_msg(fd, msg, server);
    int numbytes = -1;
    std::cout << "checking path: " << path << std::endl;

    if (file_contents == "FILE_NOT_EXIST") {
        if (print_msg)
            std::cout << "File does not exist" << std::endl;
        else
            send_msg(fd, "File does not exist");
        return;
    }
    else{
	numbytes = stoi(file_contents);
	memset(msg, '\0', strlen(msg));
    }
    int curbytes = 0;

    //initial write should not append, all further writes should
    bool append = false;
    file_contents = recv_msg(fd, msg, server);
    memset(msg, '\0', strlen(msg));
    //char status = 'A';
    if (file_contents != "TERMINATE" and file_contents.substr(0, file_contents.length() - 1) != "UNKNOWN STATUS"){
	curbytes = recv_and_write_loop(fd, file_contents, path, curbytes, numbytes, append, server);
	
	
	//write_file(path, file_contents, append, server);//MODIFIED FROM file_name

        //curbytes += file_contents.length();
	//status = file_check(path);//checks the file_table
	//if (server){
	//    send_msg(fd, std::string(1, status));
	//    //terminate command will be sent back, created to make decision tree easier: send_file always initiates termination
	//    std::cout << "status to send: " << status << std::endl;

	//}


    }
    append = true;

    while (curbytes < numbytes and file_contents != "TERMINATE" and file_contents.substr(0, file_contents.length() - 1) !="UNKNOWN_STATUS"){
	file_contents = recv_msg(fd, msg, server);
	memset(msg, '\0', strlen(msg));
	curbytes = recv_and_write_loop(fd, file_contents, path, curbytes, numbytes, append, server);

    }
    std::cout << "initially finished transfer" << std::endl;
    if (file_contents == "TERMINATE"){
        remove(path.c_str());//CHECK IF PROPER, MODIFIED FROM file_name
	std::string error_msg = "request for file " + path + " termination complete";//MODIFIED FROM file_name
	if (print_msg){
	    std::cout << error_msg << std::endl;
	}
	else{
	    send_msg(fd, error_msg);
	}
	return;
    }//CHECK IF OTHER ERRORS
    else if (file_contents == "UNKNOWN_STATUS"){
	std::string error_msg = "unknown error occured in recv_file: " + file_contents;
	if (print_msg){
            std::cout << error_msg << std::endl;
	}
	else{
	    std::cout << error_msg << std::endl;
	    send_msg(fd, error_msg);
	}
	return;
    }

    //write_file(file_name, file_contents, server);
    //MODIFY RECV END
    if (print_msg)
        std::cout << "File transfer complete" << std::endl;
    else
        send_msg(fd, "File transfer complete");
}


int establish_connection_to_server (const char *server_ip, const char *port_number) {
    
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
