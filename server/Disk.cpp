#include "Kernel.h"
#include <unistd.h>
// #include <sys/types.h>
// #include <sys/stat.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdio.h>
#include <iostream>
using namespace std;


Disk::Disk()
{

}

Disk::~Disk()
{

}

void Disk::initialize()
{
    this->m_BufferManager = &Kernel::Instance().GetBufferManager();

    //读写方式打开磁盘
    int fd = open(devpath, O_RDWR);
    if(fd == -1){
        fd = open(devpath, O_RDWR | O_CREAT, 0666);
        if(fd == -1){
            perror("Error opening file");
            exit(-1);
        }

        //创建磁盘时才需要初始化
        this->init_disk(fd);
    }
    this->init_mmap(fd);
    this->img_fd = fd;
}

/**
 * 磁盘不存在的情况下，初始化磁盘
 * 1、初始化超级块
 * 2、初始化数据块
 * 3、初始化外存Inode结点
 * 4、写入myDisk.img
*/
void Disk::init_disk(int fd)
{
    //初始化超级块
    SuperBlock sb;
    init_superblock(sb);

     //初始化DiskInode ， 设置根的Inode模式为目录文件，可执行
    DiskInode* diskInode_table = new DiskInode[FileSystem::INODE_ZONE_SIZE * FileSystem::INODE_NUMBER_PER_SECTOR];
    diskInode_table[0].d_mode = Inode::IFDIR | Inode::IEXEC;  // 文件类型为目录文件、文件的执行权限

    char* datablock = new char[FileSystem::DATA_ZONE_SIZE * 512];
    memset(datablock, 0, FileSystem::DATA_ZONE_SIZE * 512);
    init_datablock(datablock);

    //初始化信息写入磁盘
    write(fd, &sb, sizeof(SuperBlock));
    write(fd, diskInode_table, FileSystem::INODE_ZONE_SIZE * FileSystem::INODE_NUMBER_PER_SECTOR * sizeof(DiskInode));
    write(fd, datablock, FileSystem::DATA_ZONE_SIZE * 512);

    printf("[Info] Initializing disk successfully!\n");
    // SuperBlock spb;
    // init_superblock(spb);
    // DiskInode *di_table = new DiskInode[FileSystem::INODE_ZONE_SIZE*FileSystem::INODE_NUMBER_PER_SECTOR];

    // // 设置 rootDiskInode 的初始值
    // di_table[0].d_mode = Inode::IFDIR;  // 文件类型为目录文件
    // di_table[0].d_mode |= Inode::IEXEC; // 文件的执行权限

    // char* datablock = new char[FileSystem::DATA_ZONE_SIZE * 512];
    // memset(datablock, 0, FileSystem::DATA_ZONE_SIZE * 512);
    // init_datablock(datablock);

    // // 写入文件
    // write(fd, &spb, sizeof(SuperBlock));
    // write(fd, di_table, FileSystem::INODE_ZONE_SIZE * FileSystem::INODE_NUMBER_PER_SECTOR * sizeof(DiskInode));
    // write(fd, datablock, FileSystem::DATA_ZONE_SIZE * 512);

    // printf("[info] 格式化磁盘完毕...");
}

void Disk::init_superblock(SuperBlock& sb)
{
    sb.s_isize = FileSystem::INODE_ZONE_SIZE;
    sb.s_fsize = FileSystem::DATA_ZONE_END_SECTOR + 1;
    sb.s_nfree = (FileSystem::DATA_ZONE_SIZE - 99) % 100;
    sb.s_fmod = 0;
    sb.s_ronly = 0;

    // 找最后一个盘块组的第一个盘块
    int start_last_datablock = FileSystem::DATA_ZONE_START_SECTOR;
    while((start_last_datablock + 100 - 1) < FileSystem::DATA_ZONE_END_SECTOR){
        start_last_datablock += 100;
    }

    // 将最后一个盘块组的盘块号填入,表示空闲块列表中的空闲块号
    for(int i = 0; i < sb.s_nfree; i++)
        sb.s_free[i] = start_last_datablock + i - 1;

    //将值从 0 到 99 依次赋给 s_inode 数组的元素，表示 inode 数组中的 inode 号。
    sb.s_ninode = 100;
    for(int i = 0; i < sb.s_ninode; i++)
        sb.s_inode[i] = i;

    // sb.s_isize = FileSystem::INODE_ZONE_SIZE;
    // sb.s_fsize = FileSystem::DATA_ZONE_END_SECTOR + 1;
    // sb.s_nfree = (FileSystem::DATA_ZONE_SIZE - 99) % 100;

    // // 找到最后一个盘块组的第一个盘块
    // int start_last_datablk = FileSystem::DATA_ZONE_START_SECTOR;
    // while(true){
    //     if((start_last_datablk + 100 -1) < FileSystem::DATA_ZONE_END_SECTOR)
    //         start_last_datablk += 100;
    //     else
    //         break;
    // }
    // start_last_datablk--;
    // // 将最后一个盘块组的盘块号填入
    // for(int i = 0; i < sb.s_nfree; ++i)
    //     sb.s_free[i] = start_last_datablk + i;

    // sb.s_ninode = 100;
    // for(int i = 0; i < sb.s_ninode; ++i)
    //     sb.s_inode[i] = i;
    
    // // sb.s_flock = 0;
    // // sb.s_ilock = 0;
    // sb.s_fmod  = 0;
    // sb.s_ronly = 0;
}

void Disk::init_datablock(char* datablock)
{
    struct
    {
        int nfree;      //本组盘块空闲的个数
        int free[100];  //本组盘块空闲索引表
    }temp;

    int leftBlock = FileSystem::DATA_ZONE_SIZE, i = 0;
    while(true)
    {
        temp.nfree = min(100, leftBlock);
        leftBlock -= temp.nfree;

        for(int j = 0; j < temp.nfree; j++){
            if(i == 0 && j == 0)
                temp.free[j] = 0;               //第一个数据块组的第一个数据块索引为0，存储超级块和索引结点，不存储文件数据
            else
                temp.free[j] = 100 * i + j + FileSystem::DATA_ZONE_START_SECTOR - 1;
        }

        memcpy(&datablock[99 * 512 + i * 100 * 512], (void*)&temp, sizeof(temp));
        i++;
        if(leftBlock == 0) break;
            
    }
    // struct
    // {
    //     int nfree;     //本组空闲的个数
    //     int free[100]; //本组空闲的索引表
    // } tmp_table;

    // int last_datablk_num = FileSystem::DATA_ZONE_SIZE; //未加入索引的盘块的数量
    // // 初始化组长盘块
    // for(int i = 0; ; i++)
    // {
    //     if (last_datablk_num >= 100)
    //         tmp_table.nfree = 100;
    //     else
    //         tmp_table.nfree = last_datablk_num;
    //     last_datablk_num -= tmp_table.nfree;

    //     for (int j = 0; j < tmp_table.nfree; j++)
    //     {
    //         if (i == 0 && j == 0)
    //             tmp_table.free[j] = 0;
    //         else
    //         {
    //             tmp_table.free[j] = 100 * i + j + FileSystem::DATA_ZONE_START_SECTOR - 1;
    //         }
    //     }
    //     memcpy(&data[99 * 512 + i * 100 * 512], (void *)&tmp_table, sizeof(tmp_table));
    //     if (last_datablk_num == 0)
    //         break;
    // }
}

void Disk::init_mmap(int fd)
{
    /*
        将文件映射到虚拟内存区，void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
        addr：指定映射区域的起始地址，通常设置为 NULL，让系统自动选择合适的地址。
        length：映射区域的长度，以字节为单位。
        prot：指定映射区域的保护模式，可以是 PROT_NONE、PROT_READ、PROT_WRITE、PROT_EXEC 或它们的组合。
        flags：指定映射区域的标志，可以是 MAP_SHARED、MAP_PRIVATE、MAP_FIXED、MAP_ANONYMOUS 等。
        fd：文件描述符，指定要映射的文件。
        offset：文件的偏移量，指定从文件的哪个位置开始映射。
    */
    struct stat fileStat;

    if(fstat(fd, &fileStat) == -1){
        perror("fstat");
        close(fd);
        exit(-1);
    }
    off_t fileSize = fileStat.st_size;

    char* mappedData = (char*)mmap(NULL, fileSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(mappedData == MAP_FAILED){
        perror("mmap");
        exit(-1);
    }
    printf("Mapping successfully\n");

    this->m_BufferManager->SetMMapAddr(mappedData);
    // struct stat st; //定义文件信息结构体
    // /*取得文件大小*/
    // int r = fstat(fd, &st);
    // if (r == -1)
    // {
    //     printf("[error]获取img文件信息失败，文件系统启动中止\n");
    //     close(fd);
    //     exit(-1);
    // }
    // int len = st.st_size;
    // /*把文件映射成虚拟内存地址*/
    // char *addr = (char*)mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    // printf("File content:\n%.*s\n", (int)len, addr);
    // this->m_BufferManager->SetMMapAddr(addr);
}

void Disk::Exit()
{
    /**
     * int msync(void *addr, size_t length, int flags);
     * 将内存中的数据同步到对应的文件或设备上，以确保数据的持久性和一致性。
        addr：指向需要同步的内存区域的起始地址。
        length：需要同步的内存区域的长度。
        flags：指定同步的方式，可以使用以下标志之一：
        MS_ASYNC：异步地将数据同步到文件或设备。
        MS_SYNC：同步地将数据同步到文件或设备，需要等待操作完成。
        MS_INVALIDATE：在同步数据之前使文件的对应部分失效。
    */
    struct stat st;
    if(fstat(this->img_fd, &st) == -1){
        perror("Exit");
        close(this->img_fd);
        exit(-1);
    }
    char* p = this->m_BufferManager->GetMMapAddr();
    if(msync((void*)p, st.st_size, MS_ASYNC) == -1){
        perror("msync");
        close(img_fd);
        exit(-2);
    }

    /**
     * munmap 函数用于解除通过 mmap 函数创建的内存映射。
        addr：指向映射区域起始地址的指针，与之前调用 mmap 返回的地址参数一致。
        length：映射区域的长度，与之前调用 mmap 返回的长度参数一致。
    */
    // if(munmap((void*)p, st.st_size) == -1){
    //     perror("munmap");
    //     close(img_fd);
    //     exit(-3);
    // }  这里用户退出，不应该调用munmap，这取消了磁盘到内存的映射，但实际上服务器还要服务于其他用户
}