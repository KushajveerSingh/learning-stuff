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

    int n_sockfd = establish_connection_to_server(SERVER_ADDRESS, NPORT_NUMBER);
    int t_sockfd = establish_connection_to_server(SERVER_ADDRESS, TPORT_NUMBER);
    
    std::cout << "(client) Waiting for server to accept connection" << std::endl;
    recv_msg(n_sockfd, buffer, false);
    std::cout << "(client) Connection established with server" << std::endl;
    
    while (1) {
        std::string command="", args="";
        bool terminate_cmd = false, seen_space = false;

        // Get user input from cmd
        show_prompt();
        getline(std::cin, user_input);

        // Check for terminate command
        if (user_input[user_input.size()-1] == '&') {
            terminate_cmd = true;
            user_input = user_input.substr(0, user_input.size()-2);
        }
        
        // Parse user_input to get "command" and "args"
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
        if (command == "get" or command == "put") {
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
        else if (command == "put") {
            send_file(n_sockfd, args, buffer, false);
            reply = recv_msg(n_sockfd, buffer, false);
            std::cout << reply << std::endl;
        }
        else if (command == "get") {
            if (reply == "NO_FILE") {
                std::cout << "File does not exist" << std::endl;
                continue;
            }
            else {
                send_msg(n_sockfd); // handshake message for get
                recv_and_write_file(n_sockfd, buffer, true, false);
            }
        }
        else {
            if (reply == "NO_MESSAGE")
                continue;
            else
                std::cout << reply << std::endl;
        }
    }
}