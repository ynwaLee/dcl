
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>


fd_set g_fds;
int g_maxfd;

inline void clear_file_set()
{
    FD_ZERO(&g_fds);
}


inline void check_file_add_fd(int fd)
{
    FD_SET(fd,&g_fds);
    if (fd > g_maxfd)
        g_maxfd = fd;
}


inline int check_files_readable()
{
    struct timeval to;
    int ret;

    to.tv_sec = 0;
    to.tv_usec = 1000;
    ret = select(g_maxfd+1, &g_fds, NULL, NULL, &to);
    return ret;
}

inline int file_readable(int fd)
{
    return FD_ISSET(fd, &g_fds);
}

int check_file_readable(int fd)
{
    fd_set fds;
    struct timeval to;
    int maxfd;
    int ret;

    FD_ZERO(&fds);
    FD_SET(fd,&fds);
    maxfd = fd + 1;
    to.tv_sec = 0;
    to.tv_usec = 1000;
    ret = select(maxfd, &fds, NULL, NULL, &to);
    if ((ret == 1) && (FD_ISSET(fd, &fds)))
        return 1;
    return 0;
}


int sock_send_buf(int fd, unsigned char *buf, int size)
{
    int len = 0;
    int ret;

    while (len < size)
    {
        ret = send(fd, &buf[len], size-len, 0);
        if (ret<0)
            return -1;
        len+=ret;
    }
    return 0;
}


int sock_recv_buf(int fd, unsigned char *buf, int size)
{
    int ret;
    int len = 0;

    while (len < size)
    {
        ret = recv(fd, &buf[len], (size-len), 0);
        if (ret <= 0)
            return -1;
        len += ret;
    }
    return 0;
}


