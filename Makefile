# 编译器
CC = gcc
CFLAGS = -Wall -fPIC -Iinclude -pthread
LDFLAGS = -lrt -pthread

# 源码目录
SRC_DIR = src
OBJ_DIR = obj
LIB_DIR = lib

# 库名称
TARGET = $(LIB_DIR)/libfile_backend.so

# 源文件
SRC = $(wildcard $(SRC_DIR)/*.c)
OBJ = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRC))

# 默认目标
all: $(LIB_DIR) $(OBJ_DIR) $(TARGET)

# 创建目录
$(LIB_DIR):
	mkdir -p $(LIB_DIR)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# 编译目标
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# 生成动态库
$(TARGET): $(OBJ)
	$(CC) -shared -o $@ $^ $(LDFLAGS)
	@echo "生成动态库: $@"

# 清理
clean:
	rm -rf $(OBJ_DIR)/*.o $(TARGET)
