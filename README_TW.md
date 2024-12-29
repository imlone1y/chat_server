# 網路聊天伺服器

繁體中文 | [English](README.md)

本專案是一個使用 C 語言搭建的網路聊天伺服器，基於 TCP/IP 協定，運行於 Linux 平台。

---

## 專案結構

```
server
├── bin/           # 可執行的指令
├── include/       # 標頭檔案
├── object/        # 編譯後的物件檔
├── src/           # 原始碼
├── tmp/
│   ├── userlist   # 紀錄當前用戶並存放 FIFO 檔案
├── init_mysql.sql # 初始化 MySQL 資料庫的腳本
├── Makefile       # 編譯用的腳本
└── shell          # 主要執行檔
```

---

## 運行指南

1. **Windows 系統**請使用 **WSL**。
2. 切換到專案目錄：
   ```bash
   cd ./server
   ```
3. 執行 `shell` 程式：
   ```bash
   ./shell
   ```
4. 在另一個終端視窗中，連線到伺服器：
   ```bash
   telnet 127.0.0.1 6000
   ```

---

## 可用指令

| 指令           | 說明                                     | 格式                                   |
|----------------|------------------------------------------|----------------------------------------|
| `quit`         | 退出伺服器。                             |                                        |
| `who`          | 顯示所有在線用戶資訊。                   |                                        |
| `tell`         | 發送私人訊息給指定用戶。                 | `tell <user_id> <message>`            |
| `yell`         | 向所有用戶廣播訊息。                     | `yell <message>`                      |
| `name`         | 修改您的使用者名稱。                     | `name <new_name>`                     |
| `listMail`     | 顯示信箱中的所有郵件。                   |                                        |
| `mailto`       | 向指定用戶發送郵件。                     | `mailto <user_name> <message>`        |
| `delMail`      | 刪除信箱中的郵件。                       | `delMail <mail_id>`                   |
| `gyell`        | 向群組內的成員廣播訊息。                 | `gyell <group_name> <message>`        |
| `createGroup`  | 創建一個新群組。                         | `createGroup <group_name>`            |
| `delGroup`     | 刪除群組（僅限群組擁有者操作）。         | `delGroup <group_name>`               |
| `addTo`        | 邀請用戶加入群組（僅限群組擁有者操作）。 | `addTo <group_name> <user1> ...`      |
| `leaveGroup`   | 離開一個群組。                           | `leaveGroup <group_name>`             |
| `listGroup`    | 顯示您所屬的群組。                       |                                        |
| `remove`       | 將用戶從群組中移除（僅限群組擁有者操作）。| `remove <group_name> <user1> ...`     |

---

## Makefile 使用方法

- **編譯所有檔案**: 編譯 `src/` 資料夾中的所有檔案，並將可執行檔放置到 `bin/` 資料夾。
  ```bash
  make all
  ```

- **清理執行檔**: 刪除 `bin/` 資料夾中的所有可執行檔（不包含非 `src/` 檔案）。
  ```bash
  make clean
  ```

---

## 參考資料

此專案為 **2024 東華大學秋季網路程式設計**課程的作業之一。  
詳情請參考 [作業說明](https://hackmd.io/@chtsai/networkProgramming)。
