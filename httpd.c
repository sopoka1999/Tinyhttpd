#include <stdio.h> 
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <strings.h>
#include <string.h>
#include <sys/stat.h>
#include <pthread.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/select.h>

#define ISspace(x) isspace((int)(x))

#define SERVER_STRING "Server: jdbhttpd/0.1.0\r\n"
#define STDIN   0
#define STDOUT  1
#define STDERR  2
#define S_IFMT	0170000
#define S_IFDIR	0040000
void not_found(int);
void bad_request(int);
void cannot_execute(int);
void unimplemented(int);

void accept_request(int);
void error_die(const char *);
void execute_cgi(int, const char *, const char *, const char *);
int get_line(int, char *, int);
void serve_file(int, const char *);
int startup(unsigned short *);

void bad_request(int client){
    char buf[1024];
    sprintf(buf, "HTTP/1.0 400 BAD REQUEST\r\n");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "Content-type: text/html\r\n");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "<P>Your browser sent a bad request, ");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "such as a POST without a Content-Length.\r\n");
    send(client, buf, sizeof(buf), 0);
}

void cannot_execute(int client){
    char buf[1024];
    sprintf(buf, "HTTP/1.0 500 Internal Server Error\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<P>Error prohibited CGI execution.\r\n");
    send(client, buf, strlen(buf), 0);
}
void not_found(int client){
    char buf[1024];
    sprintf(buf, "HTTP/1.0 404 NOT FOUND\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, SERVER_STRING);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<HTML><TITLE>Not Found</TITLE>\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<BODY><P>The server could not fulfill\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "your request because the resource specified\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "is unavailable or nonexistent.\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "</BODY></HTML>\r\n");
    send(client, buf, strlen(buf), 0);
}

void unimplemented(int client){
    char buf[1024];
    sprintf(buf, "HTTP/1.0 501 Method Not Implemented\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, SERVER_STRING);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<HTML><HEAD><TITLE>Method Not Implemented\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "</TITLE></HEAD>\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<BODY><P>HTTP request method not supported.\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "</BODY></HTML>\r\n");
    send(client, buf, strlen(buf), 0);
}


void get_method_and_url(char buf[1024],char* method,char* url){

    size_t i = 0, j = 0;
    while (i < 1024 && buf[i] != ' '){
        method[i] = buf[i];
        i++;
    }
    method[i] = '\0';
    i++;

    while (i < 1024 && j < 255 && buf[i] != ' ' ){
        url[j] = buf[i];
        i++;
        j++;
    }
    url[j] = '\0';

    //处理http://45.145.228.39:4000/assets/index-32589002.js 到 /assets/index-32589002.js
    char *path = strstr(url, "://");  // 查找第一个出现的 "://"
    if (path != NULL) {
        path += 3;  // 跳过 "://"
        char *slash = strchr(path, '/');  // 查找下一个出现的 "/"
        if (slash != NULL) {
            sprintf(url,slash);
            printf("slash != NULL %s %s\n\n",slash,url);
        } else {
            url[0] = '/';
            url[1] = '\0';
        }
    }

}

void accept_request(int client){
    char buf[1024];
    char method[255];
    char url[255];
    char path[512];
    size_t numchars;
    struct stat st;
    char * query_string = NULL;

    fd_set readfds;
    struct timeval timeout;
    int result;
    FD_ZERO(&readfds);
    FD_SET(client, &readfds);
    timeout.tv_sec = 10;  // 设置超时为60秒
    timeout.tv_usec = 0;
    result = select(client + 1, &readfds, NULL, NULL, &timeout);
    if (result == 0) {
        printf("超时!\n");
        close(client);
        return ;
    }

    printf("a new client connect, %d\n\n",client);
    // 读取第一行： Method url protocol
    numchars = get_line(client, buf, sizeof(buf)); 
    // printf("hearder|%s|\n",buf);
    get_method_and_url(buf,method,url);

    // 读取后面的请求头
    while ((numchars > 0) && strcmp("\n", buf)){
        numchars = get_line(client, buf, sizeof(buf));
    }

    printf("method = %s \nurl = %s\n\n",method,url);
    
    if(strcasecmp(method, "GET") == 0){
        query_string = strchr(url, '?');
        if(query_string == NULL){

            sprintf(path, "docs%s", url);

            if (path[strlen(path) - 1] == '/')
                strcat(path, "index.html");

            if (stat(path, &st) == -1) {
                not_found(client);
                printf("没有找到文件 %s\n",path);
                close(client);
                return;
            }else if ((st.st_mode & S_IFMT) == S_IFDIR){ //判断是否为目录
                strcat(path, "/index.html");
                if (stat(path, &st) == -1) {
                    not_found(client);
                    close(client);
                    return;
                }
            }
            printf("path = %s\n",path);
            serve_file(client, path);
        }else{
            query_string = query_string + 1;
            printf("%d\n",query_string);
        }
    }else if(strcasecmp(method, "POST") == 0){
        execute_cgi(client, path, method, query_string);
    }else{
        unimplemented(client);
    }
    accept_request(client);
    return;
}


void error_die(const char *sc){
    perror(sc);
    exit(1);
}

void execute_cgi(int client, const char *path, const char *method, const char *query_string){
    char buf[1024];
    int cgi_output[2];
    int cgi_input[2];
    pid_t pid;
    int status;
    int i;
    char c;
    int numchars = 1;
    int content_length = -1;

    buf[0] = 'A'; buf[1] = '\0';
    numchars = get_line(client, buf, sizeof(buf));
    while ((numchars > 0) && strcmp("\n", buf))
    {
        buf[15] = '\0';
        if (strcasecmp(buf, "Content-Length:") == 0)
            content_length = atoi(&(buf[16]));
        numchars = get_line(client, buf, sizeof(buf));
    }
    if (content_length == -1) {
        bad_request(client);
        return;
    }

    if (pipe(cgi_output) < 0) {
        cannot_execute(client);
        return;
    }
    if (pipe(cgi_input) < 0) {
        cannot_execute(client);
        return;
    }

    if ( (pid = fork()) < 0 ) {
        cannot_execute(client);
        return;
    }
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    send(client, buf, strlen(buf), 0);
    if (pid == 0)  /* child: CGI script */
    {
        char meth_env[255];
        char query_env[255];
        char length_env[255];

        dup2(cgi_output[1], STDOUT);
        dup2(cgi_input[0], STDIN);
        close(cgi_output[0]);
        close(cgi_input[1]);
        sprintf(meth_env, "REQUEST_METHOD=%s", method);
        putenv(meth_env);
        if (strcasecmp(method, "GET") == 0) {
            sprintf(query_env, "QUERY_STRING=%s", query_string);
            putenv(query_env);
        }
        else {   /* POST */
            sprintf(length_env, "CONTENT_LENGTH=%d", content_length);
            putenv(length_env);
        }
        execl(path, NULL);
        exit(0);
    } else {    /* parent */
        close(cgi_output[1]);
        close(cgi_input[0]);
        if (strcasecmp(method, "POST") == 0)
            for (i = 0; i < content_length; i++) {
                recv(client, &c, 1, 0);
                write(cgi_input[1], &c, 1);
            }
        while (read(cgi_output[0], &c, 1) > 0)
            send(client, &c, 1, 0);

        close(cgi_output[0]);
        close(cgi_input[1]);
        waitpid(pid, &status, 0);
    }
}


int get_line(int sock, char *buf, int size)
{
    int i = 0;
    char c = '\0';
    int n;

    while ((i < size - 1) && (c != '\n'))
    {
        n = recv(sock, &c, 1, 0);
        /* DEBUG printf("%02X\n", c); */
        if (n > 0)
        {
            if (c == '\r')
            {
                n = recv(sock, &c, 1, MSG_PEEK);
                /* DEBUG printf("%02X\n", c); */
                if ((n > 0) && (c == '\n'))
                    recv(sock, &c, 1, 0);
                else
                    c = '\n';
            }
            buf[i] = c;
            i++;
        }
        else
            c = '\n';
    }
    buf[i] = '\0';

    return(i);
}

long getFileSize(FILE *file) {
    long size;
    fseek(file, 0, SEEK_END); // 将文件指针移到文件末尾
    size = ftell(file); // 获取文件指针的位置，即文件大小
    fseek(file, 0, SEEK_SET); // 将文件指针重置回文件开头
    printf("size = %d\n",size);
    return size;
}

void serve_file(int client, const char* filename){
    FILE *resource = fopen(filename, "r");
    if (resource == NULL){
        not_found(client);
    }else{
        char buf[1024];
        char Content_Type[128];
        char* filename_suffix = strchr(filename, '.');

        if(filename_suffix == NULL){
            sprintf(Content_Type, "application/octet-stream");
        }else{
            if(strcmp(filename_suffix,".html") == 0){
                sprintf(Content_Type, "text/html");
            }else if(strcmp(filename_suffix,".txt") == 0){
                sprintf(Content_Type, "text/plain");
            }else if(strcmp(filename_suffix,".css") == 0){
                sprintf(Content_Type, "text/css");
            }else if(strcmp(filename_suffix,".mp4") == 0){
                sprintf(Content_Type, "video/mpeg4");
            }else if(strcmp(filename_suffix,".jpg") == 0){
                sprintf(Content_Type, "application/x-jpg");
            }else if(strcmp(filename_suffix,".png") == 0){
                sprintf(Content_Type, "image/png");
            }else if(strcmp(filename_suffix,".jpeg") == 0){
                sprintf(Content_Type, "image/jpeg");
            }else if(strcmp(filename_suffix,".ico") == 0){
                sprintf(Content_Type, "image/x-icon");
            }else if(strcmp(filename_suffix,".gif") == 0){
                sprintf(Content_Type, "image/gif");
            }else if(strcmp(filename_suffix,".pdf") == 0){
                sprintf(Content_Type, "application/pdf");
            }else if(strcmp(filename_suffix,".js") == 0){
                sprintf(Content_Type, "application/x-javascript");
            }else if(strcmp(filename_suffix,".jsp") == 0){
                sprintf(Content_Type, "text/html");
            }else if(strcmp(filename_suffix,".svg") == 0){
                sprintf(Content_Type, "image/svg+xml");
            }else{
                sprintf(Content_Type, "text/html");
            }
        }
        sprintf(buf, "HTTP/1.1 200 OK\r\n");
        send(client, buf, strlen(buf), 0);
        sprintf(buf, SERVER_STRING);
        send(client, buf, strlen(buf), 0);
        sprintf(buf, "Content-Type: %s\r\n",Content_Type);
        send(client, buf, strlen(buf), 0);
        sprintf(buf, "Connection: keep-alive\r\n");
        send(client, buf, strlen(buf), 0);
        sprintf(buf, "Content-Length: %d\r\n",getFileSize(resource));
        send(client, buf, strlen(buf), 0);
        sprintf(buf, "Keep-Alive: timeout=60, max=100\r\n");
        send(client, buf, strlen(buf), 0);
        sprintf(buf, "\r\n");
        send(client, buf, strlen(buf), 0);
        size_t bytes;
        size_t total_bytes = 0;
        
        while ((bytes = fread(buf, sizeof(char), sizeof(buf), resource)) > 0){
            total_bytes += bytes;
            send(client, buf, bytes, 0);
            // fwrite(buf, sizeof(char), bytes, stdout);
        }
        printf("total_bytes = %d\n",total_bytes);
        fclose(resource);
    }
}

int startup(unsigned short *port){
    int httpd = 0;
    int on = 1;
    struct sockaddr_in name;

    httpd = socket(PF_INET, SOCK_STREAM, 0);
    if (httpd == -1)
        error_die("socket");
    memset(&name, 0, sizeof(name));
    name.sin_family = AF_INET;
    name.sin_port = htons(*port);
    name.sin_addr.s_addr = htonl(INADDR_ANY);
    // 如果想让内核选择IP地址，则可以将地址设为INADDR_ANY
    
    //允许立刻重用端口，建立新的服务，而不必等待操作系统释放
    if ((setsockopt(httpd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))) < 0)  {
        error_die("setsockopt failed");
    }

    if (bind(httpd, (struct sockaddr *)&name, sizeof(name)) < 0)
        error_die("bind");

    if (*port == 0){   /* if dynamically allocating a port */
        socklen_t namelen = sizeof(name);
        if (getsockname(httpd, (struct sockaddr *)&name, &namelen) == -1)
            error_die("getsockname");
        *port = ntohs(name.sin_port);
    }

    if (listen(httpd, 5) < 0)
        error_die("listen");

    return httpd;
}

/**********************************************************************/
// 45.145.228.39
int main(){
    int server_sock = -1;
    unsigned short port = 4000;
    struct sockaddr_in client_addr;
    socklen_t  client_addr_len = sizeof(client_addr);
    pthread_t newthread;

    server_sock = startup(&port);
    printf("httpd running on port %d\n", port);

    while (1){
        //在服务端建立一个socket，这个socket与客户端的socket连接
        int client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_sock == -1){
            perror("get a error connect\n");
            continue;
        }
        
        if (pthread_create(&newthread , NULL, (void *)accept_request, client_sock) != 0)
            perror("pthread_create");
    }
    close(server_sock);
    return(0);
}
