#include "myhdr.h"
#include "ntools.h"
#include <mysql.h>
#include <mysqlsetting.h>

char pwd[1024]; 
int uid = 0;
char myIP[16];
int myPORT;
char prompt[150]; // 全域變數，用於保存 prompt
pid_t my_pid;     // 全域變數，用於保存進程 ID


#define USER_FILE "./tmp/userlist"
#define FIFO_DIR "./tmp/"

typedef struct commandType {
    char command[100];
    char paramater[100];
} command_t;

command_t *parser(char *commandStr) {
    command_t *cmd = (command_t *)malloc(sizeof(command_t));
    sscanf(commandStr, "%s", cmd->command);
    int x = strlen(cmd->command) + 1;
    sscanf(commandStr + x, "%[^\n]", cmd->paramater);
    return cmd;
}

extern char **environ;
int ExecCommand(char *string, int connfd);
void clearCommand(command_t *cmd);
void registerUser(int uid, const char *username, const char *ip_addr, int port, int pid);
void unregisterUser(int pid);


void printenv(char *parameter) {
    if (parameter == NULL || strlen(parameter) == 0) {
        char **ep;
        for (ep = environ; *ep != NULL; ep++) {
            printf("%s\n", *ep);
            fflush(stdout); // 刷新輸出緩衝區
        }
    } else {
        char *value = getenv(parameter);
        if (value) {
            printf("    %s\n", value);
            fflush(stdout); // 刷新輸出緩衝區
        } else {
            printf("\n");
            fflush(stdout); // 刷新輸出緩衝區
        }
    }
}


void Setenv(char *parameter) {
    char varName[5000] = {0}, varValue[100] = {0};
    sscanf(parameter, "%s %s", varName, varValue);

    if (strlen(varValue) == 0) {
        printf("No set value found.\n");
        fflush(stdout); // 確保輸出即時生效
    } else {
        printf("varName = %s\n", varName);
        printf("varValue = %s\n", varValue);
        printf("set %s in %s\n", varName, varValue);
        fflush(stdout); // 確保輸出即時生效
        setenv(varName, varValue, 1); // 設定環境變數
    }
}

int read_line(int fd, char *buffer, int maxlen) {
    int n = 0;
    while (n < maxlen - 1) {
        char c;
        int rc = read(fd, &c, 1);
        if (rc == 1) {
            buffer[n++] = c;
            if (c == '\n') {
                break;
            }
        } else if (rc == 0) {
            // EOF，連線被關閉
            break;
        } else {
            if (errno == EINTR) {
                continue; 
            }
            return -1; 
        }
    }
    buffer[n] = '\0';
    return n;
}

void strip_trailing_whitespace(char *str) {
    int len = strlen(str);
    while (len > 0 &&
           (str[len - 1] == '\n' ||
            str[len - 1] == '\r' ||
            isspace((unsigned char)str[len - 1]))) {
        str[len - 1] = '\0';
        len--;
    }
}



int isValidCommand(char *command) {
    if (command == NULL || strlen(command) == 0) {
        return 0;
    }

    // 提取命令名稱
    char commandName[256];
    sscanf(command, "%255s", commandName); // 確保不超過 255 字節

    // 檢查是否為內建命令
    if (strcmp(commandName, "printenv") == 0 || strcmp(commandName, "setenv") == 0) {
        return 1;
    }

    // 檢查外部命令
    char fullpath[256];
    int len = snprintf(fullpath, sizeof(fullpath), "./bin/%s", commandName);
    if (len >= sizeof(fullpath)) {
        return 0; // 路徑過長，視為無效命令
    }

    return access(fullpath, X_OK) == 0;
}




int flag = 0;// check wether the command is legal

void number_pipe(char* string, int pipe_num, int connfd) {
    char *pch;
    pch = strchr(string, '|');
    char cmd[100] = {0};
    strncpy(cmd, string, pch - string);
    cmd[pch - string] = '\0';

    char tempCommandStr[256] = {0};
    strcpy(tempCommandStr, cmd);

    char pipeformat[] = " | ";

    for (int a = 0; a < pipe_num; a++) {
        flag = 0; // for number pipe
        char NewcommandStr[5000] = {0};
        // 提示符
        write(connfd, "% ", 2);

        // 從 connfd 讀取輸入
        int n = read_line(connfd, NewcommandStr, sizeof(NewcommandStr));
        if (n <= 0) {
            a--; // 沒有讀到輸入，重試
            continue;
        }

        // 移除尾部的換行符和空白字符
        strip_trailing_whitespace(NewcommandStr);

        if (NewcommandStr[0] == '\0') {
            // 空命令，繼續下一次迴圈
            a--;
            continue;
        }

        // 提取命令名稱
        char commandName[256];
        sscanf(NewcommandStr, "%s", commandName);

        if (!isValidCommand(commandName)) {
            dprintf(connfd, "Unknown command: [%s].\n", commandName);
            continue;
        } else {
            char currentCommandStr[256] = {0};
            strcpy(currentCommandStr, tempCommandStr);
            strcat(currentCommandStr, pipeformat);
            strcat(currentCommandStr, NewcommandStr);

            if (ExecCommand(currentCommandStr, connfd) == 0 || flag == 1) {
                a--;
            } else {
                // 更新 tempCommandStr，為下一次迴圈做準備
                strcpy(tempCommandStr, currentCommandStr);
                continue;
            }
        }
    }
}


int refreshUserNameByPid(unsigned int pid, char *outName, size_t outSize) {
    // 這裡示範從 userlist 檔案讀
    FILE *fin = fopen(USER_FILE, "r");
    if (!fin) return 0;
    unsigned int uid, port, p;
    char ip_addr[16], name[30];
    while (fscanf(fin, "%d %s %s %d %d", &uid, name, ip_addr, &port, &p) != EOF) {
        if (p == pid) {
            strncpy(outName, name, outSize - 1);
            outName[outSize - 1] = '\0';
            fclose(fin);
            return 1; // 成功
        }
    }
    fclose(fin);
    return 0; // 沒找到
}


int ExecCommand(char *string, int connfd) {
    char *commands[256] = {0};
    int cmdCount = 0;
    int execFailure = 0;  // 用於記錄是否有命令執行失敗

    command_t *input = parser(string);

    char *pch = NULL;
    pch = strchr(input->paramater, '|');
    if (pch != NULL) {
        int pipe_num = 0;
        int flag = 1;
        for (int a = pch - (input->paramater) + 1; a < strlen(input->paramater); a++) {
            int num = input->paramater[a] - '0';
            if (num >= 0 && num <= 9) {
                pipe_num = pipe_num * 10 + num;
            } else {
                flag = 0;  // 檢查數字管道格式是否合法
            }
        }
        if (flag) {
            number_pipe(string, pipe_num, connfd);
            clearCommand(input);
            free(input);
            return 1;
        }
    }

    char *cmd = strtok(string, "|");
    while (cmd != NULL && cmdCount < 256) {
        while (*cmd == ' ') {
            cmd++;  // 移除開頭的空格
        }
        commands[cmdCount++] = cmd;
        cmd = strtok(NULL, "|");
    }

    int pipefd[2 * (cmdCount - 1)];

    for (int i = 0; i < cmdCount - 1; i++) {
        if (pipe(pipefd + 2 * i) == -1) {
            perror("pipe");
            clearCommand(input);
            free(input);
            return 0;
        }
    }

    for (int i = 0; i < cmdCount; i++) {
        char commandCopy[5000];
        strcpy(commandCopy, commands[i]);

        // 創建子進程來執行命令
        pid_t pid = fork();
        if (pid == 0) {
            // 子進程
            if (i > 0) {
                dup2(pipefd[2 * (i - 1)], STDIN_FILENO);
            } else {
                // 將 STDIN_FILENO 重定向到 connfd
                dup2(connfd, STDIN_FILENO);
            }
            if (i < cmdCount - 1) {
                dup2(pipefd[2 * i + 1], STDOUT_FILENO);
            } else {
                // 最後一個命令，確保輸出到 connfd
                dup2(connfd, STDOUT_FILENO);
                dup2(connfd, STDERR_FILENO);
            }
            for (int j = 0; j < 2 * (cmdCount - 1); j++) {
                close(pipefd[j]);
            }
            // 檢查是否為內建命令
            char *token = strtok(commandCopy, " ");
            if (strcmp(token, "printenv") == 0) {
                char *param = strtok(NULL, " ");
                printenv(param);
                exit(0);
            } else if (strcmp(token, "setenv") == 0) {
                char *param = strtok(NULL, "");
                Setenv(param);
                exit(0);
            }

            // 外部命令
            char *argVec[256] = {0};
            int argc = 0;
            token = strtok(commands[i], " ");
            while (token != NULL) {
                argVec[argc++] = token;
                token = strtok(NULL, " ");
            }
            argVec[argc] = NULL;

            char fullpath[256] = "./bin/";
            strcat(fullpath, argVec[0]);
            execv(fullpath, argVec);

            // 如果 execv 失敗
            fprintf(stderr, "Unknown command: [%s].\n", argVec[0]);
            exit(1);
        }
    }

    // 父進程關閉所有管道並等待子進程結束
    for (int i = 0; i < 2 * (cmdCount - 1); i++) {
        close(pipefd[i]);
    }
    for (int i = 0; i < cmdCount; i++) {
        int status;
        wait(&status);
        if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
            execFailure = 1;
        }
    }

    if (strncmp(commands[0], "name", 4) == 0) {
        // 如果執行的是 name 指令，更新 prompt
        char newName[30];
        if (refreshUserNameByPid(my_pid, newName, sizeof(newName))) {
            snprintf(prompt, sizeof(prompt), "(%s) %% ", newName);
        }
    } else {
        // 其他指令時才印 prompt
        write(connfd, prompt, strlen(prompt));
    }

    clearCommand(input);
    free(input);
    return execFailure ? 0 : 1;
}



void sig_handler(int sig) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

void clearString(char *str) {
    memset(str, 0, strlen(str));
}

void clearCommand(command_t *cmd) {
    if (cmd != NULL){
        clearString(cmd->command);
        clearString(cmd->paramater);
    }
}


int getNextUID() {
    FILE *fin = fopen(USER_FILE, "r");
    if (fin == NULL) {
        perror("fopen");
        return 1; // 如果檔案不存在，從 1 開始分配 UID
    }

    int uid, port, pid;
    char name[30], ip_addr[16];
    int minUID = 1; // 從 1 開始尋找最小的可用 UID
    int isUsed;     // 標記當前 UID 是否已被使用

    while (1) {
        rewind(fin); // 將檔案指標重置到檔案開頭
        isUsed = 0;  // 初始化標記為未使用

        // 檢查當前 minUID 是否已被使用
        while (fscanf(fin, "%d %s %s %d %d", &uid, name, ip_addr, &port, &pid) != EOF) {
            if (uid == minUID) {
                isUsed = 1; // 當前 UID 已被使用
                break;
            }
        }

        if (!isUsed) {
            // 如果 minUID 未被使用，則找到可用 UID
            break;
        }

        // 否則繼續檢查下一個 UID
        minUID++;
    }

    fclose(fin);
    return minUID; // 返回找到的最小可用 UID
}


void finish(MYSQL *con, int errorCode) {
    if (con != NULL) {
        mysql_close(con); // 釋放 MySQL 連接資源
    }
    exit(errorCode);
}


void register_account(MYSQL *con, int connfd) {
    char username[100], password[100];

    while (1) {
        write(connfd, "Your user name: ", 16);
        read_line(connfd, username, sizeof(username));

        char sql[512];
        snprintf(sql, sizeof(sql), "SELECT * FROM users WHERE user_name='%s'", username);
        if (mysql_query(con, sql)) {
            dprintf(connfd, "MySQL query failed: %s\n", mysql_error(con));
            finish(con, 1);
        }

        MYSQL_RES *result = mysql_store_result(con);
        if (result == NULL) {
            dprintf(connfd, "MySQL result retrieval failed: %s\n", mysql_error(con));
            finish(con, 1);
        }

        if (mysql_num_rows(result) > 0) {
            dprintf(connfd, "User name already exists!\n");
            mysql_free_result(result);
        } else {
            mysql_free_result(result);
            break;
        }
    }

    write(connfd, "Your password: ", 15);
    read_line(connfd, password, sizeof(password));

    char sql[512];
    snprintf(sql, sizeof(sql), "INSERT INTO users (user_name, user_password) VALUES ('%s', '%s')", username, password);
    if (mysql_query(con, sql)) {
        dprintf(connfd, "Failed to create account: %s\n", mysql_error(con));
        finish(con, 1);
    }

    dprintf(connfd, "Account created successfully!\n");
}



/** 
 * server: 主要負責 handle 用戶連線 -> 登入 -> 命令執行 -> 結束
 */
void server(int connfd, struct sockaddr_in *cliaddr, int uid) {
    char commandStr[5000] = {0};
    char username[100], password[100], db_password[256];
    char prompt[150];

    setenv("PATH", "bin:.", 1);
    command_t *cmd = NULL;
    inet_ntop(AF_INET, &cliaddr->sin_addr, myIP, sizeof(myIP));
    myPORT = ntohs(cliaddr->sin_port);
    getcwd(pwd, sizeof(pwd));

    pid_t my_pid = getpid();
    char fifoName[100];
    snprintf(fifoName, sizeof(fifoName), "%sfifo_%d", FIFO_DIR, my_pid);

    if (mkfifo(fifoName, 0666) == -1) {
        perror("mkfifo");
        exit(1);
    }

    int fifo_fd = open(fifoName, O_RDONLY | O_NONBLOCK);
    if (fifo_fd == -1) {
        perror("open fifo");
    }

    registerUser(uid, username, myIP, myPORT, my_pid);

    /** 登入流程 */
    while (1) {
        write(connfd, "user_name: ", 11);
        fsync(connfd);
        if (read_line(connfd, username, sizeof(username)) <= 0) break;
        strip_trailing_whitespace(username);

        write(connfd, "password: ", 10);
        fsync(connfd);
        if (read_line(connfd, password, sizeof(password)) <= 0) break;
        strip_trailing_whitespace(password);

        char *args[] = {"./bin/login", username, password, NULL};
        int pipefd[2];
        if (pipe(pipefd) == -1) {
            perror("pipe");
            close(connfd);
            return;
        }

        pid_t pid = fork();
        if (pid == 0) {
            close(pipefd[0]);
            dup2(pipefd[1], STDOUT_FILENO);
            close(pipefd[1]);
            execvp(args[0], args);
            perror("execvp failed");
            exit(1);
        } else {
            close(pipefd[1]);
            waitpid(pid, NULL, 0);
            memset(db_password, 0, sizeof(db_password));
            read(pipefd[0], db_password, sizeof(db_password) - 1);
            close(pipefd[0]);
            strip_trailing_whitespace(db_password);

            if (strcmp(db_password, "NOTFOUND") == 0) {
                write(connfd, "User not found. Create account or login again? <1/2>: ", 53);
                fsync(connfd);

                char choiceBuf[16] = {0};
                if (read_line(connfd, choiceBuf, sizeof(choiceBuf)) <= 0) {
                    continue;
                }
                strip_trailing_whitespace(choiceBuf);

                if (choiceBuf[0] != '1') {
                    continue;
                }

                MYSQL *con = mysql_init(NULL);
                mysql_options(con, MYSQL_SET_CHARSET_NAME, "utf8");
                mysql_options(con, MYSQL_INIT_COMMAND, "SET NAMES utf8");
                if (con == NULL || 
                    mysql_real_connect(con, "localhost", "root", "justin0706", "mydb", 0, NULL, 0) == NULL) {
                    dprintf(connfd, "MySQL connection failed.\n");
                    if (con) mysql_close(con);
                    continue;
                }

                write(connfd, "your user name: ", 16);
                fsync(connfd);

                char newUsername[100] = {0};
                if (read_line(connfd, newUsername, sizeof(newUsername)) <= 0) {
                    mysql_close(con);
                    continue;
                }
                strip_trailing_whitespace(newUsername);

                write(connfd, "your password: ", 15);
                fsync(connfd);

                char newPassword[100] = {0};
                if (read_line(connfd, newPassword, sizeof(newPassword)) <= 0) {
                    mysql_close(con);
                    continue;
                }
                strip_trailing_whitespace(newPassword);

                char sql[512];
                snprintf(sql, sizeof(sql),
                         "INSERT INTO users (user_name, user_password) VALUES ('%s', '%s')",
                         newUsername, newPassword);
                if (mysql_query(con, sql)) {
                    dprintf(connfd, "Failed to create account: %s\n", mysql_error(con));
                } else {
                    write(connfd, "Create success!\n", 17);
                }
                mysql_close(con);
                continue;
            }

            if (strcmp(db_password, password) == 0) {
                // write(connfd, "Login successful!\n", 19);

                setenv("CURRENT_USER", username, 1);

                char pidStr[32];
                snprintf(pidStr, sizeof(pidStr), "%d", my_pid);
                setenv("CURRENT_PID", pidStr, 1);

                registerUser(uid, username, myIP, myPORT, my_pid);

                MYSQL *con = mysql_init(NULL);
                if (mysql_real_connect(con, ip, user_name, user_password, database_name, 0, NULL, 0)) {
                    char sql[256];
                    snprintf(sql, sizeof(sql), "UPDATE users SET pid=%d WHERE user_name='%s'", my_pid, username);
                    if (mysql_query(con, sql)) {
                        dprintf(connfd, "Failed to update PID: %s\n", mysql_error(con));
                    }
                    mysql_close(con);
                } else {
                    dprintf(connfd, "MySQL connection failed.\n");
                }

                snprintf(prompt, sizeof(prompt), "(%s) %% ", username);
                break;
            } else {
                write(connfd, "Password error.\n", 17);
            }
        }
    }

    // 主循環處理命令
    fd_set readfds;
    int maxfd = (connfd > fifo_fd) ? connfd : fifo_fd;
    int shouldPrintPrompt = 1; // 新增變數，用來控制 prompt 是否要打印
    do {
        if (shouldPrintPrompt) {
            write(connfd, prompt, strlen(prompt));
            shouldPrintPrompt = 0; // 清除標誌，防止重複打印
        }
        FD_ZERO(&readfds);
        FD_SET(connfd, &readfds);
        FD_SET(fifo_fd, &readfds);

        int retval = select(maxfd + 1, &readfds, NULL, NULL, NULL);
        if (retval == -1) {
            perror("select");
            break;
        }

        if (FD_ISSET(connfd, &readfds)) {
            int n = read_line(connfd, commandStr, sizeof(commandStr));
            if (n <= 0) break;

            strip_trailing_whitespace(commandStr);
            if (commandStr[0] == '\0') continue;

            if (strcmp(commandStr, "quit") == 0) break;

            cmd = parser(commandStr);
            if (isValidCommand(cmd->command)) {
                if (strcmp(cmd->command, "listMail") == 0) {
                    char fullpath[256];
                    snprintf(fullpath, sizeof(fullpath), "./bin/listMail");
                    char *args[] = {fullpath, username, NULL};

                    pid_t pid = fork();
                    if (pid == 0) {
                        dup2(connfd, STDOUT_FILENO);
                        dup2(connfd, STDERR_FILENO);
                        execvp(fullpath, args);
                        dprintf(connfd, "Failed to execute: %s\n", cmd->command);
                        exit(1);
                    } else {
                        waitpid(pid, NULL, 0);
                    }
                } else if (strcmp(cmd->command, "mailto") == 0) {
                    char *receiver = strtok(cmd->paramater, " ");
                    if (!receiver) {
                        dprintf(connfd, "Usage: mailto <user_name> message or mailto <user_name> < command\n");
                        continue;
                    }

                    char *second = strtok(NULL, " ");
                    if (!second) {
                        dprintf(connfd, "Error: Missing message.\n");
                        continue;
                    }

                    if (strcmp(second, "<") == 0) {
                        char *cmdline = strtok(NULL, "");
                        if (!cmdline) {
                            dprintf(connfd, "Error: No command provided for redirect.\n");
                            continue;
                        }

                        char *args[] = {"./bin/mailto", receiver, "<", cmdline, NULL};
                        pid_t pid = fork();
                        if (pid == 0) {
                            dup2(connfd, STDOUT_FILENO);
                            dup2(connfd, STDERR_FILENO);
                            execvp(args[0], args);
                            dprintf(connfd, "Failed to execute: %s\n", cmd->command);
                            exit(1);
                        } else {
                            waitpid(pid, NULL, 0);
                        }
                    } else {
                        char message[1024];
                        memset(message, 0, sizeof(message));
                        snprintf(message, sizeof(message), "%s", second);

                        char *rest = strtok(NULL, "");
                        if (rest) {
                            strncat(message, " ", sizeof(message) - strlen(message) - 1);
                            strncat(message, rest, sizeof(message) - strlen(message) - 1);
                        }

                        char *args[] = {"./bin/mailto", receiver, message, NULL};
                        pid_t pid = fork();
                        if (pid == 0) {
                            dup2(connfd, STDOUT_FILENO);
                            dup2(connfd, STDERR_FILENO);
                            execvp(args[0], args);
                            dprintf(connfd, "Failed to execute: %s\n", cmd->command);
                            exit(1);
                        } else {
                            waitpid(pid, NULL, 0);
                        }
                    }
                } else {
                    ExecCommand(commandStr, connfd);
                }
            } else {
                dprintf(connfd, "Unknown command: [%s].\n", cmd->command);
            }
            shouldPrintPrompt = 1; // 指令執行後，允許打印下一個 prompt
        }

        if (FD_ISSET(fifo_fd, &readfds)) {
            char fifo_buffer[1024];
            int n = read(fifo_fd, fifo_buffer, sizeof(fifo_buffer) - 1);
            if (n > 0) {
                fifo_buffer[n] = '\0';

                // 過濾 END_OF_MESSAGE 標記
                char *end_marker = strstr(fifo_buffer, "END_OF_MESSAGE");
                if (end_marker) {
                    *end_marker = '\0'; // 移除 END_OF_MESSAGE
                }

                // 發送過濾後的訊息給客戶端
                write(connfd, fifo_buffer, strlen(fifo_buffer));
                shouldPrintPrompt = 1;  
            } else if (n == 0) {
                close(fifo_fd);
                fifo_fd = open(fifoName, O_RDONLY | O_NONBLOCK);
                if (fifo_fd == -1) {
                    perror("open fifo");
                    break;
                }
                maxfd = (connfd > fifo_fd) ? connfd : fifo_fd;
            }
        }
    } while (1);

    close(fifo_fd);
    unlink(fifoName);
    unregisterUser(my_pid);
    close(connfd);
}



void registerUser(int uid, const char *username, const char *ip_addr, int port, int pid) {
    // 暫存用戶資訊
    struct User {
        unsigned int uid;
        char name[30];
        char ip_addr[16];
        unsigned int port;
        unsigned int pid;
    } users[100]; // 假設最多支持 100 個用戶

    int user_count = 0;

    // 讀取現有的用戶資訊
    FILE *fin = fopen(USER_FILE, "r");
    if (fin != NULL) {
        while (fscanf(fin, "%d %s %s %d %d", 
                      &users[user_count].uid, 
                      users[user_count].name, 
                      users[user_count].ip_addr, 
                      &users[user_count].port, 
                      &users[user_count].pid) != EOF) {
            user_count++;
        }
        fclose(fin);
    }

    // 查找是否存在相同 PID 或名稱的用戶
    int found = 0;
    for (int i = 0; i < user_count; i++) {
        if (users[i].pid == pid) { // 匹配 PID
            strncpy(users[i].name, username, sizeof(users[i].name) - 1);
            users[i].name[sizeof(users[i].name) - 1] = '\0';
            strncpy(users[i].ip_addr, ip_addr, sizeof(users[i].ip_addr) - 1);
            users[i].ip_addr[sizeof(users[i].ip_addr) - 1] = '\0';
            users[i].port = port;
            found = 1;
            break;
        }
    }

    // 如果未找到，則新增用戶
    if (!found) {
        users[user_count].uid = uid;
        strncpy(users[user_count].name, username, sizeof(users[user_count].name) - 1);
        users[user_count].name[sizeof(users[user_count].name) - 1] = '\0';
        strncpy(users[user_count].ip_addr, ip_addr, sizeof(users[user_count].ip_addr) - 1);
        users[user_count].ip_addr[sizeof(users[user_count].ip_addr) - 1] = '\0';
        users[user_count].port = port;
        users[user_count].pid = pid;
        user_count++;
    }

    // 按 UID 排序
    for (int i = 0; i < user_count - 1; i++) {
        for (int j = 0; j < user_count - i - 1; j++) {
            if (users[j].uid > users[j + 1].uid) {
                struct User temp = users[j];
                users[j] = users[j + 1];
                users[j + 1] = temp;
            }
        }
    }

    // 寫回排序後的用戶清單
    FILE *fout = fopen(USER_FILE, "w");
    if (fout == NULL) {
        perror("fopen");
        return;
    }
    for (int i = 0; i < user_count; i++) {
        fprintf(fout, "%d %s %s %d %d\n",
                users[i].uid, 
                users[i].name, 
                users[i].ip_addr, 
                users[i].port, 
                users[i].pid);
    }
    fclose(fout);
}


void unregisterUser(int pid) {
    // 讀取 userlist，移除對應 pid 的用戶資訊
    FILE *fin = fopen(USER_FILE, "r");
    if (fin == NULL) {
        perror("fopen");
        return;
    }

    // 暫存所有用戶資訊
    struct User {
        unsigned int uid;
        char name[30];
        char ip_addr[16];
        unsigned int port;
        unsigned int pid;
    } users[100];

    int user_count = 0;
    unsigned int uid, port, user_pid;
    char ip_addr[16], name[30];

    while (fscanf(fin, "%d %s %s %d %d", &uid, name, ip_addr, &port, &user_pid) != EOF) {
        if (user_pid != pid) {
            users[user_count].uid = uid;
            strcpy(users[user_count].name, name);
            strcpy(users[user_count].ip_addr, ip_addr);
            users[user_count].port = port;
            users[user_count].pid = user_pid;
            user_count++;
        }
    }
    fclose(fin);

    // 寫回更新後的用戶列表
    FILE *fout = fopen(USER_FILE, "w");
    if (fout == NULL) {
        perror("fopen");
        return;
    }

    for (int i = 0; i < user_count; i++) {
        fprintf(fout, "%d %s %s %d %d\n",
                users[i].uid, users[i].name, users[i].ip_addr, users[i].port, users[i].pid);
    }
    fclose(fout);

    // 刪除 FIFO
    char fifoName[100];
    snprintf(fifoName, sizeof(fifoName), "%sfifo_%d", FIFO_DIR, pid);
    unlink(fifoName);
}


int main() {
    int listenfd, connfd;
    struct sockaddr_in servaddr, cliaddr;
    char buff[100];

    signal(SIGCHLD, sig_handler);

    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket error");
        exit(1);
    }

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(LISTEN_PORT);

    if (bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
        perror("bind error");
        exit(1);
    }

    if (listen(listenfd, 5) != 0) {
        perror("listen error");
        exit(0);
    }

    uid = 0; // 初始化 uid

    while (1) {
        int len = sizeof(cliaddr);
        connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &len); // 接受新連線
        if (connfd > 0) {
            fprintf(stderr, "connection from %s, port %d\n",
                    inet_ntop(AF_INET, &cliaddr.sin_addr, buff, sizeof(buff)),
                    ntohs(cliaddr.sin_port));

            int uid = getNextUID(); // 動態計算新的 UID

            int child = fork(); // 建立子進程
            if (child == 0) {
                close(listenfd); // 子進程關閉監聽 Socket
                server(connfd, &cliaddr, uid); // 傳遞 UID 給伺服器
                exit(0);
            } else {
                close(connfd); // 父進程關閉客戶端連線
            }
        }
    }

    return 0;
}