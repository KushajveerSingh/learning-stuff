# CSCI 6780 Distributed Computing Systems (Project 1)
Submitted by:
* Arthur LeBlanc (apl65875@uga.edu)
* Kushajveer Singh (ks56866@uga.edu)

## Declaration
This project was done in its entirety by **Arthur LeBlanc** and **Kushajveer Singh**. We hereby state that we have not received unauthorized help of any form.

## Compile instructions
* **server.cpp**

    `make compile-server` will compile *server.cpp* to produce the executable *myftpserver*, which can then be run as shown below (with port number = 8800)
    ```
    ./myftpserver 8800
    ```

* **client.cpp**

    `make compile-client` will compile *client.cpp* to produce the executable *myftp*, which can then be run as shown below (with given ip address and port number)
    ```
    ./myftp 127.0.0.1 8800
    ```

## Usage guide
Start the server using `./myftpserver 8800`. The prompt at server end becomes
```
(server) Started. Waiting for connections...
```

Now start the client using `./myftp 0.0.0.0 8800`. The client would be greeted with the following prompt
```
(client) Waiting for server to accept connection
```
The server will accept the connection, if it is serving no other client. In case the serve is busy with another client, this client can wait till server becomes free (at which point it will be automatically connected) or try connecting to the server after some time.

When the server accepts the client, the prompt at the client side becomes as shown below
```
(client) Waiting for server to accept connection
(client) Connection established with server at 0.0.0.0
mytftp> 
```

Now the client can start giving commands to the server to execute. The result of various commands is shown below for reference
```
mytftp> quit
(client) Connection closed with 0.0.0.0

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