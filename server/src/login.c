#include <mysql.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // for getppid()
#include <mysqlsetting.h>

void finish(MYSQL *con) {
    if (con) mysql_close(con);
    exit(1);
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <username> <password>\n", argv[0]);
        exit(1);
    }

    const char *username = argv[1];
    const char *input_password = argv[2];

    MYSQL *con = mysql_init(NULL);
    mysql_options(con, MYSQL_SET_CHARSET_NAME, "utf8");
    mysql_options(con, MYSQL_INIT_COMMAND, "SET NAMES utf8");

    if (con == NULL) {
        fprintf(stderr, "MySQL initialization failed.\n");
        exit(1);
    }

    if (mysql_real_connect(con, ip, user_name, user_password, database_name, 0, NULL, 0) == NULL) {
        fprintf(stderr, "MySQL connection failed: %s\n", mysql_error(con));
        finish(con);
    }

    // 查詢用戶密碼
    char sql[256];
    snprintf(sql, sizeof(sql),
             "SELECT user_password FROM users WHERE user_name='%s'", username);

    if (mysql_query(con, sql)) {
        fprintf(stderr, "MySQL query failed: %s\n", mysql_error(con));
        finish(con);
    }

    MYSQL_RES *result = mysql_store_result(con);
    if (result == NULL) {
        fprintf(stderr, "MySQL store result failed: %s\n", mysql_error(con));
        finish(con);
    }

    if (mysql_num_rows(result) > 0) {
        MYSQL_ROW row = mysql_fetch_row(result);
        const char *db_password = row[0];

        if (strcmp(input_password, db_password) == 0) {
            // 登入成功，更新 PID
            pid_t current_pid = getppid();
            snprintf(sql, sizeof(sql),
                     "UPDATE users SET pid=%d WHERE user_name='%s' AND user_password='%s'",
                     current_pid, username, input_password);

            if (mysql_query(con, sql)) {
                fprintf(stderr, "MySQL UPDATE pid error: %s\n", mysql_error(con));
                mysql_free_result(result);
                finish(con);
            }

            printf("%s\n", db_password); // 將密碼輸出給 `test.c`
        } else {
            printf("NOTFOUND\n"); // 密碼不匹配
        }
    } else {
        printf("NOTFOUND\n"); // 找不到用戶名
    }

    mysql_free_result(result);
    mysql_close(con);
    return 0;
}
