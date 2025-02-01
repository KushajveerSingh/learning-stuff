#include "network.hpp"

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

    // Establish connection with server
    int n_sockfd = establish_connection_to_server(SERVER_ADDRESS, NPORT_NUMBER);
    
    reply = recv_msg(n_sockfd, buffer, false);
    if (reply == "CONNECTION_REFUSED") {
        std::cout << "(client) Maximum connections reached on server. Try after some time." << std::endl;
        if (close(n_sockfd) == -1)
            perror("(client) close nport socket on connection refuse");
        return 0;
    }
    else {
        std::cout << "(client) Connection established with server" << std::endl;
    }
    
    while (1) {
        std::string command="", args="";
        bool terminate = false;

        // Get user_input from cmd
        show_prompt();
        getline(std::cin, user_input);

        // Process the input
        // Handle terminate command
        if (user_input[user_input.size()-1] == '&') {
            terminate = true;
            user_input = user_input.substr(0, user_input.size()-1);
        }
        // Get command, args
        bool seen_space = false;
        for (int i=0; i<user_input.size(); i++) {
            if (seen_space == true)
                args += user_input[i];
            else {
                if (user_input[i] == ' ')
                    seen_space = true;
                else
                    command += user_input[i];
            }
        }
        
        // Check valid input for get,put
        if (command == "get" or command == "put" or command == "delete" or command == "mkdir") {
            if (args.size() == 0) {
                std::cout << "did not provide file name" << std::endl;
                continue;
            }
        }

        // Send message to server
        send_msg(n_sockfd, user_input, false);

        // Get reply from server
        reply = recv_msg(n_sockfd, buffer, false);
        
        // Take action as per command
        if (command == "quit") {
            if (close(n_sockfd) == -1)
                perror("(client) close socket");
            std::cout << "(client) Connection closed with server" << std::endl;
            exit(0);
        }
        
        else if (command == "pwd") {
            std::cout << reply << std::endl;
        }

        else if (command == "mkdir") {
            if (reply != "NO_MESSAGE")
                std::cout << reply << std::endl;
        }

        else if (command == "cd") {
            if (reply != "NO_MESSAGE")
                std::cout << reply << std::endl;
        }

        else if (command == "ls") {
            std::cout << reply << std::endl;
        }

        else if (command == "delete") {
            if (reply != "NO_MESSAGE")
                std::cout << reply << std::endl;
        }

        else if (command == "put") {
            if (terminate) {
                std::cout << "Id: " << reply << std::endl;
            }
            else {
                send_file(n_sockfd, args, buffer, false);
                reply = recv_msg(n_sockfd, buffer, false);
                std::cout << reply << std::endl;
            }
        }

        else if (command == "get") {
            if (reply == "FILE_NOT_EXIST") {
                std::cout << "File does not exist" << std::endl;
            }
            else {
                send_msg(n_sockfd);
                recv_and_write_file(n_sockfd, args, buffer, true, false);
            }
        }
    }
}

char file_check(std::string path){
    //function is defined in network.hpp
    //necessary for server, unnecessary for client
    return 'A'; // A is practically continue for this function
}
