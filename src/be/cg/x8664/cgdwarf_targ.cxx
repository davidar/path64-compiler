/*
 * Copyright (C) 2007, 2008. PathScale, LLC. All Rights Reserved.
 */
/*
 * Copyright 2003, 2004, 2005, 2006 PathScale, Inc.  All Rights Reserved.
 */

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


#include <stdio.h>
#include <stdlib.h>
#include "libelf/libelf.h"
#include <list>
#include "elf_defines.h"
#include <elfaccess.h>

#include "defs.h"
#include "erglob.h"
#include "glob.h"
#include "flags.h"
#include "tracing.h"
#include "config.h"
#include "config_asm.h"
#include "be_util.h"
#include "cgir.h"
#include "register.h"
#include "tn_map.h"
#include "em_elf.h"
#include "em_dwarf.h"
#include "cgtarget.h"
#include "calls.h"
#include "cgemit.h"
#include "data_layout.h"
#include "cgdwarf_targ.h"

// call per-PU
void
Init_Unwind_Info (BOOL trace)
{
  return;
}

void
Emit_Unwind_Directives_For_OP(OP *op, FILE *f)
{
  return;
}

void
Finalize_Unwind_Info(void)
{
  return;
}

/* construct the fde for the current procedure. */
extern void
Build_Fde_For_Proc (FILE *asm_file, Dwarf_P_Debug dw_dbg, BB *firstbb,
		    INT32     end_offset,
		    // The following two arguments need to go away
		    // once libunwind gives us an interface that
		    // supports symbolic ranges.
		    INT       low_pc,
		    INT       high_pc)
{
  Dwarf_Error dw_error;

  if ( ! CG_emit_unwind_info) return;

  fprintf(asm_file, ".cfi_def_cfa_offset 16\n");
  fprintf(asm_file, ".cfi_offset %u, %i\n", Is_Target_64bit() ? 6 : 5, 2*data_alignment_factor);
  fprintf(asm_file, ".cfi_def_cfa_register %u\n", Is_Target_64bit() ? 6 : 5);
  if (Cgdwarf_Num_Callee_Saved_Regs()) {
    INT num = Cgdwarf_Num_Callee_Saved_Regs();    
    for (INT i = num - 1; i >= 0; i --) {
      TN* tn = Cgdwarf_Nth_Callee_Saved_Reg(i);
      ST* sym = Cgdwarf_Nth_Callee_Saved_Reg_Location(i);
      INT n = Is_Target_64bit() ? 16 : 8;
      mUINT8 reg_id = REGISTER_machine_id (TN_register_class(tn), TN_register(tn));
      // If we need the DWARF register id's for all registers, we need a 
      // general register mapping from REGISTER_machine_id to DWARF register
      // id. But the following suffices for this case,
      // The machine_id is the same as the DWARF id for all callee-saved 
      // registers except rbx, so give it the proper id here.
      //
      // And for -m32, handle the 2 additional callee-saved registers
      if (Is_Target_32bit())
      {
      	if (reg_id == 5) // %esi
	  reg_id = 6;
        else if (reg_id == 4) // %edi
	  reg_id = 7;
      }
      if (reg_id == 1) reg_id = 3; // %rbx
      fprintf(asm_file, ".cfi_offset %u, %i\n", reg_id,
              ((ST_base(sym) == FP_Sym ? -1 : 1)*ST_ofst(sym)+n));
    }
  }
}


void
Check_Dwarf_Rel(const Elf32_Rel &current_reloc)
{
  FmtAssert(REL32_type(current_reloc) == R_IA_64_DIR32MSB,
	    ("Unimplemented 32-bit relocation type %d",
	     REL32_type(current_reloc)));
}

void
Check_Dwarf_Rel(const Elf64_AltRel &current_reloc)
{
  FmtAssert(REL64_type(current_reloc) == R_IA_64_DIR64MSB,
	    ("Unimplemented 64-bit relocation type %d",
	     REL64_type(current_reloc)));
}

void
Check_Dwarf_Rela(const Elf64_AltRela &current_reloc)
{
  FmtAssert(FALSE,
	    ("Unimplemented 64-bit relocation type %d",
	     REL64_type(current_reloc)));
}

void
Check_Dwarf_Rela(const Elf32_Rela &current_reloc)
{
  FmtAssert(FALSE,
	    ("Unimplemented 32-bit relocation type %d",
	     REL32_type(current_reloc)));
}
static const char *drop_these[] = {
#if ! defined(BUILD_OS_DARWIN)
      // Assembler generates .debug_line from directives itself, so we
      // don't output it.
	DEBUG_LINE_SECTNAME,
#endif /* defined(BUILD_OS_DARWIN) */
     // gdb does not use the MIPS sections
     // debug_weaknames, etc.
	DEBUG_VARNAMES_SECTNAME,
	DEBUG_TYPENAMES_SECTNAME,
	DEBUG_WEAKNAMES_SECTNAME,
	DEBUG_FUNCNAMES_SECTNAME,
     // we don't use debug_frame in IA-64.
	0
};
// return TRUE if we want to emit the section (IA-64).
// return FALSE if we do not want to for IA-64.
extern BOOL Is_Dwarf_Section_To_Emit(const char *name)
{

	for(int  i = 0; drop_these[i]; ++i) {
	  if(strcmp(name,drop_these[i]) == 0) {
	    return FALSE;
	  }
	}
        // Bug 1516 - do not emit .debug_* sections if not -g
	if (Debug_Level < 1 &&
#if defined(BUILD_OS_DARWIN)
	is_debug_section(name)
#else /* defined(BUILD_OS_DARWIN) */
	strncmp(name, ".debug_", 7) == 0
#endif /* defined(BUILD_OS_DARWIN) */
	)
	  return FALSE;
        return TRUE;
}

