import ctypes
import os

# 动态库路径
LIB_PATH = os.path.join(os.path.dirname(__file__), "../lib/libfile_backend.so")

# 加载动态库
_lib = ctypes.CDLL(LIB_PATH)

# ---------------------------------
# 设置参数类型和返回值
# ---------------------------------
# int fb_copy_file(const char *src, const char *dst);
_lib.fb_copy_file.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
_lib.fb_copy_file.restype = ctypes.c_int

# int fb_move_file(const char *src, const char *dst);
_lib.fb_move_file.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
_lib.fb_move_file.restype = ctypes.c_int

# int fb_delete_file(const char *path);
_lib.fb_delete_file.argtypes = [ctypes.c_char_p]
_lib.fb_delete_file.restype = ctypes.c_int

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
    return _lib.fb_copy_file(src.encode('utf-8'), dst.encode('utf-8'))

def move_file(src, dst):
    return _lib.fb_move_file(src.encode('utf-8'), dst.encode('utf-8'))

def delete_file(path):
    return _lib.fb_delete_file(path.encode('utf-8'))

if __name__ == "__main__":
    config_init(shm_size=4096, log_file="file_backend.log")

    # 复制文件
    res = copy_file("/tmp/a.txt", "/tmp/b.txt")
    print("复制结果:", res)

    # 移动文件
    res = move_file("/tmp/b.txt", "/tmp/c.txt")
    print("移动结果:", res)

    # 删除文件
    res = delete_file("/tmp/c.txt")
    print("删除结果:", res)

    # 清理配置
    config_destroy()