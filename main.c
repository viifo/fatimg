#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "include/fatimg.h"


/** 显示程序用法 */
void help(char* programName);
/** 错误的参数 */
int badArg();
/** 错误的命令 */
int badCommand();
/** 自定义FAT镜像创建 */
int customCreateImg(char* imgPath, char* bootPath, float size, FAT_TYPE type);


/**
 * FAT12 软盘镜像工具
 * 主要用于拷贝文件至软盘中
 * @param argc - 控制台命令参数数量
 * @param argv - 控制台命令参数
 * @return
 */
int main(int argc, char* argv[]) {
    int i;
    char* str = NULL;
    FAT_TYPE type = FAT12;
    float size = 0;

    // --help
    // 显示提示信息
    if(argc == 2 && !strcasecmp(argv[1], "--help")) {

        // 从路径中取出程序名
        str = strrchr(argv[0], SEPARATOR);
        if (str != NULL) str = str + 1;
        else str = argv[0];
        help(str);

    }
    // -c <dest file>
    // 拷贝目标文件到fat镜像中
    else if(argc == 4 && !strcasecmp(argv[2], "-c")) {

        // FAT12文件属性 :
        // 0x00 - 普通文件，0x01 - 只读，0x02 - 隐藏，0x04 - 系统文件，0x10 - 目录
        type = getImageFatType(argv[1]);
        if (type == FAT12) {
            printf("argv[1] = %s, argv[3] = %s\n", argv[1], argv[3]);
            i = copyFileToFat12img(argv[1], argv[3], 0);
        } else {
            printf("Bad FAT image format.\n");
            return BAD_FORMAT;
        }

        if (i == NO_FIND) {
            printf("not find file.\n");
            return NO_FIND;
        } else if (i == INSUFFICIENT_SPACE) {
            printf("Insufficient disk image space.\n");
            return INSUFFICIENT_SPACE;
        }

    }
    // 创建FAT镜像文件
    else if (argc >= 2) {

        // 默认创建标准 FAT12 镜像文件.
        if(argc == 2 && argv[1][0] != '-') {
            i = createEmptyFat12img(argv[1]);
            if (i == ERROR) {
                printf("Create image file fail.\n");
                return ERROR;
            }
        }
        else if(argc >= 4 && argc % 2 == 0) {
            // 循环提取信息
            for(i = 2; i < argc; i += 2) {
                // -b <boot file>
                // 使用自定义的引导扇区文件创建FAT镜像
                if (!strcasecmp(argv[i], "-b")) {
                    str = argv[i + 1];
                    if (str == NULL) {
                        return badArg();
                    }
                }
                // -f <formatNum>
                // 指定创建的FAT格式
                else if (!strcasecmp(argv[i], "-f")) {
                    type = (char)atoi(argv[i + 1]);
                    if (type != FAT12 && type != FAT16 && type != FAT32 && type != EXFAT) {
                        return badArg();
                    }
                }
                // -s <size>
                // 指定创建的FAT镜像文件大小(对fat12无效)
                else if (!strcasecmp(argv[i], "-s")) {

                    size = atof(argv[i + 1]);
                    if (size == 0) {
                        return badArg();
                    }

                } else return badCommand();
            }

            // 自定义FAT镜像创建
            return customCreateImg(argv[1], str, size, type);

        } else return badArg();

    } else return badArg();

    return 0;
}


/**
 * 自定义FAT镜像创建
 * @param imgPath - 镜像文件路径
 * @param bootPath - 自定义引导扇区文件路径
 * @param size - 文件大小
 * @param type - 文件格式
 * @return
 */
int customCreateImg(char* imgPath, char* bootPath, float size, FAT_TYPE type) {
    int result;
    switch (type) {
        case FAT12: {
            if (bootPath == NULL) result = createEmptyFat12img(imgPath);
            else result = createCustomBootFat12img(imgPath, bootPath);
            if (result == NO_FIND) {
                printf("not find boot file.\n");
                return NO_FIND;
            } else if (result == ERROR) {
                printf("Create image file fail.\n");
                return ERROR;
            } else if (result == BAD_FORMAT) {
                printf("Invalid boot file.\n");
                return BAD_FORMAT;
            }
        } break;
        case FAT16:
        case FAT32:
        case EXFAT: break;
        default: {
            return badArg();
        }
    }

    return OK;
}


/**
 * 错误的参数
 * @return
 */
int badArg() {
    printf("Missing arguments. \nTry `--help' for more information.\n");
    return ERROR;
}


/**
 * 错误的参数
 * @return
 */
int badCommand() {
    printf("Bad Command. \nTry `--help' for more information.\n");
    return ERROR;
}


/**
 * 显示程序用法
 * @param programName - 程序名
 */
void help(char* programName) {
    printf("Usage: %s <image file> [options]...\n", programName);
    printf("Options: \n");
    printf("  %-15s\t%s\n", "--help", "Display this information.");
    printf("  %-15s\t%s\n", "-c <dest file>", "Copy dest file to FAT12 image. \n\t\t\tThis command can only be used alone.\n");
    printf("  %-15s\t%s\n", "-b <boot file>", "Create a standard FAT12 image and init the image with boot file.");
    printf("  %-15s\t%s\n", "-f <12/16/32/64>", "Create a FAT12/FAT16/FAT32/EXFAT image.");
    printf("  %-15s\t%s\n", "-s <img size(MB)>", "Create a standard FAT12 image.");
}

