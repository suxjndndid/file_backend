#include "fb_helper.h"

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <functional>
#include <iostream>

#include "fb_file.h"
#include "fb_log.h"

// ⚠️ 通过子进程执行任务，避免线程与 fork 的混用
static int run_in_process(std::function<int()> func) {
  log_debug("run_in_process 开始");

  pid_t pid = fork();  // 创建子进程
  if (pid < 0) {
    log_error("fork 失败");
    return -1;
  }

  if (pid == 0) {
    // 子进程中执行任务
    try {
      int ret = func();  // 执行传入的函数
      log_debug("run_in_process 子进程执行完毕, ret=%d", ret);
      // 使用 _exit 以避免调用父进程的 atexit handlers 或刷新不应重复的缓冲区
      _exit(ret & 0xFF);  // exit code 限制为 0-255
    } catch (const std::exception& e) {
      log_error("子进程执行失败: %s", e.what());
      _exit(1);
    } catch (...) {
      log_error("子进程发生未知异常");
      _exit(1);
    }
  } else {
    // 父进程等待子进程完成并通过退出码获取结果
    int status = 0;
    if (waitpid(pid, &status, 0) < 0) {
      log_error("waitpid 失败");
      return -1;
    }
    log_debug("run_in_process 子进程退出, status=%d", status);
    if (WIFEXITED(status)) {
      int exitcode = WEXITSTATUS(status);
      return exitcode;
    } else if (WIFSIGNALED(status)) {
      int sig = WTERMSIG(status);
      log_error("子进程被信号 %d 终止", sig);
      return -1;
    } else {
      return -1;
    }
  }
}

// 复制文件的helper，使用进程执行文件复制操作
int fb_copy_file_helper(const char* src, const char* dst) {
  log_debug("fb_copy_file_helper 开始");

  if (!src || !dst) {
    log_error("src 或 dst 为 NULL，参数错误");
    return -1;
  }

  return run_in_process([=]() {
    log_debug("fb_copy_file_helper 在子进程中开始执行文件复制");

    int ret = fb_copy_file(src, dst);

    if (ret == 0) {
      log_info("文件复制成功：%s -> %s", src, dst);
    } else {
      log_error("文件复制失败：%s -> %s", src, dst);
    }

    return ret;
  });
}

// 移动文件的helper，使用进程执行文件移动操作
int fb_move_file_helper(const char* src, const char* dst) {
  log_debug("fb_move_file_helper 开始");

  if (!src || !dst) {
    log_error("src 或 dst 为 NULL，参数错误");
    return -1;
  }

  return run_in_process([=]() {
    log_debug("fb_move_file_helper 在子进程中开始执行文件移动");

    int ret = fb_move_file(src, dst);

    if (ret == 0) {
      log_info("文件移动成功：%s -> %s", src, dst);
    } else {
      log_error("文件移动失败：%s -> %s", src, dst);
    }

    return ret;
  });
}

// 删除文件的helper，使用进程执行文件删除操作
int fb_delete_file_helper(const char* path) {
  log_debug("fb_delete_file_helper 开始");

  if (!path) {
    log_error("path 为 NULL，参数错误");
    return -1;
  }

  return run_in_process([=]() {
    log_debug("fb_delete_file_helper 在子进程中开始执行文件删除");

    int ret = fb_delete_file(path);

    if (ret == 0) {
      log_info("文件删除成功：%s", path);
    } else {
      log_error("文件删除失败：%s", path);
    }

    return ret;
  });
}
