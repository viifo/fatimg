cmake_minimum_required(VERSION 3.15)
project(fatimg C)

set(CMAKE_C_STANDARD 99)

# FAT12 镜像工具
add_executable(fatimg main.c fat12img.c fat32img.c include/fatimg.h utils/fatUtil.c utils/formatUtil.c)

