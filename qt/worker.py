from PySide6.QtCore import QThread, Signal
import backend_wrapper  # 你现有的C接口封装

class FileWorker(QThread):
    task_done = Signal(str, bool)  # 信号: 文件路径 + 是否成功

    def __init__(self, mode, src, dst=None):
        super().__init__()
        self.mode = mode  # "copy", "move", "delete"
        self.src = src
        self.dst = dst

    def run(self):
        success = False
        if self.mode == "copy":
            result = backend_wrapper.copy_file(self.src, self.dst)
            success = (result == 0)
        elif self.mode == "move":
            result = backend_wrapper.move_file(self.src, self.dst)
            success = (result == 0)
        elif self.mode == "delete":
            result = backend_wrapper.delete_file(self.src)
            success = (result == 0)

        # 发出信号通知主线程
        self.task_done.emit(self.mode, success)
