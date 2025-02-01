#include "network.hpp"

const int MAX_CONNECTIONS = 1;
const char *SERVER_ADDRESS = "127.0.0.1";

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