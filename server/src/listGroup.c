#include <stdio.h>
#include <stdlib.h>
#include <mysql/mysql.h>
#include <string.h>
#include <mysqlsetting.h>

void listGroup(const char *current_user_name) {
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

    // 查詢當前用戶的 user_id
    char query[512];
    snprintf(query, sizeof(query), "SELECT user_id FROM users WHERE user_name = '%s'", current_user_name);
    if (mysql_query(conn, query)) {
        fprintf(stderr, "SELECT error: %s\n", mysql_error(conn));
        mysql_close(conn);
        exit(EXIT_FAILURE);
    }

    res = mysql_store_result(conn);
    if (res == NULL || mysql_num_rows(res) == 0) {
        printf("User not found!\n");
        mysql_free_result(res);
        mysql_close(conn);
        return;
    }

    MYSQL_ROW user_row = mysql_fetch_row(res);
    int user_id = atoi(user_row[0]);
    mysql_free_result(res);

    // 查詢用戶加入的群組
    snprintf(query, sizeof(query),
             "SELECT g.group_name, u.user_name "
             "FROM GroupMembers gm "
             "JOIN GroupTable g ON gm.group_id = g.group_id "
             "JOIN users u ON g.owner_id = u.user_id "
             "WHERE gm.user_id = %d", user_id);

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
        printf("Empty!\n");
    } else {
        printf("<owner>      <group>\n");
        while ((row = mysql_fetch_row(res)) != NULL) {
            printf("%-12s %s\n", row[1], row[0]);
        }
    }

    mysql_free_result(res);
    mysql_close(conn);
}

int main(int argc, char *argv[]) {
    const char *current_user_name = getenv("CURRENT_USER");
    if (!current_user_name) {
        fprintf(stderr, "Error: CURRENT_USER environment variable not set.\n");
        return 1;
    }

    listGroup(current_user_name);
    return 0;
}
