#include "csapp.h"

int main(int argc, char **argv)
{
    int clientfd;
    char *host, *port, buf[MAXLINE];
    rio_t rio;
    
    if (argc != 3)
    {
        fprintf(stderr, "usage : %s <host> <port> \n", argv[0]);
        exit(0);
    }
    host = argv[1];
    port = argv[2];
    clientfd = Open_clientfd(host, port);
    Rio_readinitb(&rio, clientfd);

    //Fgets는 stdin에서 MAXLINE만큼 읽는다. 읽을 때 \n을 만나거나 MAXLINE-1만큼 읽고 buf에 저장한다.
    while (Fgets(buf, MAXLINE, stdin) != NULL)
    {   
        //buf에 있는 값을 buf의 길이만큼 clienfd에 전송한다.
        Rio_writen(clientfd, buf, strlen(buf));
        //ssize_t rio_readlineb(rio_t rp, void usrbuf, size_t maxlen) 함수는 
        //다음 텍스트 줄을 파일 rp(종료 세 줄 문자를 포함해서)에서 읽고, 이것을 메모리 위치 usrbuf로 복사하고, 텍스트 라인을 NULL 문자로 종료시킨다.
        Rio_readlineb(&rio, buf, MAXLINE);
        Fputs(buf, stdout);
    }
    Close(clientfd);
    exit(0);
}