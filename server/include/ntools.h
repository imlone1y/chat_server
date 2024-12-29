#define MAXLINE 80
#define SERV_PORT 8000
#define LISTEN_PORT 6000


extern char myIP[16];
extern int myPORT;


#include <netinet/in.h>
#include <arpa/inet.h>

ssize_t readn(int filedes, void *buff, size_t nbytes);
ssize_t writen(int filedes, const void *buff, size_t nbytes);
ssize_t readline(int filedes, void *buff, size_t maxlen);


ssize_t readn(int fd, void *buff, size_t n) {
    ssize_t nread;
    size_t nleft = n;
    char *ptr = buff;

    while (nleft > 0) {
        if ((nread=read(fd, buff, nleft))<0) {
            if (errno == EINTR)
                nread = 0;
            else  return(-1);
        } else if (nread==0)
            break;  //EOF
        nleft -= nread;
        ptr += nread;
    }
    return (n-nleft);  //return >=0
}

ssize_t writen(int fd, const void *buff, size_t n) {
    ssize_t nwritten;
    size_t nleft = n;
    const char *ptr = buff;

    while (nleft > 0) {
        if ((nwritten=write(fd, buff, nleft))<=0) {
            if (nwritten <0 && errno == EINTR)
                nwritten = 0; //call write again
            else  return(-1);
        } 
        nleft -= nwritten;
        ptr += nwritten;
    }
    return (n);
}

static int read_cnt;
static char *read_ptr;
static char read_buf[MAXLINE];

static ssize_t my_read(int fd, char *c) {
    again:
    if (read_cnt <=0) {
        if ((read_cnt = read(fd, read_buf,sizeof(read_buf)))<0) {
            if (errno == EINTR)
                goto again;
            return(-1);
        } else if (read_cnt==0) 
            return (0);
        read_ptr = read_buf;
    }
    read_cnt--;
    *c = *read_ptr++;
    return(1);
}

ssize_t readline(int fd, void *buff, size_t maxlen) {
    ssize_t n, rc;
    char c, *ptr=buff;

    for(n==1;n<maxlen;n++){
        again:
        //if ((rc=read(fd,&c,1))==1) {
        if ((rc=my_read(fd,&c))==1) {            
            *ptr++ = c;
            if (c=='\n') break;
        }else if (rc==0){
            *ptr = 0;
            return (n-1); // EOF is read, n-1 bytes were read
        }else {
            if (errno == EINTR) 
                goto again;
            return(-1);
        }
    }
    *ptr = 0;
    return(n);
}