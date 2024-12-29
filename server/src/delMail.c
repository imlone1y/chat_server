#include <stdio.h>
#include <mysql/mysql.h>
#include <stdlib.h>
#include <string.h>
#include <mysqlsetting.h>

void delMail(const char *current_user_name, int mail_id) {
    MYSQL *conn;
    MYSQL_RES *res;

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

    // 查詢當前使用者的 user_id
    char query[256];
    snprintf(query, sizeof(query),
             "SELECT user_id FROM users WHERE user_name = '%s'", current_user_name);

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

    int user_id = -1;
    if (mysql_num_rows(res) > 0) {
        MYSQL_ROW row = mysql_fetch_row(res);
        user_id = atoi(row[0]);
    } else {
        printf("User not found!\n");
        mysql_free_result(res);
        mysql_close(conn);
        return;
    }
    mysql_free_result(res);

    // 驗證郵件是否存在並屬於該使用者
    snprintf(query, sizeof(query),
             "SELECT id FROM Mails WHERE id = %d AND receiver_id = %d", mail_id, user_id);

    if (mysql_query(conn, query)) {
        fprintf(stderr, "SELECT error: %s\n", mysql_error(conn));
        mysql_close(conn);
        exit(EXIT_FAILURE);
    }

    res = mysql_store_result(conn);
    if (res == NULL || mysql_num_rows(res) == 0) {
        printf("Mail id unexist !\n");
    } else {
        // 刪除郵件
        snprintf(query, sizeof(query), "DELETE FROM Mails WHERE id = %d", mail_id);

        if (mysql_query(conn, query)) {
            fprintf(stderr, "DELETE error: %s\n", mysql_error(conn));
        } else {
            printf("Delete accept !\n");
        }
    }

    mysql_free_result(res);
    mysql_close(conn);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: delMail <mail_id>\n");
        return 1;
    }

    const char *current_user_name = getenv("CURRENT_USER");
    if (!current_user_name) {
        fprintf(stderr, "Error: CURRENT_USER environment variable not set.\n");
        return 1;
    }

    int mail_id = atoi(argv[1]); // 定義和初始化 mail_id
    if (mail_id <= 0) {
        fprintf(stderr, "Error: Invalid mail_id.\n");
        return 1;
    }

    delMail(current_user_name, mail_id); // 呼叫 delMail 函數

    return 0;
}
