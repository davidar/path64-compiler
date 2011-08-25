
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

/* ====================================================================
 * ====================================================================
 *
 * Module: ercg.cxx
 * $Revision$
 * $Date$
 * $Author$
 * $Source$
 *
 *
 * Description:
 *
 * Define the code generator error message descriptors for use with
 * the error message handler errors.c.  The associated error codes are
 * defined in the file ercg.h.
 *
 * ====================================================================
 * ====================================================================
 */

#include "ercg.h"

ERROR_DESC EDESC_CG[] = {
  
  /* Scheduling preparation: */
  { EC_Ill_Cycle,	EM_Compiler | ES_ERRPHASE,	RAG_EN_NONE,
    "Illegal cycle kind (%d) in %s",
    2, ET_INT, ET_STRING, 0,0,0,0 },

  /* Register Allocation: */
  { EC_Ill_Reg_Spill1,	EM_Compiler | ES_ERRPHASE,	RAG_EN_NONE,
    "Attempted to store register %s illegally",
    1, ET_STRING, 0,0,0,0,0 },
  { EC_Ill_Reg_Spill2b,	
    EM_Continuation | EM_Compiler | ES_ERRABORT,	RAG_EN_NONE,
    "Try using -O%d",
    1, ET_INT, 0,0,0,0,0 },
  { EC_ASM_Bad_Operand,	EM_User | ES_ERRPHASE, RAG_EN_NONE,
    "asm statement on line %d uses bad operand number (%d). (%s)",
    3, ET_INT, ET_INT, ET_STRING, 0,0,0 },

  /* All error descriptor lists must end with a -1 error code: */
  { -1,	0, RAG_EN_NONE, "", 0, 0,0,0,0,0,0 }
};
