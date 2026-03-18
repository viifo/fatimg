#include <string.h>
#include <time.h>
#include <ctype.h>
#include <stdio.h>
#include <sys/time.h>
#include "../include/fatimg.h"


/**
 * 格式化短文件/目录名为 8字节文件名 + 3字节扩展名 + 字符串结束符'\0' 格式
 * 使用字符串结束符'\0'是为了方便比较两个字符串
 * strcmp()函数基于'\0'比较，否则可能会出现比较结果不正确
 * @param orgName - 原始短文件/目录名名
 * @param desName - 格式化后的短文件/目录名名
 */
void formatFileName(char* orgName, char* desName) {
    int i = 0, endFlag = 0;
    char* suffixName;

    // 获取文件后缀名(包含'.')
    suffixName = strrchr(orgName, '.');
    if (NULL == suffixName) {
        // 此文件没有扩展名
        // 文件名最多 8 字符
        for(i = 0; i < 11; i ++) {
            if (i < 8 && orgName[i] != '\0') {
                desName[i] = toupper((unsigned char) orgName[i]);
            } else {
                desName[i] = ' ';
            }
        }
    } else {
        // 此文件存在扩展名
        // 计算从符号'.'到字符串首字符的长度（地址相减）
        int nameLen = (int)(suffixName - orgName);
        // 填充文件名部分，文件名最多 8 字符
        for (i = 0; i < 8; i ++) {
            if (i < nameLen && orgName[i] != '\0') {
                desName[i] = toupper((unsigned char) orgName[i]);
            } else {
                desName[i] = ' '; // 补足空格
            }
        }
        // 填充扩展名部分 (点之后的 3 位, 跳过 '.')
        char* ext = suffixName + 1;
        for (i = 0; i < 3; i++) {
            if (!endFlag && ext[i] != '\0') {
                desName[8 + i] = toupper((unsigned char) ext[i]);
            } else {
                endFlag = 1;
                desName[8 + i] = ' '; // 补足空格
            }
        }
    }
    // 字符串结尾符
    desName[11] = '\0';
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
    unsigned short fDate = 0;

    // 获取年月日
    time(&t);
    st = localtime(&t);
    year = st->tm_year + 1900 - 1980;
    mon = st->tm_mon + 1;
    day = st->tm_mday;

    // 结果的高7位表示目标年到1980年的偏移
    year <<= 9;
    fDate |= year;
    // 结果的中间4位表示月
    mon <<= 5;
    fDate |= mon;
    // 结果的低5位表示日
    fDate |= day;

    return fDate;
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
    // 结果的中间6位表示分
    min <<= 5;
    ftime |= min;
    // 结果的低5位表示秒/2取整
    ftime |= sec;

    return ftime;
}

/**
 * 文件创建日期数组转FAT12日期格式
 * @param time 文件创建时间数组
 * @return
 */
unsigned short formatCreateDateArray(int *time) {
    int year = time[0];
    if (year < 1980) year = 1980;
    return (unsigned short)(((year - 1980) << 9) | (time[1] << 5) | time[2]);
}

/**
 * 文件创建时间数组转FAT12时间格式
 * @param time 文件创建时间数组
 * @return
 */
unsigned short formatCreateTimeArray(int *time) {
    // 秒数在 FAT 中以 2 秒为单位存储，所以要除以 2
    return (unsigned short)((time[3] << 11) | (time[4] << 5) | (time[5] / 2));
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
 * 生成一个卷标序号
 * $$VolumeID = [ (Month + Day) \ll 24 ] \ | \ [ Hour \ll 16 ] \ | \ [ (Sec \ll 8) + MSec ]$$
 * @return
 */
unsigned int getVolumeID() {
    struct timeval tv;
    struct tm *tm_info;

    // 获取当前精确时间
    gettimeofday(&tv, NULL);
    time_t current_time = (time_t) tv.tv_sec;
    tm_info = localtime(&current_time);

    // 提取各部分分量
    unsigned int month = (unsigned int)tm_info->tm_mon + 1; // tm_mon 是 0-11
    unsigned int day   = (unsigned int)tm_info->tm_mday;
    unsigned int hour  = (unsigned int)tm_info->tm_hour;
    unsigned int sec   = (unsigned int)tm_info->tm_sec;
    unsigned int msec  = (unsigned int)(tv.tv_usec / 1000); // 微秒转毫秒

    // 按照公式合成
    unsigned int volumeID = 0;
    volumeID |= ((month + day) << 24);
    volumeID |= (hour << 16);
    volumeID |= ((sec << 8) + msec);

    return volumeID;
}

/**
 * 格式化 FAT12 卷标
 *
 * @param dest 目标缓冲区，必须至少 12 字节（11字节+1个结束符）
 * @param src 源卷标字符串，可以为 NULL
 */
void formatFat12VolumeLabel(char *dest, const char* src) {
    const char* input = NULL;

    if (src && src[0] != '\0') {
        input = src;
    } else {
        input = "FATIMG     ";  // 最终默认值
    }
    // 计算长度（最大11）
    size_t input_len = strlen(input);
    if (input_len > 11) {
        input_len = 11;
    }
    // 转换为大写并复制
    int i;
    for (i = 0; i < input_len; i++) {
        unsigned char c = (unsigned char)input[i];
        dest[i] = toupper(c);
    }
    // 用空格填充剩余部分
    for (; i < 11; i++) {
        dest[i] = ' ';
    }
    // 添加字符串结束符
    dest[11] = '\0';
}

/**
 * 检查卷标是否合法
 * FAT卷标不允许的字符: * ? . , ; : / \ | + = < > [ ]
 */
char isValidFat12VolumeLabel(const char* label) {
    if (!label) return 0;
    const char* invalid_chars = "*?.,;:/\\|+=> <[]";
    for (int i = 0; i < 11 && label[i] && label[i] != ' '; i++) {
        for (int j = 0; invalid_chars[j]; j++) {
            if (label[i] == invalid_chars[j]) {
                return 0;
            }
        }
    }
    return 1;
}
