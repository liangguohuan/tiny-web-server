#include <arpa/inet.h>
#include <signal.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "functions.h"


/*--------------------------------------------------------------------------
| Disabled warning: implicit-function-declaration
|---------------------------------------------------------------------------
| As we all know
| function calls need to be declared first and then called in C.
*/
DIR *fdopendir(int fd);
int openat(int dirfd, const char *pathname, int flags, ...);

/*--------------------------------------------------------------------------
| macro define
|---------------------------------------------------------------------------
*/
#define LISTENQ  1024  /* second argument to listen() */
#define MAXLINE 1024   /* max length of a line */
#define RIO_BUFSIZE 1024
#define HOME getenv("HOME")
#define CONF_PATH ".config/tinyserver"
#define CACHE_PATH CONF_PATH"/cache"

/*--------------------------------------------------------------------------
| struct define
|---------------------------------------------------------------------------
*/
typedef struct {
    int rio_fd;                 /* descriptor for this buf */
    int rio_cnt;                /* unread byte in this buf */
    char *rio_bufptr;           /* next unread byte in this buf */
    char rio_buf[RIO_BUFSIZE];  /* internal buffer */
} rio_t;

/* Simplifies calls to bind(), connect(), and accept() */
typedef struct sockaddr SA;

typedef struct {
    int refresh;
    char *host;
    char *cache_control;
    char filename[512];
    off_t offset;              /* for support Range */
    size_t end;
} http_request;

typedef struct {
    const char *extension;
    const char *mime_type;
} mime_map;

void rio_readinitb(rio_t *rp, int fd){
    rp->rio_fd = fd;
    rp->rio_cnt = 0;
    rp->rio_bufptr = rp->rio_buf;
}

/*--------------------------------------------------------------------------
| global variable
|---------------------------------------------------------------------------
*/
char *ROOT;

mime_map meme_types [] = {
    {".css", "text/css"},
    {".gif", "image/gif"},
    {".htm", "text/html"},
    {".html", "text/html"},
    {".jpeg", "image/jpeg"},
    {".jpg", "image/jpeg"},
    {".ico", "image/x-icon"},
    {".js", "application/javascript"},
    {".pdf", "application/pdf"},
    {".mp4", "video/mp4"},
    {".png", "image/png"},
    {".svg", "image/svg+xml"},
    {".xml", "text/xml"},
    {NULL, NULL},
};

char *default_mime_type = "text/plain";

/*--------------------------------------------------------------------------
| functions
|---------------------------------------------------------------------------
*/
ssize_t writen(int fd, void *usrbuf, size_t n){
    size_t nleft = n;
    ssize_t nwritten;
    char *bufp = usrbuf;

    while (nleft > 0){
        if ((nwritten = write(fd, bufp, nleft)) <= 0){
            if (errno == EINTR)  /* interrupted by sig handler return */
                nwritten = 0;    /* and call write() again */
            else
                return -1;       /* errorno set by write() */
        }
        nleft -= nwritten;
        bufp += nwritten;
    }
    return n;
}


/*
 * rio_read - This is a wrapper for the Unix read() function that
 *    transfers min(n, rio_cnt) bytes from an internal buffer to a user
 *    buffer, where n is the number of bytes requested by the user and
 *    rio_cnt is the number of unread bytes in the internal buffer. On
 *    entry, rio_read() refills the internal buffer via a call to
 *    read() if the internal buffer is empty.
 */
/* $begin rio_read */
static ssize_t rio_read(rio_t *rp, char *usrbuf, size_t n){
    int cnt;
    while (rp->rio_cnt <= 0){  /* refill if buf is empty */

        rp->rio_cnt = read(rp->rio_fd, rp->rio_buf,
                           sizeof(rp->rio_buf));
        if (rp->rio_cnt < 0){
            if (errno != EINTR) /* interrupted by sig handler return */
                return -1;
        }
        else if (rp->rio_cnt == 0)  /* EOF */
            return 0;
        else
            rp->rio_bufptr = rp->rio_buf; /* reset buffer ptr */
    }

    /* Copy min(n, rp->rio_cnt) bytes from internal buf to user buf */
    cnt = n;
    if (rp->rio_cnt < n)
        cnt = rp->rio_cnt;
    memcpy(usrbuf, rp->rio_bufptr, cnt);
    rp->rio_bufptr += cnt;
    rp->rio_cnt -= cnt;
    return cnt;
}

/*
 * rio_readlineb - robustly read a text line (buffered)
 */
ssize_t rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen){
    int n, rc;
    char c, *bufp = usrbuf;

    for (n = 1; n < maxlen; n++){
        if ((rc = rio_read(rp, &c, 1)) == 1){
            *bufp++ = c;
            if (c == '\n')
                break;
        } else if (rc == 0){
            if (n == 1)
                return 0; /* EOF, no data read */
            else
                break;    /* EOF, some data was read */
        } else
            return -1;    /* error */
    }
    *bufp = 0;
    return n;
}

void handle_directory_request(int out_fd, int dir_fd, http_request *req){

    char cachename[255], cachename_encode[255], cachefile[255], templatefile[50];

    sprintf(cachename, "%s/%s", ROOT, req->filename);
    url_encode(cachename_encode, cachename);

    sprintf(cachefile, "%s/%s/%s", HOME, CACHE_PATH, cachename_encode);
    sprintf(templatefile, "%s/%s/%s", HOME, CONF_PATH, "dir.template.html");

    mlog(1, "cachefile: %s", cachefile);
    mlog(1, "templatefile: %s", templatefile);

    char buf[MAXLINE], m_time[32], size[16];
    struct stat statbuf;
    sprintf(buf, "HTTP/1.1 200 OK\r\n%s",
            "Content-Type: text/html\r\n\r\n");
    writen(out_fd, buf, strlen(buf));

    // read from cache
    if (req->refresh == 0 && file_exists(cachefile)) {
        char *cache_content = read_file(cachefile);
        writen(out_fd, cache_content, strlen(cache_content));
        free(cache_content);
        mlog(1, "%s", "[CACHE] load cache file.");
        return ;
    }

    long t1 = now();
    // Read dir
    char *data = "";
    DIR *d = fdopendir(dir_fd);
    struct dirent *dp;
    int ffd;
    while ((dp = readdir(d)) != NULL){
        if(!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, "..")){
            continue;
        }
        if ((ffd = openat(dir_fd, dp->d_name, O_RDONLY)) == -1){
            perror(dp->d_name);
            continue;
        }
        fstat(ffd, &statbuf);
        strftime(m_time, sizeof(m_time),
                 "%Y-%m-%d %H:%M", localtime(&statbuf.st_mtime));
        format_size(size, &statbuf);
        if(S_ISREG(statbuf.st_mode) || S_ISDIR(statbuf.st_mode)){
            char *d = S_ISDIR(statbuf.st_mode) ? "/" : "";
            char *dattr = S_ISDIR(statbuf.st_mode) ? "data-dir" : "data-file";
            sprintf(buf, "[\"<a %s href=\\\"%s%s\\\">%s%s</a>\", \"%s\", \"%s\", \"%ld\", \"%d\"],",
                    dattr, dp->d_name, d, dp->d_name, d, m_time, size, (long)statbuf.st_size, S_ISDIR(statbuf.st_mode) ? 0 : 1);
            data = join(data, buf);
        }
        close(ffd);
    }

    // Read dir template file
    char *content = read_file(templatefile);
    content = str_replace(content, "{{data}}", data);
    writen(out_fd, content, strlen(content));

    long t2 = now();

    // write cache file if overtime
    if (t2 - t1 > 3) {
        write_file(cachefile, content, "w");
        mlog(1, "%s", "[CACHE] write cachefile.");
    }

    // free resource
    free(data);
    free(content);

    closedir(d);
}

static const char* get_mime_type(char *filename){
    char *dot = strrchr(filename, '.');
    if(dot){ // strrchar Locate last occurrence of character in string
        mime_map *map = meme_types;
        while(map->extension){
            if(strcmp(map->extension, dot) == 0){
                return map->mime_type;
            }
            map++;
        }
    }
    return default_mime_type;
}


int open_listenfd(int port){
    int listenfd, optval=1;
    struct sockaddr_in serveraddr;

    /* Create a socket descriptor */
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return -1;

    /* Eliminates "Address already in use" error from bind. */
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,
                   (const void *)&optval , sizeof(int)) < 0)
        return -1;

    // 6 is TCP's protocol number
    // enable this, much faster : 4000 req/s -> 17000 req/s
    if (setsockopt(listenfd, 6, TCP_CORK,
                   (const void *)&optval , sizeof(int)) < 0)
        return -1;

    /* Listenfd will be an endpoint for all requests to port
       on any IP address for this host */
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons((unsigned short)port);
    if (bind(listenfd, (SA *)&serveraddr, sizeof(serveraddr)) < 0)
        return -1;

    /* Make it a listening socket ready to accept connection requests */
    if (listen(listenfd, LISTENQ) < 0)
        return -1;
    return listenfd;
}

void parse_request(int fd, http_request *req){
    rio_t rio;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], cache_control[10], host[100];
    req->offset = 0;
    req->end = 0;              /* default */
    req->refresh = 0;

    rio_readinitb(&rio, fd);
    rio_readlineb(&rio, buf, MAXLINE);
    // mlog(2, "[REQUEST] %s", buf);
    sscanf(buf, "%s %s", method, uri); /* version is not cared */
    /* read all */
    int nlimit = 0;
    while(buf[0] != '\n' && buf[1] != '\n' && nlimit < 20) { /* \n || \r\n */
        // make sure accept standar header
        if (buf[strlen(buf) -1] != '\n') break;

        rio_readlineb(&rio, buf, MAXLINE);
        // mlog(2, "[REQUEST] %s", buf);
        if(buf[0] == 'R' && buf[1] == 'a' && buf[2] == 'n'){
            sscanf(buf, "Range: bytes=%lu-%lu", &req->offset, &req->end);
            // Range: [start, end]
            if( req->end != 0) req->end ++;
        } else if(buf[0] == 'C' && buf[1] == 'a' && buf[2] == 'c') {
            sscanf(buf, "Cache-Control: %s", cache_control);
            req->cache_control = cache_control;
            if(strcmp(cache_control, "no-cache") == 0) req->refresh = 1;
        } else if(buf[0] == 'H' && buf[1] == 'o' && buf[2] == 's') {
            sscanf(buf, "Host: %s", host);
            req->host = host;
        }
        nlimit++;
    }
    char* filename = uri;
    if(uri[0] == '/'){
        filename = uri + 1;
        int length = strlen(filename);
        if (length == 0){
            filename = ".";
        } else {
            for (int i = 0; i < length; ++ i) {
                if (filename[i] == '?') {
                    filename[i] = '\0';
                    break;
                }
            }
        }
    }

    mlog(2, "[REQUEST] filename: %s", filename);
    url_decode(filename, req->filename, MAXLINE);

    /* Auto index file */
    if (filename[strlen(filename) - 1] == '/' || strcmp(filename, ".") == 0) {
        char ifile[512];
        sprintf(ifile, "%sindex.html", strcmp(filename, ".") == 0 ? "./" : filename);
        if(access(ifile, F_OK) != -1) {
            sprintf(req->filename, "%s", ifile);
            mlog(1, "Auto Index File: %s", ifile);
        }
    }
}


void log_access(int status, struct sockaddr_in *c_addr, http_request *req){
    mlog(1, "[ACCESS] %s:%d %d - %s", inet_ntoa(c_addr->sin_addr),
           ntohs(c_addr->sin_port), status, req->filename);
}

void client_error(int fd, int status, char *msg, char *longmsg){
    char buf[MAXLINE];
    sprintf(buf, "HTTP/1.1 %d %s\r\n", status, msg);
    sprintf(buf + strlen(buf),
            "Content-length: %lu\r\n\r\n", strlen(longmsg));
    sprintf(buf + strlen(buf), "%s", longmsg);
    writen(fd, buf, strlen(buf));
}

void serve_301(int out_fd, int in_fd, char* location) {
    char buf[256];
    sprintf(buf, "HTTP/1.1 301 Moved Permanently\r\n");
    sprintf(buf + strlen(buf), "Location: %s\r\n", location);
    writen(out_fd, buf, strlen(buf));
    close(out_fd);
    mlog(4, "http 301 moved permanently: %s.", location);
}

char *mimetype_not_support = ".flv, .mkv, .avi, .wmv";
void serve_static(int out_fd, int in_fd, http_request *req,
                  size_t total_size){

    char *extname = strrchr(req->filename, '.');
    // open file with xdg-open and send status 403 if not support
    if (extname != NULL && strstr(mimetype_not_support, extname) != NULL) {
        client_error(out_fd, 403, "Forbidden", "Mime Type Not Support.");
        char cmd[256];
        sprintf(cmd, "xdg-open \"%s\" &", req->filename);
        if (system(cmd) == -1) {
            mlog(4, "system call error");
        }
        close(out_fd);
        return ;
    }

    char buf[256];
    if (req->offset > 0){
        sprintf(buf, "HTTP/1.1 206 Partial\r\n");
        sprintf(buf + strlen(buf), "Content-Range: bytes %lu-%lu/%lu\r\n",
                req->offset, req->end - 1, total_size);
    } else {
        sprintf(buf, "HTTP/1.1 200 OK\r\nAccept-Ranges: bytes\r\n");
    }
    sprintf(buf + strlen(buf), "Cache-Control: no-cache\r\n");
    // sprintf(buf + strlen(buf), "Cache-Control: public, max-age=315360000\r\nExpires: Thu, 31 Dec 2037 23:55:55 GMT\r\n");

    /*
     * char *date= now_rfc();
     * sprintf(buf + strlen(buf), "Date: %s\r\n",
     *         rfctime_now());
     * free(date);
     *
     * char *last_modified = get_fmtime(req->filename);
     * sprintf(buf + strlen(buf), "Last-Modified: %s\r\n",
     *         last_modified);
     * free(last_modified)
     */

    sprintf(buf + strlen(buf), "Content-length: %lu\r\n",
            req->end - req->offset);
    sprintf(buf + strlen(buf), "Content-type: %s\r\n\r\n",
            get_mime_type(req->filename));

    writen(out_fd, buf, strlen(buf));
    off_t offset = req->offset;
    while(offset < req->end){
        mlog(2, "start send file to client");
        if(sendfile(out_fd, in_fd, &offset, req->end - req->offset) <= 0) {
            break;
        }
        mlog(1, "offset: %ld", offset);
        close(out_fd);
        break;
    }
}

void process(int fd, struct sockaddr_in *clientaddr){
    mlog(2, "[PROCESS] accept request, fd is %d, pid is %d", fd, getpid());
    http_request req;
    parse_request(fd, &req);

    mlog(1, "[HEADER] f:%s, h:%s, c:%s, r:%d", req.filename, req.host, req.cache_control, req.refresh);

    struct stat sbuf;
    int status = 200, ffd = open(req.filename, O_RDONLY, 0);
    if(ffd <= 0){
        status = 404;
        char *msg = "File not found";
        client_error(fd, status, "Not found", msg);
    } else {
        fstat(ffd, &sbuf);
        if(S_ISREG(sbuf.st_mode)){
            if (req.end == 0){
                req.end = sbuf.st_size;
            }
            if (req.offset > 0){
                status = 206;
            }
            serve_static(fd, ffd, &req, sbuf.st_size);
        } else if(S_ISDIR(sbuf.st_mode)){
            char lc = req.filename[strlen(req.filename) - 1];
            if (lc != '/' && lc != '.') {
                char dfile[512];
                sprintf(dfile, "/%s/", req.filename);
                serve_301(fd, ffd, dfile);
            } else {
                status = 200;
                handle_directory_request(fd, ffd, &req);
            }
        } else {
            mlog(1, "Handle Unknow Error");
            status = 400;
            char *msg = "Unknow Error";
            client_error(fd, status, "Error", msg);
        }
        close(ffd);
    }
    log_access(status, clientaddr, &req);
}

int main(int argc, char** argv){
    struct sockaddr_in clientaddr;
    int default_port = 9999,
        listenfd,
        connfd;
    char buf[256];
    char *path = getcwd(buf, 256);
    socklen_t clientlen = sizeof clientaddr;
    if(argc == 2) {
        if(argv[1][0] >= '0' && argv[1][0] <= '9') {
            default_port = atoi(argv[1]);
        } else {
            path = argv[1];
            if(chdir(argv[1]) != 0) {
                perror(argv[1]);
                exit(1);
            }
        }
    } else if (argc == 3) {
        default_port = atoi(argv[2]);
        path = argv[1];
        if(chdir(argv[1]) != 0) {
            perror(argv[1]);
            exit(1);
        }
    }

    ROOT = path;
    mlog(2, "[ROOT] %s", path);

    listenfd = open_listenfd(default_port);
    if (listenfd > 0) {
        mlog(2, "listen on port %d, fd is %d", default_port, listenfd);
    } else {
        perror("ERROR");
        exit(listenfd);
    }
    // Ignore SIGPIPE signal, so if browser cancels the request, it
    // won't kill the whole process.
    signal(SIGPIPE, SIG_IGN);

    for(int i = 0; i < 10; i++) {
        int pid = fork();
        if (pid == 0) {         //  child
            while(1){
                connfd = accept(listenfd, (SA *)&clientaddr, &clientlen);
                process(connfd, &clientaddr);
                close(connfd);
            }
        } else if (pid > 0) {   //  parent
            // mlog(1, "child pid is %d\n", pid);
        } else {
            perror("fork");
        }
    }

    while(1){
        connfd = accept(listenfd, (SA *)&clientaddr, &clientlen);
        process(connfd, &clientaddr);
        close(connfd);
    }

    return 0;
}
