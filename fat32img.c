#include <stdio.h>
#include <math.h>
#include "include/fatimg.h"


/** FAT32 引导扇区结构 */
typedef struct {

    // 短跳指令 3字节
    unsigned char jmpBoot[3];
    // 软盘厂商名
    unsigned char OEMName[8];
    // 每个扇区的大小
    unsigned short bytesPerSector;
    // 每簇扇区数
    unsigned char sectorsPerCluster;
    // 保留扇区数
    unsigned short reservedSectors;
    // fat文件分配表个数表
    unsigned char FATNum;
    // 根目录文件数最大值（FAT32不使用此项）
    unsigned short rootEntCount;
    // 磁盘扇区总数 (FAT32不使用此项)
    unsigned short totalSectors16;
    // 介质描述符
    unsigned char media;
    // 一个FAT表所占扇区数(FAT32不使用此项)
    unsigned short sectorsPerFAT16;
    // 每磁道扇区数
    unsigned short sectorsPerTrack;
    // 磁头数
    unsigned short headsNum;
    // 隐藏扇区数
    unsigned int hiddenSectors;
    // totalSectors16 = 0， 由此项记录磁盘扇区总数
    unsigned int totalSectors32;
    // FAT32的每个FAT表所占扇区数
    unsigned int sectorsPerFAT32;
    // FAT32扩展标志
    unsigned short extendFlag;
    // 文件系统版本
    unsigned short fileSystemVersion;
    // 根目录起始簇号
    unsigned int rootFirstClusterNum;
    // 文件系统信息扇区扇区号
    unsigned short FSInfoSectorNum;
    // 备份引导扇区扇区号
    unsigned short backBootSectorNum;
    // 保留
    unsigned char reserved1[12];
    // 中断13的驱动器号
    unsigned char driverNum;
    // 保留
    unsigned char reserved2;
    // 扩展引导标记
    unsigned char bootSignature;
    // 卷序列号
    unsigned int volumeID;
    // 卷标
    unsigned char volumeLabel[11];
    // 文件系统类型名
    unsigned char fileSystemTypeName[8];
    // 引导代码
    unsigned char bootCode[420];
    // 引导扇区结束标记
    unsigned char bootEndFlag[2];

} __attribute__((packed)) BootSector;


/** 短文件名目录项结构 */
typedef struct {

    // 文件名
    unsigned char name[11];
    //  扩展名
    unsigned char extName[3];
    // 文件属性
    unsigned char attr;
    // 保留项
    unsigned char reserved;
    // 创建时间的毫秒数/10
    unsigned char createMS;
    // 文件/目录创建时间
    unsigned short createTime;
    // 文件/目录创建日期
    unsigned short createDate;
    // 最后访问日期
    unsigned short accessDate;
    // 起始簇号的高16位
    unsigned short clusterNumH16;
    // 最近修改时间
    unsigned short modifyTime;
    // 最近修改日期
    unsigned short modifyDate;
    // 起始簇号的低16位
    unsigned short clusterNumL16;
    // 文件大小
    unsigned int size;

} __attribute__((packed)) ShortDirItem;


/** 长文件名目录项结构 */
typedef struct {

    // 文件属性
    unsigned char attr;
    // 长文件名1
    unsigned char name1[10];
    // 长文件名目录项标志
    unsigned char longNameDirItemFlag;
    // 保留项
    unsigned char reserved;
    // 校验值
    unsigned char checksum;
    // 长文件名2
    unsigned char name2[12];
    // 文件起始簇号 (目前长置0)
    unsigned short firstCluster;
    // 长文件名3
    unsigned char name3[4];

} __attribute__((packed)) LongDirItem;

/**
 * 此结构体用于返回 镜像BPM信息计算结果
 */
typedef struct {
    // 镜像总扇区数
    unsigned int totalSectors;
    // 每个FAT所占扇区数
    unsigned int fatSectors;
    // 每簇扇区数
    unsigned char sectorsPerCluster;
    // 数据区总簇数
    unsigned int dataClusters;
    // 状态
    int status;
} CalResult;



/** 根据镜像大小计算最佳的每簇扇区数和FAT所占扇区数 */
CalResult getFAT32SectorsPerCluster(float size, int cluster);


/**
 * 创建自定义引导扇区的fat32软盘镜像
 * @param imgPath - 软盘镜像名
 * @param size - 镜像大小
 * @param cluster - 用户指定簇大小
 * @return
 */
int createCustomBootFat32img(char *imgPath, char *bootPath, float size, int cluster) {

    FILE *fp, *bf;
    unsigned long int i;
    // FSINFO 扩展引导标志
    unsigned int extBootFlag = 0x41615252;
    // FSINFO 签名
    unsigned int FSINFOFlag = 0x61417272;
    // FSINFO 下一可用簇号
    unsigned int nextEmptyClusNo = 2;
    // FAT32镜像BPM信息
    CalResult result;
    // FAT表保留项及设置一簇(2号簇)的根目录
    unsigned int fat32Flag[3] = {0x0FFFFFF0, 0x0FFFFFFF, 0x0FFFFFF8};
    // fat32软盘镜像引导扇区信息(512字节)
    BootSector bootSector;

    // 打开引导扇区文件并将引导扇区信息读入数组
    bf = fopen(bootPath, "rb");
    if(bf == NULL) return NO_FIND;
    fread(&bootSector, 512, 1, bf);
    fclose(bf);
    // 引导扇区无效
    if (bootSector.bootEndFlag[0] != 0x55 && bootSector.bootEndFlag[1] != 0xaa) {
        return BAD_FORMAT;
    }

    // 获取到数据区总簇数
    result = getFAT32SectorsPerCluster((float)bootSector.totalSectors32 * 512 / 1024 / 1024, bootSector.sectorsPerCluster);
    if (result.status == BAD_FORMAT) return BAD_FORMAT;

    // 新建镜像文件
    // wb, 文件存在会覆盖数据
    fp = fopen(imgPath, "wb");
    if(fp == NULL) return ERROR;

    // 将引导扇区信息写入软盘镜像
    fwrite(&bootSector, bootSector.bytesPerSector, 1, fp);

    // 写入文件系统信息扇区(FSINFO)
    // 写入扩展引导标志
    fwrite(&extBootFlag, 4, 1, fp);
    // 写入保留位
    for(i = 0; i < 480; i ++) {
        fputc(0, fp);
    }

    // 写入FSINFO签名
    fwrite(&FSINFOFlag, 4, 1, fp);
    // 写入文件系统空簇数
    fwrite(&result.dataClusters, 4, 1, fp);
    // 写入下一可用簇号
    fwrite(&nextEmptyClusNo, 4, 1, fp);
    // 写入保留位
    for(i = 0; i < 14; i ++) {
        fputc(0, fp);
    }
    // 写入扇区结束标记
    fwrite(&bootSector.bootEndFlag, 2, 1, fp);

    // 用0填充n个扇区直到引导备份扇区
    // 用0填充4扇区
    for(i = bootSector.bytesPerSector * 2; i < bootSector.backBootSectorNum * bootSector.bytesPerSector; i ++) {
        fputc(0, fp);
    }

    // 写入引导扇区备份
    fwrite(&bootSector, bootSector.bytesPerSector, 1, fp);

    // 用0填充n个扇区直到保留扇区结束
    for(i = bootSector.bytesPerSector * (2 + bootSector.backBootSectorNum);
        i < bootSector.reservedSectors * bootSector.bytesPerSector; i ++) {
        fputc(0, fp);
    }

    // 写 FAT1 表信息
    fwrite(fat32Flag, sizeof(fat32Flag), 1, fp);
    for(i = sizeof(fat32Flag); i < bootSector.sectorsPerFAT32 * bootSector.bytesPerSector; i ++) {
        fputc(0, fp);
    }

    // 写 FAT2 表信息
    // FAT2表紧随FAT1表
    fwrite(fat32Flag, sizeof(fat32Flag), 1, fp);
    for(i = sizeof(fat32Flag); i < bootSector.sectorsPerFAT32 * bootSector.bytesPerSector; i ++) {
        fputc(0, fp);
    }

    // 用0填充FAT32数据区及残留空间
    // 数据区及残留空间扇区 = 总扇区 - FAT表扇区数 * 2 - 保留扇区数
    for(i = 0; i < (bootSector.totalSectors32 - bootSector.sectorsPerFAT32 * 2
                    - bootSector.reservedSectors) * bootSector.bytesPerSector; i ++) {
        fputc(0, fp);
    }

    // 关闭文件
    fclose(fp);

    return OK;
}


/**
 * 创建空的fat32软盘镜像
 * @param imgPath - 软盘镜像名
 * @param size - 镜像大小
 * @param cluster - 用户指定簇大小(每簇扇区数)
 * @return
 */
int createEmptyFat32img(char *imgPath, float size, int cluster) {

    FILE *fp;
    unsigned long int i;
    // FSINFO 扩展引导标志
    unsigned int extBootFlag = 0x41615252;
    // FSINFO 签名
    unsigned int FSINFOFlag = 0x61417272;
    // FSINFO 下一可用簇号
    unsigned int nextEmptyClusNo = 2;
    // FAT表保留项及设置一簇(2号簇)的根目录
    unsigned int fat32Flag[3] = {0x0FFFFFF0, 0x0FFFFFFF, 0x0FFFFFF8};
    // 计算FAT32镜像BPM信息
    CalResult result = getFAT32SectorsPerCluster(size, cluster);
    // fat32软盘镜像引导扇区信息(512字节)
    BootSector bootSector = {
            {0xeb, 0x58, 0x90},
            "FATIMG  ",
            512,
            result.sectorsPerCluster,
            32,
            2,
            0,
            0,
            0xf0,
            0,
            32,
            2,
            0,
            result.totalSectors,
            result.fatSectors,
            0,
            0,
            2,
            1,
            6,
            {0},
            0,
            0,
            0x29,
            getVolumeID(),
            "FATIMG     ",
            "FAT32   ",
            {0},
            {0x55, 0xaa}
    };

    // 判断BPM信息是否计算成功
    if (result.status == BAD_FORMAT) return BAD_FORMAT;

    // 新建镜像文件
    // wb, 文件存在会覆盖数据
    fp = fopen(imgPath, "wb");
    if(fp == NULL) return ERROR;

    // 将引导扇区信息写入软盘镜像
    fwrite(&bootSector, bootSector.bytesPerSector, 1, fp);

    // 写入文件系统信息扇区(FSINFO)
    // 写入扩展引导标志
    fwrite(&extBootFlag, 4, 1, fp);
    // 写入保留位
    for(i = 0; i < 480; i ++) {
        fputc(0, fp);
    }

    // 写入FSINFO签名
    fwrite(&FSINFOFlag, 4, 1, fp);
    // 写入文件系统空簇数
    fwrite(&result.dataClusters, 4, 1, fp);
    // 写入下一可用簇号
    fwrite(&nextEmptyClusNo, 4, 1, fp);
    // 写入保留位
    for(i = 0; i < 14; i ++) {
        fputc(0, fp);
    }
    // 写入扇区结束标记
    fwrite(&bootSector.bootEndFlag, 2, 1, fp);

    // 用0填充4扇区
    for(i = 0; i < 4 * bootSector.bytesPerSector; i ++) {
        fputc(0, fp);
    }

    // 在6扇区写入引导扇区备份
    fwrite(&bootSector, bootSector.bytesPerSector, 1, fp);

    // 用0填充25扇区
    for(i = 0; i < 25 * bootSector.bytesPerSector; i ++) {
        fputc(0, fp);
    }

    // 写 FAT1 表信息
    fwrite(fat32Flag, sizeof(fat32Flag), 1, fp);
    for(i = sizeof(fat32Flag); i < bootSector.sectorsPerFAT32 * bootSector.bytesPerSector; i ++) {
        fputc(0, fp);
    }

    // 写 FAT2 表信息
    // FAT2表紧随FAT1表
    fwrite(fat32Flag, sizeof(fat32Flag), 1, fp);
    for(i = sizeof(fat32Flag); i < bootSector.sectorsPerFAT32 * bootSector.bytesPerSector; i ++) {
        fputc(0, fp);
    }

    // 用0填充FAT32数据区及残留空间
    // 数据区及残留空间扇区 = 总扇区 - FAT表扇区数 * 2 - 保留扇区数
    for(i = 0; i < (bootSector.totalSectors32 - bootSector.sectorsPerFAT32 * 2
        - bootSector.reservedSectors) * bootSector.bytesPerSector; i ++) {
        fputc(0, fp);
    }

    // 关闭文件
    fclose(fp);

    return OK;
}


/**
 * 根据镜像大小计算FAT32镜像BPM信息
 * @param size - 镜像大小
 * @param cluster - 用户指定簇大小(每簇扇区数)
 * @return 计算结果及状态
 */
CalResult getFAT32SectorsPerCluster(float size, int cluster) {
    CalResult result;
    // 每簇扇区数
    int sectorsPerCluster;
    // 镜像总扇区数
    unsigned int totalSectors = (unsigned int)size * 1024 * 1024 / 512;
    // FAT表及数据区总簇数
    unsigned int fatAndDataClusters;
    // FAT总簇数
    unsigned int fatClusters;
    // 数据区总簇数
    unsigned int dataClusters;
    // 每FAT所占扇区数
    unsigned int sectorsPerFat;

    // 用户指定簇大小
    if (cluster != 0 ) {
        // FAT表及数据区总簇数 = (扇区总数 - 保留扇区数) / 每簇扇区数
        fatAndDataClusters = (totalSectors - 32) / cluster;
        // FAT32必须至少包含65527个簇
        if (fatAndDataClusters < 65527) {
            result.status = BAD_FORMAT;
            return result;
        }

        // FAT总簇数 = (FAT表及数据区总簇数 * FAT项大小 + FAT保留项) / 扇区字节数 / 每簇扇区数 * FAT表个数;
        // 若存在小数，使用 ceil 向上取整
        fatClusters = ceil(((double)fatAndDataClusters * 4 + 8) / 512 / cluster * 2);
        // 数据区总簇数 = FAT表及数据区总簇数 - FAT总簇数
        dataClusters = fatAndDataClusters - fatClusters;
        // FAT32必须至少包含65527个簇
        if (dataClusters < 65527) {
            result.status = BAD_FORMAT;
            return result;
        }

        // 每FAT所占扇区数 = (数据区总簇数 * FAT项大小 + FAT保留项) / 扇区字节数
        // 若存在小数，使用 ceil 向上取整
        sectorsPerFat = ceil(((double)dataClusters * 4 + 8) / 512);

        result.totalSectors = totalSectors;
        result.fatSectors = sectorsPerFat;
        result.sectorsPerCluster = cluster;
        result.dataClusters = dataClusters;
        result.status = OK;
        return result;
    }

    // 循环计算最佳每簇扇区数
    for (sectorsPerCluster = 64; sectorsPerCluster >= 4; sectorsPerCluster /= 2) {
        // FAT表及数据区总簇数 = (扇区总数 - 保留扇区数) / 每簇扇区数
        fatAndDataClusters = (totalSectors - 32) / sectorsPerCluster;
        // FAT32必须至少包含65527个簇
        if (fatAndDataClusters < 65527) continue;

        // FAT总簇数 = (FAT表及数据区总簇数 * FAT项大小 + FAT保留项) / 扇区字节数 / 每簇扇区数 * FAT表个数;
        // 若存在小数，使用 ceil 向上取整
        fatClusters = ceil(((double)fatAndDataClusters * 4 + 8) / 512 / sectorsPerCluster * 2);
        // 数据区总簇数 = FAT表及数据区总簇数 - FAT总簇数
        dataClusters = fatAndDataClusters - fatClusters;
        // FAT32必须至少包含65527个簇
        if (dataClusters < 65527) continue;

        // 每FAT所占扇区数 = (数据区总簇数 * FAT项大小 + FAT保留项) / 扇区字节数
        // 若存在小数，使用 ceil 向上取整
        sectorsPerFat = ceil(((double)dataClusters * 4 + 8) / 512);

        result.totalSectors = totalSectors;
        result.fatSectors = sectorsPerFat;
        result.sectorsPerCluster = sectorsPerCluster;
        result.dataClusters = dataClusters;
        result.status = OK;
        return result;
    }

    result.status = BAD_FORMAT;
    return result;
}



