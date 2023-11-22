#include "csapp.h"

int main(void) 
{
  char *buf, *p, *method;
  char *arg1, *arg2, content[MAXLINE], val1[MAXLINE], val2[MAXLINE];
  int n1=0, n2=0;

  /* Extract the two arguments */
  if ((buf = getenv("QUERY_STRING")) != NULL) 
  { arg1 = strtok(buf, "&");
    arg2 = strtok(NULL, "&");
    if (arg1 != NULL && arg2 != NULL) {
      n1 = atoi(arg1 + strlen("fnum="));
      n2 = atoi(arg2 + strlen("snum="));
    }
  }
  method = getenv("REQUEST_METHOD");
  /* Make the response body */
  sprintf(content, "QUERY_STRING=%s", buf);
  sprintf(content, "Welcome to add.com: ");
  sprintf(content, "%sTHE Internet addition portal.\r\n<p>", content);
  sprintf(content, "%sThe answer is: %d + %d = %d\r\n<p>", content, n1, n2, n1 + n2);
  sprintf(content, "%sThanks for visiting!\r\n", content);
  
  /* Generate the HTTP response */
  printf("Connection: close\r\n");
  printf("Content-length: %d\r\n", (int)strlen(content));
  printf("Content-type: text/html\r\n\r\n");
  printf("%s", content);
  fflush(stdout);

  exit(0);
}