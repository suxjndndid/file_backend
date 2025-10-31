#include <stdio.h>
#include <stdlib.h>
#include "../include/fb_file.h"  // 头文件路径根据你项目结构

int main() {
    const char *src = "test_src.txt";
    const char *dst = "test_dst.txt";

    // 创建测试源文件
    FILE *f = fopen(src, "w");
    if (!f) {
        perror("创建源文件失败");
        return 1;
    }
    fprintf(f, "这是一个共享内存复制测试文件。\nThis is a shared memory copy test.\n");
    fclose(f);

    printf("[Test] 开始复制文件: %s -> %s\n", src, dst);

    // 调用动态库函数
    int ret = fb_copy_file(src, dst);
    if (ret == 0) {
        printf("[Test] ✅ 文件复制成功。\n");
    } else {
        printf("[Test] ❌ 文件复制失败。\n");
    }

    // 验证复制结果
    FILE *f1 = fopen(src, "r");
    FILE *f2 = fopen(dst, "r");
    if (!f1 || !f2) {
        printf("[Test] 打开文件失败，无法验证结果。\n");
        return 1;
    }

    int diff = 0;
    int c1, c2;
    while ((c1 = fgetc(f1)) != EOF && (c2 = fgetc(f2)) != EOF) {
        if (c1 != c2) {
            diff = 1;
            break;
        }
    }
    fclose(f1);
    fclose(f2);

    if (!diff)
        printf("[Test] ✅ 文件内容一致。\n");
    else
        printf("[Test] ❌ 文件内容不一致。\n");

    return 0;
}
