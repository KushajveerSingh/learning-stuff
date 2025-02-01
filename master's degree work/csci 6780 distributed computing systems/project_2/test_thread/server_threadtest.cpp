#include "server.hpp"

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
    bool t_sock_open = false;
    
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
        sin_size = sizeof their_addr;
        new_fd = accept(n_sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("(server) accept");
            continue;
        }
        send_msg(new_fd); // Initial message to accept connection
        std::cout << "(server) connection established with client" << std::endl;
        
        // Create buffers for client
        char user_input[MAXINPUTSIZE] = {0};
        char buffer[MAXFILEBUFFER] = {0};
        std::string reply;

        while (1) {
            std::string command="", args="";
            bool add_daemon = false, seen_space = false, terminate_cmd = false;
            
            // Receive input from client
            reply = recv_msg(new_fd, user_input, MAXFILEBUFFER);
            
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
                if (args.size() == 0)
                    send_msg(new_fd, "did not specify file name");
                else {
                    if (remove(args.c_str()) == -1)
                        send_err_msg(new_fd);
                    else
                        send_msg(new_fd);
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
		int file_in_use = activate_file_map(complete_file_name);
		//this needs to check that the file can be activated - server
		//only once that's clear, there should be an abort option otherwise
		//
		if (file_in_use != 0) {
		    send_msg(new_fd, "ABORT_FILE");
		    std::cout <<"attempting to abort operation, file in use -server" << std::endl;
		    continue;
		}
		else{
		    send_msg(new_fd, "1s");
		    reply = recv_msg(new_fd, user_input, MAXFILEBUFFER);//TODO: likely a different MAX
		    if (reply == "ABORT_FILE"){
			std::cout << "aborting file operation, file in use from client" << std::endl;
			deactivate_file_map(complete_file_name);
			continue;
		    }
		    else{
			std::cout << "initial get reply was: " << reply << std::endl;
		    }
		    //else success
		}
                if (not add_daemon){
		    reply = get(new_fd, args, buffer, complete_file_name);
		    deactivate_file_map(complete_file_name);
		    std::cout << "final reply was: " << reply << std::endl;
		}
		else{
		    std::cout << "attempt implemention - server, please be wary" << std::endl;
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
		    //connection established, commence file transfer
		    //
		    //thread count should be performed on client daemons and number of clients
		    //int file_in_use = activate_file_map(complete_file_name);
		    std::thread daemon(get_thread, args, thread_fd, complete_file_name, file_in_use);
		    Thread_Safety safe_thread(daemon);
		    send_msg(new_fd, complete_file_name);//TODO - change thread 'id' from complete_file_str to id mapped to file
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
        }
    }

    // Stop server
    if (close(n_sockfd) == -1)
        perror("(server) close socket");
    if (close(t_sockfd) == -1)
	perror("(server) close t_socket");
    std::cout << "(server) Server stopped" << std::endl;
    exit(0);
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
