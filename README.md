# Network Chat Server

[繁體中文](README_TW.md) | English

This project is a network chat server implemented in C language using the TCP/IP protocol, running on a Linux platform.

## Project Structure

```
server
    |-- bin/           // Executable commands
    |-- include/       // All .h files
    |-- object/        // Object files
    |-- src/           // Source code
    |-- tmp/
    |   |-- userlist   // Records users on the server and contains FIFO files
    |-- init_mysql.sql // Initializes the MySQL database
    |-- Makefile       // Script for compilation
    |-- `shell`        // Executable file
```

## Running Guide

For Windows systems, use WSL. Navigate to the project directory (./server) and execute the `shell` file. Then, open another terminal tab and connect to the server listener with the following command:

```bash
telnet 127.0.0.1 6000
```

## Server Command Features

- **quit**

    > Usage: Exit the server.

- **who**

    > Usage: Display information about all users.

- **tell**

    > Usage: Send a message to a specific user.  

    > Format: `tell` `user_id` `message`

- **yell**

    > Usage: Broadcast a message.  

    > Format: `yell` `message`

- **name**

    > Usage: Change your name using this command.  

    > Format: `name` `new name`

- **listMail**

    > Usage: List all emails in your mailbox.  

- **mailto**

    > Usage: Send an email to someone.  

    > Format: 
    > `mailto` `user_name` `message`
    >> or  
    >> `mailto` `user_name` `< ls`  
    >> `<` means redirect the output of `ls` to `user_name`.

- **delMail**

    > Usage: Delete an email from your mailbox.  

    > Format: `delMail` `mail_id`

- **gyell**

    > Usage: Similar to `yell`, but `gyell` broadcasts messages to users in the same group instead of all online users.  

    > Format: `gyell` `group_name` `message`

- **createGroup**

    > Usage: Create a group.  

    > Format: `createGroup` `group_name`

- **delGroup**

    > Usage: Delete a group. Only the owner can delete it.  

    > Format: `delGroup` `group_name`

- **addTo**

    > Usage: Invite users to a group. Only the owner can invite others.  

    > Format: `addTo` `group_name` `user_name1` `user_name2` …  

- **leaveGroup**

    > Usage: Leave a group.  

    > Format: `leaveGroup` `group_name`

- **listGroup**

    > Usage: List groups you belong to.  

- **remove**

    > Usage: Remove a user from a group. Only the group owner can do this.  

    > Format: `remove` `group_name` `user_name` `user_name` …  

## Makefile Syntax

- `make all`: Compile all files under `src/` and place the executable files in the `bin/` folder.

- `make clean`: Clean up all executable files in the `bin/` folder, except those not under `src/`.

## References

This project is an assignment for the 2024 Fall Network and Programming course at Dong Hwa University. [Assignment Link](https://hackmd.io/@chtsai/networkProgramming)
