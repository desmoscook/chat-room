#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define BUF_SIZE 100
#define MAX_CLNT 256

pthread_mutex_t mutx;
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
    int clnt_adr_sz;
    pthread_t t_id;

    if (argc != 2) {
        printf("Usgae : %s <port> \n", argv[0]);
        exit(1);
    }

    pthread_mutex_init(&mutx, NULL);
    
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
        pthread_mutex_lock(&mutx);
        clnt_socks[clnt_cnt++] = clnt_sock;
        pthread_mutex_unlock(&mutx);

        //创建新的线程处理新链接
        pthread_create(&t_id, NULL, handle_clnt, (void *)&clnt_sock);
        pthread_detach(t_id); //引导线程销毁，不阻塞
        printf("connect..\n");
    }

    close(serv_sock);
    return 0;
}

void * handle_clnt(void * arg) {
    int clnt_sock = *((int *)arg);
    int str_len = 0, i;
    char msg[BUF_SIZE];

    //TODO 加入欢迎和退出公示

    while ((str_len = read(clnt_sock, msg, sizeof(msg))) != 0)
        send_msg(msg, str_len);

    //清除断开连接的clnt_sock，需要加锁
    pthread_mutex_lock(&mutx);
    for (int i = 0; i < clnt_cnt; i++) {
        if (clnt_socks[i] == clnt_sock) {
            while (i++ < clnt_cnt - 1)
                clnt_socks[i] = clnt_socks[i + 1];
            break; 
        }
    }
    clnt_cnt--;
    pthread_mutex_unlock(&mutx);

    close(clnt_sock);
}

//用send_msg给所有clnt发送消息
void send_msg(char * msg, int len) {
    int i;
    pthread_mutex_lock(&mutx);
    for (int i = 0; i < clnt_cnt; i++)
        write(clnt_socks[i], msg, len);
    pthread_mutex_unlock(&mutx);
}

//用于处理错误信息
void error_handling(char * message) {
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}