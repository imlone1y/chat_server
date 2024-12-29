#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mysql/mysql.h>
#include <mysqlsetting.h>

void delGroup(const char *current_user_name, const char *group_name) {
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
             "SELECT g.group_id, u.user_name FROM GroupTable g "
             "JOIN users u ON g.owner_id = u.user_id "
             "WHERE g.group_name = '%s'", group_name);

    if (mysql_query(conn, query)) {
        fprintf(stderr, "SELECT error: %s\n", mysql_error(conn));
        mysql_close(conn);
        exit(EXIT_FAILURE);
    }

    res = mysql_store_result(conn);
    if (res == NULL || mysql_num_rows(res) == 0) {
        printf("Group not found !\n");
        mysql_free_result(res);
        mysql_close(conn);
        return;
    }

    MYSQL_ROW group_row = mysql_fetch_row(res);
    int group_id = atoi(group_row[0]);
    const char *owner_name = group_row[1];
    mysql_free_result(res);

    // 檢查是否為群組擁有者
    if (strcmp(owner_name, current_user_name) != 0) {
        printf("You don't have permissions !\n");
        mysql_close(conn);
        return;
    }

    // 刪除群組的成員記錄
    snprintf(query, sizeof(query), "DELETE FROM GroupMembers WHERE group_id = %d", group_id);
    if (mysql_query(conn, query)) {
        fprintf(stderr, "DELETE error (GroupMembers): %s\n", mysql_error(conn));
        mysql_close(conn);
        exit(EXIT_FAILURE);
    }

    // 刪除群組
    snprintf(query, sizeof(query), "DELETE FROM GroupTable WHERE group_id = %d", group_id);
    if (mysql_query(conn, query)) {
        fprintf(stderr, "DELETE error (GroupTable): %s\n", mysql_error(conn));
        mysql_close(conn);
        exit(EXIT_FAILURE);
    }

    printf("Group delete success!\n");
    mysql_close(conn);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: delGroup <group_name>\n");
        return 1;
    }

    const char *current_user_name = getenv("CURRENT_USER");
    if (!current_user_name) {
        fprintf(stderr, "Error: CURRENT_USER environment variable not set.\n");
        return 1;
    }

    delGroup(current_user_name, argv[1]);
    return 0;
}
