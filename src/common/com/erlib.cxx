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
 * Module: erlib.cxx
 * $Revision$
 * $Date$
 * $Author$
 * $Source$
 *
 * Revision history:
 *  08-Sep-89 - Original Version
 *  24-Jan-91 - Copied for TP/Muse
 *
 * Description:
 *
 * Define the program librarian error message descriptors for use with
 * the error message handler errors.c.  The associated error codes are
 * defined in the file erlib.h.
 *
 * ====================================================================
 * ====================================================================
 */

#include "erlib.h"

ERROR_DESC EDESC_Lib[] = {
/* File manipulation error codes: */
  { EC_Lib_Exists,	EM_User | ES_ERRPHASE,	RAG_EN_NONE,
    "Program library file (%s) already exists",
    1, ET_STRING, 0,0,0,0,0 },
  { EC_Lib_Open,	EM_User | ES_ERRPHASE,	RAG_EN_NONE,
    "Can't open program library file (%s): %s",
    2, ET_STRING, ET_SYSERR, 0,0,0,0 },
  { EC_Lib_Create,	EM_User | ES_ERRPHASE,	RAG_EN_NONE,
    "Can't create program library file (%s): %s",
    2, ET_STRING, ET_SYSERR, 0,0,0,0 },
  { EC_Lib_Delete,	EM_User | ES_ERRPHASE,	RAG_EN_NONE,
    "Can't delete program library file (%s): %s",
    2, ET_STRING, ET_SYSERR, 0,0,0,0 },
  { EC_Lib_Close,	EM_User | ES_ERRPHASE,	RAG_EN_NONE,
    "Closing program library file (%s): %s",
    2, ET_STRING, ET_SYSERR, 0,0,0,0 },
  { EC_No_Lib,		EM_User | ES_ERRPHASE,	RAG_EN_NONE,
    "Program library file (%s) does not exist",
    1, ET_STRING, 0,0,0,0,0 },
  { EC_Obj_Exists,	EM_User | ES_ERRABORT,	RAG_EN_NONE,
    "Relocatable object file (%s) already exists",
    1, ET_STRING, 0,0,0,0,0 },
  { EC_Obj_Open,	EM_User | ES_ERRABORT,	RAG_EN_NONE,
    "Can't open relocatable object file (%s): %s",
    2, ET_STRING, ET_SYSERR, 0,0,0,0 },
  { EC_Obj_Create,	EM_User | ES_ERRABORT,	RAG_EN_NONE,
    "Can't create relocatable object file (%s): %s",
    2, ET_STRING, ET_SYSERR, 0,0,0,0 },
  { EC_Obj_Delete,	EM_User | ES_ERRABORT,	RAG_EN_NONE,
    "Can't delete relocatable object file (%s): %s",
    2, ET_STRING, ET_SYSERR, 0,0,0,0 },
  { EC_Lib_Close,	EM_User | ES_ERRABORT,	RAG_EN_NONE,
    "Closing relocatable object file (%s): %s",
    2, ET_STRING, ET_SYSERR, 0,0,0,0 },
  { EC_Invalidated,	EM_User | ES_WARNING,	RAG_EN_NONE,
    "Invalidated program unit %s",
    1, ET_STRING, 0,0,0,0,0 },
  { EC_No_Obj,		EM_User | ES_ERRABORT,	RAG_EN_NONE,
    "Relocatable object file (%s) does not exist",
    1, ET_STRING, 0,0,0,0,0 },
  { EC_No_Obj,		EM_User | ES_ERRABORT,	RAG_EN_NONE,
    "Relocatable object file (%s) does not exist",
    1, ET_STRING, 0,0,0,0,0 },
  { EC_Lib_Missing_Body,	EM_User | ES_ERRPHASE,	RAG_EN_NONE,
    "Body for %s does not exist",
    1, ET_STRING, 0,0,0,0,0 },
  { EC_Lib_Invalid_PU,		EM_User | ES_ERRPHASE,	RAG_EN_NONE,
    "Program unit %s is invalid",
    1, ET_STRING, 0,0,0,0,0 },
  { EC_Lib_Version,		EM_User | ES_ERRPHASE,	RAG_EN_NONE,
    "Program library is out of date (%d)",
    1, ET_INT, 0,0,0,0,0 },
  { EC_Lib_Invalidating_File,	EM_User | ES_ADVISORY,	RAG_EN_NONE,
    "Invalidating program unit(s) in %s",
    1, ET_STRING, 0,0,0,0,0 },

/* Interprocedural analysis: */

/* Sparse bit vector manipulation: */
  { EC_SBV_Ill_Elmt,	EM_User | ES_ERRPHASE,	RAG_EN_NONE,
    "Illegal sparse bit vector index %d (bound is %d)",
    2, ET_INT, ET_INT, 0,0,0,0 },
  
  /* All error descriptor lists must end with a -1 error code: */
  { -1,	0, RAG_EN_NONE, "", 0, 0,0,0,0,0,0 },
};

