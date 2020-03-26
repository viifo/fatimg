#include <stdio.h>
#include <string.h>
#include "include/fatimg.h"


/** FAT12软盘总扇区数 */
#define TOTAL_SECTORS 2880
/** 每扇区字节数 */
#define BYTES_SECTOR 512
/** FAT表起始扇区号 */
#define FAT_FIRST_SECTOR 1
/** FAT表所占扇区数 */
#define FAT_SECTOR_NUM 9
/** 根目录起始扇区号 */
#define ROOT_FIRST_SECTOR 19
/** 数据区起始扇区号 */
#define DATA_FIRST_SECTOR 33

/** 引导扇区结构体 */
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
    unsigned short reservedClusters;
    // fat文件分配表个数表
    unsigned char FATNum;
    // 根目录文件数最大值（根目录项数）
    unsigned short rootEntCount;
    // 磁盘扇区总数
    unsigned short totalSectors16;
    // 介质描述符
    unsigned char media;
    // 一个FAT表所占扇区数
    unsigned short sectorsPerFAT;
    // 每磁道扇区数
    unsigned short sectorsPerTrack;
    // 磁头数
    unsigned short headsNum;
    // 隐藏扇区数
    unsigned int hiddenSectors;
    // totalSectors16 = 0， 由此项记录磁盘扇区总数
    unsigned int totalSectors32;
    // 中断13的驱动器号
    unsigned char driverNum;
    // 保留
    unsigned char reserved1;
    // 扩展引导标记
    unsigned char bootSignature;
    // 卷序列号
    unsigned int volumeID;
    // 卷标
    unsigned char volumeLabel[11];
    // 文件系统类型名
    unsigned char fileSystemTypeName[8];
    // 引导代码
    unsigned char bootCode[448];
    // 引导扇区结束标记
    unsigned char bootEndFlag[2];

} __attribute__((packed)) BootSector;

/** 根目录区目录表项结构 */
typedef struct {
    // 文件名8字节 + 扩展名3字节 (11字节)
    unsigned char name[11];
    // 文件属性 (1字节)
    unsigned char attr;
    // 保留项 (10字节)
    unsigned char reserved[10];
    // 最后修改时间 (2字节)
    unsigned short writeTime;
    // 最后修改日期 (2字节)
    unsigned short writeDate;
    // 文件起始簇号 (2字节)
    unsigned short firstCluster;
    // 文件大小 (4字节)
    unsigned int size;
} __attribute__((packed)) DirItem;



/** 查找根目录区中的空值表项 */
int findEmptyRootDirItem(FILE*);
/** 在软盘镜像文件中寻找是否存在文件 */
int findFileInRootDir(FILE*, char*);
/** 从软盘镜像中删除文件 */
int deleteFileFromImg(FILE*, unsigned short);



/**
 * 创建一个标准的自定义引导扇区的fat12软盘镜像(1.44M)
 * @param imgPath - 软盘镜像
 * @param bootPath - 引导扇区二进制文件路径
 * @return
 */
int createCustomBootFat12img(char *imgPath, char *bootPath) {

    FILE *bf, *fp;
    unsigned long int i;
    unsigned char bootSector[BYTES_SECTOR] = {0};
    // FAT表的第0簇和第1簇为保留簇
    // 其中第0字节（首字节）表示磁盘类型，其值与BPB中介质描述符（BPB_Media）对应的磁盘类型相同(0xf0-软盘，0xf8-硬盘)
    // 第2，3字节代表 FAT 文件分配表标识符
    // 从第四个字节开始与用户数据区所有的簇一一对应
    char fatFlag[3] = {0xf0, 0xff, 0xff};

    // 打开引导扇区文件并将引导扇区信息读入数组
    bf = fopen(bootPath, "rb");
    if(bf == NULL) return NO_FIND;
    fread(bootSector, BYTES_SECTOR, 1, bf);
    fclose(bf);
    // 引导扇区无效
    if (bootSector[BYTES_SECTOR - 2] != 0x55
        || bootSector[BYTES_SECTOR - 1] != 0xaa) {
        return BAD_FORMAT;
    }

    // 新建镜像文件
    fp = fopen(imgPath, "wb");
    if(fp == NULL) return ERROR;

    // 将引导扇区信息写入软盘镜像
    fwrite(bootSector, BYTES_SECTOR, 1, fp);

    // 写 FAT1 表信息
    fwrite(fatFlag, 3, 1, fp);
    for(i = 3; i < FAT_SECTOR_NUM * BYTES_SECTOR; i++) {
        fputc(0, fp);
    }

    // 写 FAT2 表信息
    // FAT2表紧随FAT1表
    fwrite(fatFlag, 3, 1, fp);
    for(i = 3; i < FAT_SECTOR_NUM * BYTES_SECTOR; i++){
        fputc(0, fp);
    }

    // 用0填充FAT12用户数据区
    // 用户区数据区扇区数 = 总扇区数 - FAT表扇区数 * 2 - 引导扇区数
    for(i = 0; i < (TOTAL_SECTORS - FAT_SECTOR_NUM * 2 - 1) * BYTES_SECTOR; i++){
        fputc(0, fp);
    }

    // 关闭文件
    fclose(fp);

    return OK;
}


/**
 * 创建标准的空的fat12软盘镜像(1.44M)
 * @param imgPath - 软盘镜像名
 * @return
 */
int createEmptyFat12img(char *imgPath) {
    // FAT表的第0簇和第1簇为保留簇
    // 其中第0字节（首字节）表示磁盘类型，其值与BPB中介质描述符（BPB_Media）对应的磁盘类型相同(0xf0-软盘，0xf8-硬盘)
    // 第2，3字节代表 FAT 文件分配表标识符
    // 从第四个字节开始与用户数据区所有的簇一一对应
    char fatFlag[3] = {0xf0, 0xff, 0xff};
    FILE *fp;
    unsigned long int i;
    // fat12软盘镜像引导扇区信息(512字节)
    BootSector bootSector = {
            {0xeb, 0x4e, 0x90},
            "FATIMG  ",
            512,
            1,
            1,
            2,
            224,
            2880,
            0xf0,
            9,
            18,
            2,
            0,
            0,
            0,
            0,
            0x02,
            0,
            "FATIMG     ",
            "FAT12   ",
            {0},
            {0x55, 0xaa}
    };

    // 新建镜像文件
    // wb, 文件存在会覆盖数据
    fp = fopen(imgPath, "wb");
    if(fp == NULL) return ERROR;

    // 将引导扇区信息写入软盘镜像
    fwrite(&bootSector, BYTES_SECTOR, 1, fp);

    // 写 FAT1 表信息
    fwrite(fatFlag, 3, 1, fp);
    for(i = 3; i < FAT_SECTOR_NUM * BYTES_SECTOR; i++) {
        fputc(0, fp);
    }

    // 写 FAT2 表信息
    // FAT2表紧随FAT1表
    fwrite(fatFlag, 3, 1, fp);
    for(i = 3; i < FAT_SECTOR_NUM * BYTES_SECTOR; i++){
        fputc(0, fp);
    }

    // 用0填充FAT12用户数据区
    // 用户区数据区扇区数 = 总扇区数 - FAT表扇区数 * 2 - 引导扇区数
    for(i = 0; i < (TOTAL_SECTORS - FAT_SECTOR_NUM * 2 - 1) * BYTES_SECTOR; i++){
        fputc(0, fp);
    }

    // 关闭文件
    fclose(fp);

    return OK;
}


/**
 * 拷贝文件到FAT12软盘镜像
 * @param imgPath - 镜像文件
 * @param filePath - 要拷贝的文件
 * @param fileAttr - 要拷贝的文件属性
 * @return
 */
int copyFileToFat12img(char *imgPath, char *filePath, char fileAttr) {
    FILE *ifp, *fp;
    DirItem dirItem;
    unsigned short i;
    // 根目录表项序号
    int rootDirItemIndex;
    // 目录表项大小
    unsigned short dirItemSize = sizeof(DirItem);
    // 要拷贝的文件大小
    unsigned long fileSize = 0;
    char newFileName[12], *fileName;
    /** 拷贝数据临时中转数组 */
    unsigned char tempData[BYTES_SECTOR] = {0};
    unsigned short needSectors, remainingBytes;
    /** FAT文件簇链 */
    unsigned short fileSectorList[TOTAL_SECTORS] = {0};

    // 打开镜像文件
    // rb+ 以读写方式打开已存在的文件，若文件不存在，则打开失败
    ifp = fopen(imgPath, "rb+");
    if(ifp == NULL) return NO_FIND;
    // 打开要拷贝的文件
    fp = fopen(filePath, "rb");
    if(fp == NULL) return NO_FIND;

    // 从源文件路径中取出文件名
    // 因Windows支持两种文件分割符，所以都要判断一下
    fileName = strrchr(filePath, '\\');
    if (fileName == NULL) {
        fileName = strrchr(filePath, '/');
    }
    if (fileName == NULL) {
        // 没有文件分割符'\'或'/', 取原文件名
        fileName = filePath;
    } else {
        // 指针后移一位，去除文件名前的文件分割符
        fileName = fileName + 1;
    }

    // 若软盘镜像里存在同名文件，将此文件信息读出(获取文件大小)
    rootDirItemIndex = findFileInRootDir(ifp, fileName);
    if(rootDirItemIndex != NO_FIND) {
        fseek(ifp, ROOT_FIRST_SECTOR * BYTES_SECTOR + rootDirItemIndex * dirItemSize, SEEK_SET);
        fread(&dirItem, dirItemSize, 1, ifp);
    } else {
        dirItem.size = 0;
    }

    // ftell() 用于得到当前文件位置指针相对于文件首的偏移字节数
    // 获取要拷贝的文件大小(字节)
    fileSize = (fseek(fp, 0, SEEK_END), ftell(fp));
    // 剩余空间不足（包括同名文件部分）
    if((dirItem.size + getFreeClusterNum(ifp, FAT_FIRST_SECTOR * BYTES_SECTOR,
            FAT_SECTOR_NUM * BYTES_SECTOR, FAT12) * BYTES_SECTOR) < fileSize) {
        fclose(fp);
        fclose(ifp);
        return INSUFFICIENT_SPACE;
    }
    // 根目录区无空表项
    if(findEmptyRootDirItem(ifp) == NO_FIND) {
        fclose(fp);
        fclose(ifp);
        return INSUFFICIENT_SPACE;
    }

    // 删除同名文件
    if(rootDirItemIndex != NO_FIND) deleteFileFromImg(ifp, rootDirItemIndex);

    /**************** 向镜像中增加文件 ****************/
    // 除去所需的完整扇区后文件剩余的字节数
    remainingBytes = fileSize % BYTES_SECTOR;
    // 计算源文件所需扇区数
    needSectors = fileSize / BYTES_SECTOR + (remainingBytes > 0 ? 1 : 0);

    fileSectorList[0] = findEmptyCluster(ifp, FAT_FIRST_SECTOR * BYTES_SECTOR,
            FAT_SECTOR_NUM * BYTES_SECTOR, 2, FAT12);
    for(i = 1; i < needSectors; i++) {
        fileSectorList[i] = findEmptyCluster(ifp, FAT_FIRST_SECTOR * BYTES_SECTOR,
                FAT_SECTOR_NUM * BYTES_SECTOR, fileSectorList[i - 1] + 1, FAT12);
        setNextClusterLinkNum(ifp, FAT_FIRST_SECTOR * BYTES_SECTOR,
                (FAT_FIRST_SECTOR + FAT_SECTOR_NUM) * BYTES_SECTOR,fileSectorList[i - 1], fileSectorList[i], FAT12);
    }
    // 最后一个文件簇写入文件结束符EOF(0xff8 ~ 0xfff)
    setNextClusterLinkNum(ifp, FAT_FIRST_SECTOR * BYTES_SECTOR,
            (FAT_FIRST_SECTOR + FAT_SECTOR_NUM) * BYTES_SECTOR,fileSectorList[i - 1], 0xfff, FAT12);

    // 设置文件相关信息
    strncpy(dirItem.name, (formatFileName(fileName, newFileName), newFileName), 11);
    dirItem.attr = fileAttr;
    dirItem.writeTime = formatTime();
    dirItem.writeDate = formatDate();
    dirItem.firstCluster = fileSectorList[0];
    dirItem.size = fileSize;

    // 将带有文件信息的根目录表项写入根目录区
    rootDirItemIndex = findEmptyRootDirItem(ifp);
    fseek(ifp, ROOT_FIRST_SECTOR * BYTES_SECTOR + rootDirItemIndex * dirItemSize, SEEK_SET);
    fwrite(&dirItem, dirItemSize, 1, ifp);

    // 拷贝文件到相应扇区
    fseek(fp, 0, SEEK_SET);
    for(i = 0; i < needSectors - 1; i++) {
        // 从要拷贝的文件中读一扇区的数据到临时中转数组
        fread(tempData, BYTES_SECTOR, 1, fp);
        // FAT表项中的第0簇项和第0簇项为保留簇项，用做起始标记，
        // 但是数据区并不会浪费2个簇的空间，所以FAT表项的第2簇项对应数据区的0簇，第3簇项对应数据区的1簇..,以此类推
        // fileSectorList[i] - 2 表示FAT表项簇序号对应的数据区簇序号
        fseek(ifp, (fileSectorList[i] - 2 + DATA_FIRST_SECTOR) * BYTES_SECTOR, SEEK_SET);
        fwrite(tempData, BYTES_SECTOR, 1, ifp);
    }
    // 最后一个扇区单独处理
    // 最后一个扇区未满512字节，剩余部分填充0
    if(remainingBytes > 0) {
        fread(tempData, remainingBytes, 1, fp);
        for(i = remainingBytes; i < BYTES_SECTOR; i ++) {
            tempData[i] = 0;
        }
    } else {
        fread(tempData, BYTES_SECTOR, 1, fp);
    }
    // 将最后一扇区的数据写入软盘镜像
    fseek(ifp, (fileSectorList[needSectors - 1] - 2 + DATA_FIRST_SECTOR) * BYTES_SECTOR, SEEK_SET);
    fwrite(tempData, BYTES_SECTOR, 1, ifp);

    // 关闭文件
    fclose(fp);
    fclose(ifp);

    return OK;
}



/**
 * 从软盘镜像中删除文件
 * @param ifp - 软盘镜像文件句柄
 * @param rootDirItemIndex - 根目录表项序号
 * @return
 */
int deleteFileFromImg(FILE *ifp, unsigned short rootDirItemIndex) {
    // 文件簇链本簇号/下一个FAT文件簇链号
    unsigned short clusterLinkNum;
    // 目录表项大小
    unsigned short dirItemSize = sizeof(DirItem);
    DirItem tDirItem;

    // 读出指定的根目录表项
    fseek(ifp, ROOT_FIRST_SECTOR * BYTES_SECTOR + rootDirItemIndex * dirItemSize, SEEK_SET);
    fread(&tDirItem, dirItemSize, 1, ifp);

    // 循环清空FAT文件簇链直到文件末尾
    clusterLinkNum = tDirItem.firstCluster;
    do {
        setNextClusterLinkNum(ifp, FAT_FIRST_SECTOR * BYTES_SECTOR,
                (FAT_FIRST_SECTOR + FAT_SECTOR_NUM) * BYTES_SECTOR,clusterLinkNum, 0, FAT12);
        // 通过本簇号查找文件簇链的下一簇号
        clusterLinkNum = getNextClusterLinkNum(ifp, FAT_FIRST_SECTOR * BYTES_SECTOR, clusterLinkNum, FAT12);
    } while(clusterLinkNum < 0xff8);

    // 设置根目录区表项标记为已删除
    fseek(ifp, ROOT_FIRST_SECTOR * BYTES_SECTOR + rootDirItemIndex * dirItemSize, SEEK_SET);
    fputc(0xe5, ifp);

    return OK;
}


/**
 * 在软盘镜像文件中寻找是否存在文件
 * @param imgPath - 软盘镜像
 * @param fileName - 要查找的文件名
 * @return 文件的FAT表项序号
 */
int findFileInRootDir(FILE *ifp, char *fileName) {

    // 目录表项
    DirItem tDirItem;
    // 格式化后的文件名
    char newFileName[12];
    // 从根目录区查询出的文件名
    char desItemName[12] = {0};
    // 根目录区大小
    unsigned short rootDirSize;
    // 目录表项大小
    unsigned short dirItemSize = sizeof(DirItem);
    unsigned short i;

    // 格式化文件名为 8 + 3 + 0 格式
    formatFileName(fileName, newFileName);
    // 计算根目录区的大小
    rootDirSize = (DATA_FIRST_SECTOR - ROOT_FIRST_SECTOR) * BYTES_SECTOR;
    for(i = 0; i < rootDirSize / dirItemSize; i ++) {
        // 读取一个根目录表项
        fseek(ifp, ROOT_FIRST_SECTOR * BYTES_SECTOR + dirItemSize * i, SEEK_SET);
        fread(&tDirItem, dirItemSize, 1, ifp);

        // 获取根目录表项文件名(FAT根目录表项文件名没有'\0'结束符)
        strncpy(desItemName, tDirItem.name, 11);
        // 判断要查找的文件是否存在
        if(!strcmp(desItemName, newFileName)) return i;
    }

    return NO_FIND;
}


/**
 * 查找根目录区中的空值表项
 * @param ifp - 软盘镜像文件句柄
 * @return 第一个空值表项序号
 */
int findEmptyRootDirItem(FILE *ifp) {
    DirItem dirItem;
    unsigned short i;
    unsigned char temp;
    // 根目录表项数量
    unsigned short rootDirSize;
    // 根目录表项大小
    unsigned short dirItemSize = sizeof(DirItem);

    rootDirSize = (DATA_FIRST_SECTOR - ROOT_FIRST_SECTOR) * BYTES_SECTOR / dirItemSize;
    for(i = 0; i < rootDirSize; i++) {
        // 读取第 i + 1 个表项
        fseek(ifp, ROOT_FIRST_SECTOR * BYTES_SECTOR + dirItemSize * i, SEEK_SET);
        fread(&dirItem, dirItemSize, 1, ifp);

        // 文件名的第1字节是0xe5表示此文件已被删除，如果文件名第1字节是0表示此目录项可用
        temp = dirItem.name[0];
        if((temp == 0) || (temp == 0xe5)) return i;
    }

    return NO_FIND;
}

