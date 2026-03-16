
GCC    = gcc
OUT_DIR = outputs
TARGET = $(OUT_DIR)/fatimg
SRC    = fatimg.c fat12img.c fat32img.c utils/fatUtil.c utils/formatUtil.c

# 跨平台判断逻辑
ifeq ($(OS),Windows_NT)
    # Windows 平台
    MKDIR_P = if not exist $(OUT_DIR) mkdir $(OUT_DIR)
    # Windows 下可执行文件通常带 .exe
    TARGET  := $(TARGET).exe
    # 路径转换，某些环境下需要反斜杠
    FIX_PATH = $(subst /,\,$(1))
else
    # Unix / macOS 平台
    MKDIR_P = mkdir -p $(OUT_DIR)
    FIX_PATH = $(1)
endif

.PHONY: all

all: $(SRC)
	$(MKDIR_P)
	$(GCC) $(SRC) -o $(TARGET) -lm

