/* ITU-T G.729 Software Package Release 2 (November 2006) */
/* ITU-T G.729 - Reference C code for fixed point implementation */
/* Version 3.3    Last modified: December 26, 1995 */

/*-----------------------------------------------------------*
 *  Function  Decod_ACELP()                                  *
 *  ~~~~~~~~~~~~~~~~~~~~~~~                                  *
 *   Algebraic codebook decoder.                             *
 *----------------------------------------------------------*/

#include "typedef.h"
#include "basic_op.h"
#include "ld8k.h"

void Decod_ACELP(G729DECODER * pgd,
  Word16 sign,      /* (i)     : signs of 4 pulses.                       */
  Word16 index,     /* (i)     : Positions of the 4 pulses.               */
  Word16 cod[]      /* (o) Q13 : algebraic (fixed) codebook excitation    */
)
{
  Word16 i, j;
  Word16 pos[4];


  /* Decode the positions */

  i      = index & (Word16)7;
  pos[0] = add(pgd, i, shl(pgd, i, 2));           /* pos0 =i*5 */

  index  = shr(pgd, index, 3);
  i      = index & (Word16)7;
  i      = add(pgd, i, shl(pgd, i, 2));           /* pos1 =i*5+1 */
  pos[1] = add(pgd, i, 1);

  index  = shr(pgd, index, 3);
  i      = index & (Word16)7;
  i      = add(pgd,i, shl(pgd, i, 2));           /* pos2 =i*5+1 */
  pos[2] = add(pgd,i, 2);

  index  = shr(pgd, index, 3);
  j      = index & (Word16)1;
  index  = shr(pgd, index, 1);
  i      = index & (Word16)7;
  i      = add(pgd,i, shl(pgd, i, 2));           /* pos3 =i*5+3+j */
  i      = add(pgd,i, 3);
  pos[3] = add(pgd,i, j);

  /* decode the signs  and build the codeword */

  for (i=0; i<L_SUBFR; i++) {
    cod[i] = 0;
  }

  for (j=0; j<4; j++)
  {

    i = sign & (Word16)1;
    sign = shr(pgd, sign, 1);

    if (i != 0) {
      cod[pos[j]] = 8191;      /* Q13 +1.0 */
    }
    else {
      cod[pos[j]] = -8192;     /* Q13 -1.0 */
    }
  }

  return;
}
