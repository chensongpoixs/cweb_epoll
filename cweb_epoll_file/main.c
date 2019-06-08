
#include "cwrap.h"
#include "cpub.h"
#include <sys/epoll.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <signal.h>
#include <strings.h>



#define SERV_PORT 8080
//参数一：文件描述符
//参数二：状态码
//参数三：有没有找到
//参数四：相应文件类型 
//参数五：文件大小
int send_header(int fd, int codec, char *codeMsg,  char *fileType, int filesize) 
{
    char buf[1024] = { 0 };

    //发送头
    sprintf(buf, "HTTP/1.1%d%s\r\n", codec, codeMsg); 
    send(fd, buf, strlen(buf), 0);

    //发送文件文件
    memset(buf, 0x00, sizeof(buf));
    sprintf(buf, "Content-Type:%s\r\n",  fileType);
    printf("-------------------------- utf-8:%s\n", fileType);
    send(fd, buf, strlen(buf), 0);

    //发送文件大小
    if (filesize > 0) 
	{
        memset(buf, 0x00, sizeof(buf));
        sprintf(buf, "Content-Length:%d\r\n", filesize);
        send(fd, buf, strlen(buf), 0);
    }

    send(fd, "\r\n", 2, 0);
    return 0;
}
//参数一：文件描述符
//参数二：文件路径
int send_file(int fd, char *filepath) {

    printf("方法名：%s\n", __FUNCTION__);
    //打开文件
    int fp  = open(filepath, O_RDWR);

    if (fp < 0) {
        printf("open error\n");
        return -1;
    }
    char buf[1024] = {0};
    int ret;
    printf("send client\n");
    while (1) {
        memset(buf, 0x00, sizeof(buf));
        //读取数据
        ret = Read(fp, buf, sizeof(buf));
        if (ret > 0) {
            //发送数据
            send(fd, buf, ret, 0);
            printf("file send:%s\n", buf);
        }
        if (ret <= 0) {
            break;  
        }
    }
    //关闭文件
    Close(fp);
    return 0;

}

//目录的处理方式
int send_dir(int fd, char *filepath) {

    printf("发送信息%s, path %s\n", __FUNCTION__, filepath);
    //读取目录
    //发送头部
    send_header(fd, 200, "OK",  get_mime_type("xx.html"), -1);
    //文件名称
    struct dirent **filename = NULL;
    char strpath[256] = { 0 };

    int i, ret;
    //读取文件的个数
    ret = scandir(filepath, &filename, NULL, alphasort);

    char buf[1024] = {0};
    //utf-8发送
    memset(buf, 0x00, sizeof(buf));
    sprintf(buf, "<!doctype html> <meta http-equiv='%s' content='%s'>", "content-Type", "text/html; charset=utf8");
    send(fd, buf, strlen(buf), 0);

    printf("doctype :%s\n", buf);
    //标题的发送
    memset(buf, 0x00, sizeof(buf));
    sprintf(buf, "<html><head><title> %s </title></head><body> <ul>", "默认路径");
    send(fd, buf, strlen(buf), 0);

    //内容的发送
    for (i = 0; i < ret; i++) {
        memset(buf, 0x00, sizeof(buf));
        memset(strpath, 0x00, sizeof(strpath));
        if (filename[i]->d_type == DT_REG) {
            strcpy(strpath, filename[i]->d_name);
        } else if (filename[i]->d_type == DT_DIR) {
            sprintf(strpath, "%s/", filename[i]->d_name);
        }

        sprintf(buf, "<li><a href='%s'>%s</a></li>", strpath, filename[i]->d_name);
        printf("filename :%s\n", buf);
        send(fd, buf, strlen(buf), 0);

    }
    //发送页尾
    memset(buf, 0x00, sizeof(buf));
    sprintf(buf, "</ul></body></html>");
    send(fd, buf, strlen(buf), 0);
    return 0;
}

int do_requset(int fd, int fepol)
{
    char strLine[1024] = { 0 };

    //得到一行的数据
    int ret = get_line(fd, strLine, sizeof(strLine));
    if (ret <= 0) {
        printf(" Read line error\n");
        struct epoll_event ev;
        ev.data.fd = fd;
        ev.events = EPOLLIN;
        epoll_ctl(fepol, EPOLL_CTL_DEL, fd, &ev);
        Close(fd);
        return -1;
    }

    char buf[1024] = { 0 };
    while ( get_line(fd, buf, sizeof(buf)) > 0) {
        printf("read-client: = %s", buf);
        memset(buf, 0x00, sizeof(buf));
    }
    char method[256], path[256], conprotol[256];
    sscanf(strLine, "%[^ ] %[^ ] %[^ /r/n]", method, path, conprotol);

    printf("method = [%s], path = [%s], conprotol = [%s] \n", method, path, conprotol);

    strdecode(path, path);
    //处理client事件
    char *filepath = &path[1];
    printf("client :%s\n", filepath);
    //判断filepath是否为空
    if (filepath[0] == '\0') {
        filepath = "./";
        //默认界面  
    }
    //判断是否是get请求
    if (strcasecmp(method, "get") == 0) {
        //判断路径filepath是文件还是目录
        struct stat st;
        //没有文件的判断
        if (stat(filepath, &st) < 0) {
            //404 错误界面的信息
            send_header(fd, 404, "NOT FOUND", get_mime_type("error.html"), -1); //头
            send_file(fd, "error.html"); //文件的操作
        }
        //判断是否文件
        if (S_ISREG(st.st_mode)) {
            //
            send_header(fd, 200, "OK", get_mime_type(filepath), st.st_mode); //头
            send_file(fd, filepath); //文件的操作
        }
        //判断 目录的
        if (S_ISDIR(st.st_mode)) {
            //目录的处理
            send_dir(fd, filepath);
            //send_header(fd, 200, "OK", get_mime_type(filepath), st.st_mode); //头
            //send_file(fd, filepath); //文件的操作
        }
    }

    return 0;
}


int main(int argc, char *argv[]) 
{
    //忽略信号SIGPIPE
    struct sigaction act;
    act.sa_flags = 0;
    act.sa_handler = SIG_IGN;
    sigemptyset(&act.sa_mask);
    sigaction(SIGPIPE, &act, NULL);
    //切换到家目录
    chdir(getenv("HOME"));
    chdir("webpath");
    int listenfd, connfd;
    struct sockaddr_in servaddr, cliaddr;
    char strip[16];
    int i, ret, optval = 1;
    int fepol, nready;
    int flags;

    //创建socket
    listenfd = Socket(AF_INET, SOCK_STREAM, 0);

    //绑定ip和port
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERV_PORT);
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    //端口复用
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    Bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
    Listen(listenfd, 128);

    //================== epoll init ========================    
    fepol = epoll_create(FD_SETSIZE);

    struct epoll_event ev, evs[FD_SETSIZE];
    ev.data.fd = listenfd;
    ev.events = EPOLLIN;
    epoll_ctl(fepol, EPOLL_CTL_ADD, listenfd, &ev);

    while (1) {
        //阻塞等待
        nready = epoll_wait(fepol, evs, FD_SETSIZE, -1);
        if (nready > 0) {
            for(i = 0; i < nready; i++) {
                if (evs[i].data.fd == listenfd ) {
                    if (evs[i].events & EPOLLIN) {
                        socklen_t len = sizeof(cliaddr);
                        connfd = Accept(listenfd, (struct sockaddr *)&cliaddr, &len);
                        if (connfd > 0) {
                            printf("client ip:%s, port :%d\n",  inet_ntop(AF_INET, &cliaddr.sin_addr.s_addr, strip, sizeof(strip)), ntohs(cliaddr.sin_port));

                            ev.data.fd = connfd;
                            ev.events = EPOLLIN;
                            epoll_ctl(fepol, EPOLL_CTL_ADD, connfd, &ev);
                            //设置非阻塞文件描述符
                            flags = fcntl(connfd, F_GETFL);
                            flags |= O_NONBLOCK;
                            fcntl(connfd, F_SETFL, flags);

                        }
                    }
                }
                else {
                    if (evs[i].events & EPOLLIN) {
                        do_requset(evs[i].data.fd, fepol);
                    }
                }
            }
        }
    }


    Close(listenfd);
    return 0;
}