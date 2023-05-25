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
    u.u_error = NOERROR;
    u.u_dirp = desPath;
    u.u_arg[0] = (unsigned long long) desPath;

    FileManager& fileMgr = Kernel::Instance().GetFileManager();
    fileMgr.ChDir();

    return u.u_error;
}

void Kernel::Sys_Exit()
{
    this->m_BufferManager->Bflush();
    this->m_FileManager->m_InodeTable->UpdateInodeTable();
    this->m_FileSystem->Update();
    this->m_disk->Exit();

    //用户登出
    UserManager& userMgr = Kernel::Instance().GetUserManager();
    userMgr.Logout();
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

int Kernel::Sys_Write(int fd, size_t size, size_t nmemb, void* ptr, int& code)
{

    if(size > nmemb) return -1;
    User& u = Kernel::Instance().GetUser();

    u.u_arg[0] = fd;
    u.u_arg[1] = (unsigned long long)ptr;
    u.u_arg[2] = size;

    FileManager& fileMgr = Kernel::Instance().GetFileManager();
    fileMgr.Write();

    code = u.u_error;
    return u.u_ar0[User::EAX];
}

int Kernel::Sys_Seek(int fd, off_t offset, int whence, int& code)
{
    User& u = Kernel::Instance().GetUser();

    u.u_ar0[0] = 0;
    u.u_ar0[1] = 0;

    u.u_arg[0] = fd;
    u.u_arg[1] = offset;
    u.u_arg[2] = whence;

    FileManager& fileMgr = Kernel::Instance().GetFileManager();
    fileMgr.Seek();

    code = u.u_error;
    return u.u_ar0[User::EAX];
}

int Kernel::Sys_Close(int fd)
{
    User& u = Kernel::Instance().GetUser();

    u.u_error = NOERROR;
    u.u_arg[0] = fd;

    FileManager& fileMgr = Kernel::Instance().GetFileManager();
    fileMgr.Close();

    return u.u_ar0[User::EAX];
}

int Kernel::sys_Cat(const string fileName, stringstream& sout)
{
    User& u = Kernel::Instance().GetUser();
    u.u_error = NOERROR;
    
    int code;

    //读打开文件
    int fd = Kernel::Instance().Sys_Open(fileName, File::FREAD, code);
    if(code == -1){
        return -1;
    }

    char buf[BUF_SIZE];

    //读取文件内容
    while(true){
        memset(buf, 0, BUF_SIZE);
        if(Kernel::Instance().Sys_Read(fd, BUF_SIZE, BUF_SIZE, buf, code) <= 0)
            break;
        sout << buf;
    }
    sout << endl;
    if(code != NOERROR) return -1;

    //关闭文件
    return Kernel::Instance().Sys_Close(fd);
}

int Kernel::Sys_ReadIn(const string inName, const string outName)
{
    int out_fd = open(outName.c_str(), O_RDONLY);
    if(out_fd == -1) return -1;

    User& u = Kernel::Instance().GetUser();
    u.u_error = NOERROR;

    int code;
    if(Sys_Creat(inName) != NOERROR) return -1;

    code = NOERROR;
    int in_fd = Sys_Open(inName, File::FWRITE, code);
    if(code != NOERROR) return -1;

    char buf[BUF_SIZE];
    int size;
    size_t sum = 0;
    while(true){
        memset(buf, 0, BUF_SIZE);
        if((size = read(out_fd, buf, BUF_SIZE)) <= 0)
            break;
        else{
            sum += Sys_Write(in_fd, size, size, buf, code);
        }
    }

    close(out_fd);
    if(Sys_Close(in_fd) == -1) return -1;
    return sum;
}

int Kernel::Sys_ReadOut(const string inName, const string outName)
{
    int out_fd = open(outName.c_str(), O_WRONLY | O_CREAT, 0755);
    if(out_fd == -1) return -1;

    User& u = Kernel::Instance().GetUser();
    u.u_error = NOERROR;

    int code = NOERROR;
    int in_fd = Sys_Open(inName, File::FREAD, code);
    if(code != NOERROR) return -1;

    char buf[BUF_SIZE];
    int size;
    size_t sum = 0;
    while(true){
        memset(buf, 0, BUF_SIZE);
        if((size = Sys_Read(in_fd, BUF_SIZE, BUF_SIZE, buf, code)) <= 0)
            break;
        else{
            sum += write(out_fd, buf, size);
        }
    }
    if(code != NOERROR) return -1;

    close(out_fd);
    if(Sys_Close(in_fd) == -1) return -1;
    return sum;
}