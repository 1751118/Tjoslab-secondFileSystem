#include "Kernel.h"
#include "UserManager.h"

UserManager::UserManager()
{
    for(int i = 0; i < MAX_USER_NUM; i++)
        this->p_users[i] = NULL;

    this->user_addr.clear();

    //分配一个给系统主进程
    pthread_t pid = pthread_self();
    p_users[0] = (User*)malloc(sizeof(User));
    user_addr[pid] = 0;
}

UserManager::~UserManager()
{
    for(int i = 0; i < MAX_USER_NUM; i++)
        if((this->p_users[i]) != NULL)
            free((this->p_users)[i]);
}

bool UserManager::Login(string username)
{
    //TODO
    pthread_t pid = pthread_self();
    if(user_addr.find(pid) != user_addr.end()){
        printf("[Error] pthread %lu repeat logins!\n", pid);
        return false;
    }
    int i = 0;
    while(i < MAX_USER_NUM && p_users[i])
        i++;
    if(i == MAX_USER_NUM)
    {
        printf("[Error] Too many users! Please try later!\n");
        return false;
    }

    p_users[i] = (User*)malloc(sizeof(User));
    if(!p_users[i]){
        perror("Malloc");
        return false;
    }

    user_addr[pid] = i;
    p_users[i]->u_uid = 0;

    //创建Usr的初始目录
    //1、关联根目录
    p_users[i]->u_cdir = g_InodeTable.IGet(FileSystem::ROOTINO);
    p_users[i]->u_cdir->NFrele();
    strcpy(p_users[i]->u_curdir, "/");          

    //2、创建自己的家目录
    Kernel::Instance().Sys_Mkdir(username);
    p_users[i]->u_error = NOERROR;

    //3、转到家目录
    char pathname[512] = {0};
    stpcpy(pathname, username.c_str());
    p_users[i]->u_dirp = pathname;
    p_users[i]->u_arg[0] = (unsigned long long) pathname;
    
    FileManager& fileMgr = Kernel::Instance().GetFileManager();
    fileMgr.ChDir();

    printf("User %d logins successfully!", i);
    return true;
}

bool UserManager::Logout()
{
    pthread_t pid = pthread_self();
    int uid = user_addr[pid];

    user_addr.erase(pid);
    free(p_users[uid]);
    p_users[uid] = NULL;

    cout << "User:" << uid << " exit!" << endl;
    return true;
}

User *UserManager::GetUser()
{
    pthread_t pid = pthread_self();
    if(user_addr.find(pid) == user_addr.end())
    {
        printf("[Error] Not found pthread\n");
        exit(-1);
    }
    return p_users[user_addr[pid]];
}