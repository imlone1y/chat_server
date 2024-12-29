#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <mysql/mysql.h>
#include <mysqlsetting.h>

#define USER_FILE "./tmp/userlist"

// 更新資料庫中的 user_name
int update_database_username_by_pid(unsigned int pid, const char *new_name) {
    MYSQL *conn = mysql_init(NULL);
    if (conn == NULL) {
        fprintf(stderr, "mysql_init() failed\n");
        return 0;
    }
    if (mysql_real_connect(conn, ip, user_name, user_password, database_name, 0, NULL, 0) == NULL) {
        fprintf(stderr, "mysql_real_connect() failed: %s\n", mysql_error(conn));
        mysql_close(conn);
        return 0;
    }

    char query[512];
    snprintf(query, sizeof(query),
             "UPDATE users SET user_name='%s' WHERE pid=%u",
             new_name, pid);

    if (mysql_query(conn, query)) {
        if (mysql_errno(conn) == 1062) { // Duplicate entry
            printf("User name already exists!\n");
        } else {
            fprintf(stderr, "MySQL UPDATE error: %s\n", mysql_error(conn));
        }
        mysql_close(conn);
        return 0;
    }

    mysql_close(conn);
    return 1;
}

// 更新 userlist 中的 user_name
int update_userlist_name_by_pid(unsigned int pid, const char *new_name) {
    int fd = open(USER_FILE, O_RDWR);
    if (fd == -1) {
        perror("open");
        return 0;
    }

    struct flock lock;
    memset(&lock, 0, sizeof(lock));
    lock.l_type = F_WRLCK;
    fcntl(fd, F_SETLKW, &lock);

    FILE *fin = fdopen(fd, "r+");
    if (fin == NULL) {
        perror("fdopen");
        close(fd);
        return 0;
    }

    struct User {
        unsigned int uid;
        char name[30];
        char ip_addr[16];
        unsigned int port;
        unsigned int pid;
    } users[100];

    int user_count = 0;
    int found = 0;

    while (fscanf(fin, "%d %s %s %d %d",
                  &users[user_count].uid,
                  users[user_count].name,
                  users[user_count].ip_addr,
                  &users[user_count].port,
                  &users[user_count].pid) != EOF)
    {
        if (users[user_count].pid == pid) {
            // 用 PID 找到該筆
            strncpy(users[user_count].name, new_name, sizeof(users[user_count].name) - 1);
            users[user_count].name[sizeof(users[user_count].name) - 1] = '\0';
            found = 1;
        }
        user_count++;
    }

    if (!found) {
        fprintf(stderr, "Error: Could not find pid=%u in userlist\n", pid);
        fclose(fin);
        return 0;
    }

    fseek(fin, 0, SEEK_SET);
    ftruncate(fd, 0);

    for (int i = 0; i < user_count; i++) {
        fprintf(fin, "%d %s %s %d %d\n",
                users[i].uid,
                users[i].name,
                users[i].ip_addr,
                users[i].port,
                users[i].pid);
    }

    fflush(fin);
    fclose(fin);

    // 解鎖
    lock.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &lock);
    close(fd);

    return 1;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: name <new_name>\n");
        exit(1);
    }

    const char *new_name = argv[1];

    const char *pidStr = getenv("CURRENT_PID");
    if (!pidStr) {
        fprintf(stderr, "Error: CURRENT_PID environment variable not set\n");
        exit(1);
    }

    unsigned int target_pid = (unsigned int)atoi(pidStr);

    // 更新 userlist
    if (!update_userlist_name_by_pid(target_pid, new_name)) {
        exit(1);
    }

    // 更新資料庫
    if (!update_database_username_by_pid(target_pid, new_name)) {
        exit(1);
    }

    // 更新當前環境變數 CURRENT_USER
    setenv("CURRENT_USER", new_name, 1);

    printf("name change accept!\n");
    return 0;
}
