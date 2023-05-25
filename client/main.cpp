#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> /* See NOTES */
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <errno.h>
#include <arpa/inet.h>
#include <signal.h>
#include <iostream>
using namespace std;

#define MAX_SEND_SIZE 1024

void running(int fd)
{
    int bytes;
    char recieveMsg[2048] = {0};
    char sendMsg[2048] = {0};
    bool first_recv = 0;

    while(true){
        bytes = recv(fd, recieveMsg, sizeof(recieveMsg), 0);
        if(bytes == 0){
            printf("Connect is closed\n");
            return;
        }
        if(bytes == -1){
            if(errno != EAGAIN &&errno != EWOULDBLOCK && errno != EINTR){
                printf("[Error] Error code is %d(%s)\n", errno, strerror(errno));
                return;
            }
            continue;
        }
        if(bytes > 0){
            recieveMsg[bytes] = 0;
            cout << recieveMsg;
        }

        //获取用户输入
        bool exitFlag = false;
        while(true){
            fgets(sendMsg, sizeof(sendMsg), stdin);

            //单个回车不发送
            int len = strlen(sendMsg) - 1; 
            if(len == 0) continue;
            

            //向服务器发送用户输入
            if((bytes = send(fd, sendMsg, len, 0)) == -1){
                printf("[EROOR] send error\n");
                return;
            }

            if(!strcmp(sendMsg, "exit\n")) return;
            
            break;
        }
    }
}
int main(int argc, char** argv)
{
    if(argc < 3){
        printf("[Error]IP and Port needed\n");
        return -1;
    }
    char* ipaddr = argv[1];
    unsigned port = atoi(argv[2]);

    //1、创建套接字
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0){
        fprintf(stderr, "[Error]Failed to create socket:%s\n", strerror(errno));
        return -1;
    }

    sockaddr_in addr;
    bzero(&addr, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ipaddr, &addr.sin_addr);

    //设置套接字为非阻塞
    /*
        通过将套接字设置为非阻塞模式，可以实现以下优势：

        避免程序在等待套接字操作完成时被阻塞，提高程序的响应性。
        允许在同一个线程或进程中处理多个套接字，提高并发性能。
        方便实现超时处理，以避免无限期等待套接字操作完成。
    */

   //这里使用非阻塞模式
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
    if(flags < 0){
        fprintf(stderr, "Set flags error:%s\n", strerror(errno));
        close(sockfd);
        return -1;
    }

    //2、建立连接
    int cnt = 1;
    while(true){
        int rc = connect(sockfd, (sockaddr*)&addr, sizeof addr);
        if(rc == 0){
            printf("Successfully connect to the server\n");
            break;
        }
        if(cnt > 10){
            printf("[Error] Failed to connect to server\n");
            return 0;
        }
        printf("[Error] Try reconnecting the server for the %d time\n", cnt++);
    }
    //3、执行收发逻辑
    running(sockfd);
    close(sockfd);
    printf("Connect closed!\n");
    return 0;
}