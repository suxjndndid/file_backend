# 编译器
CC = /usr/bin/gcc
CXX = /usr/bin/g++
CFLAGS = -Wall -fPIC -Iinclude -pthread
CXXFLAGS = -Wall -fPIC -Iinclude -std=c++11 -pthread
LDFLAGS = -lrt -pthread -lstdc++

# 源码目录
SRC_DIR = src
OBJ_DIR = obj
LIB_DIR = lib

# 库名称
TARGET = $(LIB_DIR)/libfile_backend.so

# 源文件
SRC_C = $(wildcard $(SRC_DIR)/*.c)
SRC_CPP = $(wildcard $(SRC_DIR)/*.cpp)
OBJ_C = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRC_C))
OBJ_CPP = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRC_CPP))
OBJ = $(OBJ_C) $(OBJ_CPP)

# 默认目标
all: $(LIB_DIR) $(OBJ_DIR) $(TARGET)

# 创建目录
$(LIB_DIR):
	mkdir -p $(LIB_DIR)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# 编译 C 文件
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# 编译 C++ 文件
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 生成动态库
$(TARGET): $(OBJ)
	$(CXX) -shared -o $@ $^ $(LDFLAGS)
	@echo "生成动态库: $@"

# 清理
clean:
	rm -rf $(OBJ_DIR)/*.o $(TARGET)
