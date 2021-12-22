/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);

//open_listenfd 함수를 사용해 듣기 소켓을 생성하고, Accept 함수를 통해 연결을 요청한 clientfd와 연결함.
int main(int argc, char **argv) {
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  /* Check command line args */
  // argc 2가아면, port 입력 받지못한 에러를 프린트해주고 종료
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  // Open_listenfd를 호출해서 듣기 소켓을 오픈한다.
  listenfd = Open_listenfd(argv[1]);
  while (1) { //전형적인 무한 서버 루프를 실행한다. (Listen은 항상 열려있는 상태로 들어야하니까)
    clientlen = sizeof(clientaddr);
    //accept함수에서 clientaddr에 저장한 클라이언트의 주소를 getnameinfo 함수의 인자로 넣어 서버의 Ip주소와 포트번호를 얻고 프린트함
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);  
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    doit(connfd);   // line:netp:tiny:doit //트랜잭션 수행 //client에서 받은 요청을 처리하는 doit 함수 실행
    Close(connfd);  // line:netp:tiny:close //자신쪽의 연결 닫음
  }
}

//TINY doit은 한개의 HTTP 트랜잭션을 처리한다. 즉 한개의 클라이언트 요청을 처리해 클라이언트에게 컨텐츠를 제공한다.
//client로 부터 받은 request 정보를 가공해서 (Parse-uri) > client가 원하는걸 파악 (Dynamic, static) > 원하는걸 가공해서 준다.
void doit(int fd)
{
  int is_static;
  struct stat sbuf;
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE];
  rio_t rio;

  /* Read request line and headers */
  //클라이언트의 HTTP 요청에서 요청라인만 읽음
  Rio_readinitb(&rio, fd);
  Rio_readlineb(&rio, buf, MAXLINE); //rio_readlineb를 통해 요청 텍스트의 제일 위 한줄(요청라인)을 읽었다고 생각!
  printf("Request headers: \n"); 
  printf("%s", buf);
  sscanf(buf, "%s %s %s", method, uri, version); //buf에 method, uri, version에 각각 입력받는다.
  // printf("CORRECT: %s %s %s\n", method, uri, version);
  if (strcasecmp(method, "GET")) //GET메소드 인지 확인한다. //strcasecmp : 대소문자 상관없이 str비교
  {
    clienterror(fd, method, "501", "Not implemented", "Tiny does not implement this method"); //GET외에 다른 메소드 요청시 에러메시지 보내고 main으로 돌아감.
    return;
  }
  read_requesthdrs(&rio); //다른 요청 읽음

  
  /* Parse URI from GET request */
  printf("uri: %s\n", uri);
  is_static = parse_uri(uri, filename, cgiargs); //uri를 잘라 uri, filename, cigiargs로 나누고 클라이언트가 정적 컨텐츠를 원하는지, 동적컨텐츠를 원하는지 확인함.
  // printf("error::%s\n", filename);

  if (stat(filename, &sbuf)<0)
  {
    clienterror(fd, filename, "404", "Not found", "Tiny couldn't find this file");
    return;
  }
  if(is_static) //요청이 정적 컨텐츠를 위한것, 실행을 원하는 파일의 stat구조체의 st_mode를 이용해 파일이 읽기권한과 실행권한이 있는지 확인함.
  { //파일이 있는지 || 읽을 권한이 있는지
    if(!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode))
    {
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't read the file");
      return;
    }
    serve_static(fd, filename, sbuf.st_size); //정적컨텐츠 제공
  }
  else //요청이 동적 콘첸트를 위한것, 실행을 원하는 파일의 stat구조체의 st_mode를 이용해 파일이 읽기권한과 실행권한이 있는지 확인함.
  {
    if(!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) 
    {
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't run the CGI program");
      return;
    }
    serve_dynamic(fd, filename, cgiargs); //동적컨텐츠 제공
  }
}

//일부 명백한 에러에 대해서 client에게 HTTP응답을 보낸다.
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
  char buf[MAXLINE], body[MAXBUF];

  /* Build the HTTP response body */
  sprintf(body, "<html><title>Tiny Error</title>");
  sprintf(body, "%s<body bgcolor = ""ffffff"">\r\n", body);
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
  sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);
  
  /* Print the HTTP response */
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
  Rio_writen(fd, buf, strlen(buf));
  Rio_writen(fd, body, strlen(body));
}

//요청헤더를 읽고, 요청헤더를 종료하는 빈 텍스트줄 ("\r\n")이 나올때까지 요청 헤더를 모두 읽어들인다.
void read_requesthdrs(rio_t *rp)
{
  char buf[MAXLINE];

  Rio_readlineb(rp, buf, MAXLINE); //요청헤더를 읽는다.
  while(strcmp(buf, "\r\n")) //요청헤더를 종료하는 빈 텍스트줄 ("\r\n")이 나올때까지 요청 헤더를 모두 읽어들인다. > 즉 요청헤더 외엔 다 무시한다.
  {
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
  }
  return;
}

//URI파일 이름과 옵션으로 CGI 인자스트림 분석 
//말 그대로 URI나누는 함수 URI를 도메인, Path, cgiargs로 나눈다. (cgiargs란 동적 컨텐츠의 실행파일에 들어갈 인자다.)
//tiny서버의 모든 동적 콘텐츠를 위한 실행파일은 cgi-bin 이라는 디렉토리에 넣는 고전적인 방식으로 정적 컨텐츠와 분리를 시킬것임.
//따라서 uri에 cgi-bin이라는 경로가 있는지 확인을 해 정적 컨텐츠를 보내야하는지 동적 컨텐츠를 보내야하는지 판단할 수 있음.
int parse_uri(char *uri, char *filename, char *cgiargs)
{
  char *ptr;
  //static content
  if (!strstr(uri, "cgi-bin")) //strstr(대상문자열, 검색할문자열) 문자열 찾았으면 문자열로 시작하는 문자열의 포인터 반환, 없으면 NULL
  {
    strcpy(cgiargs,""); //uri에 cgi-bin과 일치하는 문자열이 없다면 cgiargs에는 빈 문자열 저장
    strcpy(filename, "."); //uri를 ./index.html과 같은 리눅스 경로이름으로 변환 
    strcat(filename, uri);
    if (uri[strlen(uri)-1] == '/') //만약 경로이름이 '/'로 끝나면 "home.html" 파일 이름 추가함.
    {
      strcat(filename, "home.html");
    }
    return 1;
  }
  else //Dynamic content
  {
    ptr = index(uri, '?');
    if(ptr) //cgi인자 추출
    {
      strcpy(cgiargs, ptr+1);
      *ptr = '\0';
    }
    else
    {
      strcpy(cgiargs, "");
    }
    strcpy(filename, "."); //uri부분을 상대 리눅스 파일 이름으로 변환함.
    strcat(filename, uri);
    return 0;
  }
}

//서버의 디스크 파일을 정적 컨텐츠라고 하며, 디스크파일을 가져와 클라이언트에게 전달하는 작업을 정적컨텐츠를 처리한다고 한다.
//1. 컨텐츠를 보내기 이전에 어떤 컨텐츠를 보낼지, 어느 정도 크기의 컨텐츠를 보낼지 포함한 response header를 보낸다.
//2. 요청 한 파일을 읽기 전용으로 열고 파일의 내용을 가상메모리 영역에 저장한다.
//3. 가상메모리에 저장된 내용을 클라이언트와 연결된 연결식별자에 작성해 컨텐츠를 클라이언트에게 보낸다.

void serve_static(int fd, char *filename, int filesize)
{
  int srcfd;
  char *srcp, filetype[MAXLINE], buf[MAXBUF];

  /* Send response headers to client */
  get_filetype(filename, filetype); //파일 이름의 접미어 부분 검사해서 파일타입 결정
  sprintf(buf, "HTTP/1.0 200 OK\r\n"); //클라이언트에 응답결과 응답헤어 보낸다.
  sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
  sprintf(buf, "%sConnection: close\r\n", buf);
  sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
  sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
  Rio_writen(fd, buf, strlen(buf));
  printf("Response headers: \n");
  printf("%s", buf);

  /* Send response body to client */
  //요청한 파일 내용을 연결식별자 fd로 복사해서 응답본체 보냄
  srcfd = Open(filename, O_RDONLY, 0); //읽기 위해서 filename 오픈하고, 식별자를 얻어옴.
  srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0); //요청한 파일을 가상메모리 영역으로 매핑한다. 즉 srcfd의 첫번째 filesize 바이트를 주소 srcp에서 시작하는 사적 익기 허용
  Close(srcfd); //파일 메모리로 매핑 후 필요 없으니 닫아줌
  Rio_writen(fd, srcp, filesize); // 파일을 클라이언트에 전송함. rio_writen 함수는 주소 srcp에서 시작하는 filesize바이트를 클라이언트의 연결식별자로 복사함.
  Munmap(srcp, filesize); //매핑된 가상메모리 주소 반환함. //메모리누수를 막기위해 끊어준다.
}

/* get_filetype -Derive file type from filename */
//response header에 들어갈 내용인 클라이언트가 요청한 파일의 타입을 확인한다.
void get_filetype(char *filename, char *filetype)
{
  if(strstr(filename, ".html"))
  {
    strcpy(filetype, "text/html");
  }
  else if(strstr(filename, ".gif"))
  {
    strcpy(filetype, "image/gif");
  }
  else if(strstr(filename, ".png"))
  {
    strcpy(filetype, "image/png");
  }
  else if(strstr(filename, ".jpg"))
  {
    strcpy(filetype, "image/jpeg");
  }
  else if(strstr(filename, ".mpeg"))
  {
    strcpy(filetype, "video/mp4");
  }
  else
  {
    strcpy(filetype, "text/plain");
  }  
}

void serve_dynamic(int fd, char *filename, char *cgiargs)
{
  char buf[MAXLINE], *emptylist[] = {NULL};

  /* Return first part of HTTP response */
  //클라이언트에 성공을 알려주는 응답라인 보내는것 시작!

  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Server: Tiny Web Server\r\n");
  Rio_writen(fd, buf, strlen(buf));

  if (Fork() == 0)
  {
    setenv("QUERY_STRING", cgiargs, 1);
    Dup2(fd, STDOUT_FILENO);
    Execve(filename, emptylist, environ);
  }
  wait(NULL);
}

