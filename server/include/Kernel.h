#pragma once

#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <stdio.h>
#include <errno.h>
#include <sstream>
#include "BufferManager.h"
#include "FileManager.h"
#include "Disk.h"
#include "User.h"
#include "UserManager.h"
#include "Error.h"

#define SYS_SEEK_STA 0

#define SYS_SEEK_CUR 1

#define SYS_SEEK_END 2

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
    void display(const stringstream& ss){
        int bytes = send(fd, ss.str().c_str(), ss.str().size(), 0);
        cout << "Send " << username << "[" << fd << "] " << bytes << " bytes" << endl;
    }
};