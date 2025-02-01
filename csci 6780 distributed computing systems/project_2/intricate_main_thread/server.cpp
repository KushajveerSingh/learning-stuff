#include "server.hpp"
#include <chrono>
int server_thread(int, int);

//void get_thread(std::string, std::string, int); //possibly better to move to server.hpp
//std::string get(int, std::string, char * , int = 0, std::string = "");
//TODO: implement connect on t port, terminate, id link on client's id into map

int main (int argc, char** argv) {
     /*
        server.cpp takes two required arguments as input
            - nport_number
            - tport_number
        
        Note:
            Check network.hpp for the maximum allowed buffer sizes. The defaults
            are shown below
            * MAXDATASIZE 1024 -> The maximum size of user input in bytes
            * MAXFILEBUFFER 4096 -> The maximum size of messages (including error
                                    messages) that will be shared by client and 
                                    server
    */

    // Parse command line arguments
    const char *NPORT_NUMBER = argv[1];
    const char *TPORT_NUMBER = argv[2];

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
    
    // Need to create two threads for these
    int n_sockfd, t_sockfd;
    n_sockfd = open_port_for_connections(NPORT_NUMBER);
    t_sockfd = open_port_for_connections(TPORT_NUMBER);
    
    int fd_arr [MAX_CLIENTS];
    int new_fd, status, yes=1;
    socklen_t sin_size;
    struct sockaddr_storage their_addr;
    struct sigaction sa;
    
    // handle zombie processes
    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("(server) sigaction");
        exit(1);
    }

    std::cout << "(server) Started. Waiting for connections..." << std::endl;

    while (1) { // keep listening for connections
	
	if (get_active_server_threads() < MAX_CLIENTS){
	    //block accepting new clients past the max client size
            sin_size = sizeof their_addr;
            new_fd = accept(n_sockfd, (struct sockaddr *)&their_addr, &sin_size);
            if (new_fd == -1) {
                perror("(server) accept");
                continue;
            }
 	    int client_main_id = thread_plus();
            send_msg(new_fd, to_string(client_main_id)); // Initial message to accept connection
            std::cout << "(server) connection established with client at " << new_fd << std::endl;
	    activate_server_map(new_fd, n_sockfd, &their_addr, client_main_id);// deactivation at end of thread loop
            std::thread client_connection(server_thread, new_fd, t_sockfd, client_main_id);
	    Server_Safety safe_server_thread(client_connection);
	}
	else{
	    //no new connections allowed, recheck every 1 second
	    std::this_thread::sleep_for(std::chrono::seconds(1));
	}
    }
    //MAIN THREAD QUESTION: wrap up of any connections (new_fd) necessary after main thread is closed?

    // Stop server
    if (close(n_sockfd) == -1)
        perror("(server) close socket");
    if (close(t_sockfd) == -1)
	perror("(server) close t_socket");
    std::cout << "(server) Server stopped" << std::endl;
    exit(0);
}


int server_thread(int new_fd, int t_sockfd, int main_thread_id){//TODO: decomment through while loop

    // Create buffers for client
    char user_input[MAXINPUTSIZE] = {0};
    char buffer[MAXFILEBUFFER] = {0};
    std::string reply;
    bool t_sock_open = false;//TODO: may be unnecessary, but idea is to help close t_sock when/if it is open
    int daemon_id_get = -1;
    int daemon_id_put = -1;
    //activate_daemon_map(thread_id, '\0');
    
    while (1) {
        std::string command="", args="";
        bool add_daemon = false, seen_space = false, terminate_cmd = false;
        
        // Receive input from client
        reply = recv_msg(new_fd, user_input, MAXFILEBUFFER);//TODO: likely MAXINPUTSIZE
        
        // Check for terminate command
        // above is wrong, check for additional thread
        if (reply[reply.size()-2] == ' ' and reply[reply.size()-1] == '&') {
    	    add_daemon = true;
    	    reply = reply.substr(0, reply.size()-2);
        }
    
        // Parse input to get "command" and "args"
        for (int i=0; i<reply.size(); i++) {
    	    if (seen_space == true)
    	        args += reply[i];
    	    else {
    	        if (user_input[i] == ' ')
    	    	    seen_space = true;
    	        else
    	    	    command += reply[i];
    	    }
        }
    
        // quit
        if (command == "quit") {
    	    send_msg(new_fd);
    	    if (close(new_fd) == -1)
    	        perror("(server) close child");
    	    std::cout << "(server) connection terminated with client" << std::endl;
    	    break;
        }
    
        // pwd
        else if (command == "pwd") {
    	    if (getcwd(buffer, MAXFILEBUFFER) != NULL)
    	        send_msg(new_fd, (std::string)buffer);
    	    else 
    	        send_err_msg(new_fd);
        }
    
        // mkdir
        else if (command == "mkdir") {
    	    if (args.size() == 0)
    	        send_msg(new_fd, "did not specify dir name");
    	    else {
    	        if (mkdir(args.c_str(), 0777) == -1)
    	    	    send_err_msg(new_fd);
    	        else
    	    	    send_msg(new_fd);
    	    }
        }
    
        // cd
        else if (command == "cd") {
    	    if (args.size() == 0)
    	        send_msg(new_fd, "did not specify dir name");
    	    else {
    	        if (chdir(args.c_str()) == -1)
    	    	    send_msg(new_fd, "incorrect file name");
    	        else
    	    	    send_msg(new_fd);
    	    }
        }
    
        // ls
        else if (command == "ls") {
    	    std::vector <std::string> files;
    	    DIR *d;
    	    std::string ls = "";
    	    struct dirent *dir;
    	    d = opendir(".");
    	    
    	    if (d) {
    	        while ((dir = readdir(d)) != NULL) {
    	    	    files.push_back(dir->d_name);
    	        }
    	        closedir(d);
    	        std::sort(files.begin(), files.end());
    	        for (int i=0; i<files.size(); i++)
    	    	    ls = ls + files[i] + " ";
    	        send_msg(new_fd, ls);
    	    }
    	    else
    	        send_msg(new_fd, "not able to open dir");
        }
    
        // delete
        else if (command == "delete") {
	    //delete will be added to queue and delete all reads in queue until a write request
	    //deletion happens at deactivation of file name, right after activation
    	    if (args.size() == 0)
    	        send_msg(new_fd, "did not specify file name");
    	    else {
    	        std::string complete_file_name = server_directory(buffer) + args;
		std::string map_error = activate_file_map(complete_file_name, -1, 'D');
		//once added to the queue, there is no way to remove it
		//this could be added later in a separate thread
		//the implementation would be similar to get and put
    	        if (map_error != ""){
    	    	    send_msg(new_fd, map_error);
    	        }
    	        else{
		    //upon deactivation, file will be deleted
		    int error = deactivate_file_map(complete_file_name);
    	    	    if (error == -1)
    	    	        send_err_msg(new_fd);
    	    	    else
    	    	        send_msg(new_fd);
    	        }
    	    }
        }
    
        // put
        else if (command == "put") {
    	    send_msg(new_fd);
    	    recv_and_write_file(new_fd, buffer);
        }
    
        // get
        else if (command == "get") {
    	    std::string directory = server_directory(buffer);
    	    std::string complete_file_name = directory + args;
    	    int file_in_use = activate_file_map(complete_file_name, , 'R');
    	    if (not add_daemon){
		activate_daemon_map(main_thread_id, 'W');
		std::string if_error = activate_file_map(complete_file_name, main_thread, 'R');
		char state = 'W';
		if (if_error == "MAP_IN_USE"){//if not success
		    while(state == 'W'){
			send_msg(new_fd, "waiting for queue");
	    		std::this_thread::sleep_for(std::chrono::seconds(0.25));
			std::this_thread::wait_time();
			state = daemon_map[thread_id];
		    }
		}
		if (state == 'A'){
		    send_msg("SUCCESS");
		    reply = get(new_fd, args, buffer, complete_file_name);
		    deactivate_file_map(complete_file_name);
		    std::cout << "final reply was " << reply << std::endl;
		}
		else if (state == 'D'){
		    send_msg(new_fd, "DELETED");
		}
		else if (state == 'T'){
		    send_msg(new_fd, "TERMINATED" + main_thread_id);
		}
		else{
		    send_msg(new_fd, if_error);
		}
	}
	else{ // daemon thread
	    int new_daemon_id = thread_plus();
     	    send_msg(new_fd, new_daemon_id);//TODO - change thread 'id' from complete_file_str to id mapped to file
    	        
    	    std::thread daemon(get_thread, args, new_fd, new_daemon_id, complete_file_name);
    	    Thread_Safety safe_thread(daemon);
   	    //this is performed after setting up file to allow connection to start before asking for client
    	    //this final send_msg pairs with final recv_msg in client main

	}


		while (
		std::string if_error = activate_file_map(complete_file_name, main_thread_id, 'R');
		if (if_error == ""){
    	            reply = get(new_fd, args, buffer, complete_file_name);
    	            deactivate_file_map(complete_file_name);
    	            std::cout << "final reply was: " << reply << std::endl;
		}

    	    }
    	    else{
    	        std::cout << "attempt implemention - server, please be wary" << std::endl;
		int new_daemon_id = thread_plus();
    	        
    	        std::thread daemon(get_thread, args, new_fd, new_daemon_id, complete_file_name, file_in_use);
    	        Thread_Safety safe_thread(daemon);
    	        send_msg(new_fd, new_daemon_id);//TODO - change thread 'id' from complete_file_str to id mapped to file
    	        //this is performed after setting up file to allow connection to start before asking for client
    	        //this final send_msg pairs with final recv_msg in client main
    	    }
        }
        else if (command == "terminate") {
    	    //terminate not implemented yet
        }
        else {
    	    send_msg(new_fd, "invalid command");
        }
	if (not t_sock_open and add_daemon){
	    t_sock_open = true; //opening of t_sock
	    //implement opening of t_sock
	    //PLEASE LOOK HERE
	    socklen_t thread_sin_size;
	    struct sockaddr_storage thread_addr;
	    struct sigaction thread_sa;

	    // handle zombie processes
	    thread_sa.sa_handler = sigchld_handler; // reap all dead processes
	    sigemptyset(&thread_sa.sa_mask);
	    thread_sa.sa_flags = SA_RESTART;
	    if (sigaction(SIGCHLD, &thread_sa, NULL) == -1) {
	        perror("(server) sigaction thread");
	        exit(1);//TODO: exit process, not program.
	        //TODO: clean up properly
	    }
	    std::cout << "(server) Thread listen started. Waiting for connections..." << std::endl;

	    thread_sin_size = sizeof thread_addr;
	    int thread_fd = accept(t_sockfd, (struct sockaddr *)&thread_addr, &thread_sin_size);
	    if (thread_fd == -1) {
	        perror("(server) accept");
	        continue;
	    }
	    send_msg(thread_fd); // Initial message to accept connection
	    std::cout << "(server) thread connection " << thread_fd << "established with client" << std::endl;
	    // PLEASE LOOK HERE END
	    //
	}
    }
    if (deactivate_server_map(new_fd) < 0){
	perror("erase attempt failure of new_fd");
    } // MAIN_THREAD TESTING POINT

    return 0;
}




//below is an old buggy implementation of get/get_thread methods, moved to server.hpp

//
//void get_thread(std::string args, std::string complete_file_name, int file_in_use){
//    //passing a char* would take some debugging
//    //code to connect - attempt
//    int t_sockfd, status, yes=1;
//    socklen_t sin_size;
//    struct sockaddr_storage their_addr;
//    struct sigaction sa;
//    std::string reply;
//    
//    // handle zombie processes
//    sa.sa_handler = sigchld_handler; // reap all dead processes
//    sigemptyset(&sa.sa_mask);
//    sa.sa_flags = SA_RESTART;
//    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
//        perror("(server) sigaction");
//        exit(1);
//    }
//
//    std::cout << "(daemon" << complete_file_name << ") Started. Waiting for connections..." << std::endl; //TODO modify to be an int
//    //
//    //
//    //
//    //
//    //
//    //
//    sin_size = sizeof their_addr;
//    t_sockfd = accept(t_sockfd, (struct sockaddr *)&their_addr, &sin_size);
//    if (t_sockfd == -1) {
//        perror("(server) accept");
//    }
//    send_msg(t_sockfd); // Initial message to accept connection
//    std::cout << "(server) connection established with client" << std::endl;
//
//    //
//    //
//    //
//    //int thread_fd;
//    char* thread_buffer[MAXFILEBUFFER] = {0};
//    reply = get(t_sockfd, args, thread_buffer, file_in_use, complete_file_name);
//    std::cout << reply << std::endl;
//}
//
//std::string get(int new_fd, std::string args, char *buffer, int file_in_use, std::string complete_file_name){ 
//    // default file_in_use = 0
//    // default complete_file_name = ""
//    bool file_exist = check_file_exists(args);
//    if (complete_file_name == ""){
//	complete_file_name = args;
//    }
//    std::string reply;
//    if (!file_exist) {
//        send_msg(new_fd, "NO_FILE"); // sends to common reply
//    }
//    else if (file_in_use < 0){
//	send_msg(new_fd, "FILE_IN_USE");
//    }
//    else {//success
//        send_msg(new_fd);
//        reply = recv_msg(new_fd, buffer, MAXFILEBUFFER);
//        send_file(new_fd, args, buffer, complete_file_name);
//    }
//    return reply;
//}
