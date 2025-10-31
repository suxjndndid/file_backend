#include "fb_log.h"
#include <time.h>
#include <stdlib.h>
#include <string.h>

static FILE *g_log_file = NULL;

void log_init(const char *log_file) {
    if (log_file) {
        g_log_file = fopen(log_file, "w");
        if (!g_log_file) {
            perror("fopen log_file");
        }
    }
}

void log_close() {
    if (g_log_file) {
        fclose(g_log_file);
        g_log_file = NULL;
    }
}

void log_write(log_level_t level, const char *fmt, ...) {
    const char *level_str = "";
    switch (level) {
        case LOG_LEVEL_INFO: level_str = "INFO"; break;
        case LOG_LEVEL_DEBUG: level_str = "DEBUG"; break;
        case LOG_LEVEL_ERROR: level_str = "ERROR"; break;
    }

    time_t now = time(NULL);
    struct tm t;
    localtime_r(&now, &t);
    char timebuf[64];
    strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", &t);

    va_list args;
    va_start(args, fmt);

    if (g_log_file) {
        fprintf(g_log_file, "[%s][%s] ", timebuf, level_str);
        vfprintf(g_log_file, fmt, args);
        fprintf(g_log_file, "\n");
        fflush(g_log_file);
    } else {
        fprintf(stderr, "[%s][%s] ", timebuf, level_str);
        vfprintf(stderr, fmt, args);
        fprintf(stderr, "\n");
    }

    va_end(args);
}
