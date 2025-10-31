#ifndef FB_LOG_H
#define FB_LOG_H
#ifdef __cplusplus
extern "C" {
#endif
#include <stdio.h>
#include <stdarg.h>

typedef enum {
    LOG_LEVEL_INFO,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_ERROR
} log_level_t;

void log_init(const char *log_file);
void log_close();
void log_write(log_level_t level, const char *fmt, ...);

#define log_info(fmt, ...) log_write(LOG_LEVEL_INFO, fmt, ##__VA_ARGS__)
#define log_debug(fmt, ...) log_write(LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)
#define log_error(fmt, ...) log_write(LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)
#ifdef __cplusplus
}
#endif
#endif // FB_LOG_H
