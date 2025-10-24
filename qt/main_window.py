import os
from PySide6.QtWidgets import (
    QMainWindow, QWidget, QHBoxLayout, QVBoxLayout, QListWidget,
    QListWidgetItem, QPushButton, QLabel, QMessageBox
)
from PySide6.QtGui import QIcon, Qt
from PySide6.QtCore import QSize
from py_backend import copy_file, move_file

ICONS_PATH = os.path.join(os.path.dirname(__file__), "icons")

# ---------------- 文件项 ----------------
class FileItem(QListWidgetItem):
    def __init__(self, name, ftype="file"):
        super().__init__(name)
        self.setData(Qt.UserRole, ftype)
        icon_file = "folder.png" if ftype == "dir" else "file.png"
        self.setIcon(QIcon(os.path.join(ICONS_PATH, icon_file)))

# ---------------- 主窗口 ----------------
class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()

        self.setWindowTitle("本地文件管理")
        self.resize(900, 500)

        self.central = QWidget()
        self.setCentralWidget(self.central)
        main_layout = QVBoxLayout(self.central)

        # 主体左右布局
        body_layout = QHBoxLayout()

        # 左侧文件列表
        self.left_list = QListWidget()
        self.left_list.setIconSize(QSize(24, 24))
        self.left_list.itemDoubleClicked.connect(lambda item: self.item_double(item, "left"))
        body_layout.addWidget(self.left_list)

        # 右侧文件列表
        self.right_list = QListWidget()
        self.right_list.setIconSize(QSize(24, 24))
        self.right_list.itemDoubleClicked.connect(lambda item: self.item_double(item, "right"))
        body_layout.addWidget(self.right_list)

        main_layout.addLayout(body_layout)

        # 操作按钮
        # 操作按钮
        btn_layout = QHBoxLayout()

        # 左边操作按钮（左→右）
        self.copy_l2r_btn = QPushButton("复制 →")
        self.move_l2r_btn = QPushButton("移动 →")
        self.copy_l2r_btn.clicked.connect(lambda: self.copy_selected("left_to_right"))
        self.move_l2r_btn.clicked.connect(lambda: self.move_selected("left_to_right"))

        # 右边操作按钮（右→左）
        self.copy_r2l_btn = QPushButton("← 复制")
        self.move_r2l_btn = QPushButton("← 移动")
        self.copy_r2l_btn.clicked.connect(lambda: self.copy_selected("right_to_left"))
        self.move_r2l_btn.clicked.connect(lambda: self.move_selected("right_to_left"))

        # 按钮顺序：左复制 → 左移动 → 右复制 ← 右移动 ←
        btn_layout.addWidget(self.copy_l2r_btn)
        btn_layout.addWidget(self.move_l2r_btn)
        btn_layout.addWidget(self.copy_r2l_btn)
        btn_layout.addWidget(self.move_r2l_btn)

        main_layout.addLayout(btn_layout)

        # 初始化路径
        self.left_path = os.path.expanduser("~")
        self.right_path = os.path.expanduser("~")
        self.update_list("left")
        self.update_list("right")

    # ---------------- 更新列表 ----------------
    def update_list(self, side):
        if side == "left":
            path = self.left_path
            lw = self.left_list
        else:
            path = self.right_path
            lw = self.right_list

        lw.clear()
        if path != "/":
            lw.addItem(FileItem("..", "dir"))

        try:
            for name in os.listdir(path):
                full_path = os.path.join(path, name)
                ftype = "dir" if os.path.isdir(full_path) else "file"
                lw.addItem(FileItem(name, ftype))
        except Exception as e:
            QMessageBox.warning(self, "错误", f"{side} 目录读取失败: {e}")

    # ---------------- 双击打开目录 ----------------
    def item_double(self, item: FileItem, side):
        if side == "left":
            path = self.left_path
        else:
            path = self.right_path

        ftype = item.data(Qt.UserRole)
        if item.text() == "..":
            path = os.path.dirname(path)
        elif ftype == "dir":
            path = os.path.join(path, item.text())

        if side == "left":
            self.left_path = path
        else:
            self.right_path = path
        self.update_list(side)

    # ---------------- 复制 ----------------
    def copy_selected(self, direction):
        if direction == "left_to_right":
            src_path, dst_path, lw_dst = self.left_path, self.right_path, "right"
            sel_items = self.left_list.selectedItems()
        else:  # right_to_left
            src_path, dst_path, lw_dst = self.right_path, self.left_path, "left"
            sel_items = self.right_list.selectedItems()

        if not sel_items:
            return

        for item in sel_items:
            if item.data(Qt.UserRole) == "file":
                src = os.path.join(src_path, item.text())
                dst = os.path.join(dst_path, item.text())
                if copy_file(src, dst) == 0:
                    QMessageBox.information(self, "成功", f"复制成功: {item.text()}")
                else:
                    QMessageBox.critical(self, "失败", f"复制失败: {item.text()}")
        self.update_list(lw_dst)

    # ---------------- 移动 ----------------
    def move_selected(self, direction):
        if direction == "left_to_right":
            src_path, dst_path, lw_dst = self.left_path, self.right_path, "right"
            sel_items = self.left_list.selectedItems()
        else:  # right_to_left
            src_path, dst_path, lw_dst = self.right_path, self.left_path, "left"
            sel_items = self.right_list.selectedItems()

        if not sel_items:
            return

        for item in sel_items:
            if item.data(Qt.UserRole) == "file":
                src = os.path.join(src_path, item.text())
                dst = os.path.join(dst_path, item.text())
                if move_file(src, dst) == 0:
                    QMessageBox.information(self, "成功", f"移动成功: {item.text()}")
                else:
                    QMessageBox.critical(self, "失败", f"移动失败: {item.text()}")

        # 刷新源和目标列表
        self.update_list("left")
        self.update_list("right")
