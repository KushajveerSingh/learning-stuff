
#include "n2ta_network.hpp"

const int MAX_CONNECTIONS = 1;
const char *SERVER_ADDRESS = "127.0.0.1";
// const mode_t write_access = 0000700; //allows write access for 'group', mkdir

void sigchld_handler(int s) {
    int saved_errno = errno;
    while(waitpid(-1, NULL, WNOHANG) > 0);
    errno = saved_errno;
}

void *get_ip_addr(struct sockaddr *sa) {
    // Overload the function from "network.hpp"
    struct sockaddr_in *ip_addr_info = (struct sockaddr_in *)sa;
    return &(ip_addr_info->sin_addr);
}

void send_system_msg (int fd, char *buf, char *file_buf) {
    string str_return = "";
    string command = (string)buf + " 2>&1";
    FILE *file = popen(command.c_str(), "r");
    if (file == NULL)
        throw runtime_error("error opening file with popen");

    while (fgets(file_buf, MAXFILEBUFFER, file) != NULL)
        str_return += file_buf;

    if (pclose(file) < 0) {}
        // perror("(server) pclose");
    
    if (str_return.size() == 0)
        str_return = "NO_MESSAGE";

    send_msg(fd, str_return);
}


bool cd(string dir){
    // change directories of server to dir
    chdir(dir.c_str());
    return 0;
}
