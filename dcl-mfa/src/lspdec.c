/* ITU-T G.729 Software Package Release 2 (November 2006) */
/* ITU-T G.729 - Reference C code for fixed point implementation */
/* Version 3.3    Last modified: December 26, 1995 */

#include "typedef.h"
#include "ld8k.h"
#include "basic_op.h"
#include "tab_ld8k.h"

/*----------------------------------------------------------------------------
 * Lsp_decw_reset -   set the previous LSP vectors
 *----------------------------------------------------------------------------
 */
void Lsp_decw_reset(G729DECODER * pgd)
{
  Word16 i;

  for(i=0; i<MA_NP; i++)
    g729_Copy( &pgd->freq_prev_reset[0], &pgd->freq_prev[i][0], M );

  pgd->prev_ma = 0;

  g729_Copy( pgd->freq_prev_reset, pgd->prev_lsp, M);
}



/*----------------------------------------------------------------------------
 * Lsp_iqua_cs -  LSP main quantization routine
 *----------------------------------------------------------------------------
 */
void Lsp_iqua_cs(G729DECODER * pgd,
 Word16 prm[],          /* (i)     : indexes of the selected LSP */
 Word16 lsp_q[],        /* (o) Q13 : Quantized LSP parameters    */
 Word16 erase           /* (i)     : frame erase information     */
)
{
  Word16 mode_index;
  Word16 code0;
  Word16 code1;
  Word16 code2;
  Word16 buf[M];     /* Q13 */

  if( erase==0 ) {  /* Not frame erasure */
    mode_index = shr(pgd, prm[0] ,NC0_B) & (Word16)1;
    code0 = prm[0] & (Word16)(NC0 - 1);
    code1 = shr(pgd, prm[1] ,NC1_B) & (Word16)(NC1 - 1);
    code2 = prm[1] & (Word16)(NC1 - 1);

    /* compose quantized LSP (lsp_q) from indexes */
    Lsp_get_quant(pgd, lspcb1, lspcb2, code0, code1, code2,
      fg[mode_index], pgd->freq_prev, lsp_q, fg_sum[mode_index]);

    /* save parameters to use in case of the frame erased situation */
    g729_Copy(lsp_q, pgd->prev_lsp, M);
    pgd->prev_ma = mode_index;
  }
  else {           /* Frame erased */
    /* use revious LSP */
    g729_Copy(pgd->prev_lsp, lsp_q, M);

    /* update freq_prev */
    Lsp_prev_extract(pgd, pgd->prev_lsp, buf,
      fg[pgd->prev_ma], pgd->freq_prev, fg_sum_inv[pgd->prev_ma]);
    Lsp_prev_update(buf, pgd->freq_prev);
  }

  return;
}



/*-------------------------------------------------------------------*
 * Function  D_lsp:                                                  *
 *           ~~~~~~                                                  *
 *-------------------------------------------------------------------*/

void D_lsp(G729DECODER * pgd,
  Word16 prm[],          /* (i)     : indexes of the selected LSP */
  Word16 lsp_q[],        /* (o) Q15 : Quantized LSP parameters    */
  Word16 erase           /* (i)     : frame erase information     */
)
{
  Word16 lsf_q[M];       /* domain 0.0<= lsf_q <PI in Q13 */


  Lsp_iqua_cs(pgd, prm, lsf_q, erase);

  /* Convert LSFs to LSPs */
  Lsf_lsp2(pgd, lsf_q, lsp_q, M);

  return;
}
