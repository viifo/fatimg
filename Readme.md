# fat镜像工具 #

## 编译fatimg ##
```shell script
# 进入fatimg目录
cd /yourpath/fatimg

# 编译生成可执行文件
make
```

## fatimg使用方法 ##
```
Usage: fatimg <image file> [options]  
Options:  
--help              Display this information.
-c                  <dest file> Copy dest file to FAT12 image. 
                    This command can only be used alone.
-b <pre file>       Create a standard FAT12 image and init the image with boot file.
-f <12/16/32/64>    Create a FAT12/FAT16/FAT32/EXFAT image.
-s <img size(MB)>   Create a standard FAT12 image.
```

## fatimg使用示例 ##
```shell script

# 创建一个标准的fat12镜像文件
fatimg imgName.img

# 创建一个自定义引导扇区的fat12镜像文件
fatimg imgName.img -b boot.o

# 创建一个528M的自定义引导扇区的FAT32的镜像文件（待完成）
fatimg imgName.img -f 32 -s 528 -b boot.o

```

**注意：写入的文件名没有做大小写转换**


