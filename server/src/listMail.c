#include <stdio.h>
#include <mysql/mysql.h>
#include <stdlib.h>
#include <string.h>
#include <mysqlsetting.h>

void listMail(const char *current_user_name) {
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

    char query[512];
    snprintf(query, sizeof(query),
             "SELECT m.id, m.sent_at, u.user_name, m.message "
             "FROM Mails m "
             "JOIN users u ON m.sender_id = u.user_id "
             "WHERE m.receiver_id = (SELECT user_id FROM users WHERE user_name = '%s') "
             "ORDER BY m.sent_at DESC",
             current_user_name);

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

    if (mysql_num_rows(res) == 0) {
        printf("Empty !\n");
    } else {
        printf("%-4s  %-19s %-10s %s\n", "<id>", "<date>", "<sender>", "<message>");
        while ((row = mysql_fetch_row(res)) != NULL) {
            // 分割多行訊息
            char *message = strdup(row[3]);
            char *line = strtok(message, "\n");
            int is_first_line = 1;

            while (line != NULL) {
                if (is_first_line) {
                    printf("%-4s  %-19s %-10s %s\n", row[0], row[1], row[2], line);
                    is_first_line = 0;
                } else {
                    printf("%-4s  %-19s %-10s %s\n", "", "", "", line);
                }
                line = strtok(NULL, "\n");
            }

            free(message); // 釋放暫存訊息的記憶體
        }
    }

    mysql_free_result(res);
    mysql_close(conn);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: listMail <current_user_name>\n");
        return 1;
    }

    const char *current_user_name = argv[1];
    listMail(current_user_name);

    return 0;
}
