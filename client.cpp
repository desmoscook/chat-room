#include <iostream>
#include <unistd.h> 
#include <string.h> 
#include <arpa/inet.h> 
#include <sys/socket.h> 
#include <thread>

#define BUF_SIZE 100 
#define NAME_SIZE 20

void *send_msg(void *arg); 
void *recv_msg(void *arg); 
void error_handling(char *msg);
char name[NAME_SIZE] = "[DEFAULT]";
char msg[BUF_SIZE];

int main(int argc, char * argv[]) {
    int sock;
    struct sockaddr_in serv_addr;
    
    if (argc != 4) {
        printf("Usage : %s <ip> <port> <name>\n", argv[0]);
        exit(1);
    }

    //把名字保存
    sprintf(name, "[%s]", argv[3]);

    sock = socket(PF_INET, SOCK_STREAM, 0);

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) 
        error_handling("connect() error");

    //创建两个线程来处理消息，一个用来发送，一个用来接受
    std::thread snd_thread(send_msg, (void *)&sock);
    std::thread rcv_thread(recv_msg, (void *)&sock);
    snd_thread.join();
    rcv_thread.join();

    close(sock);
    return 0;
}

void *send_msg(void *arg) {
    int sock = *((int *)arg);
    char name_msg[NAME_SIZE + BUF_SIZE];

    //发送name告知聊天室自己何时加入和离开
    write(sock, name, strlen(name));

    while (1) {
        fgets(msg, BUF_SIZE, stdin);
        if (!strcmp(msg, "q\n") || !strcmp(msg, "Q\n")) {
            close(sock);
            exit(0);
        }
        sprintf(name_msg, "%s %s", name, msg);
        write(sock, name_msg, strlen(name_msg));
    }
}

void *recv_msg(void * arg) {
    int sock = *((int *)arg);
    char name_msg[NAME_SIZE + BUF_SIZE];
    int str_len;
    
    while (1) {
        str_len = read(sock, name_msg, NAME_SIZE + BUF_SIZE - 1);
        if (str_len == -1)
            return (void *)-1;
        name_msg[str_len] = 0;
        fputs(name_msg, stdout);
    }
}

void error_handling(char *msg) {
    fputs("msg", stderr);
    fputc('\n', stderr);
    exit(1);
}