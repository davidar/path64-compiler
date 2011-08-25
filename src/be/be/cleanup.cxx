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
 * Module: cleanup.c
 * $Revision$
 * $Date$
 * $Author$
 * $Source$
 *
 * Revision history:
 *  21-Feb-95 - Original Version
 *
 * Description:
 *  Clean up functions for the be driver.
 *
 * ====================================================================
 * ====================================================================
 */

#include "elf_defines.h"
#include <sys/types.h>              /* for off_t */

#include "defs.h"
#include "glob.h"		    /* for Src_File_Name, etc. */
#include "erglob.h"		    /* for EC_Src_Close, etc. */
#include "erlib.h"		    /* for EC_Obj_Close, etc. */
#include "be_errors.h"		/* Set_Error_Line(), etc. */
#include "file_util.h"		    /* for unlink() */
#include "tracing.h"		    /* Set_Trace_File() */
#include "symtab.h"		    /* for wn.h */
#include "wn.h"			    /* for ir_bread.h */
#include "pu_info.h"		    /* for ir_bread.h */
#include "ir_bread.h"		    /* Free_Input_Info () */
#include "anl_driver.h"		    /* Prompf related */
#include "w2c_driver.h"		    /* Whirl2c related */
#include "w2f_driver.h"		    /* Whirl2f related */
#include "instr_reader.h"
#include "be_symtab.h"


BOOL Prompf_anl_loaded = FALSE;
BOOL Purple_loaded = FALSE;
BOOL Whirl2f_loaded = FALSE;
BOOL Whirl2c_loaded = FALSE;


/* ====================================================================
 *
 * Cleanup_Files
 *
 * Close all per-source	files involved in a compilation	and prepare the
 * global variables for	the next source.  This routine is externalized
 * for signal cleanup; the report parameter allows suppressing of error
 * reporting during such cleanup.
 *
 * ====================================================================
 */

void
Cleanup_Files (BOOL report,         /* Report errors during cleanup? */
               BOOL delete_dotofile /* remove .o file ? */)
{
    /* No current line number for errors: */
    Set_Error_Line (ERROR_LINE_UNKNOWN);

    /* Close source file: */
    if ( Src_File != NULL && Src_File != stdin && fclose (Src_File) ) {
	if ( report )
	    ErrMsg ( EC_Src_Close, Src_File_Name, errno );
    }
    Src_File = NULL;

    /* Release all memory used for reading the WHIRL file. */
    Free_Input_Info ();

    /* Close listing file: */
    if ( Lst_File != NULL && Lst_File != stdout && fclose (Lst_File) ) {
	if ( report )
	    ErrMsg ( EC_Lst_Close, Lst_File_Name, errno );
    }
    Lst_File = NULL;

    /* Close transformation log file: */
    if ( Tlog_File != NULL && Tlog_File != stdout && fclose (Tlog_File) ) {
	if ( report )
	    ErrMsg ( EC_Tlog_Close, Tlog_File_Name, errno );
    }
    Lst_File = NULL;

    if (Whirl2c_loaded)
       W2C_Cleanup();
    if (Whirl2f_loaded)
       W2F_Cleanup();
#ifdef USE_WEAK_REFERENCES
    if (Prompf_anl_loaded)
       Anl_Cleanup();
#endif

    /* Close trace file: */
    Set_Trace_File ( NULL );

    /* Close Feedback File */
    Close_Feedback_Files();

    /* Disable timing file: */
    Tim_File = NULL;

    /* Finally close error file: */
    Set_Error_File ( NULL );
    Set_Error_Source ( NULL );
} /* Cleanup_Files */


/* ====================================================================
 *
 * Terminate
 *
 * Do any necessary cleanup and	terminate the program with the given
 * status.
 *
 * ====================================================================
 */

void
Terminate (INT status)
{
    /* Close and delete files as necessary: */
    Cleanup_Files ( FALSE, TRUE);

    exit (status);
} /* Terminate */

// do early termination, not a normal clean ending.
void
Early_Terminate (INT status)
{
    // In case are doing early exit, cleanup related-symtab tables.
    // ??? Is apparently some ordering problem with the global destructors
    // ??? that can cause segfaults if we don't first manually clear these.
    if (CURRENT_SYMTAB > GLOBAL_SYMTAB && Scope_tab[CURRENT_SYMTAB].preg_tab != NULL) {
	Scope_tab[CURRENT_SYMTAB].preg_tab->Un_register(Be_preg_tab);
	Be_preg_tab.Clear();
    }
    for (SYMTAB_IDX scope_level = CURRENT_SYMTAB; 
	scope_level >= GLOBAL_SYMTAB; 
	--scope_level) 
    {
    	if (Scope_tab[scope_level].st_tab != NULL
	  && Be_scope_tab[scope_level].be_st_tab != NULL)
	{
		Scope_tab[scope_level].st_tab->
			Un_register(*Be_scope_tab[scope_level].be_st_tab);
		Be_scope_tab[scope_level].be_st_tab->Clear();
	}
    }

    Terminate(status);
}
