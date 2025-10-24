#include "fb_config.h"
#include "fb_log.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

static size_t g_shm_size = 1024;  // 默认 1K
static char *g_log_path = NULL;
static pthread_mutex_t g_config_mutex = PTHREAD_MUTEX_INITIALIZER;

int fb_config_init(size_t shm_block_size, const char *log_file_path) {
    pthread_mutex_lock(&g_config_mutex);

    g_shm_size = shm_block_size > 0 ? shm_block_size : 1024;

    if (log_file_path) {
        if (g_log_path) free(g_log_path);
        g_log_path = strdup(log_file_path);
        log_init(g_log_path);
    }

    pthread_mutex_unlock(&g_config_mutex);
    return 0;
}

size_t fb_config_get_shm_size() {
    size_t size;
    pthread_mutex_lock(&g_config_mutex);
    size = g_shm_size;
    pthread_mutex_unlock(&g_config_mutex);
    return size;
}

const char* fb_config_get_log_path() {
    const char *path;
    pthread_mutex_lock(&g_config_mutex);
    path = g_log_path;
    pthread_mutex_unlock(&g_config_mutex);
    return path;
}

void fb_config_destroy() {
    pthread_mutex_lock(&g_config_mutex);
    if (g_log_path) {
        free(g_log_path);
        g_log_path = NULL;
    }
    log_close();
    pthread_mutex_unlock(&g_config_mutex);
}
