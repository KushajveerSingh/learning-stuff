# CSCI 6780 Distributed Computing Systems (Project 4)

Submitted by
- Kushajveer Singh (ks56866@uga.edu)

## Declaration

This project was done in its entirety by **Kushajveer Singh**. We hereby state that we have not received unauthorized help of any form.

## Compilation instructions

### bnserver.cpp

Use the following command to compile the `bnserver.cpp`
```
make compile-bnserver
```

A utility command is also provided, which will first compile `bnserver.cpp` and then read the data from configuration file `bnConfigFile.txt`
```
make execute-bnserver
```

Syntax of commands is as follows
- **lookup [key]**

    ```
    > lookup 10
    Key not found
    Server IDs contacted = [0]
    Reponse from server ID = 0

    > lookup 12
    Value = Cabbage
    Server IDs contacted = [0]
    Reponse from server ID = 0

    > 
    ```

- **insert [key] [value]**

    ```
    > insert 10 Hello
    Server IDs contacted = [0]
    Value inserted in server ID = 0

    > 
    ```

- **delete [key]**

    ```
    > delete 10
    Successful deletion
    Server IDs contacted = [0]
    Deletion in server ID = 0

    > delete 10
    Key not found
    Server IDs contacted = [0]
    Deletion in server ID = 0

    > 
    ```

### nameserver.cpp

Use the following command to compile the `bnserver.cpp`
```
make compile-nameserver
```

A utility command is also provided, which will first compile `nameserver.cpp` and then read the data from configuration file `nsConfigFile{x}.txt` where `x = [1,2,3,4,5]`
```
make execute-nameserver1
```

Syntax of commands is as follows
- **enter**

    ```
    > enter
    successful entry
    Range of keys = [434,1023]
    Predecessor ID = 0
    Successor ID = 0
    Server IDs traversed = [0]
    > 
    ```

- **exit**

    ```
    > exit
    successful exit
    Successor ID = 0
    Key range = [434,1023]
    > 
    ```