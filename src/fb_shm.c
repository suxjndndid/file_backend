// src/fb_shm.c
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
static sem_t *g_sem_empty = NULL;
static sem_t *g_sem_full = NULL;
static char g_sem_empty_name[128];
static char g_sem_full_name[128];
static char g_shm_name[128];

// 初始化共享内存（创建两个命名信号量：empty/full）
int shm_init(const char *name, size_t size) {
    if (!name || size == 0) return -1;

    snprintf(g_shm_name, sizeof(g_shm_name), "%s", name);

    g_shm_fd = shm_open(name, O_CREAT | O_RDWR, 0666);
    if (g_shm_fd < 0) {
        log_error("shm_open");
        return -1;
    }

    if (ftruncate(g_shm_fd, size) < 0) {
        log_error("ftruncate");
        close(g_shm_fd);
        g_shm_fd = -1;
        return -1;
    }

    g_shm_ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, g_shm_fd, 0);
    if (g_shm_ptr == MAP_FAILED) {
        log_error("mmap");
        close(g_shm_fd);
        g_shm_fd = -1;
        return -1;
    }

    g_shm_size = size;

    // sem names: /sem_<name>_empty and /sem_<name>_full
    snprintf(g_sem_empty_name, sizeof(g_sem_empty_name), "/sem_%s_empty", name);
    snprintf(g_sem_full_name, sizeof(g_sem_full_name),  "/sem_%s_full", name);

    // empty 初始为 1（可以写），full 初始为 0（没有数据可读）
    g_sem_empty = sem_open(g_sem_empty_name, O_CREAT, 0666, 1);
    if (g_sem_empty == SEM_FAILED) {
        log_error("sem_open empty");
        munmap(g_shm_ptr, g_shm_size);
        g_shm_ptr = NULL;
        close(g_shm_fd);
        g_shm_fd = -1;
        return -1;
    }

    g_sem_full = sem_open(g_sem_full_name, O_CREAT, 0666, 0);
    if (g_sem_full == SEM_FAILED) {
        log_error("sem_open full");
        sem_close(g_sem_empty);
        sem_unlink(g_sem_empty_name);
        g_sem_empty = NULL;
        munmap(g_shm_ptr, g_shm_size);
        g_shm_ptr = NULL;
        close(g_shm_fd);
        g_shm_fd = -1;
        return -1;
    }

    log_info("共享内存初始化成功: %s, size=%zu", name, size);
    return 0;
}

// 写入共享内存（生产者）
// 语义：wait(empty) -> memcpy -> post(full)
// 返回写入字节数（或 -1）
int shm_write(const void *data, size_t size) {
    if (!g_shm_ptr || !g_sem_empty || !g_sem_full || !data || size == 0) return -1;

    // 等待可写
    if (sem_wait(g_sem_empty) != 0) {
        log_error("sem_wait empty");
        return -1;
    }

    // 可选：在共享内存首部保留 size 字段，如果上层协议需要
    // 但为保持简单：直接写入数据（调用者需要确保 size <= g_shm_size）
    size_t wsize = size > g_shm_size ? g_shm_size : size;
    memcpy(g_shm_ptr, data, wsize);

    // 通知可读
    if (sem_post(g_sem_full) != 0) {
        log_error("sem_post full");
        return -1;
    }

    return (int)wsize;
}

// 读取共享内存（消费者）
// 语义：wait(full) -> memcpy -> post(empty)
int shm_read(void *buf, size_t size) {
    if (!g_shm_ptr || !g_sem_empty || !g_sem_full || !buf || size == 0) return -1;

    // 等待可读
    if (sem_wait(g_sem_full) != 0) {
        log_error("sem_wait full");
        return -1;
    }

    size_t rsize = size > g_shm_size ? g_shm_size : size;
    memcpy(buf, g_shm_ptr, rsize);

    // 通知可写
    if (sem_post(g_sem_empty) != 0) {
        log_error("sem_post empty");
        return -1;
    }

    return (int)rsize;
}

// 销毁共享内存与信号量
void shm_destroy() {
    if (g_shm_ptr) {
        munmap(g_shm_ptr, g_shm_size);
        g_shm_ptr = NULL;
    }

    if (g_shm_fd >= 0) {
        close(g_shm_fd);
        // 注意：shm_unlink 放在外部（由创建者或最后一个进程）决定
        // 这里我们尝试 unlink（如果失败也继续）
        if (g_shm_name[0] != '\0') {
            shm_unlink(g_shm_name);
        }
        g_shm_fd = -1;
    }

    if (g_sem_empty) {
        sem_close(g_sem_empty);
        sem_unlink(g_sem_empty_name);
        g_sem_empty = NULL;
    }

    if (g_sem_full) {
        sem_close(g_sem_full);
        sem_unlink(g_sem_full_name);
        g_sem_full = NULL;
    }

    log_info("共享内存销毁完成");
}
