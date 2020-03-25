
GCC    = gcc
TARGET = fatimg
SRC    = fatimg.c fat12img.c utils/fatUtil.c utils/formatUtil.c

.PHONY: all

all: $(SRC)
	$(GCC) $(SRC) -o $(TARGET)

