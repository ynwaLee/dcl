#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "sockop.h"

#if 0
#define REQ_DATA "GET http://192.168.1.100:2010/p?systype=2&callid=6222048521124413478&vocfile=1%2f20150719%2f12%2f348%2f20150719125500-2f35-0-6a-18.amr&callflag=0&object_id=1&online=0&c1=&c2=&i1=&i2= HTTP/1.1\r\n"
#endif

#if 0
#define REQ_DATA "GET http://192.168.1.100:2010/p?systype=2&callid=6222048521124413478&vocfile=test.amr&callflag=0&object_id=1&online=1&c1=&c2=&i1=&i2= HTTP/1.1\r\n"
#endif

int main(int argc, char *argv[])
{
    if (argc != 3) {
        printf("No ip and port.\n");
        return -1;
    }

    char *ip = argv[1];
    unsigned short port = atoi(argv[2]);
    
    int fd = open("hi3.dat", O_RDWR);
	if (fd == -1) {
		perror("open");
		return -1;
	}

    int connect_flag = 0;
    signal(SIGPIPE, SIG_IGN);
    int sockfd;

while (1) {
    if (connect_flag == 0) {
        if ((sockfd = sock_connect(ip, port)) == -1) {
            printf("connect to %s %d failed.\n", ip, port);
            sleep(1);    
            continue;
        }
        connect_flag = 1;
        printf("connect to %s %d success.\n", ip, port);
    }

    int sended, recved;
    int ret;
    int bytes = 0;

	while (1) {
    unsigned char buf[512] = {0};
	int len = read(fd, buf, 512);
	if (len == 0) {
		if (lseek(fd, 0, SEEK_SET) == -1) {
            perror("lseek");
        }
			sleep(1);
    }

		{
			int i = 0;
			unsigned char out[2048] = {0};
			for (; i < len; i++) {
				snprintf(out + 2*i, 3, "%02X", buf[i]);
			}
			printf("len: %d, buf: %s\n", len, out);
		}

        if ((ret = sock_send(sockfd, buf, len, &sended)) == -1) {
            printf("send data failed.\n");
            close(sockfd);
            connect_flag = 0;
            break;
        }
    }
}
    return 0;
}
