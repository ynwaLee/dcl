#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include "http_reqpara.h"
#include "common_header.h"
#include "mfu.h"

static int http_reqpara_entry_parse(const char *str, http_reqpara_t *para)
{
    char name[MAX_PARA_NAME_LEN] = {0};
    char value[MAX_PARA_VALUE_LEN] = {0};
    int i = 0;

    for (i = 0; i < MAX_PARA_NAME_LEN - 1 && str[i] != '='; i++) {
        name[i] = str[i];
    }

    if (str[i] != '=') {
        xapplog(LOG_ERR, APP_LOG_MASK_BASE, "No symbol = in entry: %s.", str);
        return -1;
    }
    
    int k = i + 1;
    for (i = 0; i < MAX_PARA_VALUE_LEN - 1 && str[k+i] != '&' && str[k+i] != '\0'; i++) {
        value[i] = str[k+i];
    }

    if (i == MAX_PARA_VALUE_LEN - 1) {
        xapplog(LOG_ERR, APP_LOG_MASK_BASE, "the value is too long.");
        return -1;
    }

    // systype online objectid clueid callid callcode vocfile callflag channel
    if (!strcasecmp(name, "systype")) {
        if (sscanf(value, "%d", &para->systype) != 1) {
            para->systype = 0;
        }
    }
    else if (!strcasecmp(name, "callid")) {
        if (sscanf(value, "%llu", &para->callid) != 1) {
            para->callid = 0;
        }
    }
    else if (!strcasecmp(name, "online")) {
        if (sscanf(value, "%u", &para->online) != 1) {
            para->online = 0;
        }
    }
    else if (!strcasecmp(name, "clueid")) {
        if (sscanf(value, "%llu", &para->clueid) != 1) {
            para->clueid = 0;
        }
    }
    else if (!strcasecmp(name, "callcode")) {
        if (sscanf(value, "%u", &para->callcode) != 1) {
            para->callcode = 0;
        }
    }
    else if (!strcasecmp(name, "vocfile")) {
        int i = 0;
        int j = 0;
        int len = (int)strlen(value);
        while (i < len) {
            if (value[i] == '%') {
                //if (memcmp(value + i, "%2f", 3) == 0 || memcmp(value + i, "%2F", 3) == 0) {
                //    value[j++] = '/';
                //    i += 3;
                //}
                if (i + 2 < len) {
                    char tmp_chr_buf[16] = {0};
                    tmp_chr_buf[0] = value[i + 1];
                    tmp_chr_buf[1] = value[i + 2];
                    unsigned int tmp_int = 0;
                    sscanf(tmp_chr_buf, "%x", &tmp_int);
                    value[j++] = tmp_int;
                    i += 3;
                }
                else {
                    xapplog(LOG_ERR, APP_LOG_MASK_BASE, "can't parse the %% in right way.");
                    return -1;
                }
            }else {
                value[j++] = value[i++];
            }
        }
        value[j] = 0;

        if (sscanf(value, "%s", para->vocfile) != 1) {
            para->vocfile[0] = '\0';
        }
    }
    else if (!strcasecmp(name, "callflag")) {
        if (sscanf(value, "%d", &para->callflag) != 1) {
            para->callflag = 0;
        }
    }
    else if (!strcasecmp(name, "objectid")) {
        ;
    }
    else if (!strcasecmp(name, "c1")) {
        if (sscanf(value, "%d", &para->c1) != 1) {
            para->c1 = 0;
        }
    }
    else if (!strcasecmp(name, "c2")) {
        if (sscanf(value, "%d", &para->c2) != 1) {
            para->c2 = 0;
        }
    }
    else if (!strcasecmp(name, "i1")) {
        if (sscanf(value, "%d", &para->i1) != 1) {
            para->i1 = 0;
        }
    }
    else if (!strcasecmp(name, "i2")) {
        if (sscanf(value, "%d", &para->i2) != 1) {
            para->i2 = 0;
        }
    }
    else if (!strcasecmp(name, "channel")) {
        if (sscanf(value, "%d", &para->channel) != 1) {
            para->channel = 0;
        }
    }
	else if (!strcasecmp(name, "upareaid")) {
        strncpy(para->city_code , value , strlen(value));
    }
    else {
        xapplog(LOG_ERR, APP_LOG_MASK_BASE, "Bad name: %s.", name);
    }

    return 0;
}

int http_reqpara_parse(const char *str, http_reqpara_t *para)
{
    const char *ps = str;
    const char *pe = str;

    unsigned int buflen = MAX_PARA_NAME_LEN + MAX_PARA_VALUE_LEN + 2;
    char *buf = (char *)alloca(buflen);
    size_t entrylen = 0;
    while ((pe = strchr(ps, '&')) != NULL) {
        entrylen = (size_t)(pe - ps);
        if (entrylen > buflen) {
            xapplog(LOG_ERR, APP_LOG_MASK_BASE, "para entry is too long.");
            return -1;
        }
        
        memcpy(buf, ps, entrylen);
        buf[entrylen] = 0;
        if (http_reqpara_entry_parse(buf, para) == -1) {
            xapplog(LOG_ERR, APP_LOG_MASK_BASE, "Failed to parse the para entry");
            return -1;
        }
        
        ps = pe + 1;
    }

    if (strlen(ps) > 0) {
        if (http_reqpara_entry_parse(ps, para) == -1) {
            xapplog(LOG_ERR, APP_LOG_MASK_BASE, "Failed to parse the para entry");
            return -1;
        }
    }

    return 0;
}

void http_reqpara_show(http_reqpara_t *http_reqpara)
{
    xapplog(LOG_INFO, APP_LOG_MASK_BASE, "systype: %d, online: %d, clueid: %d, callid: %llu, callcode: %d, vocfile: %s, callflag: %d, c1: %d, c2: %d, i1: %d, i2: %d, channel: %d",
            http_reqpara->systype,
            http_reqpara->online,
            http_reqpara->clueid,
            http_reqpara->callid,
            http_reqpara->callcode,
            http_reqpara->vocfile,
            http_reqpara->callflag,
            http_reqpara->c1,
            http_reqpara->c2,
            http_reqpara->i1,
            http_reqpara->i2,
            http_reqpara->channel);
}

int http_reqpara_get(char *parabuf, const char *buf)
{
    const char *ps = buf;
    const char *pe = NULL;

    ps = strchr(ps, '?');
    if (ps == NULL) {
        xapplog(LOG_ERR, APP_LOG_MASK_BASE, "Not find the '?'.");
        return -1;
    }
    ps++;

    pe = strchr(ps, ' ');
    if (pe == NULL) {
        xapplog(LOG_ERR, APP_LOG_MASK_BASE, "Not find the ' ' after ps.");
        return -1;
    }


    int len = pe - ps;
    bcopy(ps, parabuf, len);
    parabuf[len] = 0;

    return 0;
}
