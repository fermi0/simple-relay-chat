#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define PORT 5555
#define MAX_CLIENTS 20
#define MAX_MSG 512
#define MAX_NAME 32
#define LOG_FILE "chat.log"

/* Message types (client -> server) */
#define CMD_JOIN "JOIN"
#define CMD_SAY  "SAY"
#define CMD_PM   "PM"
#define CMD_QUIT "QUIT"
#define CMD_LIST "LIST"

/* Message types (server -> client) */
#define TAG_BROADCAST "BROADCAST"
#define TAG_PRIVATE   "PRIVATE"
#define TAG_SYSTEM    "SYSTEM"
#define TAG_USERLIST  "USERLIST"
#define TAG_ERROR     "ERROR"

/* ANSI colors */
#define CLR_RESET   "\033[0m"
#define CLR_RED     "\033[31m"
#define CLR_GREEN   "\033[32m"
#define CLR_YELLOW  "\033[33m"
#define CLR_BLUE    "\033[34m"
#define CLR_MAGENTA "\033[35m"
#define CLR_CYAN    "\033[36m"
#define CLR_BOLD    "\033[1m"

static inline void trim_newline(char *s) {
    size_t n = strlen(s);
    while (n > 0 && (s[n - 1] == '\n' || s[n - 1] == '\r')) {
        s[--n] = '\0';
    }
}

static inline void log_event(const char *text) {
    FILE *f = fopen(LOG_FILE, "a");
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char ts[32];

    if (!f) {
        return;
    }
    strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", t);
    fprintf(f, "[%s] %s\n", ts, text);
    fclose(f);
}

static inline void send_line(int fd, const char *line) {
    char buf[MAX_MSG + 4];
    int n = snprintf(buf, sizeof(buf), "%s\n", line);
    if (n > 0) {
        send(fd, buf, (size_t)n, 0);
    }
}

#endif
