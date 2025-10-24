import os
import time
from PySide6.QtWidgets import (
    QMainWindow, QWidget, QHBoxLayout, QVBoxLayout, QTreeWidget,
    QTreeWidgetItem, QPushButton, QMessageBox, QApplication
)
from PySide6.QtGui import QIcon
from PySide6.QtCore import QSize, QTimer
from py_backend import copy_file, move_file

ICONS_PATH = os.path.join(os.path.dirname(__file__), "icons")

# ---------------- 主窗口 ----------------
class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("本地文件管理")
        self.resize(1000, 500)

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

                # 创建行
                item = QTreeWidgetItem(["", name, ftype, size, mtime])
                item.setIcon(0, QIcon(os.path.join(ICONS_PATH, icon)))
                tree.addTopLevelItem(item)
        except Exception as e:
            QMessageBox.warning(self, "错误", f"{side} 目录读取失败: {e}")

        # 固定列宽并截断超出部分
        tree.setColumnWidth(0, 30)   # 图标列
        tree.setColumnWidth(1, 250)  # 名称
        tree.setColumnWidth(2, 60)   # 类型
        tree.setColumnWidth(3, 80)   # 大小
        tree.setColumnWidth(4, 150)  # 修改时间
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

        QTimer.singleShot(0, lambda: self.update_tree(side))

    # ---------------- 复制 ----------------
    def copy_selected(self, direction):
        if direction == "left_to_right":
            src_path, dst_path = self.left_path, self.right_path
            tree_src, tree_dst = self.left_tree, self.right_tree
        else:
            src_path, dst_path = self.right_path, self.left_path
            tree_src, tree_dst = self.right_tree, self.left_tree

        sel_items = tree_src.selectedItems()
        if not sel_items:
            return

        for item in sel_items:
            if item.text(2) == "文件":
                src = os.path.join(src_path, item.text(1))
                dst = os.path.join(dst_path, item.text(1))
                if copy_file(src, dst) == 0:
                    QMessageBox.information(self, "成功", f"复制成功: {item.text(1)}")
                else:
                    QMessageBox.critical(self, "失败", f"复制失败: {item.text(1)}")

        self.update_tree("left")
        self.update_tree("right")

    # ---------------- 移动 ----------------
    def move_selected(self, direction):
        if direction == "left_to_right":
            src_path, dst_path = self.left_path, self.right_path
            tree_src, tree_dst = self.left_tree, self.right_tree
        else:
            src_path, dst_path = self.right_path, self.left_path
            tree_src, tree_dst = self.right_tree, self.left_tree

        sel_items = tree_src.selectedItems()
        if not sel_items:
            return

        for item in sel_items:
            if item.text(2) == "文件":
                src = os.path.join(src_path, item.text(1))
                dst = os.path.join(dst_path, item.text(1))
                if move_file(src, dst) == 0:
                    QMessageBox.information(self, "成功", f"移动成功: {item.text(1)}")
                else:
                    QMessageBox.critical(self, "失败", f"移动失败: {item.text(1)}")

        self.update_tree("left")
        self.update_tree("right")


if __name__ == "__main__":
    app = QApplication([])
    w = MainWindow()
    w.show()
    app.exec()
