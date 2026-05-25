#define _POSIX_C_SOURCE 200809L
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "common.h"

static int sock_fd;
static volatile int running = 1;

static void print_help(void) {
    printf(CLR_YELLOW "\nCommands:\n" CLR_RESET);
    printf("  /list              show online users\n");
    printf("  /pm <user> <msg>   private message\n");
    printf("  /quit              leave chat\n");
    printf("  anything else      send to everyone\n\n");
}

static void print_incoming(const char *line) {
    char tag[32];
    char body[MAX_MSG];
    int n;

    n = sscanf(line, "%31[^|]|%511[^\n]", tag, body);
    if (n < 2) {
        printf("%s\n", line);
        return;
    }

    if (strcmp(tag, TAG_BROADCAST) == 0) {
        char user[MAX_NAME];
        char msg[MAX_MSG];
        if (sscanf(body, "%31[^>]>%511[^\n]", user, msg) == 2) {
            printf(CLR_CYAN "[all] " CLR_BOLD "%s" CLR_RESET CLR_CYAN ": %s" CLR_RESET "\n",
                   user, msg);
        }
    } else if (strcmp(tag, TAG_PRIVATE) == 0) {
        char a[MAX_NAME];
        char b[MAX_MSG];
        if (sscanf(body, "%31[^>]>%511[^\n]", a, b) == 2) {
            printf(CLR_MAGENTA "[private] " CLR_BOLD "%s" CLR_RESET
                   CLR_MAGENTA ": %s" CLR_RESET "\n", a, b);
        }
    } else if (strcmp(tag, TAG_SYSTEM) == 0) {
        printf(CLR_GREEN "[system] %s" CLR_RESET "\n", body);
    } else if (strcmp(tag, TAG_USERLIST) == 0) {
        printf(CLR_BLUE "[users] %s" CLR_RESET "\n", body);
    } else if (strcmp(tag, TAG_ERROR) == 0) {
        printf(CLR_RED "[error] %s" CLR_RESET "\n", body);
    } else {
        printf("%s\n", line);
    }
    fflush(stdout);
}

static void *recv_thread(void *arg) {
    char buf[MAX_MSG];
    (void)arg;

    while (running) {
        ssize_t n = recv(sock_fd, buf, sizeof(buf) - 1, 0);
        if (n <= 0) {
            if (running) {
                printf(CLR_RED "\nDisconnected from server." CLR_RESET "\n");
            }
            running = 0;
            break;
        }
        buf[n] = '\0';
        trim_newline(buf);
        print_incoming(buf);
    }
    return NULL;
}

static void send_cmd(const char *line) {
    send_line(sock_fd, line);
}

int main(int argc, char *argv[]) {
    struct sockaddr_in addr;
    char host[64] = "127.0.0.1";
    int port = PORT;
    char username[MAX_NAME];
    char line[MAX_MSG];
    char join_line[MAX_MSG];
    pthread_t tid;

    if (argc >= 2) {
        strncpy(host, argv[1], sizeof(host) - 1);
    }
    if (argc >= 3) {
        port = atoi(argv[2]);
    }

    printf(CLR_BOLD "Simple Chat Client" CLR_RESET "\n");
    printf("Server %s:%d\n", host, port);
    printf("Your username: ");
    fflush(stdout);
    if (!fgets(username, sizeof(username), stdin)) {
        return 1;
    }
    trim_newline(username);
    if (username[0] == '\0') {
        printf(CLR_RED "Username required.\n" CLR_RESET);
        return 1;
    }

    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("socket");
        return 1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)port);
    if (inet_pton(AF_INET, host, &addr.sin_addr) <= 0) {
        printf(CLR_RED "Invalid host.\n" CLR_RESET);
        return 1;
    }

    if (connect(sock_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect");
        return 1;
    }

    snprintf(join_line, sizeof(join_line), "%s|%s", CMD_JOIN, username);
    send_cmd(join_line);

    if (pthread_create(&tid, NULL, recv_thread, NULL) != 0) {
        printf(CLR_RED "Could not start receive thread.\n" CLR_RESET);
        return 1;
    }

    print_help();
    printf(CLR_GREEN "Connected. Type a message:\n" CLR_RESET);

    while (running && fgets(line, sizeof(line), stdin)) {
        char target[MAX_NAME];
        char msg[MAX_MSG];
        char out[MAX_MSG];

        trim_newline(line);
        if (line[0] == '\0') {
            continue;
        }

        if (strcmp(line, "/quit") == 0) {
            send_cmd(CMD_QUIT "|");
            running = 0;
            break;
        }

        if (strcmp(line, "/list") == 0) {
            send_cmd(CMD_LIST "|");
            continue;
        }

        if (strncmp(line, "/pm ", 4) == 0) {
            if (sscanf(line + 4, "%31s %511[^\n]", target, msg) == 2) {
                snprintf(out, sizeof(out), "%s|%s|%s", CMD_PM, target, msg);
                send_cmd(out);
            } else {
                printf(CLR_RED "Usage: /pm <user> <message>\n" CLR_RESET);
            }
            continue;
        }

        snprintf(out, sizeof(out), "%s|%s", CMD_SAY, line);
        send_cmd(out);
    }

    shutdown(sock_fd, SHUT_RDWR);
    close(sock_fd);
    running = 0;
    pthread_join(tid, NULL);

    printf(CLR_YELLOW "Goodbye.\n" CLR_RESET);
    return 0;
}
