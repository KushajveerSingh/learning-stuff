#include "server.hpp"

void nport_function (int);
void tport_function (int);
void eport_function (int);
void client_nport_function (int);
void client_tport_function (int);
void file_operation_function (int, int, std::string, std::string, bool, char*);
void remove_new_fd (int, int);
void remove_file_object (std::string);

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
    int id;              // Unique ID for the operation. Used by client to terminate
    std::string path;    // Absolute path of file
    std::string command; // get, put, delete
    char status;         // 'A' or 'T'
};

int file_id = 0;  // Used to specify file_object.id
std::mutex file_id_mutex;

// If a "path" is in "file_table", then it means some thread is carrying out some
// operation on it and the current thread will sleep for 1 second and check again if
// path is in "file_table". When an operation is completed the "path" is removed 
// from "file_table".
std::vector<file_object> file_table;
std::mutex file_table_mutex;

// Store vector of (client_fd, vec(fd, free))
// Each client is identified by a unique ID that is given to it by the file descriptor
// on which the thread works on. This ID is also available to the client when needed.
// For each client "fd_object" maintains a vector storing the file descriptor (and if
// they got occupied by some thread) currently open for that client. So if a client
// uses two terminate commands to have two background threads then this vector will
// contain two entries. The second boolean is just to ensure concurrency so that multiple
// threads do not use the same file descriptor (the chances of it happening are very small)
struct fd_object {
    int client_fd;
    std::vector<std::pair<int, bool> > fds;
};
std::vector<fd_object> fd_table;
std::mutex fd_table_mutex;

int main (int argc, char** argv) {
     /*
        server.cpp takes two required arguments as input
            - nport_number
            - tport_number
    */

    // Parse command line arguments
    const char *NPORT_NUMBER = argv[1];
    const char *TPORT_NUMBER = argv[2];

    int n_sockfd = open_port_for_connections(NPORT_NUMBER);
    int t_sockfd = open_port_for_connections(TPORT_NUMBER);
    int e_sockfd = open_port_for_connections(EXTRA_PORT);

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
    std::thread eport_thread(eport_function, e_sockfd);
    nport_thread.join();
    tport_thread.join();
    eport_thread.join();
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
            
            // Add an entry for the client in "fd_table"
            fd_table_mutex.lock();
            std::vector<std::pair<int, bool> > empty;
            fd_object fd;
            fd.client_fd = new_fd;
            fd.fds = empty;
            fd_table.push_back(fd);
            fd_table_mutex.unlock();
            
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
    char buffer[MAXFILEBUFFER] = {0};

    while (1) {
        new_fd = accept(fd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("(server) tport accept");
            continue;
        }
    
        std::string reply = recv_msg(new_fd, buffer);
        int file_id = std::stoi(reply);

        file_table_mutex.lock();
        for (int i=0; i<file_table.size(); i++) {
            if (file_table[i].id == file_id) {
                file_table[i].status = 'T';
                break;
            }
        }
        file_table_mutex.unlock();

        if (close(new_fd) == -1)
            perror("(server) close tport new_fd");
    }
}

void eport_function (int fd) {
    int new_fd, status;
    socklen_t sin_size;
    struct sockaddr_storage their_addr;
    sin_size = sizeof their_addr;
    char buffer[MAXFILEBUFFER] = {0};

    while (1) {
        new_fd = accept(fd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("(server) eport accept");
            continue;
        }

        // Get the unique ID from the client
        std::string reply = recv_msg(new_fd, buffer);
        
        fd_table_mutex.lock();
        int fd = std::stoi(reply);
        int index;

        // Add the current file descriptor (new_fd) to the list of open file
        // descriptors of client_fd (stored in reply)
        for (int i=0; i<fd_table.size(); i++) {
            if (fd_table[i].client_fd == fd) {
                index = i;
                break;
            }
        }

        std::pair<int, bool> temp(new_fd, true);
        fd_table[index].fds.push_back(temp);
        fd_table_mutex.unlock();
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
            reply = reply.substr(0, reply.size()-2);
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
            // Check if there are open file descriptors for the client, if there are
            // then it means some background threads are carrying out some file operation
            // in which case the client should either wait for them to complete or
            // terminate them manually
            fd_table_mutex.lock();
            bool can_delete = false;
            int index;

            for (int i=0; i<fd_table.size(); i++) {
                if (fd_table[i].client_fd == fd) {
                    index = i;
                    // size = 0 means there are no open file descriptors
                    if (fd_table[i].fds.size() == 0)
                        can_delete = true;
                    break;
                }
            }
            
            if (can_delete) {
                // Remove the entry of the client from the table
                fd_table.erase(fd_table.begin()+index);
                fd_table_mutex.unlock();
                send_msg(fd);

                if (close(fd) == -1)
                    perror("(server) close client");
                modify_current_clients(-1);
                std::cout << "(server) connection terminated with client" << std::endl;
                return;
            }
            else {
                // Background threads are carrying out some file operations, so cannot quit
                fd_table_mutex.unlock();
                send_msg(fd, "Close background process using 'terminate {ID}' first");
            }
        }

        else if (command == "pwd") {
            send_msg(fd, current_dir);
        }

        else if (command == "mkdir") {
            file_id_mutex.lock();
            file_id++;
            int curr_id = file_id;
            file_id_mutex.unlock();

            std::string path = current_dir + "/" + args;
            file_operation_function(fd, curr_id, path, command, terminate, buffer);
        }

        else if (command == "cd") {
            if (args.size() == 0)
                send_msg(fd, "did not specify dir name");
            else {
                if (args == "..") {
                    // Get location of last "/"
                    int index = 0;
                    for (int i=current_dir.size(); i>=0; i--) {
                        if (current_dir[i] == '/') {
                            index = i;
                            break;
                        }
                    }

                    // Check if current_dir is root in which case ".." is invalid
                    if (current_dir.size() == 1)
                        send_msg(fd, "cannot go past root");
                    // The base case current_dir = "/something" needs to be handled differently 
                    else if (index == 0) {
                        current_dir = "/";
                        send_msg(fd);
                    }
                    // remove all the characters on the right of "/"
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
            file_id_mutex.lock();
            file_id++;
            int curr_id = file_id;
            file_id_mutex.unlock();

            std::string path = current_dir + "/" + args;
            file_operation_function(fd, curr_id, path, command, terminate, buffer);
        }

        else if (command == "put") {
            std::string path = current_dir + "/" + args;
            file_id_mutex.lock();
            file_id++;
            int curr_id = file_id;
            file_id_mutex.unlock();

            if (terminate) {
                // Send the client the ID of operation and its unique ID
                std::string mess = std::to_string(curr_id) + "_" + std::to_string(fd);
                send_msg(fd, mess);

                char *temp;
                std::thread handle(file_operation_function, fd, curr_id, path, command, terminate, temp);
                handle.detach();
            }
            else
                file_operation_function(fd, curr_id, path, command, terminate, buffer);
        }

        else if (command == "get") {
            std::string path = current_dir + "/" + args;
            file_id_mutex.lock();
            file_id++;
            int curr_id = file_id;
            file_id_mutex.unlock();

            bool check_exists = check_file_exists(path);
            if (!check_exists) {
                send_msg(fd, "FILE_NOT_EXIST");
                continue;
            }
            else {
                std::string mess = std::to_string(curr_id) + "_" + std::to_string(fd);
                send_msg(fd, mess);
            }

            if (terminate) {
                char *temp;
                std::thread handle(file_operation_function, fd, curr_id, path, command, terminate, temp);
                handle.detach();
            }
            else;
                file_operation_function(fd, curr_id, path, command, terminate, buffer);
        }
    }
}

void file_operation_function (int fd, int id, std::string path, std::string command, bool terminate, char *buffer) {
    // For a file operation to be carried out, the "path" must be in "file_table". The
    // while loop checks for this. If the "path" is not in "file_table" then it adds it
    // to the table and starts executing the file operations. Otherwise, it sleeps for
    // one second and then check's again. This ensures we do not do get, put, delete on 
    // same file in multiple threads.
    // After completing each operation a call is made to "remove_file_object" which
    // removes the path from "file_table". In case of (get, put) we also need to 
    // remove the file descriptor (in case of terminate) command and this is done
    // using "remove_new_fd".
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
            std::this_thread::sleep_for(long_dura);
        }
        else {
            file_object file;
            file.id = id;
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
            // Get a file descriptor to transfer messges to client. The logic implemented
            // using fd_table, eport_function, and below code ensures that server thread
            // connects to the correct client (and client thread) even if there are multiple
            // clients and multiple threads for the same client.
            char buffer[MAXFILEBUFFER] = {0};
            int new_fd;
            while (1) {
                fd_table_mutex.lock();
                bool got_fd = false;
                int index = -1;

                // Check if client's unique is even present in fd_table
                for (int i=0; i<fd_table.size(); i++) {
                    if (fd_table[i].client_fd == fd) {
                        index = i;
                        break;
                    }
                }

                // index=-1, means client's id is not in fd_table. So we sleep for one second
                // and check again
                if (index == -1) {
                    fd_table_mutex.unlock();
                    std::this_thread::sleep_for(dura);
                    continue;
                }

                // Client has an entry in fd_table and there might be multiple file descriptors
                // to choose from. For every file descriptor, we store if it has been occupied
                // by a thread and if it hasen't then the current thread occupies it and uses
                // it to transfer messages to the client.
                std::vector<std::pair<int, bool> > fds= fd_table[index].fds;
                for (int i=0; i<fds.size(); i++) {
                    if (fds[i].second) {
                        fds[i].second = false;
                        new_fd = fds[i].first;
                        got_fd = true;
                        break;
                    }
                }

                if (got_fd) {
                    fd_table_mutex.unlock();
                    break;
                }
                else {
                    fd_table_mutex.unlock();
                    std::this_thread::sleep_for(dura);
                }
            }

            // Below code needs to contain the logic on how to send file to client
            // new_fd = file descriptor using which to send message
            // path = name of the file on server to send
            // buffer = temporary buffer if needed
            send_msg(new_fd); // handshake send
            
            std::string file_contents = "";
            std::string reply;
            std::string handshake_msg;
            bool save_file = true;
            bool run_loop = true;

            reply = recv_msg(new_fd, buffer);
            if (reply == "ERROR") {
                send_msg(new_fd, "STOP");
                save_file = false;
            }
            else {
                send_msg(new_fd, "CONTINUE");
                while (true) {
                    reply = recv_msg(new_fd, buffer);
                    if (reply == "FILE_TRANSFER_COMPLETE")
                        break;
                    else {
                        file_contents = file_contents + reply;
                        file_table_mutex.lock();
                        for (int i=0; i<file_table.size(); i++) {
                            if (file_table[i].path == path) {
                                if (file_table[i].status == 'A')
                                    handshake_msg = "CONTINUE";
                                else {
                                    save_file = false;
                                    handshake_msg = "STOP";
                                }
                                break;
                            }
                        }
                        file_table_mutex.unlock();
                        send_msg(new_fd, handshake_msg);
                        
                        if (handshake_msg == "STOP")
                            break;
                    }
                }

                if (save_file)
                    write_file(path, file_contents);
            }
            
            // Cleanup to remove the path from file_table and fd from fd_table
            remove_new_fd(fd, new_fd);
            remove_file_object(path);
        }
        else {
            send_msg(fd);
            recv_and_write_file(fd, path, buffer);
            remove_file_object(path);
        }    
    }

    else if (command == "get") {
        // Get a file descriptor to transfer messges to client. The logic implemented
        // using fd_table, eport_function, and below code ensures that server thread
        // connects to the correct client (and client thread) even if there are multiple
        // clients and multiple threads for the same client.
        if (terminate) {
            char buffer[MAXFILEBUFFER] = {0};
            int new_fd;
            while (1) {
                fd_table_mutex.lock();
                bool got_fd = false;
                int index = -1;

                // Check if client's unique is even present in fd_table
                for (int i=0; i<fd_table.size(); i++) {
                    if (fd_table[i].client_fd == fd) {
                        index = i;
                        break;
                    }
                }

                // index=-1, means client's id is not in fd_table. So we sleep for one second
                // and check again
                if (index == -1) {
                    std::cout << "Index = -1" << std::endl;
                    fd_table_mutex.unlock();
                    std::this_thread::sleep_for(long_dura);
                    continue;
                }
                
                // Client has an entry in fd_table and there might be multiple file descriptors
                // to choose from. For every file descriptor, we store if it has been occupied
                // by a thread and if it hasen't then the current thread occupies it and uses
                // it to transfer messages to the client.
                std::vector<std::pair<int, bool> > fds= fd_table[index].fds;
                for (int i=0; i<fds.size(); i++) {
                    if (fds[i].second) {
                        fds[i].second = false;
                        new_fd = fds[i].first;
                        got_fd = true;
                        break;
                    }
                } 

                if (got_fd) {
                    fd_table_mutex.unlock();
                    break;
                }
                else {
                    std::cout << "Did no get fd" << std::endl;
                    fd_table_mutex.unlock();
                    std::this_thread::sleep_for(dura);
                }
            }
            std::cout << "Got fd" << std::endl;
            // Below code needs to contain the logic on how to receive file from client
            // new_fd = file descriptor using which to send message
            // path = name of the file on server to store the file in
            // buffer = temporary buffer if needed
            std::string first_send="NO_ERROR", reply;
            bool file_exist = check_file_exists(path);
            if (!file_exist)
                first_send = "ERROR";
            FILE *file;
            file = fopen(path.c_str(), "rb");
            if (file == NULL)
                first_send = "ERROR";
            send_msg(new_fd, first_send);
            reply = recv_msg(new_fd, buffer);
            bool run_loop = true;

            if (first_send != "ERROR") {
                fseek(file, 0, SEEK_END);
                int numbytes = ftell(file);
                fseek(file, 0, SEEK_SET);
                fread(buffer, sizeof(char), numbytes, file);
                fclose(file);

                bool end_mess = true;
                std::string file_contents = "", temp, last_mess = "", handshake_msg, continue_get;
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
                    for (int j=start_index; j<end_index; j++)
                        send_string += file_contents[j];
                    
                    send_msg(new_fd, send_string);
                    reply = recv_msg(new_fd, buffer);

                    if (i == num_chunks-1) {
                        send_msg(new_fd, "LAST_MESSAGE");
                        reply = recv_msg(new_fd, buffer);
                        break;
                    }
                    else {
                        send_msg(new_fd, "MORE_MESSAGES_COMING");
                        reply = recv_msg(new_fd, buffer);
                    }
                    

                    continue_get = "CONTINUE";
                    file_table_mutex.lock();
                    for (int j=0; j<file_table.size(); j++) {
                        if (file_table[j].path == path) {
                            if (file_table[j].status == 'T') {
                                continue_get = "STOP";
                            }
                            break;
                        }
                    }
                    file_table_mutex.unlock();

                    send_msg(new_fd, continue_get);
                    reply = recv_msg(new_fd, buffer);

                    if (continue_get == "STOP")
                        break;
                }
            }
            
            // Cleanup to remove the path from file_table and fd from fd_table
            remove_new_fd(fd, new_fd);
            remove_file_object(path);
        }
        else {
            std::string temp = recv_msg(fd, buffer, MAXFILEBUFFER);
            send_file(fd, path, buffer);
            remove_file_object(path);
        }
    }
    
}

void remove_new_fd (int client_fd, int fd) {
    // Removes fd (file descriptor) from the list of open file descriptors of
    // client (client_fd)
    // client_fd = id of the client
    // fd = corresponding file descriptor of client to remove
    fd_table_mutex.lock();
    int index;
    for (int i=0; i<fd_table.size(); i++) {
        if (fd_table[i].client_fd == client_fd) {
            index = i;
            break;
        }
    }

    int fd_index=-1;
    std::vector<std::pair<int, bool> > temp = fd_table[index].fds;
    for (int i=0; i<temp.size(); i++) {
        if (temp[i].first == fd) {
            fd_index = i;
            break;
        }
    }
    if (fd_index == -1)
        fd_table[index].fds.erase(fd_table[index].fds.begin() + fd_index);
    fd_table_mutex.unlock();
}

void remove_file_object (std::string path) {
    // Remove the path from "file_table"
    file_table_mutex.lock();
    int index=-1;
    for (int i=0; i<file_table.size(); i++) {
        if (file_table[i].path == path) {
            index = i;
            break;
        }
    }
    if (index != -1)
        file_table.erase(file_table.begin()+index);
    file_table_mutex.unlock();
}