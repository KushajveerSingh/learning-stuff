#include "network.hpp"

void file_operation_function (int, std::string, std::string, std::string, bool, char*);
void remove_file_object (std::string);
char *SERVER_ADDRESS;

// If a "file_name" is in "file_table", then it means some thread is carrying out some
// operation on it and the current thread will sleep for 1 second and check again if
// file_name is in "file_table". When an operation is completed the "file_name" is removed 
// from "file_table".
std::vector<std::string> file_table;
std::mutex file_table_mutex;

int main (int argc, char** argv) {
     /*
        client.cpp takes three required arguments as input in the following 
        order
            - servername
            - nport_number
            - tport_number
    */

    // Parse command line arguments
    SERVER_ADDRESS = argv[1];
    const char *NPORT_NUMBER = argv[2];
    const char *TPORT_NUMBER = argv[3];

    char buffer[MAXFILEBUFFER] = {0};
    std::string reply, user_input, temp_input;

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
        temp_input = user_input;

        // Process the input
        // Handle terminate command
        if (user_input[user_input.size()-1] == '&') {
            terminate = true;
            user_input = user_input.substr(0, user_input.size()-2);
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

        // Check for terminate command
        if (command == "terminate") {
            // Connect to the TPORT and send the ID we received from get or put
            int t_sockfd = establish_connection_to_server(SERVER_ADDRESS, TPORT_NUMBER);
            send_msg(t_sockfd, args, false);
            if (close(t_sockfd) == -1)
                perror("(client) terminate socket close");
            continue;
        }

        // Send message to server
        send_msg(n_sockfd, temp_input, false);

        // Get reply from server
        reply = recv_msg(n_sockfd, buffer, false);
        
        // Take action as per command
        if (command == "quit") {
            // There are some background threads carrying out file operations. Terminate
            // them or wait for them to finish before quitting
            if (reply != "NO_MESSAGE")
                std::cout << reply << std::endl;
            
            // No background threads, so can quit
            else {
                if (close(n_sockfd) == -1)
                    perror("(client) close socket");
                std::cout << "(client) Connection closed with server" << std::endl;
                exit(0);
            }
        }
        
        else if (command == "pwd") {
            std::cout << reply << std::endl;
        }

        else if (command == "mkdir") {
            // Only print something if it is an error
            if (reply != "NO_MESSAGE")
                std::cout << reply << std::endl;
        }

        else if (command == "cd") {
            // Only print something if it is an error
            if (reply != "NO_MESSAGE")
                std::cout << reply << std::endl;
        }

        else if (command == "ls") {
            std::cout << reply << std::endl;
        }

        else if (command == "delete") {
            // Only print something if it is an error
            if (reply != "NO_MESSAGE")
                std::cout << reply << std::endl;
        }

        else if (command == "put") {
            if (terminate) {
                // reply contains information on the ID of file operation and the unique
                // client ID which we need to send back to server for identification
                int index = reply.find('_');
                std::string id = reply.substr(0, index);
                std::string client_fd = reply.substr(index+1, reply.size());

                std::cout << "Id: " << id << std::endl;
                
                char *temp;
                std::thread handle(file_operation_function, -1, client_fd, args, command, terminate, temp);
                handle.detach();
            }
            else
                file_operation_function(n_sockfd, "", args, command, false, buffer);
        }

        else if (command == "get") {
            if (reply == "FILE_NOT_EXIST") {
                std::cout << "File does not exist" << std::endl;
                continue;
            }

            if (terminate) {
                // reply contains information on the ID of file operation and the unique
                // client ID which we need to send back to server for identification
                int index = reply.find('_');
                std::string id = reply.substr(0, index);
                std::string client_fd = reply.substr(index+1, reply.size());

                std::cout << "Id: " << id << std::endl;
                
                char *temp;
                std::thread handle(file_operation_function, -1, client_fd, args, command, terminate, temp);
                handle.detach();
            }
            else
                file_operation_function(n_sockfd, "", args, command, false, buffer);
        }
    }
}

void file_operation_function (int fd, std::string client_fd, std::string args, std::string command, bool terminate, char *buffer) {
    // For a file operation to be carried out, the "file_name" must be in "file_table".
    // The while loop checks for this. If the "file_name" is not in "file_table" then it
    // adds it to the table and starts executing the file operations. Otherwise, it sleeps
    // for one second and then check's again. This ensures we do not do get, put, delete on 
    // same file in multiple threads.
    // After completing each operation a call is made to "remove_file_object" which
    // removes the path from "file_table".
    while (1) {
        file_table_mutex.lock();
        bool exists = false;
        for (int i=0; i<file_table.size(); i++) {
            if (file_table[i] == args) {
                exists = true;
                break;
            }
        }

        if (exists) {
            file_table_mutex.unlock();
            std::this_thread::sleep_for(long_dura);
        }
        else {
            file_table.push_back(args);
            file_table_mutex.unlock();
            break;
        }
    }
    
    if (command == "put") {
        if (terminate) {
            char buffer[MAXFILEBUFFER] = {0};
            int fd = establish_connection_to_server(SERVER_ADDRESS, EXTRA_PORT);
            send_msg(fd, client_fd); // Handshake message to send unique ID of client

            // Below code needs to contain the logic on how to receive file from server
            // fd = file descriptor to use for communication
            // args = name of file to store on client
            // buffer = temporary buffer if needed
            // Also, do not print anything. As this thread runs in background printing
            // can mess up std::cout
            std::string reply = recv_msg(fd, buffer, false); // handshake receive
            
            std::string first_send = "NO_ERROR";
            bool file_exist = check_file_exists(args);
            if (!file_exist)
                first_send = "ERROR";
            FILE *file;
            file = fopen(args.c_str(), "rb");
            if (file == NULL)
                first_send = "ERROR";
            send_msg(fd, first_send);
            reply = recv_msg(fd, buffer, false);
            bool send_end_message = true;

            if (reply == "CONTINUE") {
                fseek(file, 0, SEEK_END);
                int numbytes = ftell(file);
                fseek(file, 0, SEEK_SET);
                fread(buffer, sizeof(char), numbytes, file);
                fclose(file);

                std::string file_contents = "", temp;
                for (int i=0; i<numbytes; i++)
                    file_contents += buffer[i];
    
                int num_chunks = std::ceil((float)numbytes / (float)CHUNK_SIZE);
                int start_index, end_index;
                for (int i=0; i<num_chunks; i++) {
                    start_index = CHUNK_SIZE*i;
                    end_index = start_index + CHUNK_SIZE;
                    if (end_index > numbytes)
                        end_index = numbytes;
                    std::string send_string = "";
                    for (int i=start_index; i<end_index; i++)
                        send_string += file_contents[i];
                    send_msg(fd, send_string, false);
                    reply = recv_msg(fd, buffer, false);

                    if (reply == "STOP") { 
                        send_end_message = false;
                        break;
                    }
                }
                if (send_end_message)
                    send_msg(fd, "FILE_TRANSFER_COMPLETE");
            }
               
            // Cleanup to close the connection and remove file_name from file_table
            remove_file_object(args);
        }
        else {
            send_file(fd, args, buffer, false);
            std::string reply = recv_msg(fd, buffer, false);
            std::cout << reply << std::endl;
            remove_file_object(args);
        }
    }
    
    if (command == "get") {
        if (terminate) {
            char buffer[MAXFILEBUFFER] = {0};
            int fd = establish_connection_to_server(SERVER_ADDRESS, EXTRA_PORT);
            send_msg(fd, client_fd); // handshake message to send unique ID of client
            
            // Below code needs to contain the logic on how to receive file from server
            // fd = file descriptor to use for communication
            // args = name of file to store on client
            // buffer = temporary buffer if needed
            // Also, do not print anything. As this thread runs in background printing
            // can mess up std::cout
            std::string file_contents = "", reply;
            bool save_file = true;

            reply = recv_msg(fd, buffer, false);
            send_msg(fd, "", false);
            if (reply == "ERROR") {
                save_file = false;
            }
            else {
                while (true) {
                    reply = recv_msg(fd, buffer, false);
                    send_msg(fd);

                    file_contents += reply;

                    reply = recv_msg(fd, buffer, false);
                    send_msg(fd);

                    if (reply == "LAST_MESSAGE") {
                        break;
                    }

                    reply = recv_msg(fd, buffer, false);
                    send_msg(fd);
                    
                    if (reply == "STOP") {
                        save_file = false;
                        break;
                    }
                }
            }

            if (save_file)
                write_file(args, file_contents, false);

            // Cleanup to close the connection and remove file_name from file_table
            remove_file_object(args);
        }
        else {
            send_msg(fd);
            recv_and_write_file(fd, args, buffer, true, false);
            remove_file_object(args);
        }
    }
}

void remove_file_object (std::string path) {
    // Remove file_name (stored in path) from file_table
    file_table_mutex.lock();
    int index;
    for (int i=0; i<file_table.size(); i++) {
        if (file_table[i] == path) {
            index = i;
            break;
        }
    }
    file_table.erase(file_table.begin()+index);
    file_table_mutex.unlock();
}