#include "Kernel.h"
using namespace std;

int Kernel::Sys_OpenDir(const string path, int mode)
{
    User& u = Kernel::Instance().GetUser();
    char _path[256];
    strcpy(_path, path.c_str());

    u.u_dirp = _path;
    u.u_arg[1] = mode;

    FileManager& fileMgr = Kernel::Instance().GetFileManager();
    fileMgr.Open();

    return u.u_ar0[User::EAX];
}


int Kernel::Sys_Read(int fd, size_t size, size_t nmemb, void* ptr,int& code)
{
    if(size > nmemb) return -1;

    User& u = Kernel::Instance().GetUser();

    u.u_arg[0] = fd;
    u.u_arg[1] = (unsigned long long)ptr;
    u.u_arg[2] = size;

    FileManager& fileMgr = Kernel::Instance().GetFileManager();
    fileMgr.Read();

    code = u.u_error;
    return u.u_ar0[User::EAX];
}

int Kernel::Sys_Mkdir(const string path)
{
    //默认模式为 rwx-rx-rx
    int mode = 040755;
    User &u = Kernel::Instance().GetUser();
    u.u_error = NOERROR;

    char newPath[100] = {0};
    strcpy(newPath, path.c_str());
    u.u_dirp = newPath;
    u.u_arg[1] = mode;
    u.u_arg[2] = 0;
    
    FileManager& fileMgr = Kernel::Instance().GetFileManager();
    fileMgr.MkNod();

    return u.u_error;

}

int Kernel::Sys_ChDir(const string desDir)
{
    char desPath[100];
    strcpy(desPath, desDir.c_str());

    User& u = Kernel::Instance().GetUser();
    u.u_dirp = desPath;
    u.u_arg[0] = (unsigned long long) desPath;

    FileManager& fileMgr = Kernel::Instance().GetFileManager();
    fileMgr.ChDir();

    return u.u_ar0[User::EAX];
}

void Kernel::Sys_Exit()
{
    this->m_BufferManager->Bflush();
    
    this->m_FileManager->m_InodeTable->UpdateInodeTable();
    this->m_FileSystem->Update();
    this->m_disk->Exit();

    UserManager& userMgr = Kernel::Instance().GetUserManager();
    int uid = userMgr.user_addr[pthread_self()];
    cout << "User:" << uid <<" exit..." << endl;
}

int Kernel::Sys_Creat(const string fileName)
{
    User& u = Kernel::Instance().GetUser();
    char _fileName[100] = { 0 };
    strcpy(_fileName, fileName.c_str());

    u.u_error = NOERROR;
    u.u_ar0[0] = 0;
    u.u_ar0[0] = 0;
    u.u_dirp = _fileName;
    u.u_arg[1] = Inode::IRWXU;

    FileManager& fileMgr = Kernel::Instance().GetFileManager();
    fileMgr.Creat();

    return u.u_error;
}

int Kernel::Sys_Remove(const string path)
{
    User& u = Kernel::Instance().GetUser();
    char _path[100] = { 0 };
    strcpy(_path, path.c_str());

    u.u_error = NOERROR;
    u.u_ar0[0] = 0;
    u.u_ar0[1] = 0;
    u.u_dirp = _path;

    FileManager& fileMgr = Kernel::Instance().GetFileManager();
    fileMgr.UnLink();

    return u.u_error;
}

int Kernel::Sys_Open(const string fileName, int mode, int& code)
{
    User& u = Kernel::Instance().GetUser();
    char _fileName[100] = { 0 };
    strcpy(_fileName, fileName.c_str());

    u.u_error = NOERROR;
    u.u_dirp = _fileName;
    u.u_arg[1] = mode;

    FileManager& fileMgr = Kernel::Instance().GetFileManager();
    fileMgr.Open();

    code = u.u_error;
    return u.u_ar0[User::EAX];
}