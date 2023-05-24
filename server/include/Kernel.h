#pragma once

#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <stdio.h>
#include <errno.h>
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "BufferManager.h"
#include "FileManager.h"
#include "Disk.h"
#include "User.h"
#include "UserManager.h"
#include "Error.h"

#define SYS_SEEK_STA 0

#define SYS_SEEK_CUR 1

#define SYS_SEEK_END 2

#define BUF_SIZE 2048

typedef unsigned Fd;

class Kernel
{
private:
    static Kernel instance;     //单体实例

    BufferManager* m_BufferManager;
    Disk* m_disk;
    FileSystem* m_FileSystem;
    FileManager* m_FileManager;
    User* m_User;
    UserManager* m_UserManager;
    

private:
    void InitBuffer();
    void InitDisk();
    void InitFileSystem();
    void InitUser();
    
    

public:
    Kernel(){}
    ~Kernel(){}
    BufferManager& GetBufferManager();
    FileManager& GetFileManager();
    User& GetUser();
    UserManager& GetUserManager();
    FileSystem& GetFileSystem();
    
public:
    static Kernel& Instance();
    void Initialize();

    /*系统调用*/
public:
    int Sys_OpenDir(const string path, int mode);
    int Sys_Read(int fd, size_t size, size_t nmemb, void* ptr, int& code);
    int Sys_ChDir(const string desDir);
    int Sys_Mkdir(const string path);
    void Sys_Exit();
    int Sys_Creat(const string fileName);
    int Sys_Remove(const string path);
    int Sys_Open(const string fileName, int mode, int& code);
    int Sys_Write(int fd, size_t size, size_t nmemb, void* ptr, int& code);
    int Sys_Seek(int fd, off_t offset, int whence, int& code);
    int Sys_Close(int fd);
    int sys_Cat(const string fileName, stringstream& sout);
    int Sys_ReadIn(const string inName, const string outName);
    int Sys_ReadOut(const string inName, const string outName);
};

class Message
{
private:
    int fd;
    string username;
public:
    Message(int fd, string username){
        this->fd = fd;
        this->username = username;
    }
    void display(stringstream& ss){

        //每次显示信息之后，都显示当前路径
        FileManager& fileMgr = Kernel::Instance().GetFileManager();
        char disPath[100];
        strcpy(disPath, ((string)"root@" + (fileMgr.GetCurDir() + 1)).c_str());
        ss << strcat(disPath, ":/# ");
        
        int bytes = send(fd, ss.str().c_str(), ss.str().size(), 0);
        cout << "Send " << username << "[" << fd << "] " << bytes << " bytes" << endl;

        
    }
};