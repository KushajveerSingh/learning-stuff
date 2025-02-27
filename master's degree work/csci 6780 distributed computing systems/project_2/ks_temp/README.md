# CSCI 6780 Distributed Computing Systems (Project 2)
Submitted by:
* Arthur LeBlanc (apl65875@uga.edu)
* Kushajveer Singh (ks56866@uga.edu)

## Declaration
This project was done in its entirety by **Arthur LeBlanc** and **Kushajveer Singh**. We hereby state that we have not received unauthorized help of any form.

## Compile instructions
* **server.cpp**

    `make compile-server` will compile *server.cpp* to produce the executable *myftpserver*, which can then be run as shown below
    ```
    ./myftpserver 8800 9800
    ```

* **client.cpp**

    `make compile-client` will compile *client.cpp* to produce the executable *myftp*, which can then be run as shown below
    ```
    ./myftp 127.0.0.1 8800 9800
    ```

## Code explanation
these are the updated commands and usage since project 1. Usage of project 1 commands is appeded at the bottom of this README

### Daemon Threads
get and put commands can now be created using & after the file name:
```
get file1.txt &
```
large files will be transferred using sizes around 1kb for each chunk.


### Ports used
Internally there ports are used to handle all the connections
* **NPORT** (provided by user): Accepts client connections and then creates a new thread to handle the client commands.
* **TPORT** (provided by user): When a client needs to terminate a command, the client will connect to this socket and give the ID.
* **EPORT** (internally set to `9093`): This is an extra port that was needed to implement some of the functionality of the project. The value of this is stored in `network.hpp` under `const char *EXTRA_PORT = "9093"`. When the server receives a command that end's with `&`, it must create a new thread and that thread should connect with the client thread. This port is used to implement this functionality.

### Maximum number of clients
The limit is set in `server.hpp` under `const int MAX_CLIENTS = 2`. If the client is able to connect to the server then the following output is shown
```
(client) Connection established with server
mytftp> 
```
If the server does not accept the connection (due to number of active clients increasing than max allowed limit), then the following message is shown and the client execution stops.
```
(client) Maximum connections reached on server. Try after some time.
```

### Handling concurrency on client side
In `client.cpp`, `std::vector<std::string> file_table` and `std::mutex file_table_mutex` are the two main structures used to implement this functionality. Here, `file_name` refers to the argument of (get, put, delete).
If a thread has the mutex lock on the file name, other threads will be unable to access the file immediately, and must wait (in 1 second increments) until the mutex lock on the file name has been unlocked.
```
function check_table(file_name)
    if file_name in file_table
        // Some other thread is already working on this file
        sleep for 1 second
        check_table(file_name)
    else
        add file_name to file_table
        carry out operation
        remove file_name from file_table
```

### Handling concurrency on server side
In `server.cpp`, `std::vector<file_object> file_table` and `std::mutex file_table_mutex` are the two main structures used to implement his functionality. Every file is stored as `file_object` which has the following implementation
```
struct file_object {
    int id;              // Will store the FILE_ID that is sent to the client
    std::string path;    // Absolute path of file
    std::string command; // get, put, delete
    char status;         // 'A' or 'T'
};
```

To check for concurrency, we simply check if `path` is already in the `file_table`. If it is, then it means some other thread is working on that file. Similar to client side mutex locks, once a thread has control of the mutex lock, other threads must wait 1s before checking if the lock is open and gain control of the lock to access the path. Then, we can carry out the desired operation. The logic can be summarized as follows (here `file_name` refers to the argument of get, put, delete)
```
path = current_directory + "/" + file_name
function check_table(path)
    if path in file_table
        // Some other thread is already working on this file
        sleep for 1 second
        check_table(path)
    else
        add path to file_table
        carry out operation
        remove path from file_table
```

ID's are assigned starting from 1.

### How EPORT works and connections with threads?
If the server receives a command, say `put file.txt &`. Then the server and client must create a new thread, a connection should be established between these two threads and then the operation is carried out in the background.

First we give each client a new ID which is same as the file descriptor that server uses to send messages to client. So,
```
fd = accept(...)
// fd is the unique ID of the client (and the client knows this ID)
```
When a client issues a terminate command, we create a new thread on client and that thread connects to the server on `EPORT` and transfer the unique ID to the server.

Now on the server side, we have a separate thread listening for connections on the EPORT. When it receives a connection, it adds the file descriptor of the connection to a table where we maintain the open file descriptors for each client. In this way, a thread on the server on the server can use the correct file descriptor to send commands to the client.

### Handling quit command
If all background threads have completed processing or are terminated, the client will stop execution. 
```
mytftp> quit
(client) Connection closed with server
```
To handle the case where the client issues a long file transfer command and then calls quit, we simply added a warning message that you cannot quit due to running background processes. The client should either wait for the completion of these commands or terminate them manually.
```
mytftp> put some_big_file &
Id: 1
mytftp> quit
Close background process using 'terminate {ID}' first
mytftp> 
```

#project 1 commands
mytftp> pwd
/home/kushaj/Desktop/Github/working/project_1/

mytftp> mkdir temp

mytftp> cd temp

mytftp> ls
. .. Makefile client.cpp myftp myftpserver network.hpp server.cpp server.hpp temp

mytftp> delete temp

echo "This is a cat" > cat.txt // Run at client location

mytftp> put cat.txt
File transfer successful

mytftp> get cat.txt
File transfer complete
```

## Maximum buffer size
In `network.hpp` we have defined the following defaults
```
#define MAXDATASIZE 1024
#define MAXFILEBUFFER 4096
```
* **MAXDATASIZE** refers to the maximum allowed length of user command (which means the command `cd some_dir`, should have length less than this value)
* **MAXFILEBUFFER** is the buffer size we use to communicate between the client and server for everything (except user input). This includes all the error message, file transfer strings. In case you want to transfer a file that has size more than 4KB, modify *MAXFILEBUFFER* to an appropriate value.
