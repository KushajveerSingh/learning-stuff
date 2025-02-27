#include "server.hpp"

void nport_function (int);
void tport_function (int);
void client_nport_function (int);
void client_tport_function (int);
void file_operation_function (int, std::string, std::string, bool, char*);
void remove_file_object (std::string);
void modify_id_table ();

// Global variables for threads
std::string global_dir;

int current_clients = 0;
std::mutex current_clients_mutex;
void modify_current_clients (int x) {
    current_clients_mutex.lock();
    current_clients += x;
    current_clients_mutex.unlock();
}

struct file_object {
    int id;
    std::string path;
    std::string command;
    char status;
};

std::vector<file_object> file_table;
int file_id = 0;
std::mutex file_table_mutex;
std::mutex file_id_mutex;

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

    int n_sockfd = open_port_for_connections(NPORT_NUMBER);
    int t_sockfd = open_port_for_connections(TPORT_NUMBER);
    
    // handle zombie processes
    struct sigaction sa;
    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("(server) sigaction");
        exit(1);
    }

    char temp_buffer[MAXFILEBUFFER];
    getcwd(temp_buffer, MAXFILEBUFFER);
    global_dir = (std::string)temp_buffer;

    std::cout << "(server) Started. Waiting for connections..." << std::endl;

    std::thread nport_thread(nport_function, n_sockfd);
    std::thread tport_thread(tport_function, t_sockfd);
    // nport_thread.join();
    tport_thread.join();
}


void nport_function (int fd) {
    int new_fd, status;
    socklen_t sin_size;
    struct sockaddr_storage their_addr;
    sin_size = sizeof their_addr;
    
    while (1) {
        new_fd = accept(fd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("(server) accept");
            continue;
        }

        if (current_clients >= MAX_CLIENTS) {
            send_msg(new_fd, "CONNECTION_REFUSED");
            if (close(new_fd) == -1)
                perror("(server) refuse connection due to max clients");
            std::cout << "(server) connection refused with client" << std::endl;
        }
        else {
            modify_current_clients(1);
            send_msg(new_fd, "CONNECTION_ACCEPTED");
            std::cout << "(server) connection established with client" << std::endl;
            
            std::thread client_thread(client_nport_function, new_fd);
            client_thread.detach();
        }
    }
    
}

void tport_function (int fd) {
    int new_fd, status;
    socklen_t sin_size;
    struct sockaddr_storage their_addr;
    sin_size = sizeof their_addr;
    
    while (1) {
        new_fd = accept(fd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("(server) tport accept");
            continue;
        }

        // std::thread client_thread(client_tport_function, new_fd);
        // client_thread.detach();
    }
}

void client_nport_function (int fd) {
    char user_input[MAXINPUTSIZE] = {0};
    char buffer[MAXFILEBUFFER] = {0};
    std::string reply;
    std::string current_dir = global_dir;

    while (1) {
        std::string command="", args="";
        bool terminate = false;

        // Receive input from client
        reply = recv_msg(fd, user_input, MAXINPUTSIZE);

        // Process the input
        // Handle terminate command
        if (reply[reply.size()-1] == '&') {
            terminate = true;
            reply = reply.substr(0, reply.size()-1);
        }
        // Get command, args
        bool seen_space = false;
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

        // Handle individual commands
        if (command == "quit") {
            send_msg(fd);
            if (close(fd) == -1)
                perror("(server) close client");
            modify_current_clients(-1);
            std::cout << "(server) connection terminated with client" << std::endl;
            return;
        }

        else if (command == "pwd") {
            send_msg(fd, current_dir);
        }

        else if (command == "mkdir") {
            std::string path = current_dir + "/" + args;
            file_operation_function(fd, path, command, terminate, buffer);
        }

        else if (command == "cd") {
            if (args.size() == 0)
                send_msg(fd, "did not specify dir name");
            else {
                if (args == "..") {
                    int index = 0;
                    for (int i=current_dir.size(); i>=0; i--) {
                        if (current_dir[i] == '/') {
                            index = i;
                            break;
                        }
                    }
                    if (current_dir.size() == 1)
                        send_msg(fd, "cannot go past root");
                    else if (index == 0) {
                        current_dir = "/";
                        send_msg(fd);
                    }
                    else {
                        current_dir = current_dir.substr(0, index);
                        send_msg(fd);
                    }
                }
                else {
                    std::string path = current_dir + "/" + args;
                    if (!opendir(path.c_str()))
                        send_msg(fd, "invalid dir name");
                    else {
                        current_dir = path;
                        send_msg(fd);
                    }
                }
            }
        }

        else if (command == "ls") {
            std::vector <std::string> files;
            DIR *d;
            std::string ls = "";
            struct dirent *dir;
            d = opendir(current_dir.c_str());
            
            if (d) {
                while ((dir = readdir(d)) != NULL) {
                    files.push_back(dir->d_name);
                }
                closedir(d);
                std::sort(files.begin(), files.end());
                for (int i=0; i<files.size(); i++)
                    ls = ls + files[i] + " ";
                send_msg(fd, ls);
            }
            else
                send_msg(fd, "not able to open dir");
        }

        else if (command == "delete") {
            std::string path = current_dir + "/" + args;
            file_operation_function(fd, path, command, terminate, buffer);
        }

        else if (command == "put") {
            std::string path = current_dir + "/" + args;
            if (terminate) {
                char *temp;
                // std::thread handle(file_operation_function, fd, path, command, terminate, temp);
                // handle.detach();
            }
            else
                file_operation_function(fd, path, command, terminate, buffer);
        }

        else if (command == "get") {
            std::string path = current_dir + "/" + args;
            if (terminate) {
                char *temp;
                // std::thread handle(file_operation_function, fd, path, command, terminate, temp);
                // handle.detach();
            }
            else;
                file_operation_function(fd, path, command, terminate, buffer);
        }
    }
}

void file_operation_function (int fd, std::string path, std::string command, bool terminate, char *buffer) {
    file_id_mutex.lock();
    file_id++;
    int curr_id = file_id;
    file_id_mutex.unlock();
    
    while (1) {
        file_table_mutex.lock();
        bool exists = false;
        for (int i=0; i<file_table.size(); i++) {
            if (file_table[i].path == path) {
                exists = true;
                break;
            }
        }

        if (exists) {
            file_table_mutex.unlock();
            std::this_thread::sleep_for(dura);
        }
        else {
            file_object file;
            file.id = curr_id;
            file.path = path;
            file.command = command;
            file.status = 'A';
            file_table.push_back(file);
            file_table_mutex.unlock();
            break;
        }
    }

    if (command == "mkdir") {
        if (mkdir(path.c_str(), 0777) == -1)
            send_err_msg(fd);
        else
            send_msg(fd);
        remove_file_object(path);
    }

    else if (command == "delete") {
        if (remove(path.c_str()) == -1)
            send_err_msg(fd);
        else
            send_msg(fd);
        remove_file_object(path);
    }

    else if (command == "put") {
        if (terminate) {
            
        }
        else {
            send_msg(fd);
            recv_and_write_file(fd, path, buffer);
            remove_file_object(path);
        }
        
    }

    else if (command == "get") {
        if (terminate) {

        }
        else {
            bool file_exists = check_file_exists(path);
            if (!file_exists)
                send_msg(fd, "FILE_NOT_EXIST");
            else {
                send_msg(fd);
                std::string temp = recv_msg(fd, buffer, MAXFILEBUFFER);
                send_file(fd, path, buffer);
            }
            remove_file_object(path);
        }
    }
    
}

void remove_file_object (std::string path) {
    file_table_mutex.lock();
    int index;
    for (int i=0; i<file_table.size(); i++) {
        if (file_table[i].path == path) {
            index = i;
            break;
        }
    }
    file_table.erase(file_table.begin()+index);
    file_table_mutex.unlock();
}


char file_check(std::string path){//function to allow network.hpp function to access file_ declared in server
    file_table_mutex.lock();
    int index = -1;
    for (int i=0; i<file_table.size(); i++) {
        if (file_table[i].path == path) {
            index = i;
            break;
        }
    }
    char status  = file_table[index].status;
    file_table_mutex.unlock();
    //possible send_msg(fd, "" + status);
    return status;
}
