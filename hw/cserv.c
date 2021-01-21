include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define BUF_SIZE 100
#define MAX_CLNT 256

void * handle_clnt(void * arg);
void send_msg(char * msg, int len);
void error_handling(char * msg);

int clnt_cnt=0;
int clnt_socks[MAX_CLNT];
pthread_mutex_t mutx;

int main(int argc, char *argv[])
{
    int serv_sock, clnt_sock; // 서버 소켓과 클라이언트 소켓 선언
    struct sockaddr_in serv_adr, clnt_adr; // 서버 어드레스와 클라이언트 어드레스 구조체 선언
    int clnt_adr_sz; 클라이언트 어드레스 사이즈
    pthread_t t_id;
    if(argc!=2) { // main함수의 파라미터로 포트를 입력해야 함
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    }

    pthread_mutex_init(&mutx, NULL); 
    serv_sock=socket(PF_INET, SOCK_STREAM, 0); // 서버 소켓 생성

    /*서버 어드레스 구조체 초기화*/
    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family=AF_INET; 
    serv_adr.sin_addr.s_addr=htonl(INADDR_ANY);
    serv_adr.sin_port=htons(atoi(argv[1]));

    /*listen 상태와 bind 상태 확인*/
    if(bind(serv_sock, (struct sockaddr*) &serv_adr, sizeof(serv_adr))==-1)
        error_handling("bind() error");
    if(listen(serv_sock, 5)==-1)
        error_handling("listen() error");

    printf("Welcome to GBC Network Server!\n");

    while(1)
    {
        clnt_adr_sz=sizeof(clnt_adr);
        clnt_sock=accept(serv_sock, (struct sockaddr*)&clnt_adr,&clnt_adr_sz); //클라이언트 소켓이 연결 요청을 수락함 

        pthread_mutex_lock(&mutx); // 겹치는 통신 제거
        clnt_socks[clnt_cnt++]=clnt_sock; 
        pthread_mutex_unlock(&mutx); // 다시 언락

        pthread_create(&t_id, NULL, handle_clnt, (void*)&clnt_sock); // thread를 통해 handle_clnt 반복
        pthread_detach(t_id); // thread t_id를 분리시킴
        printf("Connected client IP: %s \n", inet_ntoa(clnt_adr.sin_addr));
    }
    close(serv_sock); // 서버 닫음
    return 0;
}

void * handle_clnt(void * arg)
{
    int clnt_sock=*((int*)arg);
    int str_len=0, i;
    char msg[BUF_SIZE];

    while((str_len=read(clnt_sock, msg, sizeof(msg)))!=0) //read가 일어나지 않을 때 까지 send_msg를 통해 메시지를 보냄
        send_msg(msg, str_len);

    pthread_mutex_lock(&mutx); // 겹치는 통신 제거
    for(i=0; i<clnt_cnt; i++)   // remove disconnected client
    {
        if(clnt_sock==clnt_socks[i])
        {
            while(i++<clnt_cnt-1)
                clnt_socks[i]=clnt_socks[i+1];
            break;
        }
    }
    clnt_cnt--;
    pthread_mutex_unlock(&mutx); // 다시 언락
    close(clnt_sock); // 클라이언트 종료
    return NULL;
}
void send_msg(char * msg, int len)   // send to all
{
    int i;
    pthread_mutex_lock(&mutx); // 겹치는 통신 제거 
    for(i=0; i<clnt_cnt; i++)
        write(clnt_socks[i], msg, len); // 클라이언트 카운트 만큼 wirte 하며 메시지 전달
    pthread_mutex_unlock(&mutx); // 언락
}
void error_handling(char * msg) // 에러 처리 
{
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}
