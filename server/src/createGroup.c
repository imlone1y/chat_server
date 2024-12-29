#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mysql/mysql.h>
#include <mysqlsetting.h>

void createGroup(const char *current_user_name, const char *group_name) {
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
    int owner_id = atoi(user_row[0]);
    mysql_free_result(res);

    // 檢查群組是否已存在
    snprintf(query, sizeof(query), "SELECT * FROM GroupTable WHERE group_name = '%s'", group_name);
    if (mysql_query(conn, query)) {
        fprintf(stderr, "SELECT error: %s\n", mysql_error(conn));
        mysql_close(conn);
        exit(EXIT_FAILURE);
    }

    res = mysql_store_result(conn);
    if (res && mysql_num_rows(res) > 0) {
        printf("Group already exist !\n");
        mysql_free_result(res);
        mysql_close(conn);
        return;
    }
    mysql_free_result(res);

    // 創建群組
    snprintf(query, sizeof(query), 
             "INSERT INTO GroupTable (group_name, owner_id) VALUES ('%s', %d)", 
             group_name, owner_id);
    if (mysql_query(conn, query)) {
        fprintf(stderr, "INSERT error: %s\n", mysql_error(conn));
        mysql_close(conn);
        exit(EXIT_FAILURE);
    }

    // 獲取剛剛創建的群組 ID
    int group_id = (int)mysql_insert_id(conn);

    // 創建者自動加入群組
    snprintf(query, sizeof(query), 
             "INSERT INTO GroupMembers (group_id, user_id) VALUES (%d, %d)", 
             group_id, owner_id);
    if (mysql_query(conn, query)) {
        fprintf(stderr, "INSERT into GroupMembers error: %s\n", mysql_error(conn));
        mysql_close(conn);
        exit(EXIT_FAILURE);
    }

    printf("Create success !\n");
    mysql_close(conn);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: createGroup <group_name>\n");
        return 1;
    }

    const char *current_user_name = getenv("CURRENT_USER");
    if (!current_user_name) {
        fprintf(stderr, "Error: CURRENT_USER environment variable not set.\n");
        return 1;
    }

    createGroup(current_user_name, argv[1]);
    return 0;
}
