// tell.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>   // 為了使用 O_WRONLY 和 O_NONBLOCK
#include <errno.h>   // 為了使用 errno

#define USER_FILE "./tmp/userlist"
#define FIFO_DIR "./tmp/"
#define MAX_MESSAGE_LENGTH 512 // 最大訊息長度

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: tell user_id message\n");
        exit(1);
    }

    unsigned int uid, port, pid;
    char ip[16], name[30];

    // 獲取自己的父進程 ID（處理我們連接的伺服器進程）
    pid_t my_ppid = getppid();

    // 獲取自己的 uid 和名稱
    unsigned int my_uid = 0;
    int who_send_mes = -1;
    char my_name[30];

    // 打開 userlist 檔案
    FILE *fin = fopen(USER_FILE, "r");
    if (fin == NULL) {
        perror("fopen");
        exit(1);
    }

    // 找到自己的 uid 和名稱
    while (fscanf(fin, "%d %s %s %d %d", &uid, name, ip, &port, &pid) != EOF) {
        if (pid == (unsigned int)my_ppid) {
            my_uid = port;
            who_send_mes = uid;
            strncpy(my_name, name, sizeof(my_name) - 1);
            my_name[sizeof(my_name) - 1] = '\0'; // 確保以空字符結尾
            break;
        }
    }
    fclose(fin);

    if (my_uid == 0) {
        fprintf(stderr, "Error: Could not find your own uid\n");
        exit(1);
    }

    // 解析目標用戶 ID
    unsigned int target_uid = atoi(argv[1]);

    // 合併訊息參數
    char message[MAX_MESSAGE_LENGTH] = {0};
    for (int i = 2; i < argc; i++) {
        strncat(message, argv[i], MAX_MESSAGE_LENGTH - strlen(message) - 1);
        if (i < argc - 1)
            strncat(message, " ", MAX_MESSAGE_LENGTH - strlen(message) - 1);
    }

    // 找到目標用戶的 pid
    fin = fopen(USER_FILE, "r");
    if (fin == NULL) {
        perror("fopen");
        exit(1);
    }

    pid_t target_pid = 0;
    char target_name[30];
    int found = 0;
    while (fscanf(fin, "%d %s %s %d %d", &uid, name, ip, &port, &pid) != EOF) {
        if (uid == target_uid) {
            target_pid = pid;
            strncpy(target_name, name, sizeof(target_name) - 1);
            target_name[sizeof(target_name) - 1] = '\0';
            found = 1;
            break;
        }
    }
    fclose(fin);

    if (!found) {
        printf("User %d does not exist.\n", target_uid);
        exit(0);
    }

    // 以非阻塞的寫入模式開啟目標用戶的 FIFO
    char fifoName[100];
    snprintf(fifoName, sizeof(fifoName), "%sfifo_%d", FIFO_DIR, target_pid);
    int fifo_fd = open(fifoName, O_WRONLY | O_NONBLOCK);
    if (fifo_fd == -1) {
        if (errno == ENXIO) {
            fprintf(stderr, "Error: The receiver doesn't exist or is not available.\n");
        } else {
            perror("open fifo");
        }
        exit(1);
    }

    // 寫入訊息
    char fifo_buffer[1024];
    int written = snprintf(fifo_buffer, sizeof(fifo_buffer), "<user(%d) told you>: %s\n", who_send_mes, message);
    if (written >= sizeof(fifo_buffer)) {
        fprintf(stderr, "Warning: Message truncated.\n");
    }

    ssize_t bytes_written = write(fifo_fd, fifo_buffer, strlen(fifo_buffer));
    if (bytes_written == -1) {
        perror("write");
        close(fifo_fd);
        exit(1);
    }
    fprintf(stderr, "send accept!\n");
    close(fifo_fd);
    
    return 0;
}
