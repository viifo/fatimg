/**
 * fatimg FAT镜像工具头文件
 */
#ifndef FATIMG_FATIMG_H
#define FATIMG_FATIMG_H


#define OK 0
#define ERROR -1
#define NO_FIND -2
#define BAD_FORMAT -3
#define INSUFFICIENT_SPACE -4


/** 定义文件分割符 */
#if defined(WIN32) || defined (WIN64)
#define SEPARATOR   '\\'
#else
#define SEPARATOR   '/'
#endif


/** 定义FAT镜像类型 */
typedef char FAT_TYPE;
#define FAT12 12
#define FAT16 16
#define FAT32 32
#define EXFAT 64
#define UNKOWN 0



/****************************************************************
 * FAT12
 ****************************************************************/
/** 创建标准的空的fat12软盘镜像(1.44M) */
int createEmptyFat12img(char*);
/** 创建一个标准的自定义引导扇区的fat12软盘镜像(1.44M) */
int createCustomBootFat12img(char*, char*);
/** 拷贝文件到FAT12软盘镜像 */
int copyFileToFat12img(char*, char*, char);


/****************************************************************
 * FAT32
 ****************************************************************/
/** 创建空的fat32软盘镜像 */
int createEmptyFat32img(char *imgPath, float size,  int cluster);
/** 创建自定义引导扇区的fat32软盘镜像 */
int createCustomBootFat32img(char *imgPath, char *bootPath, float size, int cluster);


/****************************************************************
 * FAT 公共函数
 ****************************************************************/
/** 随机产生一个分区序号 */
unsigned int getVolumeID();
/** 字符串转大写字母 */
char* strUpper(char* str);
/** 获取FAT镜像文件类型 */
FAT_TYPE getImageFatType(const char* path);
/** 格式化时间为FAT时间格式 */
unsigned short formatTime();
/** 格式化日期为日期格式 */
unsigned short formatDate();
/** 格式化短文件名为 8 + 3 + '\0' 格式 */
void formatFileName(char*, char*);
/** 根据短文件名获取长文件名目录项的校验值 @return 校验值 */
unsigned char getChecksumByShortName(const char*);


/** 向本簇号指向的FAT文件分配表表项中写入簇链的下一簇号 */
void setNextClusterLinkNum(FILE *fp, long fat1Pos, long fat2Pos, unsigned int clusterNum, unsigned int nextClusterNum, FAT_TYPE);
/** 通过本簇号在FAT中查找文件/目录簇链的下一簇号 */
unsigned int getNextClusterLinkNum(FILE* fp, long fatPos, unsigned int clusterNum, FAT_TYPE type);
/** 在FAT中寻找空簇 */
unsigned int findEmptyCluster(FILE *fp, long fatPos, long fatSize, unsigned int startNum, FAT_TYPE);
/** 查找FAT空闲簇数 */
unsigned int getFreeClusterNum(FILE *fp, long fatPos, long fatSize, FAT_TYPE type);


#endif // FATIMG_FATIMG_H
