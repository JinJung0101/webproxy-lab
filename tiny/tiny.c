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
void serve_static(int fd, char *filename, int filesize, char *method);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs, char *method);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);

int main(int argc, char **argv) {
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  /* Check command line args */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  listenfd = Open_listenfd(argv[1]);
  while (1) {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);  // line:netp:tiny:accept
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    doit(connfd);   // line:netp:tiny:doit
    Close(connfd);  // line:netp:tiny:close
  }
}

// 클라이언트로부터 HTTP 요청을 받아 처리하고, 해당 요청에 맞는 응답을 반환하는 함수
void doit(int fd)
{
  int is_static;  // 정적인 파일인지 동적인 파일인지 판단하는 변수
  struct stat sbuf;  // 파일 상태를 저장할 구조체
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE];
  rio_t rio;

  /* Read request line and headers */
  //rio 구조체를 초기화하고, 이를 파일 디스크립터 fd와 연결
  Rio_readinitb(&rio, fd); 
  //rio가 가리키는 파일 디스크립터에서 한 줄을 읽어 buf에 저장하고, 읽은 바이트 수를 반환
  Rio_readlineb(&rio, buf, MAXLINE);
  printf("Request headers:\n");
  printf("%s", buf);
  sscanf(buf, "%s %s %s", method, uri, version); // 요청에서 메서드, URI, 버전을 추출
  if ((strcasecmp(method, "GET") != 0 && strcasecmp(method, "HEAD") != 0)) { 
    clienterror(fd, method, "501", "Not implemented", "Tiny does not implement this method");
    return;
  }
  // if (!(strcasecmp(method, "GET") == 0 || strcasecmp(method, "HEAD") == 0)) { 
  //   clienterror(fd, method, "501", "Not implemented", "Tiny does not implement this method");
  //   return;
  // }
  read_requesthdrs(&rio);

  /* GET 요청에서 URI를 파싱 */
  is_static = parse_uri(uri, filename, cgiargs); // URI에서 파일 이름과 CGI 인자를 추출
  if (stat(filename, &sbuf) < 0) {  // 파일이 존재하지 않으면 에러 반환
    clienterror(fd, filename, "404", "Not found", "Tiny couldn’t find this file");
    return;
  }

  if (is_static) { // 정적인 파일인 경우
  // 파일이 일반 파일이 아니거나 읽을 수 없으면 에러 반환
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn’t read the file");
      
      return;
    }
    // 정적인 내용을 제공
    serve_static(fd, filename, sbuf.st_size, method);
  }
  else { // 동적인 파일인 경우
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {
      // 파일이 일반 파일이 아니거나 실행할 수 없으면 에러 반환
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn’t run the CGI program");
      return;
    }
    // 동적인 내용을 제공
    serve_dynamic(fd, filename, cgiargs, method);
  }
}


//웹 서버에서 오류가 발생했을 때, 적절한 HTTP 오류 메시지를 생성하고 클라이언트에게 전송하는 역할
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
  char buf[MAXLINE], body[MAXBUF];

  /* Build the HTTP response body */
  sprintf(body, "<html><title>Tiny Error</title>"); // HTML 시작 및 제목 설정
  sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body); // 본문 시작 및 배경색 설정
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg); // 오류 번호와 짧은 메시지 추가
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause); // 긴 메시지와 오류 원인 추가
  sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body); // 웹 서버 식별자 추가

  /* Print the HTTP response */
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg); // 상태 라인 생성
  Rio_writen(fd, buf, strlen(buf));   // 상태 라인 전송
  sprintf(buf, "Content-type: text/html\r\n"); // Content-Type 헤더 생성
  Rio_writen(fd, buf, strlen(buf));   // Content-Type 헤더 전송
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body)); // Content-Length 헤더 생성
  Rio_writen(fd, buf, strlen(buf)); // Content-Length 헤더 전송
  Rio_writen(fd, body, strlen(body)); // 본문 전송
}


// HTTP 요청 헤더를 읽는 함수. rp는 robust I/O 버퍼 포인터
void read_requesthdrs(rio_t *rp)
{
  char buf[MAXLINE];

  // 첫 번째 헤더 라인 읽기
  Rio_readlineb(rp, buf, MAXLINE);
  while(strcmp(buf, "\r\n")) {
    Rio_readlineb(rp, buf, MAXLINE); // 첫 번째 헤더 라인 읽기
    printf("%s", buf); // 읽은 헤더 라인 출력
  }
  return;
}


// URI를 분석하여 파일 이름과 CGI 인수를 추출
// uri는 파싱할 URI, filename은 결과 파일 이름, cgiargs는 결과 CGI 인수를 저장할 버퍼
int parse_uri(char *uri, char *filename, char *cgiargs)
{
  char *ptr;

  if (!strstr(uri, "cgi-bin")) { /* 정적 컨텐츠 */
    strcpy(cgiargs, ""); // CGI 인수는 없음
    strcpy(filename, "."); // 파일 이름 시작
    strcat(filename, uri); // URI를 파일 이름에 추가
    if (uri[strlen(uri)-1] == '/') // URI가 '/'로 끝나면
      strcat(filename, "home.html"); // 기본 파일 이름 추가
    return 1; // 정적 컨텐츠임을 나타내는 1 반환
  }
  else { /* 동적 컨텐츠 */
    ptr = index(uri, '?'); // '?' 위치 찾기
    if (ptr) { // '?'가 있으면
      strcpy(cgiargs, ptr+1); // '?' 다음부터 끝까지를 CGI 인수로 복사
      *ptr = '\0'; // URI를 '?' 이전까지로 잘라내기
    }
    else
      strcpy(cgiargs, ""); // '?'가 없으면 CGI 인수는 없음
    strcpy(filename, "."); // 파일 이름 시작
    strcat(filename, uri); // URI를 파일 이름에 추가
    return 0; // 동적 컨텐츠임을 나타내는 0 반환
  }
}


// 웹 서버에 저장되어 있는 정적 파일을 찾아서 사용자에게 전달하는 역할
void serve_static(int fd, char *filename, int filesize, char *method)
{
  int srcfd;
  char *srcp, filetype[MAXLINE], buf[MAXBUF];

  /* 클라이언트에게 응답 헤더를 전송합니다. */
  get_filetype(filename, filetype); // 파일 이름에서 파일 타입 추출
  sprintf(buf, "HTTP/1.0 200 OK\r\n"); // 상태 라인 작성
  sprintf(buf, "%sServer: Tiny Web Server\r\n", buf); // 서버 소프트웨어 헤더 추가
  sprintf(buf, "%sConnection: close\r\n", buf); // 연결 헤더 추가
  sprintf(buf, "%sContent-length: %d\r\n", buf, filesize); // 콘텐츠 길이 헤더 추가
  sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype); // 콘텐츠 타입 헤더 추가
  Rio_writen(fd, buf, strlen(buf)); // 헤더 전송
  printf("Response headers:\n");
  printf("%s", buf); // 헤더 출력

  if (strcasecmp(method, "GET") == 0) {
    srcfd = Open(filename, O_RDONLY, 0); //파일의 파일 디스크립터 srcfd에 저장
    // srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0); //Mmap으로 매핑된 메모리의 주소가 srcp에 저장
    srcp = (char*)Malloc(filesize);
    Rio_readn(srcfd, srcp, filesize);
    Close(srcfd); //파일을 메모리에 매핑이후 필요하지 않은 파일 디스크립터를 Close 
    Rio_writen(fd, srcp, filesize); // 매핑된 메모리에서 파일 내용을 읽어서 클라이언트에게 전송
    // Munmap(srcp, filesize); // 필요하지 않은 메모리 매핑을 해제
    free(srcp);
  }
  // else {
  //   srcfd = Open(filename, O_RDONLY, 0);
  //   // mmap: 큰 파일을 메모리에 매핑하거나, 여러 프로세스가 공유하는 메모리 영역을 생성하는데 주로 사용
  //   srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
  //   Close(srcfd);
  //   Rio_writen(fd, srcp, filesize);
  //   Munmap(srcp, filesize);
  // }
}

  
  // 파일 이름을 입력으로 받아 해당 파일의 MIME 타입을 결정하는 역할
  // MIME 타입은 클라이언트가 어떻게 파일을 처리할지를 결정하는 데 사용
  void get_filetype(char *filename, char *filetype)
  {
    if (strstr(filename, ".html"))
      strcpy(filetype, "text/html");
    else if (strstr(filename, ".gif"))
      strcpy(filetype, "image/gif");
    else if (strstr(filename, ".png"))
      strcpy(filetype, "image/png");
    else if (strstr(filename, ".jpg"))
      strcpy(filetype, "image/jpeg");
    else if (strstr(filename, ".mp4"))
      strcpy(filetype, "video/mp4");
    else
    strcpy(filetype, "text/plain");
}


//클라이언트의 요청에 따라 CGI 프로그램을 실행하여 결과를 반환
void serve_dynamic(int fd, char *filename, char *cgiargs, char *method)
{
  char buf[MAXLINE], *emptylist[] = { NULL };

  // HTTP 응답의 첫 부분을 버퍼에 저장하는 부분
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  // 버퍼의 내용을 클라이언트에게 전송
  Rio_writen(fd, buf, strlen(buf)); 
  // HTTP 응답 헤더의 일부로 서버 정보를 버퍼에 저장
  sprintf(buf, "Server: Tiny Web Server\r\n"); 
  // 버퍼의 내용을 클라이언트에게 전송하는 부분
  Rio_writen(fd, buf, strlen(buf));

  if (Fork() == 0) { // 자식 프로세스를 생성
    /* Real server would set all CGI vars here */
    setenv("QUERY_STRING", cgiargs, 1); // QUERY_STRING(CGI 프로그램에게 전달되는 인자 담고 있음)설정
    setenv("REQUEST_METHOD", method, 1); // REQUEST_METHOD(HTTP 요청 메소드 담고 있음)설정
    // 파일 디스크립터를 복제하여 표준 출력(STDOUT_FILENO)에 할당
    Dup2(fd, STDOUT_FILENO); 
    // filename으로 지정된 CGI 프로그램을 실행 Dup2 함수를 통해 리디렉션된 표준 출력을 통해 클라이언트에게 전송
    Execve(filename, emptylist, environ); /* Run CGI program */
  }
  Wait(NULL); /* Parent waits for and reaps child */
}