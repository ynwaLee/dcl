#include "sockop.h"
#include <alloca.h>
#include <strings.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>

int sock_recv(int sockfd, void *buf, int size, int *recved)
{
	*recved = 0;
	int ret = 0;

	while (*recved < size) {
		ret = recv(sockfd, buf + *recved, size - *recved, 0);
		if (ret <= 0) {
			return ret;
		}
		*recved += ret;
	}
	return 1;
}

int sock_send(int sockfd, void *buf, int size, int *sended)
{
	*sended = 0;
	int ret;

	while (*sended < size) {
		ret = send(sockfd, buf + *sended, size - *sended, 0);
		if (ret < 0) {
			return ret;
		}
		*sended += ret;
	}
	return 1;
}

int sock_connect(const char *ip, unsigned short port)
{
    struct timeval timeo = {1, 0};
    socklen_t len = sizeof(timeo);
	int sockfd = -1;
	
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		return -1;
	}

	struct sockaddr_in *srvaddr = alloca(sizeof(struct sockaddr_in));
	bzero(srvaddr, sizeof(struct sockaddr_in));
	srvaddr->sin_family = AF_INET;
	srvaddr->sin_port = htons(port);
	if (!inet_aton(ip, &srvaddr->sin_addr)) {
		close(sockfd);
		return -1;
	}
	
    //set the timeout period
    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeo, len);

	if (connect(sockfd, (struct sockaddr *)srvaddr, sizeof(struct sockaddr)) == -1) {
		close(sockfd);
		return -1;
	}

	return sockfd;
}

int sock_listen(const char *ip, unsigned short port)
{
	int sockfd = -1;
	
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		return -1;
	}

	struct sockaddr_in *srvaddr = alloca(sizeof(struct sockaddr_in));
	bzero(srvaddr, sizeof(struct sockaddr_in));
	srvaddr->sin_family = AF_INET;
	srvaddr->sin_port = htons(port);
	if (!inet_aton(ip, &srvaddr->sin_addr)) {
		close(sockfd);
		return -1;
	}

	if (bind(sockfd, (struct sockaddr *)srvaddr, sizeof(struct sockaddr)) == -1) {
		close(sockfd);
		return -1;
	}

	if (listen(sockfd, 100) == -1) {
		close(sockfd);
		return -1;
	}

	return sockfd;
}

bool sock_readable(int sockfd, int sec, int usec)
{
	struct timeval *tv = alloca(sizeof(struct timeval));
	tv->tv_sec = sec;
	tv->tv_usec = usec;

	fd_set rfds;
	FD_ZERO(&rfds);
	FD_SET(sockfd, &rfds);

	int ret = select(sockfd + 1, &rfds, NULL, NULL, tv);
	if (ret > 0 && FD_ISSET(sockfd, &rfds)) {
		return true;
	}

	return false;
}
