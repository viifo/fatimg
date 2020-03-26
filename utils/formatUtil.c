#include <string.h>
#include <time.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include "../include/fatimg.h"


/**
 * 格式化短文件/目录名为 8字节文件名 + 3字节扩展名 + 字符串结束符'\0' 格式
 * 使用字符串结束符'\0'是为了方便比较两个字符串
 * strcmp()函数基于'\0'比较，否则可能会出现比较结果不正确
 * @param orgName - 原始短文件/目录名名
 * @param desName - 格式化后的短文件/目录名名
 */
void formatFileName(char* orgName, char* desName) {
    int i, j;
    char* suffixName;

    // 获取文件后缀名(包含'.')
    suffixName = strrchr(orgName, '.');
    // 此文件没有扩展名
    if(NULL == suffixName) {
        // 文件名最多8字符
        for(i = 0; i < 8 && orgName[i]; i ++) {
            desName[i] = orgName[i];
        }
        // 文件名 + 扩展名(0字节) 不足11位, 补空格
        while(i < 11) desName[i++] = ' ';
        // 字符串结尾符
        desName[i] = 0;
    }
        // 此文件存在扩展名
    else {
        // 计算从符号'.'到字符串首字符的长度
        j = (int)(strlen(orgName) - strlen(suffixName));
        // 文件名最多8字符
        for(i = 0; i < 8 && i < j; i ++) {
            desName[i] = orgName[i];
        }
        // 文件名不足8位，补空格
        while(i < 8) desName[i++] = ' ';
        // 扩展名最多3字符
        for(j += 1; i < 11 && desName[j]; i ++, j ++) {
            desName[i] = orgName[j];
        }
        // 扩展名不足3位，补空格
        while(i < 11) desName[i++] = ' ';
        // 字符串结尾符
        desName[i] = 0;
    }
}


/**
 * 格式化日期为FAT12定义的时间格式
 * 第25、26位表示日期：共16位，从高到低，7位表示年到1980年的偏移，4位表示月，5位表示日
 * @return
 */
unsigned short formatDate() {
    time_t t;
    struct tm *st;
    short year, mon, day;
    unsigned short fdate = 0;

    // 获取年月日
    time(&t);
    st = localtime(&t);
    year = st->tm_year + 1900 - 1980;
    mon = st->tm_mon + 1;
    day = st->tm_mday;

    // 结果的高7位表示目标年到1980年的偏移
    year <<= 9;
    fdate |= year;
    // 结果的中4位表示月
    mon <<= 5;
    fdate |= mon;
    // 结果的低5位表示日
    fdate |= day;

    return fdate;
}


/**
 * 格式化时间为FAT12定义的时间格式
 * 第23、24位表示时间：共16位，从高到低，5位表示小时到08：00的偏移(时间早于08:00的，使用24+时间-8)，6位表示分钟数，5位表示秒/2取整；
 * @return
 */
unsigned short formatTime() {
    time_t t;
    struct tm *st;
    short hour, min, sec;
    unsigned short ftime = 0;

    // 获取小时、分钟
    time(&t);
    st = localtime(&t);
    hour = st->tm_hour;
    if (hour > 8) hour = hour - 8;
    else hour = hour + 24 - 8;
    min = st->tm_min;
    sec = st->tm_sec / 2;

    // 结果的高5位表示小时
    hour <<= 11;
    ftime |= hour;
    // 结果的中6位表示分
    min <<= 5;
    ftime |= min;
    // 结果的低5位表示秒/2取整
    ftime |= sec;

    return ftime;
}


/**
 * 根据短文件名获取长文件名目录项的校验值
 * @param shortName - 短文件名
 * @return 校验值
 */
unsigned char getChecksumByShortName(const char* shortName) {
    // 校验值为无符号数
    unsigned char checksum = 0;
    if (shortName == NULL) return 0;
    while (*shortName) {
        checksum = ((checksum & 1) ? 0x80 : 0) + (checksum >> 1) + *shortName ++;
    }
    return checksum;
}


/**
 * 获取FAT镜像文件类型
 * @param path - 镜像文件路径
 * @return FAT类型
 */
FAT_TYPE getImageFatType(const char* path) {
    FILE *fp;
    // FAT文件系统类型名大小为8字节
    char typeName[9] = {0};

    // 打开镜像文件
    // rb+ 以读写方式打开已存在的文件，若文件不存在，则打开失败
    fp = fopen(path, "rb+");
    if(fp == NULL) return NO_FIND;

    // 读取相对于0扇区的0x36偏移处
    // 此处存放着FAT12的文件系统类型名
    fseek(fp, 0x36, SEEK_SET);
    fread(&typeName, 8, 1, fp);
    if (strstr(strUpper(typeName), "FAT12") != NULL) {
        fclose(fp);
        return FAT12;
    }

    // 读取相对于0扇区的0x52偏移处
    // 此处存放着FAT32的文件系统类型名
    fseek(fp, 0x52, SEEK_SET);
    fread(&typeName, 8, 1, fp);
    if (strstr(strUpper(typeName), "FAT32") != NULL) {
        fclose(fp);
        return FAT32;
    }

    return UNKOWN;
}

/**
 * 字符串转大写字母
 * @param str
 * @return
 */
char* strUpper(char* str) {
    int count = 0;
    while (*str != '\0') {
        if (*str >= 97 && *str <= 122) {
            *str -= 32;
        }
        str ++;
        count ++;
    }
    return str - count;
}


/**
 * 随机产生一个分区序号
 * @return
 */
unsigned int getVolumeID() {
    // 使用当前系统时间作为随机数种子
    srand(time(NULL));
    return (unsigned int)rand();
}
