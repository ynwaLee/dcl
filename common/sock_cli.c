




#include <sys/select.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "applog.h"

int connect2srv(char *ip, unsigned short port)
{
    int sockfd;
    struct sockaddr_in servaddr;
    struct timeval timeo = {1, 0};
    socklen_t len = sizeof(timeo);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        applog(LOG_ERR, APP_LOG_MASK_BASE, "create socket to server fail");
        return -1;
    }
    applog(LOG_DEBUG, APP_LOG_MASK_BASE, "create socket to server success");

    //set the timeout period
    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeo, len);

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    if(inet_pton(AF_INET, ip, &servaddr.sin_addr) != 1){  
        applog(LOG_ERR, APP_LOG_MASK_BASE, "inet_pton error for %s", ip);  
        close(sockfd);
        return -1; 
    }

    if(connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) != 0){
        applog(LOG_ERR, APP_LOG_MASK_BASE, "connect to server(ip: %s, port: %d) is %s.",
                ip, port, (errno == EINPROGRESS)?"timeout":"fail");
        close(sockfd);
        return -1;
    }
    applog(LOG_INFO, APP_LOG_MASK_BASE, "connect to server(ip: %s, port: %d) success.", ip, port);

    return sockfd;
}

