#include "fb_file.h"
#include "fb_log.h"
#include "fb_shm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>

#define SHM_BLOCK_NAME "file_block"
#define SHM_BLOCK_SIZE 1024  // 共享内存块大小，可配置

// 写进程：负责把文件写入共享内存
static void writer_proc(const char *src) {
    pid_t pid = getpid();
    log_info("[Writer %d] 启动，准备写入共享内存", pid);

    FILE *fsrc = fopen(src, "rb");
    if (!fsrc) {
        log_error("[Writer %d] 打开源文件失败: %s", pid, src);
        exit(1);
    }

    unsigned char tmp[SHM_BLOCK_SIZE];
    size_t max_payload = SHM_BLOCK_SIZE - sizeof(size_t);
    size_t n;
    long total_written = 0;

    while ((n = fread(tmp + sizeof(size_t), 1, max_payload, fsrc)) > 0) {
        memcpy(tmp, &n, sizeof(size_t));

        if (shm_write(tmp, n + sizeof(size_t)) != (int)(n + sizeof(size_t))) {
            log_error("[Writer %d] 写入共享内存失败", pid);
            fclose(fsrc);
            exit(1);
        }

        total_written += n;
        log_debug("[Writer %d] 已写入 %ld 字节到共享内存", pid, total_written);
    }

    // 写入结束标志
    size_t zero = 0;
    memcpy(tmp, &zero, sizeof(size_t));
    shm_write(tmp, sizeof(size_t));

    log_info("[Writer %d] 写入完成，总计 %ld 字节", pid, total_written);
    fclose(fsrc);
    log_debug("[Writer %d] 退出写进程", pid);
    exit(0);
}

// 读进程：负责从共享内存读数据并写入文件
static void reader_proc(const char *dst) {
    pid_t pid = getpid();
    log_info("[Reader %d] 启动，准备从共享内存读取数据", pid);

    FILE *fdst = fopen(dst, "wb");
    if (!fdst) {
        log_error("[Reader %d] 打开目标文件失败: %s", pid, dst);
        exit(1);
    }

    unsigned char tmp[SHM_BLOCK_SIZE];
    long total_read = 0;

    while (1) {
        int r = shm_read(tmp, SHM_BLOCK_SIZE);
        if (r <= 0) {
            log_error("[Reader %d] 从共享内存读取失败", pid);
            fclose(fdst);
            exit(1);
        }

        size_t n = 0;
        memcpy(&n, tmp, sizeof(size_t));
        if (n == 0) {
            log_info("[Reader %d] 收到结束标志，读取完成", pid);
            break;
        }

        if (fwrite(tmp + sizeof(size_t), 1, n, fdst) != n) {
            log_error("[Reader %d] 写入目标文件失败", pid);
            fclose(fdst);
            exit(1);
        }

        total_read += n;
        log_debug("[Reader %d] 已写入目标文件 %ld 字节", pid, total_read);
    }

    log_info("[Reader %d] 成功完成写入，总计 %ld 字节", pid, total_read);
    fclose(fdst);
    log_debug("[Reader %d] 退出读进程", pid);
    exit(0);
}

// 父进程：创建两个子进程进行复制

static int copy_file_shm(const char *src, const char *dst) {
    log_info("[Parent %d] 初始化共享内存", getpid());

    if (shm_init(SHM_BLOCK_NAME, SHM_BLOCK_SIZE) != 0) {
        log_error("共享内存初始化失败");
        return FB_ERROR;
    }

    log_info("[Parent %d] 创建写进程...", getpid());
    pid_t pid_writer = fork();
    if (pid_writer < 0) {
        log_error("fork 写进程失败");
        shm_destroy();
        return FB_ERROR;
    } else if (pid_writer == 0) {
        // 子进程负责写文件到共享内存
        writer_proc(src);  // 子进程调用 writer_proc 并执行文件写入
        exit(0);  // 确保子进程正常退出
    } else {
        log_info("[Parent %d] 写进程创建成功，PID: %d", getpid(), pid_writer);
    }

    // 父进程进行读操作
    log_info("[Parent %d] 创建读进程...", getpid());
    pid_t pid_reader = fork();
    if (pid_reader < 0) {
        log_error("fork 读进程失败");
        kill(pid_writer, SIGTERM);  // 如果读进程失败，杀死写进程
        waitpid(pid_writer, NULL, 0);
        shm_destroy();
        return FB_ERROR;
    } else if (pid_reader == 0) {
        // 子进程负责从共享内存读取数据并写入目标文件
        reader_proc(dst);  // 子进程调用 reader_proc 并执行文件读取
        exit(0);  // 确保子进程正常退出
    } else {
        log_info("[Parent %d] 读进程创建成功，PID: %d", getpid(), pid_reader);
    }

    // 父进程等待写进程结束
    int status_writer;
    log_info("[Parent %d] 等待写进程结束...", getpid());
    waitpid(pid_writer, &status_writer, 0);
    if (!WIFEXITED(status_writer) || WEXITSTATUS(status_writer) != 0) {
        log_error("[Parent %d] 写进程异常退出", getpid());
    } else {
        log_info("[Parent %d] 写进程正常结束", getpid());
    }

    log_debug("[Parent %d] 写进程已结束，等待读进程...", getpid());

    // 父进程等待读进程结束
    int status_reader;
    log_info("[Parent %d] 等待读进程结束...", getpid());
    waitpid(pid_reader, &status_reader, 0);  // 等待读进程
    if (!WIFEXITED(status_reader) || WEXITSTATUS(status_reader) != 0) {
        log_error("[Parent %d] 读进程异常退出", getpid());
    } else {
        log_info("[Parent %d] 读进程正常结束", getpid());
    }

    log_debug("[Parent %d] 读进程已结束，共享内存清理...", getpid());

    shm_destroy();
    log_info("[Parent %d] 文件复制完成: %s -> %s", getpid(), src, dst);
    return FB_SUCCESS;
}

// 外部接口
API_EXPORT int fb_copy_file(const char *src, const char *dst) {
    return copy_file_shm(src, dst);
}

API_EXPORT int fb_move_file(const char *src, const char *dst) {
    if (fb_copy_file(src, dst) != FB_SUCCESS) {
        return FB_ERROR;
    }

    if (unlink(src) != 0) {
        log_error("删除源文件失败: %s", src);
        return FB_ERROR;
    }

    log_info("文件移动成功: %s -> %s", src, dst);
    return FB_SUCCESS;
}

API_EXPORT int fb_delete_file(const char *path) {
    if (unlink(path) != 0) {
        log_error("删除文件失败: %s", path);
        return FB_ERROR;
    }

    log_info("文件删除成功: %s", path);
    return FB_SUCCESS;
}
