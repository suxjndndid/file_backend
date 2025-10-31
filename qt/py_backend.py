import ctypes
import os

sys_library_path = "/usr/lib/x86_64-linux-gnu"
os.environ["LD_PRELOAD"] = os.path.join(sys_library_path, "libstdc++.so.6")
os.environ["LD_LIBRARY_PATH"] = sys_library_path + ":" + os.environ.get("LD_LIBRARY_PATH", "")
# 动态库路径
LIB_PATH = os.path.join(os.path.dirname(__file__), "../lib/libfile_backend.so")

# 加载动态库
_lib = ctypes.CDLL(LIB_PATH)

# ---------------------------------
# 设置参数类型和返回值（使用 helper 接口）
# ---------------------------------
# int fb_copy_file_helper(const char *src, const char *dst);
_lib.fb_copy_file_helper.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
_lib.fb_copy_file_helper.restype = ctypes.c_int

# int fb_move_file_helper(const char *src, const char *dst);
_lib.fb_move_file_helper.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
_lib.fb_move_file_helper.restype = ctypes.c_int

# int fb_delete_file_helper(const char *path);
_lib.fb_delete_file_helper.argtypes = [ctypes.c_char_p]
_lib.fb_delete_file_helper.restype = ctypes.c_int

# int fb_config_init(size_t shm_block_size, const char *log_file_path);
_lib.fb_config_init.argtypes = [ctypes.c_size_t, ctypes.c_char_p]
_lib.fb_config_init.restype = ctypes.c_int

# void fb_config_destroy();
_lib.fb_config_destroy.argtypes = []
_lib.fb_config_destroy.restype = None

# ---------------------------------
# Python 封装函数
# ---------------------------------
def config_init(shm_size=1024, log_file=None):
    log_file_bytes = log_file.encode('utf-8') if log_file else None
    return _lib.fb_config_init(shm_size, log_file_bytes)

def config_destroy():
    _lib.fb_config_destroy()

def copy_file(src, dst):
    print("调用 copy_file:", src, "->", dst)
    ret = _lib.fb_copy_file_helper(src.encode('utf-8'), dst.encode('utf-8'))
    print("返回值:", ret)
    return ret

def move_file(src, dst):
    print("调用 move_file:", src, "->", dst)
    ret = _lib.fb_move_file_helper(src.encode('utf-8'), dst.encode('utf-8'))
    print("返回值:", ret)
    return ret

def delete_file(path):
    print("调用 delete_file:", path)
    ret = _lib.fb_delete_file_helper(path.encode('utf-8'))
    print("返回值:", ret)
    return ret

# ---------------------------------
# 测试示例
# ---------------------------------
if __name__ == "__main__":
    config_init(shm_size=4096, log_file="file_backend.log")

    # # 复制文件
    # res = copy_file("/tmp/a.txt", "/tmp/b.txt")
    # print("复制结果:", res)

    # # 移动文件
    # res = move_file("/tmp/b.txt", "/tmp/c.txt")
    # print("移动结果:", res)

    # 删除文件
    res = delete_file("/home/wang/file_backend/tests/test_src.txt")
    print("删除结果:", res)

    # 清理配置
    config_destroy()
