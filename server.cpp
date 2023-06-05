#include <iostream>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <thread>
#include <mutex>

#define BUF_SIZE 100
#define MAX_CLNT 256
#define NAME_SIZE 20

std::mutex mutx;
//用于记录当前连接的clnt的数量
int clnt_cnt = 0; 
//使用clnt_socks[]数组保存已经连接的clnt_sock
int clnt_socks[MAX_CLNT];
void error_handling(char * msg);
void * handle_clnt(void * arg);
void send_msg(char * msg, int len);


int main(int argc, char * argv[]) {
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_adr, clnt_adr;
    socklen_t clnt_adr_sz;
    pthread_t t_id;

    if (argc != 2) {
        printf("Usgae : %s <port> \n", argv[0]);
        exit(1);
    }
    
    serv_sock = socket(PF_INET, SOCK_STREAM, 0);

    //初始化serv_adr
    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[1]));

    if (bind(serv_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1)
        error_handling("bind() error");
    if (listen(serv_sock, 5) == -1)
        error_handling("listen() error");

    while (1) {
        clnt_adr_sz = sizeof(clnt_adr);
        clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr, &clnt_adr_sz);    
        if (clnt_sock == -1) 
            error_handling("accept() error");

        //修改clnt_socks[]时，使用互斥锁上锁，防止同时多个连接导致同时修改出现错误
        mutx.lock();
        clnt_socks[clnt_cnt++] = clnt_sock;
        mutx.unlock();

        // 创建新的线程处理新链接,并引导线程销毁。
        // 如果对线程还会有其他操作，则不应使用匿名的线程
        std::thread(handle_clnt, (void *)&clnt_sock).detach();
    }

    close(serv_sock);
    return 0;
}

void * handle_clnt(void * arg) {
    int clnt_sock = *((int *)arg);
    int str_len = 0, i;
    char msg[BUF_SIZE];

    //接受该客户端的名称，在其加入及离开时显示到聊天室
    char name[NAME_SIZE];
    str_len = read(clnt_sock, name, sizeof(name));
    sprintf(msg, "welcome %s join the chatRoom\n", name);
    send_msg(msg, strlen(msg));

    while ((str_len = read(clnt_sock, msg, sizeof(msg))) != 0)
        send_msg(msg, str_len);

    sprintf(msg, "%s leave the chatRoom\n", name);
    send_msg(msg, strlen(msg));

    //清除断开连接的clnt_sock，需要加锁
    mutx.lock();
    for (int i = 0; i < clnt_cnt; i++) {
        if (clnt_socks[i] == clnt_sock) {
            while (i++ < clnt_cnt - 1)
                clnt_socks[i] = clnt_socks[i + 1];
            break; 
        }
    }
    clnt_cnt--;
    mutx.unlock();

    close(clnt_sock);
}

//用send_msg给所有clnt发送消息
void send_msg(char * msg, int len) {
    int i;
    mutx.lock();
    for (int i = 0; i < clnt_cnt; i++)
        write(clnt_socks[i], msg, len);
    mutx.unlock();
}

//用于处理错误信息
void error_handling(char * message) {
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}