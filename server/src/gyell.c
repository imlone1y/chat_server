#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>   // For O_WRONLY and O_NONBLOCK
#include <unistd.h>
#include <errno.h>
#include <mysql/mysql.h>
#include <mysqlsetting.h>

#define USER_FILE "./tmp/userlist"
#define FIFO_DIR "./tmp/"
#define MAX_MESSAGE_LENGTH 512

void broadcast_to_group(const char *group_name, const char *message, const char *current_user_name) {
    MYSQL *conn;
    MYSQL_RES *res;
    MYSQL_ROW row;

    const char *server = ip;
    const char *user = user_name;
    const char *password = user_password; // 替換為您的 MySQL 密碼
    const char *database = database_name;

    conn = mysql_init(NULL);
    if (conn == NULL) {
        fprintf(stderr, "mysql_init() failed\n");
        exit(EXIT_FAILURE);
    }

    if (mysql_real_connect(conn, server, user, password, database, 0, NULL, 0) == NULL) {
        fprintf(stderr, "mysql_real_connect() failed\n");
        mysql_close(conn);
        exit(EXIT_FAILURE);
    }

    // 獲取當前用戶的 PID 和群組 ID
    char query[512];
    snprintf(query, sizeof(query),
             "SELECT g.group_id "
             "FROM GroupTable g "
             "JOIN GroupMembers gm ON g.group_id = gm.group_id "
             "JOIN users u ON gm.user_id = u.user_id "
             "WHERE g.group_name = '%s' AND u.user_name = '%s'",
             group_name, current_user_name);

    if (mysql_query(conn, query)) {
        fprintf(stderr, "SELECT error: %s\n", mysql_error(conn));
        mysql_close(conn);
        exit(EXIT_FAILURE);
    }

    res = mysql_store_result(conn);
    if (res == NULL || mysql_num_rows(res) == 0) {
        printf("Group not found!\n");
        mysql_free_result(res);
        mysql_close(conn);
        return;
    }

    MYSQL_ROW group_row = mysql_fetch_row(res);
    int group_id = atoi(group_row[0]);
    mysql_free_result(res);

    // 查詢群組成員的用戶名和 PID
    snprintf(query, sizeof(query),
             "SELECT u.user_name, u.pid "
             "FROM GroupMembers gm "
             "JOIN users u ON gm.user_id = u.user_id "
             "WHERE gm.group_id = %d",
             group_id);

    if (mysql_query(conn, query)) {
        fprintf(stderr, "SELECT error: %s\n", mysql_error(conn));
        mysql_close(conn);
        exit(EXIT_FAILURE);
    }

    res = mysql_store_result(conn);
    if (res == NULL) {
        fprintf(stderr, "mysql_store_result() failed\n");
        mysql_close(conn);
        exit(EXIT_FAILURE);
    }

    // 廣播訊息給群組成員
    while ((row = mysql_fetch_row(res)) != NULL) {
        const char *member_name = row[0];
        int member_pid = atoi(row[1]);

        char fifoName[100];
        snprintf(fifoName, sizeof(fifoName), "%sfifo_%d", FIFO_DIR, member_pid);

        int fifo_fd = open(fifoName, O_WRONLY | O_NONBLOCK);
        if (fifo_fd == -1) {
            if (errno == ENXIO) {
                fprintf(stderr, "User with PID %d is not available. Skipping.\n", member_pid);
            } else {
                // perror("open fifo");
            }
            continue;
        }

        char fifo_buffer[1024];
        int written = snprintf(fifo_buffer, sizeof(fifo_buffer),
                               "<%s-%s>: %s\nEND_OF_MESSAGE\n", group_name, current_user_name, message);
        if (written >= sizeof(fifo_buffer)) {
            fprintf(stderr, "Warning: Message truncated for PID %d\n", member_pid);
        }

        ssize_t bytes_written = write(fifo_fd, fifo_buffer, strlen(fifo_buffer));
        if (bytes_written == -1) {
            fprintf(stderr, "Failed to write to FIFO %s for PID %d: %s\n",
                    fifoName, member_pid, strerror(errno));
            close(fifo_fd);
            continue;
        }

        close(fifo_fd);
    }

    mysql_free_result(res);
    mysql_close(conn);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: gyell <group_name> <message>\n");
        exit(1);
    }

    const char *group_name = argv[1];
    const char *current_user_name = getenv("CURRENT_USER");
    if (!current_user_name) {
        fprintf(stderr, "Error: CURRENT_USER environment variable not set.\n");
        exit(1);
    }

    // Concatenate the message arguments
    char message[MAX_MESSAGE_LENGTH] = {0};
    for (int i = 2; i < argc; i++) {
        strncat(message, argv[i], MAX_MESSAGE_LENGTH - strlen(message) - 1);
        if (i < argc - 1)
            strncat(message, " ", MAX_MESSAGE_LENGTH - strlen(message) - 1);
    }

    broadcast_to_group(group_name, message, current_user_name);
    return 0;
}
