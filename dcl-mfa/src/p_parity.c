/* ITU-T G.729 Software Package Release 2 (November 2006) */
/* ITU-T G.729 - Reference C code for fixed point implementation */
/* Version 3.3    Last modified: December 26, 1995 */

/*------------------------------------------------------*
 * Parity_pitch - compute parity bit for first 6 MSBs   *
 *------------------------------------------------------*/

#include "typedef.h"
#include "basic_op.h"
#include "ld8k.h"

Word16 Parity_Pitch(G729DECODER * pgd,    /* output: parity bit (XOR of 6 MSB bits)    */
   Word16 pitch_index   /* input : index for which parity to compute */
)
{
  Word16 temp, sum, i, bit;

  temp = shr(pgd, pitch_index, 1);

  sum = 1;
  for (i = 0; i <= 5; i++) {
    temp = shr(pgd, temp, 1);
    bit = temp & (Word16)1;
    sum = add(pgd,sum, bit);
  }
  sum = sum & (Word16)1;


  return sum;
}

/*--------------------------------------------------------------------*
 * check_parity_pitch - check parity of index with transmitted parity *
 *--------------------------------------------------------------------*/

Word16  Check_Parity_Pitch(G729DECODER * pgd, /* output: 0 = no error, 1= error */
  Word16 pitch_index,       /* input : index of parameter     */
  Word16 parity             /* input : parity bit             */
)
{
  Word16 temp, sum, i, bit;

  temp = shr(pgd, pitch_index, 1);

  sum = 1;
  for (i = 0; i <= 5; i++) {
    temp = shr(pgd, temp, 1);
    bit = temp & (Word16)1;
    sum = add(pgd,sum, bit);
  }
  sum = add(pgd,sum, parity);
  sum = sum & (Word16)1;

  return sum;
}
