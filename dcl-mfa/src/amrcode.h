#ifndef __AMRCODE_H__
#define __AMRCODE_H__
#include "interf_dec.h"
#include "sp_dec.h"
#include "typedef.h"


#define AMR_CODE_NB  0x00
#define AMR_CODE_WB  0x10

#define AMR_CODE_NBSC  0x00
#define AMR_CODE_NBMC  0x01
#define AMR_CODE_WBSC  0x10
#define AMR_CODE_WBMC  0x11

/* 
 * narrow band
 * 
 */

#define MAX_AMRNB_CODE  8
#define AMRNB_SC_FILE_HEADER "#!AMR\n"
#define G729_FILE_HEADER "#!G729_MC1.0\n"

#define AMRNB_MC_FILE_HEADER "#!AMR_MC1.0\n"
typedef struct amrnb_code_s {
    int id;
    char name[12];
    double bps;
    int frame_size;
    int frame_header;
}amrnb_code_t;
extern amrnb_code_t amrnb_code[MAX_AMRNB_CODE];
extern int amrnb_code_index(int frameheader);
/* 
 * wide band
 * 
 */
#define AMRWB_SC_FILE_HEADER "#!AMR-WB\n"


/*
 * common function
 * */
 int g7292wave(int g729fd, int wavefd,unsigned int filesize);
extern inline int bps2framesize(double bps);
extern int amr2wave(int armfd, int wavefd, int amrcode);
#endif
