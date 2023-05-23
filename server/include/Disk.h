#pragma once

#include "BufferManager.h"
#include "FileSystem.h"

class Disk
{
private:
    const char* devpath = "myDisk.img";
    int img_fd;                             /*devpath的fd*/
    BufferManager* m_BufferManager;         /*FileSystem类需要缓存管理的接口*/

private:
    void init_superblock(SuperBlock& sb);
    void init_disk(int fd);
    void init_datablock(char* datablock);
    void init_mmap(int fd);

public:
    Disk();
    ~Disk();

    void initialize();
    void Exit();
};