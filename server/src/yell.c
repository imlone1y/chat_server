// yell.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h> // For O_WRONLY and O_NONBLOCK
#include <errno.h>

#define USER_FILE "./tmp/userlist"
#define FIFO_DIR "./tmp/"
#define MAX_MESSAGE_LENGTH 512 // Maximum message length

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: yell message\n");
        exit(1);
    }

    unsigned int uid, port, pid;
    char ip[16], name[30];

    pid_t my_ppid = getppid();

    unsigned int my_uid = 0;
    unsigned int who_is_yelling = 0;
    char my_name[30];

    // Open userlist file
    FILE *fin = fopen(USER_FILE, "r");
    if (fin == NULL) {
        // perror("fopen");
        exit(1);
    }

    // Find our own uid and name
    while (fscanf(fin, "%d %s %s %d %d", &uid, name, ip, &port, &pid) != EOF) {
        if (pid == (unsigned int)my_ppid) {
            my_uid = port; // Use port as UID
            who_is_yelling = uid;
            strncpy(my_name, name, sizeof(my_name) - 1);
            my_name[sizeof(my_name) - 1] = '\0'; // Ensure null-termination
            break;
        }
    }
    fclose(fin);

    if (my_uid == 0) {
        fprintf(stderr, "Error: Could not find your own uid\n");
        exit(1);
    }

    // Concatenate the message arguments
    char message[MAX_MESSAGE_LENGTH] = {0};
    for (int i = 1; i < argc; i++) {
        strncat(message, argv[i], MAX_MESSAGE_LENGTH - strlen(message) - 1);
        if (i < argc - 1)
            strncat(message, " ", MAX_MESSAGE_LENGTH - strlen(message) - 1);
    }

    // Broadcast to all users
    fin = fopen(USER_FILE, "r");
    if (fin == NULL) {
        // perror("fopen");
        exit(1);
    }

    int clients_total = 0;
    int clients_sent = 0;

    while (fscanf(fin, "%d %s %s %d %d", &uid, name, ip, &port, &pid) != EOF) {
        clients_total++;

        char fifoName[100];
        snprintf(fifoName, sizeof(fifoName), "%sfifo_%d", FIFO_DIR, pid);

        int fifo_fd = open(fifoName, O_WRONLY | O_NONBLOCK);

        if (fifo_fd == -1) {
            if (errno == ENXIO) {
                // fprintf(stderr, "User with pid %d is not available. Skipping.\n", pid);
            } else {
                // perror("open fifo");
            }
            continue;
        }

        char fifo_buffer[1024];
        // int written = snprintf(fifo_buffer, sizeof(fifo_buffer), "<%s(%d) yelled>: %s\n", my_name, who_is_yelling, message);
        int written = snprintf(fifo_buffer, sizeof(fifo_buffer), "<user(%d) yelled>: %s\n", who_is_yelling, message);
        if (written >= sizeof(fifo_buffer)) {
            fprintf(stderr, "Warning: Message truncated for pid %d\n", pid);
        }

        ssize_t bytes_written = write(fifo_fd, fifo_buffer, strlen(fifo_buffer));
        if (bytes_written == -1) {
            fprintf(stderr, "Failed to write to FIFO %s for pid %d: %s\n", fifoName, pid, strerror(errno));
            close(fifo_fd);
            continue;
        }

        close(fifo_fd);
        clients_sent++;
    }
    fclose(fin);

    // Print accept message with summary
    // printf("yell accept! Message sent to %d out of %d clients.\n", clients_sent, clients_total);
    return 0;
}
