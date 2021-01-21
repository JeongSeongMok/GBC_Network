#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> 
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define BUF_SIZE 100
#define NAME_SIZE 20

void * send_msg(void * arg);
void * recv_msg(void * arg);
void error_handling(char * msg);

char name[NAME_SIZE]="[DEFAULT]";
char msg[BUF_SIZE];

int main(int argc, char *argv[])
{
    int sock;  // 소켓번호 
    struct sockaddr_in serv_addr ; // 서버의 주소 구조체 선언
    pthread_t snd_thread, rcv_thread; // 다중 일 처리
    void * thread_return;  
    if(argc!=4) {     // main함수의 파라미터로 IP, port, 이름을 넣어야함.
        printf("Usage : %s <IP> <port> <name>\n", argv[0]);
        exit(1);
    }

    sprintf(name, "[%s]", argv[3]);
    sock=socket(PF_INET, SOCK_STREAM, 0);  // 소켓 생성

    /* 서버의 주소 구조체를 초기화*/
    memset(&serv_addr, 0, sizeof(serv_addr)); 
    serv_addr.sin_family=AF_INET;  
    serv_addr.sin_addr.s_addr=inet_addr(argv[1]);
    serv_addr.sin_port=htons(atoi(argv[2]));

    if(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))==-1) // 커넥션 확인
        error_handling("connect() error");

    printf("Welcome to GBC Network Client!\n");

    pthread_create(&snd_thread, NULL, send_msg, (void*)&sock); // send_msg함수를 thread를 통해 반복함
    pthread_create(&rcv_thread, NULL, recv_msg, (void*)&sock); // recv_msg함수를 thread를 통해 반복함
    pthread_join(snd_thread, &thread_return); // snd_thread가 종료될 때 까지 기다림
    pthread_join(rcv_thread, &thread_return); // rcv_thread가 종료될 때 까지 기다림
    close(sock);  
    return 0;
}

void * send_msg(void * arg)   // send thread main
{
    int sock=*((int*)arg);
    char name_msg[NAME_SIZE+BUF_SIZE];
    while(1) 
    {
        fgets(msg, BUF_SIZE, stdin);
        if(!strcmp(msg,"q\n")||!strcmp(msg,"Q\n"))  // q가 입력될 때 까지 계속 반복
        {
            close(sock);
            exit(0);
        }
        sprintf(name_msg,"%s %s", name, msg);
        write(sock, name_msg, strlen(name_msg)); // 소켓에 메시지를 전달함
    }
    return NULL;
}

void * recv_msg(void * arg)   // read thread main
{
    int sock=*((int*)arg);
    char name_msg[NAME_SIZE+BUF_SIZE];
    int str_len;
    while(1)
    {
        str_len=read(sock, name_msg, NAME_SIZE+BUF_SIZE-1); // 메시지를 소켓에 읽음
        if(str_len==-1) 
            return (void*)-1;
        name_msg[str_len]=0;
        fputs(name_msg, stdout);  
    }
    return NULL;
}

void error_handling(char *msg) // 에러처리 
{
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}

