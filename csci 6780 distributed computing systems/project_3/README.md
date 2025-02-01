# CSCI 6780 Distributed Computing Systems (Project 3)

Submitted by
- Kushajveer Singh (ks56866@uga.edu)

## Declaration

This project was done in its entirety by **Kushajveer Singh**. We hereby state that we have not received unauthorized help of any form.

## Compile instructions

- coordinator

    To compile `coordinator.cpp`, use this command
    ```
    make compile-coordinator

    OR

    g++ --std=c++11 -pthread coordinator.cpp -o coordinator.out
    ```

    To run `coordinator` use this command. It will compile the file and use the configuration provided in `coordinator.txt`.
    ```
    make execute-coordinator
    ```

    Format of `coordinator.txt`
    ```
    8800
    15
    ```
    
    First line indicates the port at which participants will connect to and the second line refers to time threshold beyond which the participants should not receive messages if they are disconnected.

- participant

    To compile `participant.cpp`, use this command
    ```
    make compile-participant

    OR

    g++ --std=c++11 -pthread participant.cpp -o participant.out
    ```

    To run `participant` use this command. It will compile the file and use the configuration provided in `participant1.txt` and `participant2.txt`
    ```
    make execute-participant1

    AND

    make execute-participant2
    ```

    Format of `participant1.txt`
    ```
    101
    101_log.txt
    0.0.0.0:8800
    ```

    First line refers to the ID of participant, second line refers to the name of log file where the messages will be stored and third line refers to ip address and port number of server (separated by `:`).