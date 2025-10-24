#include "fb_file.h"
#include "fb_log.h"
#include "fb_shm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#define SHM_BLOCK_NAME "file_block"
#define SHM_BLOCK_SIZE 1024  // 共享内存块大小，可配置

// 内部函数：通过共享内存复制文件
static int copy_file_shm(const char *src, const char *dst) {
    if (shm_init(SHM_BLOCK_NAME, SHM_BLOCK_SIZE) != 0) {
        log_error("共享内存初始化失败");
        return FB_ERROR;
    }

    FILE *fsrc = fopen(src, "rb");
    if (!fsrc) {
        log_error("打开源文件失败: %s", src);
        shm_destroy();
        return FB_ERROR;
    }

    FILE *fdst = fopen(dst, "wb");
    if (!fdst) {
        log_error("打开目标文件失败: %s", dst);
        fclose(fsrc);
        shm_destroy();
        return FB_ERROR;
    }

    unsigned char buf[SHM_BLOCK_SIZE];
    size_t n;
    long total_written = 0;

    // 获取源文件大小
    fseek(fsrc, 0, SEEK_END);
    long file_size = ftell(fsrc);
    fseek(fsrc, 0, SEEK_SET);

    int last_percent = -1;

    while ((n = fread(buf, 1, SHM_BLOCK_SIZE, fsrc)) > 0) {
        // 写入共享内存
        if (shm_write(buf, n) != n) {
            log_error("写入共享内存失败");
            fclose(fsrc);
            fclose(fdst);
            shm_destroy();
            return FB_ERROR;
        }

        // 读取共享内存到目标文件
        if (shm_read(buf, n) != n) {
            log_error("从共享内存读取失败");
            fclose(fsrc);
            fclose(fdst);
            shm_destroy();
            return FB_ERROR;
        }

        if (fwrite(buf, 1, n, fdst) != n) {
            log_error("写入目标文件失败");
            fclose(fsrc);
            fclose(fdst);
            shm_destroy();
            return FB_ERROR;
        }

        total_written += n;

        // 计算进度百分比，每变化 10% 输出一次
        int percent = (int)((total_written * 100) / file_size);
        if (percent / 10 != last_percent / 10) {
            log_info("复制进度: %d%% (%ld/%ld)", percent, total_written, file_size);
            last_percent = percent;
        }
    }

    fclose(fsrc);
    fclose(fdst);
    shm_destroy();
    log_info("文件复制成功: %s -> %s", src, dst);
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
