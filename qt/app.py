import sys
import os
from PySide6.QtWidgets import QApplication
from main_window import MainWindow
from py_backend import config_init, config_destroy

# ---------------- 配置后端 ----------------
SHM_SIZE = 4096  # 共享内存块大小
LOG_FILE = os.path.join(os.path.dirname(__file__), "file_backend.log")
# with open(LOG_FILE, "w") as f:
#     pass  # 打开模式 "w" 会清空文件
# 初始化后端配置
config_init(shm_size=SHM_SIZE, log_file=LOG_FILE)
os.environ["QT_QPA_PLATFORM"] = "xcb"

# ---------------- 启动 Qt 应用 ----------------
if __name__ == "__main__":
    app = QApplication(sys.argv)

    # 创建主窗口
    window = MainWindow()
    window.show()

    # 执行事件循环
    ret = app.exec()

    # 退出前销毁后端配置
    config_destroy()
    sys.exit(ret)
