#include <stdio.h>
#include <mysql/mysql.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <mysqlsetting.h>

void mailto(const char *current_user_name, const char *receiver_name, const char *message, int is_command) {
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

    // 查詢 sender_id
    char query[4096];
    snprintf(query, sizeof(query), "SELECT user_id FROM users WHERE user_name = '%s'", current_user_name);

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

    int sender_id = -1;
    if (mysql_num_rows(res) > 0) {
        row = mysql_fetch_row(res);
        sender_id = atoi(row[0]);
    } else {
        printf("User not found!\n");
        mysql_free_result(res);
        mysql_close(conn);
        return;
    }
    mysql_free_result(res);

    // 查詢 receiver_id
    snprintf(query, sizeof(query), "SELECT user_id FROM users WHERE user_name = '%s'", receiver_name);

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

    int receiver_id = -1;
    if (mysql_num_rows(res) > 0) {
        row = mysql_fetch_row(res);
        receiver_id = atoi(row[0]);
    } else {
        printf("User not found!\n");
        mysql_free_result(res);
        mysql_close(conn);
        return;
    }
    mysql_free_result(res);

    // 准備郵件內容
    char final_message[2048] = {0};

    if (is_command) { // 執行命令並捕獲輸出
        FILE *fp = popen(message, "r");
        if (fp == NULL) {
            perror("popen failed");
            mysql_close(conn);
            return;
        }

        while (fgets(final_message + strlen(final_message), sizeof(final_message) - strlen(final_message) - 1, fp)) {
            if (strlen(final_message) >= sizeof(final_message) - 1) {
                fprintf(stderr, "Warning: Command output truncated.\n");
                break;
            }
        }
        pclose(fp);
    } else {
        strncpy(final_message, message, sizeof(final_message) - 1);
    }

    char escaped_message[3500]; // Ensure sufficient space for escaping
    mysql_real_escape_string(conn, escaped_message, final_message, sizeof(escaped_message) - 1);
    // 插入郵件記錄到資料庫
    snprintf(query, sizeof(query),
            "INSERT INTO Mails (sender_id, receiver_id, message) VALUES (%d, %d, '%s')",
            sender_id, receiver_id, escaped_message);

    if (mysql_query(conn, query)) {
        fprintf(stderr, "INSERT error: %s\n", mysql_error(conn));
    } else {
        printf("Send accept !\n");
    }

    mysql_close(conn);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: mailto <user_name> message | mailto <user_name> < command\n");
        return 1;
    }

    const char *receiver_name = argv[1];

    if (strcmp(argv[2], "<") == 0 && argc == 4) { // 重定向模式
        const char *command = argv[3];
        const char *current_user_name = getenv("CURRENT_USER");
        if (!current_user_name) {
            fprintf(stderr, "Error: CURRENT_USER environment variable not set.\n");
            return 1;
        }
        mailto(current_user_name, receiver_name, command, 1);
    } else { // 普通消息模式
        char message[512] = {0};
        for (int i = 2; i < argc; i++) {
            strncat(message, argv[i], sizeof(message) - strlen(message) - 1);
            if (i < argc - 1) {
                strncat(message, " ", sizeof(message) - strlen(message) - 1);
            }
        }

        const char *current_user_name = getenv("CURRENT_USER");
        if (!current_user_name) {
            fprintf(stderr, "Error: CURRENT_USER environment variable not set.\n");
            return 1;
        }
        mailto(current_user_name, receiver_name, message, 0);
    }

    return 0;
}
