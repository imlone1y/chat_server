# 網路聊天伺服器

繁體中文 | [English](README.md)

本項目使用 c 語言及 TCP/IP 協定搭建於 linux 底下運行的網路聊天伺服器。

## 項目結構

```
server
    |-- bin/           // executable command
    |-- include/       // all the .h file
    |-- object/        // object file
    |-- src/           // source code
    |-- tmp/
    |   |-- userlist   // record who's using the server and place the fifo file
    |-- init_mysql.sql // init the database for mysql
    |-- Makefile       // script for compile
    |-- `shell`        // this is executable 
```


## 運行指南

windows 系統使用 wsl，先在項目目錄下(./server)執行 `shell` 執行檔，再開其他分頁使用以下指令連上 server listen 端。

```bash
telnet 127.0.0.1 6000
```

## 伺服器指令功能

- quit

    > usage : exit server.

- who

    > usage : Show information of all users.

- tell

    > usage : Send the message to the specific user.    

    > format : `tell` `user_id` `message`

- yell

    > usage : Broadcast the message.    

    > format : `yell` `message`

- name
    
    > usage : Change your name by this command.   

    > format : `name` `new name`

- listMail

    > usage : List all mails in your mail account.  

- mailto

    > usage : Mail message to someone.  

    > format : `mailto` `user_name` `message`
    >> or   
    >> `mailto` `user_name` `< ls`   
    >> `<` this means redirect output from `ls` to `user_name`

- delMail

    > usage : Delete mail in your mailbox.  

    > format : `delMail` `mail_id`

- gyell

    > usage : This command same as `yell`, only different between two   command, is yell broadcast to every users they are online and `gyell` broadcase to users in the same group.     

    > format : `gyell` `group_name` `message`

- createGroup

    > usage : Create a group.

    > format : `createGroup` `group_name`

- delGroup

    > usage : Delete a group, only the owner can delete group.

    > format : `delGroup` `group_name`

- addTo

    > usaege : invite user come in group, only the owner can invite other user.

    > format : `addTo` `group_name` `user_name1` `user_name2` …

- leaveGroup

    > usage : Leave a group.

    > format : `leaveGroup` `group name`

- listGroup

    > usage : List groups that you belong to.

- remove

    > usage : Remove user from group, only the group owner can remove a user.

    > format : `remove` `group_name` `user_name` `user_name` …

## Makefile 語法

- make all : 編譯所有在 `src/` 底下的所有檔案，並將執行檔放入 `bin/` 資料夾底下。

- make clean : 清除所有在 `bin/` 資料夾裡的執行檔，除了不在 `src/` 底下的檔案。

## 參考資料

本項目為 2024 東華大學秋季網路與程式設計作業，作業項目[連結](https://hackmd.io/@chtsai/networkProgramming)。
