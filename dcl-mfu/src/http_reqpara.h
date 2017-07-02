#ifndef __HTTP_REQPARA_H__
#define __HTTP_REQPARA_H__

#define MAX_PARA_NAME_LEN  1024
#define MAX_PARA_VALUE_LEN 1024

typedef struct http_reqpara_s {
    int systype;
    unsigned long long callid;
    char vocfile[MAX_PARA_VALUE_LEN];
    int callflag;
    unsigned long long clueid; //ruleid
    unsigned int callcode; //channel
    unsigned int online;
    int c1;
    int c2;
    int i1;
    int i2;
    int channel;
	char city_code[16];
}http_reqpara_t;

int http_reqpara_get(char *parabuf, const char *buf);
void http_reqpara_show(http_reqpara_t *http_reqpara);
extern int http_reqpara_parse(const char *str, http_reqpara_t *para);
#endif
