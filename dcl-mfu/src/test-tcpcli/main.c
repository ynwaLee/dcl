#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#include "sockop.h"

#if 0
#define REQ_DATA "GET http://192.168.1.40:2010/p?systype=1&callid=6222048521124413478&vocfile=1%2f20150719%2f12%2f348%2f20150719125500-2f35-0-6a-18.amr&callflag=0&object_id=1&online=0&c1=&c2=&i1=&i2= HTTP/1.1\r\n"
#endif

#if 1
#define REQ_DATA "GET http://192.168.40.229:2010/p?systype=1&online=1&objectid=&clueid=11&callid=6237036909176782851&callcode=56&vocfile=1%2f20150719%2f12%2f348%2f20150719125500-2f35-0-6a-18.amr&callflag=0&c1=&c2=&i1=&i2= HTTP/1.1\r\n"
#endif

#define MAX_VRS_RULE_NUM 16
typedef struct vrs_voice_cdr {
	uint64_t callid;
	unsigned int up_userid;
	unsigned int down_userid;
	unsigned int up_voiceid;
	unsigned int down_voiceid;
	unsigned int up_ruleids[MAX_VRS_RULE_NUM];
	unsigned int down_ruleids[MAX_VRS_RULE_NUM];
	unsigned int up_rule_num;   //匹配规则数量
	unsigned int down_rule_num;    //匹配规则数量
	unsigned int up_percent;
	unsigned int down_percent;
	unsigned int stage;
	unsigned int flag;
}vrs_voice_cdr_t;


int main(int argc, char *argv[])
{
    if (argc != 3) {
        printf("No ip and port.\n");
        return -1;
    }
    printf("req: %s\n", REQ_DATA);

    char *ip = argv[1];
    unsigned short port = atoi(argv[2]);

    int sockfd;
    if ((sockfd = sock_connect(ip, port)) == -1) {
        printf("connect to %s %d failed.\n", ip, port);
        return -1;
    }
    printf("connect to %s %d success.\n", ip, port);

    int sended, recved;
    char buf[1024] = {0};

#if 1
	while (1) {
		vrs_voice_cdr_t cdr = {0};
		cdr.callid = 123456789;
		cdr.up_userid = 123;
		cdr.down_userid = 456;
		cdr.up_voiceid = 123;
		cdr.down_voiceid = 456;
		cdr.up_percent = 99;	
		cdr.down_percent = 99;	
		cdr.up_rule_num = 2;
		cdr.stage = 1;
		cdr.flag = 10;
		sock_send(sockfd, &cdr, sizeof(cdr), &sended);
		printf("send ok\n");
		recv(sockfd, buf, 4, 0);
		printf("recv ok\n");
		sleep(1);
	}
#endif
    memcpy(buf, REQ_DATA, sizeof(REQ_DATA));
    if (sock_send(sockfd, buf, sizeof(REQ_DATA), &sended) == -1) {
        printf("send request data failed.\n");
        return -1;
    }

    int ret = 0;
    int bytes = 0;
    while (1) {
        if ((ret = recv(sockfd, buf, 1024, 0)) == -1) {
            printf("recv data failed.\n");
            return -1;
        }else if (ret == 0) {
			bytes += ret;
            printf("success: recved %d bytes.\n", bytes);
            return 0;
        }
        
        bytes += ret;
		printf("bytes: %d\n", bytes);
    }

    return 0;
}
