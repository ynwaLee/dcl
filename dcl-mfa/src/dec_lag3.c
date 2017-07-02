/* ITU-T G.729 Software Package Release 2 (November 2006) */
/* ITU-T G.729 - Reference C code for fixed point implementation */
/* Version 3.3    Last modified: December 26, 1995 */

/*------------------------------------------------------------------------*
 *    Function Dec_lag3                                                   *
 *             ~~~~~~~~                                                   *
 *   Decoding of fractional pitch lag with 1/3 resolution.                *
 * See "Enc_lag3.c" for more details about the encoding procedure.        *
 *------------------------------------------------------------------------*/

#include "typedef.h"
#include "basic_op.h"
#include "ld8k.h"

void Dec_lag3(G729DECODER * pgd, 
  Word16 index,       /* input : received pitch index           */
  Word16 pit_min,     /* input : minimum pitch lag              */
  Word16 pit_max,     /* input : maximum pitch lag              */
  Word16 i_subfr,     /* input : subframe flag                  */
  Word16 *T0,         /* output: integer part of pitch lag      */
  Word16 *T0_frac     /* output: fractional part of pitch lag   */
)
{
  Word16 i;
  Word16 T0_min, T0_max;

  if (i_subfr == 0)                  /* if 1st subframe */
  {
    if (sub(pgd,index, 197) < 0)
    {
      /* *T0 = (index+2)/3 + 19 */

      *T0 = add(pgd,mult(pgd,add(pgd,index, 2), 10923), 19);

      /* *T0_frac = index - *T0*3 + 58 */

      i = add(pgd,add(pgd,*T0, *T0), *T0);
      *T0_frac = add(pgd,sub(pgd,index, i), 58);
    }
    else
    {
      *T0 = sub(pgd,index, 112);
      *T0_frac = 0;
    }

  }

  else  /* second subframe */
  {
    /* find T0_min and T0_max for 2nd subframe */

    T0_min = sub(pgd,*T0, 5);
    if (sub(pgd,T0_min, pit_min) < 0)
    {
      T0_min = pit_min;
    }

    T0_max = add(pgd,T0_min, 9);
    if (sub(pgd,T0_max, pit_max) > 0)
    {
      T0_max = pit_max;
      T0_min = sub(pgd,T0_max, 9);
    }

    /* i = (index+2)/3 - 1 */
    /* *T0 = i + t0_min;    */

    i = sub(pgd,mult(pgd,add(pgd,index, 2), 10923), 1);
    *T0 = add(pgd,i, T0_min);

    /* t0_frac = index - 2 - i*3; */

    i = add(pgd,add(pgd,i, i), i);
    *T0_frac = sub(pgd,sub(pgd,index, 2), i);
  }

  return;
}

