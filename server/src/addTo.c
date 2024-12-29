#include <stdio.h>
#include <stdlib.h>
#include <mysql/mysql.h>
#include <string.h>
#include <mysqlsetting.h>

void addTo(const char *current_user_name, const char *group_name, char **user_names, int user_count) {
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

    // 確認當前用戶是否為群組擁有者
    char query[512];
    snprintf(query, sizeof(query),
             "SELECT g.group_id FROM GroupTable g "
             "JOIN users u ON g.owner_id = u.user_id "
             "WHERE g.group_name = '%s' AND u.user_name = '%s'",
             group_name, current_user_name);

    if (mysql_query(conn, query)) {
        fprintf(stderr, "SELECT error: %s\n", mysql_error(conn));
        mysql_close(conn);
        exit(EXIT_FAILURE);
    }

    res = mysql_store_result(conn);
    if (res == NULL || mysql_num_rows(res) == 0) {
        printf("You don't have permissions!\n");
        mysql_free_result(res);
        mysql_close(conn);
        return;
    }

    MYSQL_ROW group_row = mysql_fetch_row(res);
    int group_id = atoi(group_row[0]);
    mysql_free_result(res);

    // 初始化狀態字串
    char already_in_group[512] = {0};
    char not_found[512] = {0};
    char add_success[512] = {0};

    for (int i = 0; i < user_count; i++) {
        const char *user_name_char = user_names[i];

        // 查詢用戶是否存在
        snprintf(query, sizeof(query), "SELECT user_id FROM users WHERE user_name = '%s'", user_name_char);
        if (mysql_query(conn, query)) {
            fprintf(stderr, "SELECT error: %s\n", mysql_error(conn));
            continue;
        }

        res = mysql_store_result(conn);
        if (res == NULL || mysql_num_rows(res) == 0) {
            // 用戶不存在
            strncat(not_found, user_name_char, sizeof(not_found) - strlen(not_found) - 1);
            strncat(not_found, " ", sizeof(not_found) - strlen(not_found) - 1);
            mysql_free_result(res);
            continue;
        }

        MYSQL_ROW user_row = mysql_fetch_row(res);
        int user_id = atoi(user_row[0]);
        mysql_free_result(res);

        // 檢查用戶是否已在群組中
        snprintf(query, sizeof(query),
                 "SELECT * FROM GroupMembers WHERE group_id = %d AND user_id = %d",
                 group_id, user_id);

        if (mysql_query(conn, query)) {
            fprintf(stderr, "SELECT error: %s\n", mysql_error(conn));
            continue;
        }

        res = mysql_store_result(conn);
        if (res && mysql_num_rows(res) > 0) {
            // 用戶已在群組
            strncat(already_in_group, user_name_char, sizeof(already_in_group) - strlen(already_in_group) - 1);
            strncat(already_in_group, " ", sizeof(already_in_group) - strlen(already_in_group) - 1);
            mysql_free_result(res);
            continue;
        }
        mysql_free_result(res);

        // 添加用戶到群組
        snprintf(query, sizeof(query),
                 "INSERT INTO GroupMembers (group_id, user_id) VALUES (%d, %d)",
                 group_id, user_id);

        if (mysql_query(conn, query)) {
            fprintf(stderr, "INSERT error: %s\n", mysql_error(conn));
        } else {
            strncat(add_success, user_name_char, sizeof(add_success) - strlen(add_success) - 1);
            strncat(add_success, " ", sizeof(add_success) - strlen(add_success) - 1);
        }
    }

    // 合併輸出結果
    if (strlen(not_found) > 0) {
        not_found[strlen(not_found) - 1] = '\0'; // 移除最後的空格
        printf("%s not found!\n", not_found);
    }
    if (strlen(already_in_group) > 0) {
        already_in_group[strlen(already_in_group) - 1] = '\0'; // 移除最後的空格
        printf("%s already in group!\n", already_in_group);
    }
    if (strlen(add_success) > 0) {
        add_success[strlen(add_success) - 1] = '\0'; // 移除最後的空格
        printf("%s add success!\n", add_success);
    }

    mysql_close(conn);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: addTo <group_name> <user_name1> <user_name2> ...\n");
        return 1;
    }

    const char *current_user_name = getenv("CURRENT_USER");
    if (!current_user_name) {
        fprintf(stderr, "Error: CURRENT_USER environment variable not set.\n");
        return 1;
    }

    addTo(current_user_name, argv[1], &argv[2], argc - 2);
    return 0;
}