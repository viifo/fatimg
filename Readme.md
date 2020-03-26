# fat镜像工具 #

## 编译fatimg ##
```shell script
# 进入fatimg目录
cd /yourpath/fatimg

# 编译生成可执行文件
make

# 根据需要将fatimg添加到path环境变量
```

## fatimg使用方法 ##
```
Usage: fatimg <image file> [options]  
Options:  
--help               Display this information.
-cp <dest file>      Copy dest file to FAT12 image. 
                     This command can only be used alone.
-b  <pre file>       Create a standard FAT12 image and init the image with boot file.
-f  <12/16/32/64>    Create a FAT12/FAT16/FAT32/EXFAT image.
-s  <img size(MB)>   Create a standard FAT12 image.
-sc <4/8/16/32/64>   Specify sectors per cluster (Except FAT12).
```

## fatimg使用示例 ##
```shell script

# 创建一个标准的fat12镜像文件
fatimg imgName.img

# 创建一个自定义引导扇区的fat12镜像文件
fatimg imgName.img -b boot.o

# 创建一个 260M & 每簇8扇区 的FAT32的镜像文件
fatimg imgName.img -f 32 -s 260 -sc 8

# 创建一个 260M & 自定义引导扇区 & 每簇8扇区 的FAT32的镜像文件
fatimg imgName.img -b boot.o -f 32 -s 260 -sc 8

```

**注意：写入镜像的文件名没有做大小写转换**


