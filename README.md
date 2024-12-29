# Network Chat Server

[繁體中文](README.md) | English

This project is a network chat server built in C, using the TCP/IP protocol. It runs on a Linux platform.

---

## Project Structure

```
server
├── bin/           # Executable commands
├── include/       # Header files
├── object/        # Object files
├── src/           # Source code
├── tmp/
│   ├── userlist   # Tracks current users and stores FIFO files
├── init_mysql.sql # MySQL database initialization script
├── Makefile       # Compilation script
└── shell          # Main executable file
```

---

## How to Run

1. For **Windows systems**, use **WSL**.
2. Navigate to the project directory:
   ```bash
   cd ./server
   ```
3. Execute the `shell` program:
   ```bash
   ./shell
   ```
4. In another terminal window, connect to the server:
   ```bash
   telnet 127.0.0.1 6000
   ```

---

## Available Commands

| Command        | Description                                                    | Format                              |
|----------------|----------------------------------------------------------------|-------------------------------------|
| `quit`         | Exit the server.                                              |                                     |
| `who`          | Show all active users.                                        |                                     |
| `tell`         | Send a private message to a specific user.                    | `tell <user_id> <message>`         |
| `yell`         | Broadcast a message to all users.                             | `yell <message>`                   |
| `name`         | Change your username.                                         | `name <new_name>`                  |
| `listMail`     | Display all emails in your mailbox.                           |                                     |
| `mailto`       | Send an email to a specific user.                             | `mailto <user_name> <message>`     |
| `delMail`      | Delete an email from your mailbox.                            | `delMail <mail_id>`                |
| `gyell`        | Broadcast a message to your group.                            | `gyell <group_name> <message>`     |
| `createGroup`  | Create a new group.                                           | `createGroup <group_name>`         |
| `delGroup`     | Delete a group (only by the owner).                           | `delGroup <group_name>`            |
| `addTo`        | Invite users to a group (only by the owner).                  | `addTo <group_name> <user1> ...`   |
| `leaveGroup`   | Leave a group.                                                | `leaveGroup <group_name>`          |
| `listGroup`    | Display the groups you belong to.                             |                                     |
| `remove`       | Remove users from a group (only by the owner).                | `remove <group_name> <user1> ...`  |

---

## Makefile Usage

- **Compile All**: Compile all files in the `src/` directory and output executables to `bin/`.
  ```bash
  make all
  ```

- **Clean Executables**: Remove all executables in the `bin/` folder (excluding non-`src/` files).
  ```bash
  make clean
  ```

---

## References

This project was developed as part of the **2024 Dong Hwa University Fall Network Programming** course.  
For more details, see the [assignment description](https://hackmd.io/@chtsai/networkProgramming).
