#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mysql/mysql.h>
#include <mysqlsetting.h>

void leaveGroup(const char *current_user_name, const char *group_name) {
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

    // 查詢群組是否存在
    char query[512];
    snprintf(query, sizeof(query),
             "SELECT group_id FROM GroupTable WHERE group_name = '%s'",
             group_name);

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

    // 查詢用戶是否在群組中
    snprintf(query, sizeof(query),
             "SELECT * FROM GroupMembers gm "
             "JOIN users u ON gm.user_id = u.user_id "
             "WHERE gm.group_id = %d AND u.user_name = '%s'",
             group_id, current_user_name);

    if (mysql_query(conn, query)) {
        fprintf(stderr, "SELECT error: %s\n", mysql_error(conn));
        mysql_close(conn);
        exit(EXIT_FAILURE);
    }

    res = mysql_store_result(conn);
    if (res == NULL || mysql_num_rows(res) == 0) {
        printf("Leave failed!\n");
        mysql_free_result(res);
        mysql_close(conn);
        return;
    }
    mysql_free_result(res);

    // 移除用戶與群組的關聯
    snprintf(query, sizeof(query),
             "DELETE FROM GroupMembers WHERE group_id = %d AND user_id = "
             "(SELECT user_id FROM users WHERE user_name = '%s')",
             group_id, current_user_name);

    if (mysql_query(conn, query)) {
        fprintf(stderr, "DELETE error: %s\n", mysql_error(conn));
        mysql_close(conn);
        exit(EXIT_FAILURE);
    }

    printf("Leave success!\n");
    mysql_close(conn);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: leaveGroup <group_name>\n");
        return 1;
    }

    const char *current_user_name = getenv("CURRENT_USER");
    if (!current_user_name) {
        fprintf(stderr, "Error: CURRENT_USER environment variable not set.\n");
        return 1;
    }

    leaveGroup(current_user_name, argv[1]);
    return 0;
}
