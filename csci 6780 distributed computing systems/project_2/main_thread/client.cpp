#include "network.hpp"
//TODO: implement connect on t port, terminate, id link properly to map
void get_thread(int, std::string, std::string);
void get(int, char*, std::string);


int main (int argc, char** argv) {
     /*
        client.cpp takes three required arguments as input in the following 
        order
            - servername
            - nport_number
            - tport_number
        
        Note:
            Check network.hpp for the maximum allowed buffer sizes. The defaults
            are shown below
            * MAXINPUTASIZE 1024 -> The maximum size of user input in bytes
            * MAXFILEBUFFER 4096 -> The maximum size of messages (including error
                                    messages) that will be shared by client and 
                                    server
    */

    // Parse command line arguments
    const char *SERVER_ADDRESS = argv[1];
    const char *NPORT_NUMBER = argv[2];
    const char *TPORT_NUMBER = argv[3];

    char buffer[MAXFILEBUFFER] = {0};
    std::string reply, user_input;

    int n_sockfd = establish_connection_to_server(SERVER_ADDRESS, NPORT_NUMBER);
    // int t_sockfd = establish_connection_to_server(SERVER_ADDRESS, TPORT_NUMBER);
    
    std::cout << "(client) Waiting for server to accept connection" << std::endl;
    recv_msg(n_sockfd, buffer, false);
    std::cout << "(client) Connection established with server" << std::endl;
    
    while (1) {
        std::string command="", args="";
        bool add_daemon = false, seen_space = false, terminate_cmd = false;

        // Get user input from cmd
        show_prompt();
        getline(std::cin, user_input);

        // Check for terminate command
        if (user_input[user_input.size()-2] == ' ' and user_input[user_input.size()-1] == '&') {
            add_daemon = true;
        }
        
         // Check valid input for get,put
        if (command == "get" or command == "put") {
            if (args.size() == 0) {
                std::cout << "did not provide file name" << std::endl;
                continue;
            }
	    else if (add_daemon == true and get_active_daemon_threads() >= MAX_CLIENT_DAEMONS){
		std::cout << "too many threads in use" << std::endl;
		continue;
	    }
        }

	// Parse user_input to get "command" and "args"
	int range = user_input.size();
	if (add_daemon){
	    range = range - 2;
	}
        for (int i=0; i<range; i++) {
            if (seen_space == true)
                args += user_input[i];
            else {
                if (user_input[i] == ' ')
                    seen_space = true;
                else
                    command += user_input[i];
            }
        }
        


        // Send message to server
        send_msg(n_sockfd, user_input, false);

        // Get reply from server
        reply = recv_msg(n_sockfd, buffer, false);

	memset(buffer, '\0', reply.length());//TODO: this may be for fewer commands

        // Take action as per command
        if (command == "quit") {
            if (close(n_sockfd) == -1)
                perror("(client) close socket");
            std::cout << "(client) Connection closed with server" << std::endl;
            exit(0);
        }
        else if (command == "put") {
            //send_file(n_sockfd, args, buffer, false);
            //reply = recv_msg(n_sockfd, buffer, false);
            //std::cout << reply << std::endl;
	    std::cout << "broken put, please exit program" << std::endl;
        }
        else if (command == "get") {
	    bool error_terminate = false;
	    //check that server was able to activate the file
	    if (reply == "ABORT_FILE"){
		std::cout << "aborting file operation, file in use at server" << std::endl;
		continue;
	    }
	    else{
		std::cout << "initial get reply was: " << reply << std::endl;
	    }
	    //need to check that the file can be activated - client
	    if (activate_file_map(args) != 0){
		send_msg(n_sockfd, "ABORT_FILE");
		std::cout << "attempting to abort operation, file in use - client" << std::endl;
		continue;
	    }
	    else{
		send_msg(n_sockfd, "1c"); // success
	    }
            if (not add_daemon){
		//TODO: reply is useless here
		reply = "";
		get(n_sockfd, buffer, reply);
		deactivate_file_map(args);
	    }
	    else{
		std::cout <<"attempt implemented - client, please be wary" << std::endl;
		// PLEASE LOOK HERE START
	        int t_thread_fd = establish_connection_to_server(SERVER_ADDRESS, TPORT_NUMBER);
    
	        std::cout << "(client, opening thread) Waiting for server to accept connection" << std::endl;
	        recv_msg(t_thread_fd, buffer, false);
	        std::cout << "(client, opening thread) Connection established at " << t_thread_fd << " with server" << std::endl;
		// PLEASE LOOK HERE END
	
		std::string thread_id = recv_msg(n_sockfd, buffer, false);
		// thread_id should be recieve end of final send_msg in server main
		//
		//
		std::thread daemon(get_thread, n_sockfd, thread_id, reply);//TODO: implement thread_id as an int
		std::cout << "thread id: " << reply << std::endl;
		Thread_Safety safe_thread(daemon);
	    }
        }
	else if (command == "terminate") {
	    //
	    std::cout << "terminate not implemented yet" << std::endl;
	}
        else {
            if (reply == "NO_MESSAGE")
                continue;
            else
                std::cout << reply << std::endl;
        }
    }
}

void get_thread(int t_thread_fd, std::string thread_id, std::string reply){
    //code to connect - attempt
    ////TODO: pass in actual server_address, tport_number
    //std::string poor_server_add = "8800";
    //std::string poor_tport_add = "8900";
    //const char* SERVER_ADDRESS = poor_server_add.c_str();
    //const char* TPORT_NUMBER = poor_tport_add.c_str();
    //int t_sockfd = establish_connection_to_server(SERVER_ADDRESS, TPORT_NUMBER);
    //
    //std::cout << "(client thread) Waiting for server to accept connection" << std::endl;
    //recv_msg(t_sockfd, thread_buffer, false);
    //std::cout << "(client thread) Connection established with server" << std::endl;
    //memset(thread_buffer, '\0', strlen(thread_buffer));
    //int t_n_sockfd;
    //strcpy(thread_buffer, str_buf);
    //
    //
    //
    //
    char thread_buffer[MAXFILEBUFFER] = {0};
    get(t_thread_fd, thread_buffer, reply);
    std::cout << "finished get" << std::endl;
    std::cout << reply << std::endl;
}

void get(int n_sockfd, char* buffer, std::string reply){
    reply = recv_msg(n_sockfd, buffer, false);
    memset(buffer, '\0', reply.size()); 
    //already stored in reply, resetting buffer 
    if (reply == "NO_FILE") {
        std::cout << "File does not exist" << std::endl;
    }
    else if (reply == "FILE_IN_USE"){
	std::cout << "File is in use - server";
    }
    else {//success
	std::cout << "final reply: " << reply << std::endl;
        send_msg(n_sockfd, "fc"); // final handshake message for get
	std::cout << "sent fc null message" << std::endl;
	std::cout << "attempting to receive file" << std::endl;
        recv_and_write_file(n_sockfd, buffer, true, false);
	std::cout << "supposedly received file" << std::endl;
    }
}
