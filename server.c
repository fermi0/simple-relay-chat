#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include "common.h"

typedef struct {
    int fd;
    int active;
    char name[MAX_NAME];
} Client;

static Client clients[MAX_CLIENTS];
static int server_fd;

static void send_to(int fd, const char *tag, const char *body) {
    char line[MAX_MSG];

    snprintf(line, sizeof(line), "%s|%s", tag, body);
    send_line(fd, line);
}

static Client *find_by_name(const char *name) {
    int i;

    for (i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active && strcmp(clients[i].name, name) == 0) {
            return &clients[i];
        }
    }
    return NULL;
}

static void broadcast(const char *from, const char *msg, int skip_fd) {
    char body[MAX_MSG];
    char logbuf[MAX_MSG];
    int i;

    snprintf(body, sizeof(body), "%s>%s", from, msg);
    snprintf(logbuf, sizeof(logbuf), "BROADCAST %s: %s", from, msg);
    log_event(logbuf);

    printf(CLR_CYAN "[broadcast] %s: %s" CLR_RESET "\n", from, msg);

    for (i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active && clients[i].fd != skip_fd) {
            send_to(clients[i].fd, TAG_BROADCAST, body);
        }
    }
}

static void send_userlist(int fd) {
    char list[MAX_MSG] = "";
    int i;

    for (i = 0; i < MAX_CLIENTS; i++) {
        if (!clients[i].active) {
            continue;
        }
        if (list[0] != '\0') {
            strncat(list, ",", sizeof(list) - strlen(list) - 1);
        }
        strncat(list, clients[i].name, sizeof(list) - strlen(list) - 1);
    }
    send_to(fd, TAG_USERLIST, list);
}

static void remove_client(int idx) {
    char logbuf[MAX_MSG];

    if (!clients[idx].active) {
        return;
    }

    snprintf(logbuf, sizeof(logbuf), "LEAVE %s", clients[idx].name);
    log_event(logbuf);

    printf(CLR_YELLOW "[left] %s" CLR_RESET "\n", clients[idx].name);
    close(clients[idx].fd);
    clients[idx].active = 0;
    clients[idx].name[0] = '\0';
    clients[idx].fd = -1;
}

static void handle_join(int idx, const char *username) {
    char logbuf[MAX_MSG];
    char welcome[MAX_MSG];
    int i;

    if (username[0] == '\0') {
        send_to(clients[idx].fd, TAG_ERROR, "Username cannot be empty");
        return;
    }
    if (find_by_name(username)) {
        send_to(clients[idx].fd, TAG_ERROR, "Username already taken");
        return;
    }

    strncpy(clients[idx].name, username, MAX_NAME - 1);
    clients[idx].name[MAX_NAME - 1] = '\0';

    snprintf(logbuf, sizeof(logbuf), "JOIN %s", username);
    log_event(logbuf);

    printf(CLR_GREEN "[join] %s" CLR_RESET "\n", username);

    snprintf(welcome, sizeof(welcome),
             "Welcome %s. Commands: /list  /pm <user> <msg>  /quit",
             username);
    send_to(clients[idx].fd, TAG_SYSTEM, welcome);

  {
    char body[MAX_MSG];
    snprintf(body, sizeof(body), "%s joined the chat", username);
    broadcast("SERVER", body, clients[idx].fd);
  }

    for (i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active) {
            send_userlist(clients[i].fd);
        }
    }
}

static void handle_pm(int idx, const char *target, const char *msg) {
    Client *to = find_by_name(target);
    char body[MAX_MSG];
    char logbuf[MAX_MSG];

    if (!to) {
        send_to(clients[idx].fd, TAG_ERROR, "User not found");
        return;
    }
    if (strcmp(target, clients[idx].name) == 0) {
        send_to(clients[idx].fd, TAG_ERROR, "Cannot PM yourself");
        return;
    }

    snprintf(body, sizeof(body), "%s>%s", clients[idx].name, msg);
    send_to(to->fd, TAG_PRIVATE, body);

    snprintf(body, sizeof(body), "to %s>%s", target, msg);
    send_to(clients[idx].fd, TAG_PRIVATE, body);

    snprintf(logbuf, sizeof(logbuf), "PM %s -> %s",
             clients[idx].name, target);
    log_event(logbuf);

    printf(CLR_MAGENTA "[private] %s -> %s" CLR_RESET "\n",
           clients[idx].name, target);
}

static void handle_line(int idx, const char *line) {
    char cmd[16];
    char arg1[MAX_NAME];
    char arg2[MAX_MSG];
    int n;

    if (strncmp(line, CMD_JOIN "|", 5) == 0) {
        handle_join(idx, line + 5);
        return;
    }

    if (clients[idx].name[0] == '\0') {
        send_to(clients[idx].fd, TAG_ERROR, "Send JOIN|<username> first");
        return;
    }

    n = sscanf(line, "%15[^|]|%31[^|]|%511[^\n]", cmd, arg1, arg2);
    if (n < 1) {
        return;
    }

    if (strcmp(cmd, CMD_SAY) == 0 && n >= 2) {
        broadcast(clients[idx].name, arg1, clients[idx].fd);
        return;
    }

    if (strcmp(cmd, CMD_PM) == 0 && n >= 3) {
        handle_pm(idx, arg1, arg2);
        return;
    }

    if (strcmp(cmd, CMD_LIST) == 0) {
        send_userlist(clients[idx].fd);
        return;
    }

    if (strcmp(cmd, CMD_QUIT) == 0) {
        char body[MAX_MSG];
        snprintf(body, sizeof(body), "%s left the chat", clients[idx].name);
        broadcast("SERVER", body, clients[idx].fd);
        remove_client(idx);
        return;
    }

    send_to(clients[idx].fd, TAG_ERROR, "Unknown command");
}

static int add_client(int fd) {
    int i;

    for (i = 0; i < MAX_CLIENTS; i++) {
        if (!clients[i].active) {
            clients[i].fd = fd;
            clients[i].active = 1;
            clients[i].name[0] = '\0';
            send_to(fd, TAG_SYSTEM, "Connected. Send JOIN|<username>");
            return i;
        }
    }
    send_to(fd, TAG_ERROR, "Server full");
    close(fd);
    return -1;
}

int main(int argc, char *argv[]) {
    struct sockaddr_in addr;
    int port = PORT;
    fd_set readfds;
    int max_fd;
    int i;

    if (argc >= 2) {
        port = atoi(argv[1]);
    }

    for (i = 0; i < MAX_CLIENTS; i++) {
        clients[i].active = 0;
        clients[i].fd = -1;
    }

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons((uint16_t)port);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return 1;
    }
    if (listen(server_fd, 10) < 0) {
        perror("listen");
        return 1;
    }

    printf(CLR_BOLD CLR_GREEN "Chat server running on port %d" CLR_RESET "\n", port);
    printf("Log file: %s\n", LOG_FILE);
    log_event("SERVER started");

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        max_fd = server_fd;

        for (i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].active) {
                FD_SET(clients[i].fd, &readfds);
                if (clients[i].fd > max_fd) {
                    max_fd = clients[i].fd;
                }
            }
        }

        if (select(max_fd + 1, &readfds, NULL, NULL, NULL) < 0) {
            perror("select");
            break;
        }

        if (FD_ISSET(server_fd, &readfds)) {
            int cfd = accept(server_fd, NULL, NULL);
            if (cfd >= 0) {
                printf(CLR_BLUE "[connect] new client (fd %d)" CLR_RESET "\n", cfd);
                log_event("CLIENT connected");
                add_client(cfd);
            }
        }

        for (i = 0; i < MAX_CLIENTS; i++) {
            char buf[MAX_MSG];
            ssize_t n;

            if (!clients[i].active || !FD_ISSET(clients[i].fd, &readfds)) {
                continue;
            }

            n = recv(clients[i].fd, buf, sizeof(buf) - 1, 0);
            if (n <= 0) {
                if (clients[i].name[0] != '\0') {
                    char body[MAX_MSG];
                    snprintf(body, sizeof(body), "%s disconnected", clients[i].name);
                    broadcast("SERVER", body, clients[i].fd);
                }
                remove_client(i);
                continue;
            }

            buf[n] = '\0';
            trim_newline(buf);
            if (buf[0] != '\0') {
                handle_line(i, buf);
            }
        }
    }

    close(server_fd);
    return 0;
}
