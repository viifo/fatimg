#include <stdio.h>
#include <time.h>
#include "../include/fatimg.h"

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#include <direct.h>
#else
#include <sys/stat.h>
#endif

/**
 * 获取文件类型
 * @param path
 * @return FileType
 */
FILE_TYPE getFileType(const char *path) {
#ifdef _WIN32
    DWORD dwAttrib = GetFileAttributes(path);

    if (dwAttrib == INVALID_FILE_ATTRIBUTES) {
        return TYPE_NOT_FOUND;
    }

    if (dwAttrib & FILE_ATTRIBUTE_DIRECTORY) {
        return TYPE_DIRECTORY;
    } else {
        return TYPE_FILE;
    }
#else
    struct stat path_stat;
    if (stat(path, &path_stat) != 0) {
        return TYPE_NOT_FOUND;
    }

    if (S_ISDIR(path_stat.st_mode)) {
        return TYPE_DIRECTORY;
    } else if (S_ISREG(path_stat.st_mode)) {
        return TYPE_FILE;
    } else {
        return TYPE_UNKNOWN;
    }
#endif
}

/**
 * 获取文件创建时间
 * dest[0] - 年,
 * dest[1] - 月,
 * dest[2] - 日,
 * dest[3] - 时,
 * dest[4] - 分,
 * dest[5] - 秒
 * @param path
 * @param dest
 */
void getFileCreateTimeArray(const char *path, int *dest) {
#if defined(_WIN32) || defined(_WIN64)
    WIN32_FILE_ATTRIBUTE_DATA data;
    if (!GetFileAttributesExA(path, GetFileExInfoStandard, &data)) return;
    SYSTEMTIME st;
    // 使用 ftCreationTime (创建时间)
    FileTimeToSystemTime(&data.ftCreationTime, &st);
    dest[0] = st.wYear;
    dest[1] = st.wMonth;
    dest[2] = st.wDay;
    dest[3] = st.wHour;
    dest[4] = st.wMinute;
    dest[5] = st.wSecond;
#else
    struct stat st;
    if (stat(path, &st) != 0) return;
    // 使用 st.st_birthtimespec.tv_sec 创建时间 (秒)
    struct tm *tm_info = localtime(&st.st_birthtimespec.tv_sec);
    dest[0] = tm_info->tm_year + 1900;
    dest[1] = tm_info->tm_mon + 1;
    dest[2] = tm_info->tm_mday;
    dest[3] = tm_info->tm_hour;
    dest[4] = tm_info->tm_min;
    dest[5] = tm_info->tm_sec;
#endif
}


/**
 * 查找FAT空闲簇数
 * @param fp - fat镜像文件句柄
 * @param fatPos - fat起始位置(字节)
 * @param fatSize - fat大小(字节)
 * @param type - fat类型
 * @return 返回FAT空闲簇数
 */
unsigned int getFreeClusterNum(FILE *fp, long fatPos, long fatSize, FAT_TYPE type) {
    // FAT文件分配表对应的数据簇从第2个表项开始，即从第2个表项开始查找，一直查找到最后
    unsigned int startNum = 2, count = 0, temp = 0;
    switch (type) {
        case FAT12: {
            while((temp = findEmptyCluster(fp, fatPos, fatSize, startNum, type)) > 0) {
                startNum = temp + 1;
                count ++;
            }
        } break;
        case FAT32: {

        } break;
        default: {}
    }
    return count;
}


/**
 * 在FAT中寻找空簇
 * @param fp - fat镜像文件句柄
 * @param fatPos - fat起始位置(字节)
 * @param fatSize - fat大小(字节)
 * @param startNum - 从哪个fat表项开始查找
 * @param type - fat类型
 * @return 返回一个可用的空簇号
 */
unsigned int findEmptyCluster(FILE *fp, long fatPos, long fatSize, unsigned int startNum, FAT_TYPE type) {
    unsigned int i, loopCount;
    switch (type) {
        case FAT12: {
            // 计算一个FAT表有多少项表项并循环查找空值
            loopCount = (unsigned int)(fatSize / 1.5);
            for(i = startNum; i <= loopCount; i ++) {
                if(getNextClusterLinkNum(fp, fatPos, i, FAT12) == 0) {
                    return i;
                }
            }
        } break;
        case FAT32: {

        } break;
        default: {}
    }
    return 0;
}


/**
 * 通过本簇号在FAT中的偏移查找文件/目录簇链的下一簇号
 * @param fp - fat镜像文件句柄
 * @param offset - 本簇号在FAT文件分配表中的偏移
 * @param type - fat类型
 * @return 文件/目录簇链的下一簇号
 */
unsigned int getNextClusterLinkNum(FILE* fp, long fatPos, unsigned int clusterNum, FAT_TYPE type) {
    unsigned short nextClusterNum12 = 0;
    unsigned int nextClusterNum = 0;
    switch (type) {
        case FAT12: {
            // 获取FAT中的相对位移
            fseek(fp, (long)(fatPos + clusterNum * 1.5), SEEK_SET);
            // 读出两个字节（16位模式下为一个字）
            fread(&nextClusterNum12, 2, 1, fp);
            // 偶数项取低 12 位，奇数项取高 12 位
            nextClusterNum =  (clusterNum % 2 == 0 ? nextClusterNum12 & 0xFFF : nextClusterNum12 >> 4);
        } break;
        case FAT32: {

        } break;
        default: {}
    }
    return nextClusterNum;
}


/**
 * 向本簇号指向的FAT文件分配表表项中写入簇链的下一簇号
 * @param fp - fat镜像文件句柄
 * @param fat1Pos -  fat1起始位置(字节)
 * @param fat2Pos -  fat2起始位置(字节)
 * @param clusterNum - 本簇号
 * @param nextClusterNum  - 下一簇号
 * @param type - fat类型
 */
void setNextClusterLinkNum(FILE *fp, long fat1Pos, long fat2Pos, unsigned int clusterNum, unsigned int nextClusterNum, FAT_TYPE type) {
    unsigned int temp;

    switch (type) {
        case FAT12: {

            // 读取 本簇号 对应的 FAT表项 所在位置的2个字节
            fseek(fp, (long)(fat1Pos + clusterNum * 1.5), SEEK_SET);
            fread(&temp, 2, 1, fp);

            if(clusterNum % 2 == 0) {
                // 偶数项放在低 12 位
                nextClusterNum |= (temp & 0b1111000000000000);
            } else {
                // 奇数项放在高 12 位
                nextClusterNum = ((nextClusterNum << 4) | (temp & 0b1111));
            }

            // 将FAT表项值写回FAT1
            fseek(fp, (long)(fat1Pos + clusterNum * 1.5), SEEK_SET);
            fwrite(&nextClusterNum, 2, 1, fp);
            // 将FAT表项值写回FAT2
            fseek(fp, (long)(fat2Pos + clusterNum * 1.5), SEEK_SET);
            fwrite(&nextClusterNum, 2, 1, fp);

        } break;
        case FAT32: {

        } break;
        default: {}
    }

}

