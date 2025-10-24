#include "fb_shm.h"
#include "fb_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>           // O_CREAT
#include <sys/mman.h>        // shm_open, mmap
#include <sys/stat.h>
#include <semaphore.h>
#include <unistd.h>

static int g_shm_fd = -1;
static void *g_shm_ptr = NULL;
static size_t g_shm_size = 0;
static sem_t *g_sem = NULL;
static char g_sem_name[128];

// 初始化共享内存
int shm_init(const char *name, size_t size) {
    if (!name || size == 0) return -1;

    g_shm_fd = shm_open(name, O_CREAT | O_RDWR, 0666);
    if (g_shm_fd < 0) {
        perror("shm_open");
        return -1;
    }

    if (ftruncate(g_shm_fd, size) < 0) {
        perror("ftruncate");
        return -1;
    }

    g_shm_ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, g_shm_fd, 0);
    if (g_shm_ptr == MAP_FAILED) {
        perror("mmap");
        return -1;
    }

    g_shm_size = size;

    snprintf(g_sem_name, sizeof(g_sem_name), "/sem_%s", name);
    g_sem = sem_open(g_sem_name, O_CREAT, 0666, 1); // 初始值 1
    if (g_sem == SEM_FAILED) {
        perror("sem_open");
        return -1;
    }

    log_info("共享内存初始化成功: %s, size=%zu", name, size);
    return 0;
}

// 写入共享内存
int shm_write(const void *data, size_t size) {
    if (!g_shm_ptr || !g_sem || !data || size == 0) return -1;

    sem_wait(g_sem);  // 上锁

    size_t wsize = size > g_shm_size ? g_shm_size : size;
    memcpy(g_shm_ptr, data, wsize);

    sem_post(g_sem);  // 解锁
    return wsize;
}

// 读取共享内存
int shm_read(void *buf, size_t size) {
    if (!g_shm_ptr || !g_sem || !buf || size == 0) return -1;

    sem_wait(g_sem); // 上锁

    size_t rsize = size > g_shm_size ? g_shm_size : size;
    memcpy(buf, g_shm_ptr, rsize);

    sem_post(g_sem); // 解锁
    return rsize;
}

// 销毁共享内存
void shm_destroy() {
    if (g_shm_ptr) {
        munmap(g_shm_ptr, g_shm_size);
        g_shm_ptr = NULL;
    }

    if (g_shm_fd >= 0) {
        close(g_shm_fd);
        g_shm_fd = -1;
    }

    if (g_sem) {
        sem_close(g_sem);
        sem_unlink(g_sem_name);
        g_sem = NULL;
    }

    log_info("共享内存销毁完成");
}
