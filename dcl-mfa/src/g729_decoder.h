
#ifndef _G729_DECODE_H_
#define _G729_DECODE_H_

#define PCM_BUF_LEN        80

typedef struct
{
    short  Az_dec[11*2];       /* Decoded Az for post-filter  */
    short  T0_first;                     /* Pitch lag in 1st subframe   */
    short  voicing;                      /* voicing from previous frame */

    short old_exc[80+143+11];
    short *exc;
    short lsp_old[10];
    short mem_syn[10];
    short sharp;           /* pitch sharpening of previous frame */
    short old_T0;          /* integer delay of previous frame    */
    short gain_code;       /* Code gain                          */
    short gain_pitch;      /* Pitch gain                         */

    short freq_prev[4][10];   /* Q13 */
    short freq_prev_reset[10];
    short prev_ma;                  /* previous MA prediction coef.*/
    short prev_lsp[10];           /* previous LSP vector */

    short Overflow;
    short Carry;
    short past_qua_en[4];
    short seed;
    short pcm[PCM_BUF_LEN];
} G729DECODER;

G729DECODER * G729DecInit();
void G729DecClose(G729DECODER * pgd);
short * G729Decode(G729DECODER * pgd, unsigned char * g729);

#endif
