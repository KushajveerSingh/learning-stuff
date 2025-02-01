#include <iostream>
#include <map>
using namespace std;

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

#define MAXDATASIZE 1024 // max size of message in bytes
#define MAXFILEBUFFER 2048
const string delim = " ";
const int MAX = 20;

// Map from valid user commands to integer
map <string, int> COMMAND_ID = {
    {"get",    1},
    {"put",    2},
    {"delete", 3},
    {"ls",     4},
    {"cd",     5},
    {"mkdir",  6},
    {"pwd",    7},
    {"quit",   8},
};

void *get_ip_addr (struct addrinfo *sa) {
    /*
        Usage:
            void *addr = get_ip_addr(p);
            inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
    */
    struct sockaddr_in *ip_addr_info = (struct sockaddr_in *)sa->ai_addr;
    return &(ip_addr_info->sin_addr);
}

void show_prompt (bool add_newline_before = false) {
    if (add_newline_before)
        cout << "\n";
    cout << "mytftp> ";
    return;
}

void send_msg (int fd, string reply, bool server = true) {
    if (send(fd, reply.c_str(), reply.size(), 0) == -1) {
        if (server)
            perror("(server) send");
        else
            perror("(client) send");
    }
}

int recv_msg (int fd, char *buf, bool server = true, bool file = false) {
    int numbytes;
    if (file)
        numbytes = recv(fd, buf, MAXFILEBUFFER, 0);
    else
        numbytes = recv(fd, buf, MAXDATASIZE, 0);

    if (numbytes == -1) {
        if (server)
            perror("(server) receive");
        else
            perror("(client) receive");
    }
    return numbytes;
}

int get_command_id (string user_input) {
    return COMMAND_ID[user_input.substr(0, user_input.find(' '))];
}

void send_system_msg (int fd, char *buf, char *file_buf, bool server = true) {
    string str_return = "";
    FILE *file = popen(buf, "r");
    if (file == NULL){
        throw runtime_error("error opening file with popen");
    }

    while (fgets(file_buf, MAXFILEBUFFER, file) != NULL){
        str_return += file_buf;
    }

    if (pclose(file) < 0){
        cout<< "warning: pclose() did not return 0" << endl;
    }    
    
    send_msg(fd, str_return, server);
}

string command_arg(string &command){
    // modifies command to be the first argument before delim
    // and returning the rest of the string if available
    int split = command.find(delim);
    string arg;
    if (split > command.length() - 1){
	arg = "";
    }
    else{
    	arg = command.substr(split + 1, command.length());
    }
    command = command.substr(0, split);
    return arg;
}


int send_file(string file_str, int out_fd, bool server){
    // (TODO) handle and understand errors
    //currently this does cut the document from one side and paste it at the other
    //the for loop is untested and hopefully works
    cout << "initial " << endl;
    string s = "put.txt";
    char buf[MAXDATASIZE] = {0};
    
    // get the file size
    ofstream ofs;
    struct stat file_stat;
    stat(file_str.c_str(), &file_stat);
    int fsize = file_stat.st_size; //this could eventually convert to a header

    cout << "file size is "<< fsize << endl;
    int in_fd = open(file_str.c_str(), O_RDWR);
    int offset = 0;
    
    // send the file size
    if (send(out_fd, &fsize, sizeof fsize, 0) == -1){
	cout << "error sending file size" << endl;
	return -1;
    }
    
    
    for (int i = 0; i*MAXDATASIZE < fsize; ++i){
    	//this may require ntohl or equivalent later
	offset = i*MAXDATASIZE;
	int pack_size = min(fsize - offset, MAXDATASIZE);
	sendfile(out_fd, in_fd, 0, pack_size);
    }    
    
    string reply = "file " + file_str + " sent";
    cout << reply << endl;
    return 0;

}

int recv_file(string file_str, int in_fd, bool server){
    // (TODO) handle and understand errors
    
    //to try: htonl formats to change files for expected zip
    //look into tcp headers/cause of seg faults
    //
    ofstream create_file; //create the file if needed before using open()
    create_file.open(file_str);
    create_file.close();
    
    int *file_size_ptr;
    int file_size;
    recv(in_fd, file_size_ptr, sizeof file_size, 0); 
    
    file_size = *file_size_ptr;
    //cout<<"size of file is: "<<file_size<<endl;

    int file_fd = open(file_str.c_str(), O_RDWR | O_CREAT); //without creating the file, there is nothing written
    //best guess is issues with write protection, but this method works, even with O_CREAT flag nothing is written into the file
    
    
    char buf[MAXDATASIZE] = {0};
    
    int offset = 0;
    for (int i=0; i*MAXDATASIZE < file_size; ++i){
        
	int cur_size = min(file_size - offset, MAXDATASIZE);
	read(in_fd, buf, cur_size);
        write(file_fd, buf, cur_size);
	//cout<<buf<<endl;
    }
    close(file_fd);
    //string reply = "file " + file_str + " sent";
    //cout << reply << endl;
    return 0;

}

int send_file_stream(string file_str, int out_fd, bool server){
    // (TODO) handle and understand errors
    //currently this does cut the document from one side and paste it at the other
    //the for loop is untested and hopefully works
    cout << "initial " << endl;
    string s = "put.txt";
    //char buf[MAXDATASIZE] = {0};
    
    // get the file size
    //ofstream ofs;
    //struct stat file_stat;
    //stat(file_str.c_str(), &file_stat);
    //int fsize = file_stat.st_size; //this could eventually convert to a header

    //cout << "file size is "<< fsize << endl;
    //int in_fd = open(file_str.c_str(), O_RDWR);
    
    // send the file size
    //if (send(out_fd, &fsize, sizeof fsize, 0) == -1){
//	cout << "error sending file size" << endl;
//	return -1;
//    }
    

    FILE* file = fopen(file_str.c_str(), "r+");
    struct stat file_stat;
    int file_fd = fileno(file);
    fstat(file_fd, &file_stat);
    int file_size = file_stat.st_size;

    cout << "file size is " << file_size << endl;
    if (send(out_fd, &file_size, sizeof file_size, 0) == -1){
	    cout << "error sending file size" << endl;
	    return -1;
    }
    
    char buf[MAXDATASIZE] = {0};
    int offset = 0;
    int size_sent = 0;
    int pack_size = min(file_size - offset, MAXDATASIZE);
    int count = 0;
    int temp_sent;

    //for (int i = 0; i*MAXDATASIZE < fsize or size_sent < pack_size; ++i){
    while (size_sent < file_size or count > MAX){
        //this may require ntohl or equivalent later
	
	
	pack_size = min(file_size - size_sent, MAXDATASIZE);
	temp_sent = sendfile(out_fd, file_fd, 0, pack_size);
	if (temp_sent == -1){
	    cout << "-1 returned on file sent" << endl;
	    return -1;
	}
	else{
	    if (send(out_fd, &temp_sent, sizeof temp_sent, 0) == -1){
	        cout << "error sending temp size" << endl;
	        return -1;
            }
	    size_sent += temp_sent;
	}
	++count;
    }    
    
    string reply = "file " + file_str + " sent";
    cout << reply << endl;
    return 0;

}

int recv_file_stream(string file_str, int in_fd, bool server){
    // (TODO) handle and understand errors
    
    //to try: htonl formats to change files for expected zip
    //look into tcp headers/cause of seg faults
    //
    ofstream create_file; //create the file if needed before using open()
    create_file.open(file_str);
    create_file.close();
    
    int *file_size_ptr = new int;
    int file_size;
    cout << "getting file size"<< endl;
    recv(in_fd, file_size_ptr, sizeof file_size, 0); 
    
    file_size = *file_size_ptr;
    cout<<"size of file is: "<<file_size<<endl;

    int file_fd = open(file_str.c_str(), O_RDWR | O_CREAT); //without creating the file, there is nothing written
    //best guess is issues with write protection, but this method works, even with O_CREAT flag nothing is written into the file
    
    //FILE* file = fopen(file_str.c_str(), "r+");
    //struct stat file_stat;
    //int file_fd = fileno(file);
    //fstat(file_fd, &file_stat);
    //int fsize = file_stat.st_size;

    char buf[MAXDATASIZE] = {0};
    
    int offset = 0;
    int *buf_size_ptr = new int;
    int buf_size;
    //for (int i=0; i*MAXDATASIZE < file_size; ++i){
    int cur_size = 0;
    int count = 0;

    while (cur_size < file_size or count > MAX){    
	int accept_size = min(file_size - offset, MAXDATASIZE);
	read(in_fd, buf, accept_size);
        write(file_fd, buf, accept_size);

        recv(in_fd, buf_size_ptr, sizeof buf_size, 0); 
        buf_size = *buf_size_ptr;
        cur_size += buf_size;
	//cout<<buf<<endl;
	++count;
    }
    close(file_fd);


    //string reply = "file " + file_str + " sent";
    //cout << reply << endl;
    delete file_size_ptr;
    delete buf_size_ptr;
    return 0;

}

int recv_file_stream_old(string file_str, int in_fd, bool server){
    // (TODO) handle and understand errors
    
    //to try: htonl formats to change files for expected zip
    //look into tcp headers/cause of seg faults
    //
    ofstream create_file; //create the file if needed before using open()
    create_file.open(file_str);
    create_file.close();
    
    int *file_size_ptr;
    int file_size;
    recv(in_fd, file_size_ptr, sizeof file_size, 0); 
    
    file_size = *file_size_ptr;
    cout<<"size of file is: "<<file_size<<endl;

    //int file_fd = open(file_str.c_str(), O_RDWR | O_CREAT); //without creating the file, there is nothing written
    //best guess is issues with write protection, but this method works, even with O_CREAT flag nothing is written into the file
    
    FILE* file = fopen(file_str.c_str(), "r+");
    struct stat file_stat;
    int file_fd = fileno(file);
    fstat(file_fd, &file_stat);
    //int fsize = file_stat.st_size;

    char buf[MAXDATASIZE] = {0};
    
    int offset = 0;
    int size_sent = 0;
    int count = 0;
    int cur_size = 0;
    int size_change;

    //for (int i=0; i*MAXDATASIZE < file_size; ++i){
    while(cur_size < file_size or count < MAX){    
	int accept_size = min(file_size - cur_size, MAXDATASIZE);
	read(in_fd, buf, accept_size);
        write(file_fd, buf, accept_size);
	size_change = cur_size;
	cur_size = file_stat.st_size; // get current size of the file
	size_change = cur_size - size_change;
	cout << "bytes written: " << size_change << endl;
	//the current size of the file may well not work
	++count;
	//cout<<buf<<endl;
    }
    fclose(file);
    //string reply = "file " + file_str + " sent";
    //cout << reply << endl;
    return 0;

}
