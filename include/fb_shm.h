#ifndef FB_SHM_H
#define FB_SHM_H

#include <stddef.h>

// 初始化共享内存
// name: 唯一名称
// size: 大小（字节）
// 返回 0 成功，-1 失败
int shm_init(const char *name, size_t size);

// 写入共享内存（阻塞互斥）
// data: 数据指针
// size: 数据大小
// 返回实际写入字节数，-1失败
int shm_write(const void *data, size_t size);

// 从共享内存读取（阻塞互斥）
// buf: 缓冲区
// size: 读取大小
// 返回实际读取字节数，-1失败
int shm_read(void *buf, size_t size);

// 销毁共享内存和信号量
void shm_destroy();

#endif // FB_SHM_H
