#include <iostream>
#include <vector>
#include <algorithm>
#include <string>

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
#include <thread> //APL add start
#include <unordered_map>
#include <mutex> 
#include <queue> // APL add end

const int MAXINPUTSIZE = 1024;
const int MAXFILEBUFFER = 4096;
const std::string ENDOFMESSAGE = "ENDOFMESSAGE";
const std::string ENDOFFILENAME = "ENDOFFILENAME";

//APL add start
struct queue_info{
    char state;
    int thread_id;
    std::string complete_file_name;// file is only for delete
    //it should be 
};
//
struct file_info{
    char state;
    int num_reads = 0;
    std::queue<queue_info> file_queue;
};

std::unordered_map<std::string, file_info> file_map; //map for get and put to check if the file transfer has terminated
std::mutex file_map_m; //mutex for file_map

std::unordered_map<int, char> daemon_map; //map for the threads for the server or client, expecting to be for each server-client thread
//notes - std::thread::id is not an int or string, but can be 'printed out'
std::mutex daemon_map_m;


const int MAX_CLIENT_DAEMONS = 2;
const int MAX_CLIENTS = 10;
const int MAX_SERVER_THREADS = MAX_CLIENTS * (MAX_CLIENT_DAEMONS + 1); //likely unnecessary, expecting individual server threads for each client
//APL add end

int remove_file(std::string complete_file_name){//TODO: implement properly
    std::cout << "warning: remove_file currently implemented poorly" << std::endl;
    return remove(complete_file_name.c_str());
}


//APL add start
class Thread_Safety{
    //RAII for thread - used for daemons of existing server-client connections
    //depending on resulting implementation, this may require a daemon_id later
    //as the skeleton stands, this is only recommended for daemon use
    //
    //the only modification to the skeleton for client threads - use main_thread.detach(); instead of main_thread.join();
    //I am unsure whether detach can be used for all client threads, but this should work well
    //
    //
    //note - this will not remove the entry from any map (file_map only currently)
    std::thread& main_thread;
    //std::string daemon_id = ""; 

    public:
        Thread_Safety(std::thread& daemon) : main_thread(daemon){
	//below is for removing file from map on early termination of thread
	//works in tandem with deconstructor
	//
	//std::lock_guard<std::mutex> lock(file_map);//exception safe lock
	//    daemon_id = complete_file_name;
	//	    daemon_thread_num++;//id should be daemon specific for the server-client, could be appended to primary thread id if necessary later
	//    thread_map[daemon_id] = &daemon;
	//    //lock_guard goes out of scope and unlocks
	}
	~Thread_Safety(){
	    if (main_thread.joinable()){
		main_thread.join();
	    }
	//below is for removing file from map on early termination of thread
	//possible extension - remove contents of file as well - cleanup file
	//works in tandem with constructor
	//
	//    std::lock_guard<std::mutex> lock(file_map);
	//    if (file_map.find(daemon_id) != file_map.end()){
	//	file_map.erase(daemon_id);
	//    }
	//    else{
	//	std::cout << "possible errors from num_threads on thread deconstruction" << std::endl; 
	//    }
	}
};

int get_active_threads(){//TODO: changed name - modify to fit daemon_map
    std::lock_guard<std::mutex> lock(file_map_m);
    return file_map.size();
}

int activate_daemon_map(int thread_id, char state){//TODO: test thoroughly
    //the purpose of character states
    //allow proper notification of deletion and termination, and set state to waiting
    int if_error = 0;
    if (not ((state == 'A') or (state == 'T') or 
	(state == 'D') or (state == 'W'))){
	return -2;
    }
    std::lock_guard<std::mutex> lock(daemon_map_m);
    if ((state == 'T') or (state == 'D')){
	if (daemon_map.find(thread_id) == daemon_map.end()){
	    if_error = -2;
	}
	daemon_map[thread_id] = state;
	return if_error;
    }
    else if (state == 'A'){
	if (daemon_map.find(thread_id) != daemon_map.end() or daemon_map[thread_id] != 'W'){
	    return -1; //thread already in use
	}
	else{
	    daemon_map[thread_id] = state;
	}
    }
    else{
	//thread is waiting
	daemon_map[thread_id] = state;
    }
    return 0;
}

int deactivate_daemon_map(int thread_id){
    std::lock_guard<std::mutex> lock(daemon_map_m);
    daemon_map.erase(thread_id);
    return 0;
}

std::string activate_file_map(std::string complete_file_name, int thread_id, char set){//TODO: test activation/deactivation thoroughly
    if (not ((set == 'R') or (set == 'W') or (set == 'D'))){
	return "IMPROPER_MAP_CHARACTER";
    }
    //' ' is state ready for queue, D is set to delete
    //W is for write, R is for read
    //if R, struct file_info should have num_reads > 0
    //to indicate the number of reads 
    std::lock_guard<std::mutex> lock(file_map_m);
    if (file_map.find(complete_file_name) != file_map.end() and file_map[complete_file_name].state != ' '){
	queue_info info;
	if (not read or file_map[complete_file_name].state == 'W'){
	    //thread should be logged into queue
	    info.state = set;
	    info.thread_id = thread_id;
	    info.complete_file_name = complete_file_name;
	    file_map[complete_file_name].file_queue.push(info);
	    return "MAP_IN_USE";
	}
	else if (file_map[complete_file_name].state == 'D'){
	    return "MAP_DELETE_FILE";
	}
	else if (file_map[complete_file_name].state == 'R'){
	    //increment the number of reads
	    file_map[complete_file_name].num_reads++;
	}
	else{
	    return "MAP_UNKNOWN"; //file already in use, unknown state set
		//error should not occur
	}
    }
    else{
	//the first time the file is logged or state is set to ' '
	if (set == 'R'){
	    file_map[complete_file_name].state = set;
	    file_map[complete_file_name].num_reads = 1;
	}
	else{
	    file_map[complete_file_name].state = set;
	    //deletion occurs on deactivation
	}
    }
    return "";
}

int deactivate_file_map(std::string complete_file_name){//TODO: test activation/deactivation thoroughly
    std::lock_guard<std::mutex> lock(file_map_m);
    int if_error = 0;
    file_info f_info = file_map[complete_file_name];
    if (f_info.state == 'R'){
	f_info.num_reads--;
	if (f_info.num_reads < 1){
	    f_info.state = ' ';
	}
    else if (f_info.state == 'W'){
	f_info.state = ' ';
        }
    }
    queue_info q_info;
    if (f_info.state == ' '){
	if (f_info.file_queue.empty()){
	    file_map.erase(complete_file_name);
	    return if_error;
	}
	else{
	    q_info = f_info.file_queue.front();
	    if (q_info.state == 'D'){
		f_info.state = q_info.state;
	    }//final cleanup after testing for 'D'
	}
    }

    if (f_info.state == 'D'){
	//remove all elements in queue from file
	while (!f_info.file_queue.empty() or f_info.file_queue.front().state != 'W'){
	    std::cout << "deactiving file: " << complete_file_name <<", " << f_info.file_queue.front().thread_id  << std::endl;
	    q_info = f_info.file_queue.front();
	    if (q_info.state == 'R'){
	        if_error = activate_daemon_map(q_info.thread_id, 'D');//TODO: check this works
	    }
	    //'D' has no thread, will be deleted
	    f_info.file_queue.pop();
	}
	if (f_info.file_queue.empty()){
	    if_error = remove_file(complete_file_name);
	}
	f_info.state = ' ';
	//next state can only be write
	//next statements will load up or clean up	
    }
    if (f_info.state == ' '){
        //'D' state tested for by here
	//-1 thread_id should not come through
	if (f_info.file_queue.empty()){
	    file_map.erase(complete_file_name);
	}
	else{
	    //there is a possibility another thread captures
	    //control before this, but it will just load back up in queue
	    f_info.num_reads = 0;
	    q_info = f_info.file_queue.front();
	    if_error = activate_daemon_map(q_info.thread_id, 'A');
	    f_info.file_queue.pop();
	}
    }
    return if_error;
}
//APL add end

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

//APL add start
//this may eventually be modified as input to recv_file as a std::string
//in both client and server, modified on server when cd is used
std::string server_directory(char *buf) {
    //returns server directory for use in unordered_map key
    std::string directory = "";
    if (getcwd(buf, MAXFILEBUFFER) != NULL){
	directory = buf;
	memset(buf, '\0', directory.size());
    }
    else{
	perror("failed to obtain server directory");
    }
    return directory;
}
//APL add end

void send_file (int fd, std::string file_name, char *msg, std::string complete_file_name = "", bool server = true) { //TODO: complete_file_name may no longer be necessary
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
    
    //APL add start
    //necessary to check if the file exists in the table before file exists
    //would rather not call file_exist on a file that's being modified
    if (complete_file_name == ""){
	complete_file_name = file_name;
	if (server){
	    std::cout << "warning: using file name without directory at server" << std::endl;
	}
    }
    //std::unique_lock<std::mutex> u_lock(file_map_m);
    //a unique lock would be helpful for locking and unlocking
    //however, all locks seem to be from file_map, so it may be unnecessary
    //APL add end
    bool file_exist = check_file_exists(file_name);
    if (!file_exist) {
        send_msg(fd, "FILE_NOT_EXIST");
        return;
    }

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
    fread(msg, sizeof(char), numbytes, file);
    fclose(file);

    std::string reply = file_name;
    reply += ENDOFFILENAME;
    for (int i=0; i<numbytes; i++)
        reply += msg[i];
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

    fprintf(file, "%s", file_contents.c_str());
    fclose(file);
}


void recv_and_write_file (int fd, char *msg, bool print_msg = false, bool server = true) {
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
    //APL add start
    //to avoid redeclaring a char*, using msg here to get the directory
    std::cout << "entering start " << std::endl;
    std::string directory = server_directory(msg);
    memset(msg, '\0', strlen(msg));
    std::cout << "directory is " << directory << std::endl;
    //APL add end
    std::string message = recv_msg(fd, msg, server);

    if (message == "FILE_NOT_EXIST") {
        if (print_msg)
            std::cout << "File does not exist" << std::endl;
        else
            send_msg(fd, "File does not exist");
        return;
    }//TODO: merge this with NO_FILE in current get
    //APL add start
    else if (message == "FILE_IN_USE") {
        if (print_msg)
            std::cout << "File is in use, please try again later" << std::endl;
        else
            send_msg(fd, "File is in use, please try again later");
        return;
    }//TODO: merge this with FILE_IN_USE in current get
    std::cout << "starting file transfer with message" << message << std::endl;
    //APL add end
    
    std::string file_name = message.substr(0, message.find(ENDOFFILENAME));
    std::string file_contents = message.substr(message.find(ENDOFFILENAME) + ENDOFFILENAME.size());

    //APL add start
    //std::string complete_file_name = file_name;
    //if (server){
    //    complete_file_name = directory + complete_file_name;
    //}//the client can't change directories, the server may have different files in diffferent directories
    //activate_file_map(complete_file_name); - this should already be activated
    //APL add end
 
    write_file(file_name, file_contents, server);

    if (print_msg)
        std::cout << "File transfer complete" << std::endl;
    else{
        send_msg(fd, "File transfer complete");
	std::cout << "sending File transfer complete" << std::endl;
    }
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
