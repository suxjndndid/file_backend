#ifndef FB_FILE_H
#define FB_FILE_H

#include "fb_export.h"

// 文件操作返回值
#define FB_SUCCESS 0
#define FB_ERROR   -1

#ifdef __cplusplus
extern "C" {
#endif

// 复制文件 src -> dst
API_EXPORT int fb_copy_file(const char *src, const char *dst);

// 移动文件 src -> dst（复制 + 删除）
API_EXPORT int fb_move_file(const char *src, const char *dst);

// 删除文件
API_EXPORT int fb_delete_file(const char *path);

#ifdef __cplusplus
}
#endif

#endif // FB_FILE_H
