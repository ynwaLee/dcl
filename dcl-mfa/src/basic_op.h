/* Version 3.3    Last modified: December 26, 1995 */

/*___________________________________________________________________________
 |                                                                           |
 |   Constants and Globals                                                   |
 |___________________________________________________________________________|
*/
#include "g729_decoder.h"

#define MAX_32 (Word32)0x7fffffffL
#define MIN_32 (Word32)0x80000000L

#define MAX_16 (Word16)0x7fff
#define MIN_16 (Word16)0x8000


/*___________________________________________________________________________
 |                                                                           |
 |   Operators prototypes                                                    |
 |___________________________________________________________________________|
*/

Word16 sature(G729DECODER *,Word32 L_var1);             /* Limit to 16 bits,    1 */
Word16 add(G729DECODER * pgd, Word16 var1, Word16 var2);     /* Short add,           1 */
Word16 sub(G729DECODER * pgd, Word16 var1, Word16 var2);     /* Short sub,           1 */
Word16 g729_abs_s(Word16 var1);                /* Short abs,           1 */
Word16 shl(G729DECODER *,Word16 var1, Word16 var2);     /* Short shift left,    1 */
Word16 shr(G729DECODER *,Word16 var1, Word16 var2);     /* Short shift right,   1 */
Word16 mult(G729DECODER * pgd, Word16 var1, Word16 var2);    /* Short mult,          1 */
Word32 L_mult(G729DECODER *,Word16 var1, Word16 var2);  /* Long mult,           1 */
Word16 g729_negate(Word16 var1);               /* Short g729_negate,        1 */
Word16 g729_extract_h(Word32 L_var1);          /* Extract high,        1 */
Word16 g729_extract_l(Word32 L_var1);          /* Extract low,         1 */
Word16 g729_round(G729DECODER * pgd,Word32 L_var1);              /* Round,               1 */
Word32 L_mac(G729DECODER * pgd,Word32 L_var3, Word16 var1, Word16 var2); /* Mac,    1 */
Word32 L_msu(G729DECODER * pgd,Word32 L_var3, Word16 var1, Word16 var2); /* Msu,    1 */
Word32 L_macNs(G729DECODER * pgd,Word32 L_var3, Word16 var1, Word16 var2);/* Mac without sat, 1*/
Word32 L_msuNs(G729DECODER * pgd,Word32 L_var3, Word16 var1, Word16 var2);/* Msu without sat, 1*/

Word32 L_add(G729DECODER * pgd,Word32 L_var1, Word32 L_var2);   /* Long add,        2 */
Word32 L_sub(G729DECODER * pgd,Word32 L_var1, Word32 L_var2);   /* Long sub,        2 */
Word32 L_add_c(G729DECODER * pgd,Word32 L_var1, Word32 L_var2); /*Long add with c,  2 */
Word32 L_sub_c(G729DECODER * pgd,Word32 L_var1, Word32 L_var2); /*Long sub with c,  2 */
Word32 g729_L_negate(Word32 L_var1);               /* Long g729_negate,     2 */
Word16 mult_r(G729DECODER * pgd, Word16 var1, Word16 var2);  /* Mult with g729_round,     2 */
Word32 L_shl(G729DECODER * pgd,Word32 L_var1, Word16 var2); /* Long shift left,     2 */
Word32 L_shr(G729DECODER * pgd,Word32 L_var1, Word16 var2); /* Long shift right,    2 */
Word16 shr_r(G729DECODER * pgd,Word16 var1, Word16 var2);/* Shift right with g729_round, 2 */
Word16 mac_r(G729DECODER * pgd,Word32 L_var3, Word16 var1, Word16 var2);/* Mac with g729_rounding, 2*/
Word16 msu_r(G729DECODER * pgd,Word32 L_var3, Word16 var1, Word16 var2);/* Msu with g729_rounding, 2*/
Word32 g729_L_deposit_h(Word16 var1);       /* 16 bit var1 -> MSB,     2 */
Word32 g729_L_deposit_l(Word16 var1);       /* 16 bit var1 -> LSB,     2 */

Word32 L_shr_r(G729DECODER * pgd,Word32 L_var1, Word16 var2);/* Long shift right with g729_round,  3*/
Word32 g729_L_abs(Word32 L_var1);            /* Long abs,              3 */

Word32 L_sat(G729DECODER * pgd,Word32 L_var1);            /* Long saturation,       4 */

Word16 g729_norm_s(Word16 var1);             /* Short norm,           15 */

Word16 div_s(G729DECODER * pgd,Word16 var1, Word16 var2); /* Short division,       18 */

Word16 g729_norm_l(Word32 L_var1);           /* Long norm,            30 */

