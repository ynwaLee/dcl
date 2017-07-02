/* ITU-T G.729 Software Package Release 2 (November 2006) */
/* ITU-T G.729 - Reference C code for fixed point implementation */
/* Version 3.3    Last modified: December 26, 1995 */

/*
   ITU-T G.729 Speech Coder     ANSI-C Source Code
   Copyright (c) 1995, AT&T, France Telecom, NTT, Universite de Sherbrooke.
   All rights reserved.
*/

/*-----------------------------------------------------------------*
 * Main program of the ITU-T G.729  8 kbit/s decoder.              *
 *                                                                 *
 *    Usage : decoder  bitstream_file  synth_file                  *
 *                                                                 *
 *-----------------------------------------------------------------*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "typedef.h"
#include "basic_op.h"
#include "ld8k.h"
#include "tab_ld8k.h"
#include "g729_decoder.h"


G729DECODER * G729DecInit()
{
    G729DECODER * pgd = (G729DECODER *)malloc(sizeof(G729DECODER));
    memset(pgd, 0, sizeof(G729DECODER));
    pgd->voicing = 60;
    pgd->lsp_old[0] = 30000;
    pgd->lsp_old[1] = 26000;
    pgd->lsp_old[2] = 21000;
    pgd->lsp_old[3] = 15000;
    pgd->lsp_old[4] = 8000;
    pgd->lsp_old[5] = 0;
    pgd->lsp_old[6] = -8000;
    pgd->lsp_old[7] = -15000;
    pgd->lsp_old[8] = -21000;
    pgd->lsp_old[9] = -26000;

    pgd->freq_prev_reset[0] = 2339;
    pgd->freq_prev_reset[1] = 4679;
    pgd->freq_prev_reset[2] = 7018;
    pgd->freq_prev_reset[3] = 9358;
    pgd->freq_prev_reset[4] = 11698;
    pgd->freq_prev_reset[5] = 14037;
    pgd->freq_prev_reset[6] = 16377;
    pgd->freq_prev_reset[7] = 18717;
    pgd->freq_prev_reset[8] = 21056;
    pgd->freq_prev_reset[9] = 23396;

    pgd->past_qua_en[0] = -14336;
    pgd->past_qua_en[1] = -14336;
    pgd->past_qua_en[2] = -14336;
    pgd->past_qua_en[3] = -14336;
    pgd->seed = 21845;

    Init_Decod_ld8k(pgd);
//    Init_Post_Process(pgd);
    return pgd;
}

void G729DecClose(G729DECODER * pgd)
{
    free(pgd);
}

unsigned int g729_getbits(unsigned char * pkt, int len, int startbit, int bitlen)
{
    if(startbit+bitlen > len*8)
        return 0;
    int j;
    unsigned int num = 0;
    for(j=0; j<bitlen; j++)
    {
        num *= 2;
        num += (pkt[startbit/8]>>(8-startbit%8-1)) & 0x01;
        startbit += 1;
    }
    return num;
}

void g729_decode(G729DECODER * pgd, unsigned char * g729, short * pcm)
{
    Word16  parm[PRM_SIZE+1];

    int i, startbit = 0;
    for(i=0; i<PRM_SIZE; i++)
    {
        parm[i+1] = g729_getbits(g729, 1024, startbit, bitsno[i]);
        startbit += bitsno[i];
    }
    parm[0] = 0;           /* No frame erasure */
    parm[4] = Check_Parity_Pitch(pgd, parm[3], parm[4]);

    Decod_ld8k(pgd, parm, pgd->voicing, pcm, pgd->Az_dec, &pgd->T0_first);
}

short * G729Decode(G729DECODER * pgd, unsigned char * g729)
{
    g729_decode(pgd, g729, pgd->pcm);
    //g729_decode(pgd, g729+10, pgd->pcm+80);
    return pgd->pcm;
}
