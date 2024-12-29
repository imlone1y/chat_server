#include <stdio.h>
#include <unistd.h>  // For getpid(), getppid()
#include <stdlib.h>

char userFile[20] = "./tmp/userlist";

int main() {
    unsigned int uid, port, pid;
    char ip[16], name[30];

    pid_t my_ppid = getppid(); // 获取父进程的 PID

    FILE *fin = fopen(userFile, "r");
    if (fin == NULL) {
        perror("fopen");
        exit(1);
    }

    printf("<ID>\t<name>\t<IP:port>\t\t<indicate me>\n");

    while (1) {
        int items_read = fscanf(fin, "%d %s %s %d %d", &uid, name, ip, &port, &pid);
        if (items_read == EOF) {
            break;
        } else if (items_read != 5) {
            fprintf(stderr, "Error reading from %s\n", userFile);
            break;
        }

        // 比较 pid 和 my_ppid
        if (pid == (unsigned int)my_ppid)
            printf("%4d\t%s\t%s:%d\t\t<-(me)\n", uid, name, ip, port);
        else
            printf("%4d\t%s\t%s:%d\t\n", uid, name, ip, port);
    }
    fclose(fin);
    return 0;
}
