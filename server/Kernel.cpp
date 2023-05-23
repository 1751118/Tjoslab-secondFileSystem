#include "Kernel.h"
#include <string.h>

Kernel Kernel::instance;
BufferManager g_BufferManager;
FileSystem g_FileSystem;
FileManager g_FileManager;
Disk g_Disk;
User g_User;
UserManager g_UserManager;

void Kernel::InitBuffer()
{
    this->m_BufferManager = &g_BufferManager;
    this->m_BufferManager->Initialize();
}

void Kernel::InitDisk()
{
    this->m_disk = &g_Disk;
    this->m_disk->initialize();
}

void Kernel::InitFileSystem()
{
    this->m_FileSystem = &g_FileSystem;
    this->m_FileSystem->Initialize();
    this->m_FileManager = &g_FileManager;
    this->m_FileManager->Initialize();
}

void Kernel::InitUser()
{
    this->m_User = &g_User;
    this->m_UserManager = &g_UserManager;
}

void Kernel::Initialize()
{
    //初始化四大件
    InitBuffer();
    InitDisk();
    InitFileSystem();
    InitUser();

    //获得文件管理实例
    FileManager& fileMgr = Kernel::Instance().GetFileManager();
    //获得根节点的Inode
    fileMgr.rootDirInode = g_InodeTable.IGet(FileSystem::ROOTINO);
    //清除锁标志位
    fileMgr.rootDirInode->i_flag &= (~Inode::ILOCK);
    //解锁
    pthread_mutex_unlock(&fileMgr.rootDirInode->mutex);
    //加载超级块
    Kernel::Instance().GetFileSystem().LoadSuperBlock();
    User &usr = Kernel::Instance().GetUser();

    usr.u_cdir = g_InodeTable.IGet(FileSystem::ROOTINO);
    //清除锁标志位
    usr.u_cdir->i_flag &= (~Inode::ILOCK);
    //解锁
    pthread_mutex_unlock(&usr.u_cdir->mutex);
    //设置路径
    strcpy(usr.u_curdir,"/");       //这里是主线程的usr，貌似并没有什么用

    printf("[Info] System initializing successfully!\n");
}

BufferManager& Kernel::GetBufferManager()
{
    return *(this->m_BufferManager);
}

FileManager& Kernel::GetFileManager()
{
    return *(this->m_FileManager);
}

User& Kernel::GetUser()
{
    return *(this->m_UserManager->GetUser());
}

UserManager& Kernel::GetUserManager()
{
    return *(this->m_UserManager);
}

FileSystem& Kernel::GetFileSystem()
{
    return *(this->m_FileSystem);
}
Kernel& Kernel::Instance()
{
    return Kernel::instance;
}



