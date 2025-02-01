#include "network.hpp"

const int MAX_CONNECTIONS = 0;
const char *SERVER_ADDRESS = "0.0.0.0";

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

    // There can be multiple entries in servinfo, ensure we are using correct one
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
    if (listen(sockfd, MAX_CONNECTIONS) == -1) {
        perror("(server) listen");
        exit(1);
    }

    return sockfd;
}

std::unordered_map<int, struct sockaddr_storage *> server_map;
std::mutex server_map_m; //map and mutex of all client threads

int thread_num = 0; //the defined 'id' of each thread, used instead of thread::id
std::mutex thread_num_m;

int thread_plus(){
    std::lock_guard<std::mutex> lock(thread_num_m);
    return thread_num++;
}

class Server_Safety{
    //thread safety class for main server-client connections
    std::thread& main_thread;
    public:
        Server_Safety(std::thread& server_t) : main_thread(server_t){
	}

	~Server_Safety(){
	    if (main_thread.joinable()){
		main_thread.join();
		//main_thread.detach();// not completely sure which is better
	    }
	}

};

int get_active_server_threads(){
    std::lock_guard<std::mutex> lock(server_map_m);
    return server_map.size();
}

int activate_server_map(int new_fd, int n_sockfd, struct sockaddr_storage *their_addr, int client_main_id){
    //TODO: for parameters: I'm unsure what is necessary for a connection, so I include more information than necessary
    //while it is unsafe to pass the pointer addr through threads without a condition variable, the definition is only defined in main, and mutex-locked
    std::lock_guard<std::mutex> lock(server_map_m);
    if (server_map.find(new_fd) != server_map.end()){
	return -1; // thread fd already in use
    }
    else{
	server_map[new_fd] = their_addr;
    }
    return 0;
}

int deactivate_server_map(int new_fd){
    std::lock_guard<std::mutex> lock(server_map_m);
    int r = 0;
    if (server_map.find(new_fd) != server_map.end()){
	r = -1;
    }
    server_map.erase(new_fd);
    return r;
}

std::string get(int new_fd, std::string args, char *buffer, std::string complete_file_name = ""){ 
    // default file_in_use = 0
    // default complete_file_name = ""
    bool file_exist = check_file_exists(args);
    if (complete_file_name == ""){
	complete_file_name = args;
    }
    std::string reply;
    if (!file_exist) {
        send_msg(new_fd, "NO_FILE"); // sends to common reply
    }
    else {//success
        send_msg(new_fd);
	std::cout << "sending null message" << std::endl;
        reply = recv_msg(new_fd, buffer, MAXFILEBUFFER);
	std::cout << "received reply: " << reply << std::endl;
	std::cout << "attempting to send file: " << args << std::endl;
        send_file(new_fd, args, buffer, complete_file_name);
	std::cout << "supposedly finished sending file" << std::endl;
    }
    return reply;
}

void get_thread(std::string args, int new_fd, int thread_id, std::string complete_file_name){
    //using original socket to test file
    //passing a char* would take some debugging
    //code to connect - attempt
    //int t_sockfd, status, yes=1;
    //socklen_t sin_size;
    //struct sockaddr_storage their_addr;
    //struct sigaction sa;
    //
    //// handle zombie processes
    //sa.sa_handler = sigchld_handler; // reap all dead processes
    //sigemptyset(&sa.sa_mask);
    //sa.sa_flags = SA_RESTART;
    //if (sigaction(SIGCHLD, &sa, NULL) == -1) {
    //    perror("(server) sigaction");
    //    exit(1);
    //}
    //std::cout << "(daemon" << complete_file_name << ") Started. Waiting for connections..." << std::endl; //TODO modify to be an int
    ////
    //sin_size = sizeof their_addr;
    //t_sockfd = accept(t_sockfd, (struct sockaddr *)&their_addr, &sin_size);
    //if (t_sockfd == -1) {
    //    perror("(server) accept");
    //}
    //send_msg(t_sockfd); // Initial message to accept connection
    //std::cout << "(server) connection established with client" << std::endl;
    //int thread_fd;
    //
    //std::string reply;
    ////the thread_buffer may be all that's necessary for this thread
    //char thread_buffer[MAXFILEBUFFER] = {0};
    //reply = get(thread_fd, args, thread_buffer, complete_file_name);
    //std::cout << "threaded final reply was" << reply << std::endl;

    activate_daemon_map(thread_id, 'W');
    std ::string if_error = activate_file_map(complete_file_name, thread_id, 'R');

    char state = 'W';
    if (if_error == "MAP_IN_USE"){//if not success
        while(state == 'W'){
    	    send_msg(new_fd, "waiting for queue");
	    std::this_thread::sleep_for(std::chrono::seconds(0.5));
    	    state = daemon_map[thread_id];
        }
    }
    if (state == 'A'){
        reply = get(new_fd, args, buffer, complete_file_name);
        deactivate_file_map(complete_file_name);
        std::cout << "threaded final reply was " << reply << std::endl;
    }
    else if (state == 'D'){
        send_msg(new_fd, "file was deleted");
    }
    else if (state == 'T'){
        send_msg(new_fd, "terminating get command of main thread " + thread_id);
    }
    else{
        send_msg(new_fd, if_error);
    }

}
