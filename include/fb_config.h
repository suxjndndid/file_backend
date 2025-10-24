#ifndef FB_CONFIG_H
#define FB_CONFIG_H

#include <stddef.h>

// 初始化配置
// shm_block_size: 共享内存块大小（字节）
// log_file_path: 日志文件路径
// 返回 0 成功，-1 失败
int fb_config_init(size_t shm_block_size, const char *log_file_path);

// 获取共享内存块大小
size_t fb_config_get_shm_size();

// 获取日志文件路径
const char* fb_config_get_log_path();

// 销毁配置
void fb_config_destroy();

#endif // FB_CONFIG_H
