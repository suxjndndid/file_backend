import os
import time
from PySide6.QtWidgets import (
    QMainWindow, QWidget, QHBoxLayout, QVBoxLayout, QTreeWidget,
    QTreeWidgetItem, QPushButton, QMessageBox, QApplication
)
from PySide6.QtGui import QIcon
from PySide6.QtCore import QSize, QThread, Signal
import py_backend as backend_wrapper  # 你现有的C接口封装

# -------- 在代码中设置平台，防止 Wayland 问题 --------
os.environ["QT_QPA_PLATFORM"] = "xcb"

ICONS_PATH = os.path.join(os.path.dirname(__file__), "icons")


# ---------------- 后台线程 ----------------
class FileWorker(QThread):
    task_done = Signal(str, bool)  # 文件操作模式, 是否成功

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


# ---------------- 主窗口 ----------------
class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("本地文件管理")
        self.resize(1000, 500)

        # 保存正在运行的线程，防止被销毁
        self.workers = []

        self.central = QWidget()
        self.setCentralWidget(self.central)
        main_layout = QVBoxLayout(self.central)

        # 主体左右布局
        body_layout = QHBoxLayout()

        # 左侧文件树
        self.left_tree = QTreeWidget()
        self.left_tree.setColumnCount(5)
        self.left_tree.setHeaderLabels(["", "名称", "类型", "大小", "修改时间"])
        self.left_tree.setIconSize(QSize(24, 24))
        self.left_tree.itemDoubleClicked.connect(lambda item, col: self.item_double(item, "left"))
        body_layout.addWidget(self.left_tree)

        # 右侧文件树
        self.right_tree = QTreeWidget()
        self.right_tree.setColumnCount(5)
        self.right_tree.setHeaderLabels(["", "名称", "类型", "大小", "修改时间"])
        self.right_tree.setIconSize(QSize(24, 24))
        self.right_tree.itemDoubleClicked.connect(lambda item, col: self.item_double(item, "right"))
        body_layout.addWidget(self.right_tree)

        main_layout.addLayout(body_layout)

        # 操作按钮
        btn_layout = QHBoxLayout()
        self.copy_l2r_btn = QPushButton("复制 →")
        self.move_l2r_btn = QPushButton("移动 →")
        self.copy_r2l_btn = QPushButton("← 复制")
        self.move_r2l_btn = QPushButton("← 移动")

        self.copy_l2r_btn.clicked.connect(lambda: self.copy_selected("left_to_right"))
        self.move_l2r_btn.clicked.connect(lambda: self.move_selected("left_to_right"))
        self.copy_r2l_btn.clicked.connect(lambda: self.copy_selected("right_to_left"))
        self.move_r2l_btn.clicked.connect(lambda: self.move_selected("right_to_left"))

        btn_layout.addWidget(self.copy_l2r_btn)
        btn_layout.addWidget(self.move_l2r_btn)
        btn_layout.addWidget(self.copy_r2l_btn)
        btn_layout.addWidget(self.move_r2l_btn)
        main_layout.addLayout(btn_layout)

        # 初始化路径
        self.left_path = os.path.expanduser("~")
        self.right_path = os.path.expanduser("~")
        self.update_tree("left")
        self.update_tree("right")

    # ---------------- 更新树 ----------------
    def update_tree(self, side):
        if side == "left":
            path = self.left_path
            tree = self.left_tree
        else:
            path = self.right_path
            tree = self.right_tree

        tree.clear()

        # 添加上级目录
        if path != "/":
            parent = QTreeWidgetItem(["", "..", "目录", "", ""])
            parent.setIcon(0, QIcon(os.path.join(ICONS_PATH, "folder.png")))
            tree.addTopLevelItem(parent)

        try:
            for name in os.listdir(path):
                full_path = os.path.join(path, name)
                if os.path.isdir(full_path):
                    ftype = "目录"
                    size = ""
                    icon = "folder.png"
                else:
                    ftype = "文件"
                    size = f"{os.path.getsize(full_path)/1024:.1f} KB"
                    icon = "file.png"

                mtime = time.strftime("%Y-%m-%d %H:%M", time.localtime(os.path.getmtime(full_path)))

                item = QTreeWidgetItem(["", name, ftype, size, mtime])
                item.setIcon(0, QIcon(os.path.join(ICONS_PATH, icon)))
                tree.addTopLevelItem(item)
        except Exception as e:
            QMessageBox.warning(self, "错误", f"{side} 目录读取失败: {e}")

        tree.setColumnWidth(0, 30)
        tree.setColumnWidth(1, 250)
        tree.setColumnWidth(2, 60)
        tree.setColumnWidth(3, 80)
        tree.setColumnWidth(4, 150)
        tree.setAlternatingRowColors(True)

    # ---------------- 双击打开目录 ----------------
    def item_double(self, item: QTreeWidgetItem, side):
        name = item.text(1)
        if side == "left":
            path = self.left_path
        else:
            path = self.right_path

        if name == "..":
            path = os.path.dirname(path)
        elif item.text(2) == "目录":
            path = os.path.join(path, name)

        if side == "left":
            self.left_path = path
        else:
            self.right_path = path

        self.update_tree(side)

    # ---------------- 复制 ----------------
    def copy_selected(self, direction):
        if direction == "left_to_right":
            src_path, dst_path = self.left_path, self.right_path
            tree_src = self.left_tree
        else:
            src_path, dst_path = self.right_path, self.left_path
            tree_src = self.right_tree

        sel_items = tree_src.selectedItems()
        if not sel_items:
            return

        for item in sel_items:
            if item.text(2) == "文件":
                src = os.path.join(src_path, item.text(1))
                dst = os.path.join(dst_path, item.text(1))
                worker = FileWorker("copy", src, dst)
                worker.task_done.connect(self.show_result)
                worker.start()
                self.workers.append(worker)

    # ---------------- 移动 ----------------
    def move_selected(self, direction):
        if direction == "left_to_right":
            src_path, dst_path = self.left_path, self.right_path
            tree_src = self.left_tree
        else:
            src_path, dst_path = self.right_path, self.left_path
            tree_src = self.right_tree

        sel_items = tree_src.selectedItems()
        if not sel_items:
            return

        for item in sel_items:
            if item.text(2) == "文件":
                src = os.path.join(src_path, item.text(1))
                dst = os.path.join(dst_path, item.text(1))
                worker = FileWorker("move", src, dst)
                worker.task_done.connect(self.show_result)
                worker.start()
                self.workers.append(worker)

    # ---------------- 显示结果 ----------------
    def show_result(self, mode, success):
        if success:
            QMessageBox.information(self, "成功", f"{mode} 操作成功")
        else:
            QMessageBox.critical(self, "失败", f"{mode} 操作失败")

        # 操作完成后刷新左右目录
        self.update_tree("left")
        self.update_tree("right")

        # 清理已结束线程
        self.workers = [w for w in self.workers if w.isRunning()]


if __name__ == "__main__":
    app = QApplication([])
    w = MainWindow()
    w.show()
    app.exec()
