#include <iostream>
#include <string>
#include <string.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>  
#include <strings.h>      
#include <vector>
#include <algorithm>
#include <iomanip>
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <numeric>
#include "Kernel.h"

#define PORT 8888
#define BACKLOG 12


using namespace std;

//信号处理函数，接收一个整型的信号id
void handle_pipe(int sig){
    printf("Received signal %d\n", sig);
}

bool isNum(const string str){
    for(auto c : str){
        if(!isdigit(c)) return false;
    }
    return true;
}
string getLocalTime(time_t currentTimeMillis){

    // 转换为本地时间
    std::tm* localTime = std::localtime(&currentTimeMillis);

    // 获取年月日时分秒
    string s;
    int year = localTime->tm_year + 1900;  // 年份是从1900年开始计算的
    int month = localTime->tm_mon + 1;     // 月份是从0开始计算的，需要加1
    int day = localTime->tm_mday;
    int hour = localTime->tm_hour + 8;     // ?这里为什么少了8个小时
    int minute = localTime->tm_min;
    int second = localTime->tm_sec;

    // 输出结果
    s = to_string(year) + "/" + to_string(month) + "/" + to_string(day) + " " 
       +to_string(hour) + ":" + to_string(minute) + ":" + to_string(second);
    return s;
}
stringstream homepage(const string username)
{
    stringstream ss;
    ss << "------------------------------------------------" << endl;
	ss << "               SecondFileSystem v1.0.2          " << endl;
	ss << "                  1751118 Dapeng Wu             " << endl;
	ss << "------------------------------------------------" << endl;
    ss << "Please input the '--help' for command help!" << endl;
	ss << endl;
    return ss;
}
stringstream help()
{
    stringstream ss;
    ss << "我的文件系统提供以下命令：\n";
    ss << "ls                           -----------查看当前目录下的文件或目录\n";
    ss << "mkdir [dirname]              -----------创建名为dirname的目录\n";
    ss << "cd [dirname]                 -----------切换到dirname目录\n";
    ss << "rm [name]                    -----------删除名为name的文件夹或文件\n";
    ss << "open [filename] [mode]       -----------打开filename文件, mode = 1, 读打开, mode = 2, 写打开\n";
    ss << "write [fd] [content]         -----------将内容写入写打开文件fd\n";
    ss << "read [fd] [size]             -----------读取读打开文件fd的size字节\n";
    ss << "seek [fd] [offset] [whence]  -----------调整打开文件fd的指针位置, offset为偏移量, whence为基点\n";
    ss << "close [fd]                   -----------关闭打开文件fd\n";
    ss << "touch [filename]             -----------创建名为filename的文件\n"; 
    ss << "fin [inName] [outName]       -----------将系统外部的outName文件读入到系统内部的inName文件中\n";
    ss << "fout [inName] [outName]      -----------将系统内部的inName文件读出到系统外部的outName文件中\n"; 
    ss << "\n";
    ss << "[注] 以上操作均针对当前目录\n";
    return ss;
}

vector<string> split(char* buf){
    stringstream ss(buf);
    vector<string> commands;
    string temp;

    while(ss >> temp){
        transform(temp.begin(), temp.end(), temp.begin(),::tolower);
        commands.push_back(temp);
    }
    return commands;
}
void removeSpaces(std::string& str) {
    str.erase(std::remove_if(str.begin(), str.end(), [](unsigned char c) { return std::isspace(c); }), str.end());
}
void *start_routine(void *ptr)
{
    int fd = *(int*) ptr;
    char buf[BUF_SIZE];
    int bytes;

    printf("New connect! The connect id is %d\n", fd);

    memset(buf, 0, sizeof(buf));
    while(true){
        bytes = send(fd, "Please input the username:\n", sizeof("Please input the username:\n"), 0);
        bytes = recv(fd, buf, sizeof(buf), 0);

        if(bytes == -1){
            perror("[Error] recv error\n");
            return (void*) NULL;
        }
        string temp = buf;
        removeSpaces(temp);

        //去掉空格，如果只剩一个回车，就继续输入
        if(temp.size() > 1) break;
    }

    string username = buf;
    cout <<"[Info] Username: " << username << endl;

    Message msg(fd, username);

    //login待调试
    Kernel::Instance().GetUserManager().Login(username);

    stringstream sout = homepage(username);
    msg.display(sout);

    while(true){
        bytes = recv(fd, buf, BUF_SIZE, 0);
        if(bytes == -1){
            perror("recv");
            Kernel::Instance().GetUserManager().Logout();
            return (void*)NULL;
        }
        //手动添加结束符，并过滤掉单个回车
        cout << "Receive size:" << bytes << endl;
        buf[bytes] = '\0';
        if(bytes == 1)continue;

        sout.str("");
        vector<string> commands = split(buf);

        cout << "Receive command: " << string(buf) << endl;
        if(commands[0] == "mkdir"){
            if(commands.size() != 2) {
                sout << COMMAND_PROMPT << endl;
                msg.display(sout);
                break;
            }
            int code = Kernel::Instance().Sys_Mkdir(commands[1]);
            if(code == NOERROR){
                sout << "Mkdir successfully!" << endl;
            }
            else if(code == EEXIST){
                sout << "[Error] " << commands[1] << " has existed!" << endl;
            }
            else{
                sout << "[Error] Unknown error" << endl;
            }

            msg.display(sout);
        }
        else if(commands[0] == "ls"){
            if(commands.size() != 1){
                sout << COMMAND_PROMPT << endl;
                msg.display(sout);
                break;
            }
            //待调试
            User& u = Kernel::Instance().GetUser();
            u.u_error = NOERROR;
            int fd = Kernel::Instance().Sys_OpenDir(u.u_curdir,File::FREAD); 
            //sout << "Access\tsize\tdatetime\tname\n";
            sout << std::setw(10) << "Access\t" << "size\t" << "datetime\t\t" << "name" <<endl;
            while(true){
                char _buf[33] = {0};
                int code;
                if(Kernel::Instance().Sys_Read(fd, 32, 33, _buf, code) == 0)
                    break;
                else{
                    DirectoryEntry* de = (DirectoryEntry*) _buf;
                    if(de->m_ino == 0) continue;
                    Inode *pInode = g_InodeTable.IGet(de->m_ino);

                    //处理文件权限显示
                    string grant = "";
                    unsigned i_mode = pInode->i_mode & ~Inode::IALLOC;
                    grant += i_mode & Inode::IREAD? 'r' : '-';
                    grant += i_mode & Inode::IWRITE? 'w' : '-';
                    grant += i_mode & Inode::IEXEC? 'x' : '-';
                    grant += i_mode & (Inode::IREAD >> 3)? 'r' : '-';
                    grant += i_mode & (Inode::IWRITE >> 3)? 'w' : '-';
                    grant += i_mode & (Inode::IEXEC >> 3)? 'x' : '-';
                    grant += i_mode & (Inode::IREAD >> 6)? 'r' : '-';
                    grant += i_mode & (Inode::IWRITE >> 6)? 'w' : '-';
                    grant += i_mode & (Inode::IEXEC >> 6)? 'x' : '-';

                    sout << std::setw(10)<< grant <<"\t" << (pInode->i_mode & Inode::IFDIR? "": to_string(pInode->i_size))  << "\t" 
                         << getLocalTime(pInode->IGetAccessDatetime()) << "\t" << de->m_name << endl;
                    pInode->NFrele();
                }
            }
            u.u_cdir->NFrele();
            msg.display(sout);
            
        }
        else if(commands[0] == "cd"){
            if(commands.size() != 2){
                sout << COMMAND_PROMPT << endl;
                msg.display(sout);
                break;
            }
            string desDir = commands[1];
            if(Kernel::Instance().Sys_ChDir(desDir) == ENOENT){
                sout << "[Error] No such directory!" << endl;
            }
            msg.display(sout);
        }
        else if(commands[0] == "--help"){
            sout = help();
            msg.display(sout);
        }
        else if(commands[0] == "exit"){
            Kernel::Instance().Sys_Exit();
            return (void*)NULL;
        }
        else if(commands[0] == "touch"){
            if(commands.size() != 2){
                sout << COMMAND_PROMPT << endl;
                msg.display(sout);
                break;
            }
            int code = Kernel::Instance().Sys_Creat(commands[1]);
            if(code == EEXIST){
                sout << "[Error] " << commands[1] <<"has existed!" << endl;
            }
            else if(code == EPERM){
                sout << "[Error] Operation not permitted" << endl;
            }
            else if(code == NOERROR){
                sout << "Creat successfully!" << endl;
            }
            else if(code == EACCES){
                sout << "[Error] Permission denied" << endl;
            }
            else{
                sout << "[Error] Unknowned error" << endl;
            }
            msg.display(sout);
        }
        else if(commands[0] == "rm"){
             if(commands.size() != 2){
                sout << COMMAND_PROMPT << endl;
                msg.display(sout);
                break;
            }
            int code = Kernel::Instance().Sys_Remove(commands[1]);
            if(code == ENOENT){
                sout << "[Error] No such file" << endl;
            }
            else if(code == EACCES){
                sout << "[Error] Permission denied" << endl;
            }
            else if(code == NOERROR){
                sout << "Remove successfully!" << endl;
            }
            else{
                sout <<"[Error] Unknown error" << endl;
            }
            msg.display(sout);
        }
        else if(commands[0] == "open"){
            if(commands.size() != 3){
                sout << COMMAND_PROMPT << endl;
                msg.display(sout);
                break;
            }
            string fileName = commands[1];
            if(!isNum(commands[2])){
                sout << COMMAND_PROMPT << endl;
                msg.display(sout);
                break;
            }
            int mode = stoi(commands[2]);
            int code;
            int fd = Kernel::Instance().Sys_Open(fileName, mode, code);
            if(code == ENOENT){
                sout << "[Error] No such file" << endl;
            }
            else if(code == EACCES){
                sout << "[Error] Permission denied" << endl;
            }
            else if(code == NOERROR){
                sout << "Open successfully! fd = " << fd << endl;
            }
            else{
                sout <<"[Error] Unknown error" << endl;
            }
            msg.display(sout);
        }
        else if(commands[0] == "read"){
            if(commands.size() != 3){
                sout << COMMAND_PROMPT << endl;
                msg.display(sout);
                break;
            }
            if(!isNum(commands[1]) || !isNum(commands[2])){
                sout << COMMAND_PROMPT << endl;
                msg.display(sout);
                break;
            }
            int fd = stoi(commands[1]), length = stoi(commands[2]), code;
            char _buf[BUF_SIZE];
            memset(_buf, 0, sizeof(_buf));
            int readSize = Kernel::Instance().Sys_Read(fd, length, BUF_SIZE, _buf, code);
            if(code == EBADF){
                sout << "[Error] No such open file" << endl;
            }
            else if(code == NOERROR){
                sout << "Read successfully, readSize = " << readSize << endl;
                sout << "Content: " << _buf << endl;
            }
            else if(code == EACCES){
                sout << "[Error] Permission denied" << endl;
            }
            else{
                sout <<"[Error] Unknown error" << endl;
            }
            msg.display(sout);
        }
        else if(commands[0] == "write"){
            if(commands.size() < 3){
                sout << COMMAND_PROMPT << endl;
                msg.display(sout);
                break;
            }
            if(!isNum(commands[1])){
                sout << COMMAND_PROMPT << endl;
                msg.display(sout);
                break;
            }
            int fd = stoi(commands[1]), code;

            //write fd 后面的内容都是要写入文件的内容
            string content = std::accumulate(commands.begin() + 2, commands.end(), string(),
            [](const string& acc, const string& str){
                return acc + str + " ";
            });
            char _buf[BUF_SIZE];
            strcpy(_buf, content.c_str());
            int writeSize = Kernel::Instance().Sys_Write(fd, content.size(),  BUF_SIZE, _buf, code);
            if(code == EBADF){
                sout << "[Error] No such open file" << endl;
            }
            else if(code == NOERROR){
                sout << "Write successfully, writeSize = " << writeSize << endl;
                sout << "Content: " << content << endl;
            }
            else if(code == EACCES){
                sout << "[Error] Permission denied" << endl;
            }
            else{
                sout <<"[Error] Unknown error" << endl;
            }
            msg.display(sout);
        }
        else if(commands[0] == "seek"){
            if(commands.size() != 4){
                sout << COMMAND_PROMPT << endl;
                msg.display(sout);
                break;
            }
            if(!isNum(commands[1]) || !isNum(commands[2]) || !isNum(commands[3])){
                sout << COMMAND_PROMPT << endl;
                msg.display(sout);
                break;
            }
            int fd = stoi(commands[1]);
            off_t offset = stoi(commands[2]);
            int whence = stoi(commands[3]);
            int code;
            int newPos = Kernel::Instance().Sys_Seek(fd, offset, whence, code); 
            if(code == EBADF){
                sout << "[Error] No such open file" << endl;
            }
            else if(code == NOERROR){
                sout << "Seek successfully, nowOffset = " << newPos << endl;
            }
            else if(code == EACCES){
                sout << "[Error] Permission denied" << endl;
            }
            else{
                sout <<"[Error] Unknown error" << endl;
            }
            msg.display(sout);
        }
        else if(commands[0] == "close"){
            if(commands.size() != 2){
                sout << COMMAND_PROMPT << endl;
                msg.display(sout);
                break;
            }
            if(!isNum(commands[1])){
                sout << COMMAND_PROMPT << endl;
                msg.display(sout);
                break;
            }
            int fd = stoi(commands[1]);
            if(Kernel::Instance().Sys_Close(fd) == 0){
                sout << "Close fd:" << fd << endl;
            }
            else{
                sout << "[Error] No such open file" << endl;
            }
            msg.display(sout);
        }
        else if(commands[0] == "cat"){
            if(commands.size() != 2){
                sout << COMMAND_PROMPT << endl;
                msg.display(sout);
                break;
            }
            string fileName = commands[1];
            if(Kernel::Instance().sys_Cat(fileName, sout) == -1){
                sout << "[Error] Cat failed" << endl;
            }
            msg.display(sout);
        }
        else if(commands[0] == "fin"){
            if(commands.size() != 3){
                sout << COMMAND_PROMPT << endl;
                msg.display(sout);
                break;
            }
            //fin inName outName
            string inName = commands[1], outName = commands[2];
            int sum;
            if((sum = Kernel::Instance().Sys_ReadIn(inName, outName)) == -1){
                sout << "[Error] Fin! Please checkout the inName and outName." << endl;
            }
            else{
                sout << "Fin successfully for " << sum << " bytes! You can cat the "<< inName <<" to checkout the content." << endl;
            }
            msg.display(sout);
        }
        else if(commands[0] == "fout"){
            if(commands.size() != 3){
                sout << COMMAND_PROMPT << endl;
                msg.display(sout);
                break;
            }
            //fout inName outName
            string inName = commands[1], outName = commands[2];
            int sum;
            if((sum = Kernel::Instance().Sys_ReadOut(inName, outName)) == -1){
                sout << "[Error] Fin! Please checkout the inName and outName." << endl;
            }
            else{
                sout << "Fout successfully for " << sum << " bytes! You can open the "<< outName <<" to checkout the content." << endl;
            }
            msg.display(sout);
        }
        else{
            sout << COMMAND_PROMPT << endl;
            msg.display(sout);
            break;
        }
    }

    close(fd);
    return (void*) NULL;
}

int main()
{
    //信号处理
    struct sigaction action;
    action.sa_handler = handle_pipe;
    sigemptyset(&action.sa_mask);       //初始化信号集为空，不屏蔽任何信号
    action.sa_flags = 0;                //指定信号处理的行为选项，表示不使用任何特殊的信号处理行为选项。
    if(sigaction(SIGPIPE, &action, NULL) == -1){
        perror("[Error]sigaction failed.");
        return 1;
    }

    int listenfd, connectfd;
    sockaddr_in server;
    sockaddr_in client;
    int sin_size = sizeof(sockaddr_in);

    //创建监听
    if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        perror("[Error]Failed to create socket\n");
        exit(1);
    }

    int opt = SO_REUSEADDR;

    //1、服务器创建一个套接字并将其绑定到一个特定的 IP 地址和端口上。
    //SO_REUSEADDR允许套接字关闭后马上复用这个地址
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);
    bzero(&server, sizeof server);
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);          //设置端口号为8888

    //INADDR_ANY 是一个特殊的常量，用于表示服务器绑定的 IP 地址。它是一个预定义的常量，其值为 0.0.0.0。
    //在网络编程中，当服务器希望监听所有可用的网络接口时，可以将服务器的 IP 地址设置为 INADDR_ANY。这样，服务器将能够接受来自任何网络接口的连接请求。
    server.sin_addr.s_addr = htonl(INADDR_ANY);

    //绑定服务器的sockaddr_in与监听的listenfd
    if(bind(listenfd,(sockaddr*)&server, sizeof(sockaddr)) == -1){
        perror("[Error] Bind error!\n");
        exit(1);
    }

    //2、服务器调用 listen 函数开始监听传入的连接请求。
    //监听
    if(listen(listenfd, BACKLOG) == -1){
        perror("[Error] Listen error!\n");
        exit(1);
    }


    Kernel::Instance().Initialize();
    

    printf("Wait the client to connect!\n");
    
    while(1){

        //3、当有客户端发起连接请求时，服务器调用 accept 函数接受该连接请求。
        if((connectfd = accept(listenfd, (sockaddr*)&client, (socklen_t*)&sin_size)) == -1){
            perror("[Error] Accept error!\n");
            continue;
        }
        printf("The client %s is connected.\n",inet_ntoa(client.sin_addr));
        pthread_t thread;

        //start_routine 将在新的线程中执行，第四个参数为传递给start_routine的参数
        pthread_create(&thread, NULL, start_routine, (void*)&connectfd);
        
    }

    close(listenfd);
    return 0;

}