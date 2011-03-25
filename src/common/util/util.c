/*

  Copyright (C) 2000, 2001 Silicon Graphics, Inc.  All Rights Reserved.

   Path64 is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   Path64 is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with Path64; see the file COPYING.  If not, write to the Free
   Software Foundation, 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

   Special thanks goes to SGI for their continued support to open source

*/



static char *source_file = __FILE__;
static char *rcs_id = "$Source$ $Revision$";
#include <unistd.h>
#include <errno.h>
#include "defs.h"
#include "erglob.h"
#include "tracing.h"
#include "util.h"

extern pid_t wait(INT *statptr);	/* Currently not defined with
					 *  prototype in sys/wait.h
                                         */



/* ====================================================================
 *
 * Check_Range
 *
 * Check an integer value against bounds.  Return the value if in
 * range, and a default if out of range.
 *
 * ====================================================================
 */

INT
Check_Range (
  INT val,	/* Check this value */
  INT lbound,	/* ... against this lower bound */
  INT ubound,	/* ... and this upper bound, */
  INT def	/* ... with this default. */
)
{
  if ( val >= lbound && val <= ubound ) return val;
  return def;
}

/* ====================================================================
 *
 *  Mod
 *
 *  Mathematically correct integer modulus function.  Unlike C's
 *  builtin remainder function, this correctly handles the case where
 *  one of the two arguments is negative.
 *
 *  Notice the use if INT as the argument and return type.  Should
 *  this have been INT64?
 *
 * ====================================================================
 */

INT Mod(
  INT i,
  INT j
)
{
  INT rem;

  if ( j == 0 )
    return i;

  rem = i % j;

  if ( rem == 0 )
    return 0;

  if ( (i < 0) != (j < 0) )
    return j + rem;
  else
    return rem;
}

/* ====================================================================
 *
 *  Pop_Count
 *
 *  Return the count of set bits in 'x'.  This could be a lot faster,
 *  but does it matter?
 *
 *  TODO: Better algorithm.
 *
 * ====================================================================
 */

/* Count of bits in all the one byte numbers.
 */
const mUINT8 UINT8_pop_count[256] = {
  0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4, 
  1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 
  1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 
  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
  1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 
  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
  3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 
  1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 
  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
  3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 
  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
  3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 
  3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 
  4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8  
};

TARG_INT
TARG_INT_Pop_Count(
  TARG_INT x
)
{
  INT i, result=0;

  /* Find the least significant byte in x that's not zero and look it
   * up in the above table.
   */
  for (i = sizeof(x) - 1; i >= 0; --i ) {
    unsigned char y = ((TARG_UINT) x) >> (((UINT) i) * 8);

    result += UINT8_pop_count[y];
  }
  return result;
}

/* ====================================================================
 *
 *  TARG_INT Most_Sig_One
 *
 *  Return the index of the most signicant bit in x, or -1 if x is 0.
 *  Bits are labeled [63 .. 0]
 *
 * ====================================================================
 */

/* Mapping from unsigned 8 bit integers to the index of their most
 * significant one.  Notice -1 for out of range 0.
 */
static const mUINT8 UINT8_most_sig_one [256] = {
  -1, 0,  1,  1,  2,  2,  2,  2,  3,  3,  3,  3,  3,  3,  3,  3,  
  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  
  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  
  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  
  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  
  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  
  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  
  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  
  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  
  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  
  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  
  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  
  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  
  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  
  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  
  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  
};


TARG_INT
TARG_INT_Most_Sig_One(
  TARG_INT x
)
{
  INT i;

  /* Find the least significant byte in x that's not zero and look it
   * up in the above table.
   */
  for (i = sizeof(x) - 1; i >= 0; --i ) {
    unsigned char y = ((TARG_UINT) x) >> (((UINT) i) * 8);

    if (y != 0 )
      return i*8 + UINT8_most_sig_one[y];
  }

  return (TARG_INT) -1;
}


/* ====================================================================
 *
 *  TARG_INT_Least_Sig_One
 *
 *  Return the index of the least signicant bit in x, or -1 if x is 0.
 *  Bits are labeled [63 .. 0]
 *
 * ====================================================================
 */

/* Mapping from 8 bit unsigned integers to the index of the first one
 * bit:
 */
const mUINT8 UINT8_least_sig_one [256] = {
  -1, 0,  1,  0,  2,  0,  1,  0,  3,  0,  1,  0,  2,  0,  1,  0,  
  4,  0,  1,  0,  2,  0,  1,  0,  3,  0,  1,  0,  2,  0,  1,  0,  
  5,  0,  1,  0,  2,  0,  1,  0,  3,  0,  1,  0,  2,  0,  1,  0,  
  4,  0,  1,  0,  2,  0,  1,  0,  3,  0,  1,  0,  2,  0,  1,  0,  
  6,  0,  1,  0,  2,  0,  1,  0,  3,  0,  1,  0,  2,  0,  1,  0,  
  4,  0,  1,  0,  2,  0,  1,  0,  3,  0,  1,  0,  2,  0,  1,  0,  
  5,  0,  1,  0,  2,  0,  1,  0,  3,  0,  1,  0,  2,  0,  1,  0,  
  4,  0,  1,  0,  2,  0,  1,  0,  3,  0,  1,  0,  2,  0,  1,  0,  
  7,  0,  1,  0,  2,  0,  1,  0,  3,  0,  1,  0,  2,  0,  1,  0,  
  4,  0,  1,  0,  2,  0,  1,  0,  3,  0,  1,  0,  2,  0,  1,  0,  
  5,  0,  1,  0,  2,  0,  1,  0,  3,  0,  1,  0,  2,  0,  1,  0,  
  4,  0,  1,  0,  2,  0,  1,  0,  3,  0,  1,  0,  2,  0,  1,  0,  
  6,  0,  1,  0,  2,  0,  1,  0,  3,  0,  1,  0,  2,  0,  1,  0,  
  4,  0,  1,  0,  2,  0,  1,  0,  3,  0,  1,  0,  2,  0,  1,  0,  
  5,  0,  1,  0,  2,  0,  1,  0,  3,  0,  1,  0,  2,  0,  1,  0,  
  4,  0,  1,  0,  2,  0,  1,  0,  3,  0,  1,  0,  2,  0,  1,  0,  
};


TARG_INT
TARG_INT_Least_Sig_One(
  TARG_INT x
)
{
  INT  i;

  /* Find the least significant byte in x that's not zero and look it
   * up in the above table.
   */
  for (i = 0; i < sizeof(x); ++i) {
    unsigned char y = ((TARG_UINT) x) >> (((UINT) i) * 8);

    if (y != 0)
      return i*8 + UINT8_least_sig_one[y];
  }

  return (TARG_INT) -1;
}


/* ====================================================================
 *
 * INT32 nearest_power_of two(INT32 n)
 *
 * find the next nearest power of 2
 * ex.
 *      nearest_power_of two(7) = 8
 *      nearest_power_of two(4) = 4
 *
 * ==================================================================== */
#define IS_POW2(n)              (((n) & ((n)-1))==0)

INT32 nearest_power_of_two(INT32 n)
{
  INT32 i;

  Is_True((n>0), ("nearest_power_of two() not defined for <=0"));

  if (IS_POW2(n))
  {
    return n;
  }
  for(i=0; (1<<i)<n; i++)
        ;
  return 1<<i;
}



/* ====================================================================
 *
 * BOOL Immediate_Has_All_Ones(INT64 imm, INT32 ub, INT32 lb)
 *
 * return TRUE if imm has all ones from lb to ub
 *
 * ==================================================================== */
BOOL Immediate_Has_All_Ones(INT64 imm, INT32 ub, INT32 lb)
{
  TARG_UINT	field= ~0;
  INT32		fieldsize= ub - lb + 1;

  Is_True((fieldsize>0), ("nonsensical ub,lb for immediate"));

 /* create mask of ones
  * cannonicalize immediate to start at bits zero
  */
  field >>=	(sizeof(TARG_INT)*8) - fieldsize;
  imm   =	(imm >> lb) & field;
  
  if (imm ^ field)
    return FALSE;

  return TRUE; 
}
