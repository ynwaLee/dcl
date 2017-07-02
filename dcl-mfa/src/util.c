/* ITU-T G.729 Software Package Release 2 (November 2006) */
/* ITU-T G.729 - Reference C code for fixed point implementation */
/* Version 3.3    Last modified: December 26, 1995 */

/*-------------------------------------------------------------------*
 * Function  Set zero()                                              *
 *           ~~~~~~~~~~                                              *
 * Set vector x[] to zero                                            *
 *-------------------------------------------------------------------*/

#include "typedef.h"
#include "basic_op.h"
#include "ld8k.h"

void g729_Set_zero(
  Word16 x[],       /* (o)    : vector to clear     */
  Word16 L          /* (i)    : length of vector    */
)
{
   Word16 i;

   for (i = 0; i < L; i++)
     x[i] = 0;

   return;
}
/*-------------------------------------------------------------------*
 * Function  g729_Copy:                                                   *
 *           ~~~~~                                                   *
 * g729_Copy vector x[] to y[]                                            *
 *-------------------------------------------------------------------*/

void g729_Copy(
  Word16 x[],      /* (i)   : input vector   */
  Word16 y[],      /* (o)   : output vector  */
  Word16 L         /* (i)   : vector length  */
)
{
   Word16 i;

   for (i = 0; i < L; i++)
     y[i] = x[i];

   return;
}

/* Random generator  */

Word16 Random(G729DECODER * pgd)
{
  

  /* seed = seed*31821 + 13849; */
  pgd->seed = g729_extract_l(L_add(pgd, L_shr(pgd, L_mult(pgd, pgd->seed, 31821), 1), 13849L));

  return(pgd->seed);
}
