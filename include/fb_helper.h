#ifndef FB_HELPER_H
#define FB_HELPER_H

#include "fb_export.h"

#ifdef __cplusplus
extern "C" {
#endif

// Helper 接口：安全复制文件（在内部处理 fork / 子进程）
API_EXPORT int fb_copy_file_helper(const char *src, const char *dst);

// Helper 接口：安全移动文件
API_EXPORT int fb_move_file_helper(const char *src, const char *dst);

// Helper 接口：安全删除文件
API_EXPORT int fb_delete_file_helper(const char *path);

#ifdef __cplusplus
}
#endif

#endif // FB_HELPER_H
