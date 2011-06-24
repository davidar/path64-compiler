/*
 *  Copyright (C) 2008 PathScale, LLC.  All Rights Reserved.
 */

/*
 *  Copyright (C) 2007. QLogic Corporation. All Rights Reserved.
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



#ifndef cgdwarf_INCLUDED
#define cgdwarf_INCLUDED

#ifdef KEY
#include "elf_defines.h"
#include "em_elf.h"
#endif

#if ! defined(BUILD_OS_DARWIN)
#include "libelf/libelf.h"
#endif /* ! defined(BUILD_OS_DARWIN) */
#include "libdwarf.h"
#include "dwarf_DST_mem.h"

extern void Cg_Dwarf_Begin (BOOL is_64bit);

extern void Cg_Dwarf_Finish (pSCNINFO text_scninfo);

extern void Cg_Dwarf_Add_Line_Entry (INT code_address, SRCPOS srcpos);

#ifdef TARG_X8664
extern void Cg_Dwarf_Process_PU (Elf64_Word  scn_index,
				 LABEL_IDX   begin_label,
				 LABEL_IDX   end_label,
				 LABEL_IDX   *eh_pushbp_label,
				 LABEL_IDX   *eh_movespbp_label,
				 LABEL_IDX   *eh_adjustsp_label,
				 LABEL_IDX   *eh_callee_saved_reg,
				 LABEL_IDX   *first_bb_labels,
				 LABEL_IDX   *last_bb_labels,
				 INT32       pu_entries,
				 INT32       end_offset,
				 ST         *PU_st,
				 DST_IDX     pu_dst,
				 Elf64_Word  eh_index,
				 INT         eh_offset,
				 // The following two arguments need
				 // to go away once libunwind provides
				 // an interface that lets us specify
				 // ranges symbolically.
				 INT        low_pc,
				 INT        high_pc);
#elif defined(TARG_MIPS)
extern void Cg_Dwarf_Process_PU (Elf64_Word  scn_index,
				 LABEL_IDX   begin_label,
				 LABEL_IDX   end_label,
				 LABEL_IDX   *eh_adjustsp_label,
				 LABEL_IDX   *eh_callee_saved_reg,
				 INT32       new_cfa_offset,
				 INT32       end_offset,
				 ST         *PU_st,
				 DST_IDX     pu_dst,
				 Elf64_Word  eh_index,
				 INT         eh_offset,
				 // The following two arguments need
				 // to go away once libunwind provides
				 // an interface that lets us specify
				 // ranges symbolically.
				 INT        low_pc,
				 INT        high_pc);
#else
extern void Cg_Dwarf_Process_PU (Elf64_Word  scn_index,
				 LABEL_IDX   begin_label,
				 LABEL_IDX   end_label,
				 INT32       end_offset,
				 ST         *PU_st,
				 DST_IDX     pu_dst,
				 Elf64_Word  eh_index,
				 INT         eh_offset,
				 // The following two arguments need
				 // to go away once libunwind provides
				 // an interface that lets us specify
				 // ranges symbolically.
				 INT        low_pc,
				 INT        high_pc);
#endif // TARG_X8664
#ifdef KEY
// To force a line number entry after the end of preamble in entry BB.
extern BOOL Cg_Dwarf_First_Op_After_Preamble_End;
// To force a line number entry at the start op of every BB.
extern BOOL Cg_Dwarf_BB_First_Op;
// Don't produce debug info for types that are not used in the PU.
extern BOOL CG_prune_unused_debug_types;
#endif

extern void Cg_Dwarf_Gen_Asm_File_Table (void);

#ifdef KEY
extern void Cg_Dwarf_Gen_Macinfo (void);
extern void Print_Directives_For_All_Files (void);
#endif

extern void Cg_Dwarf_Write_Assembly_From_Symbolic_Relocs(FILE *asm_file,
							 INT   section_count,
							 BOOL  is_64bit);

extern INT Offset_from_FP (ST *st);

typedef enum {
  CGD_ELFSYM,
  CGD_LABIDX
} CGD_SYMTAB_ENTRY_TYPE;

extern Dwarf_Unsigned
Cg_Dwarf_Symtab_Entry(CGD_SYMTAB_ENTRY_TYPE  type,
		      Dwarf_Unsigned         index,
		      Dwarf_Unsigned         pu_base_sym_idx = 0,
		      PU_IDX                 pu = (PU_IDX) 0,
		      char                  *label_name = NULL,
		      Dwarf_Unsigned         last_offset = 0);

//TB: export these type for accessing file_table and incl_table for
//gcov support
typedef struct {
  const char *path_name;
  BOOL already_processed;
} include_info;
  
typedef struct {
  const char *filename;
  INT incl_index;
  FILE *fileptr;
  INT max_line_printed;
  BOOL already_processed;
  Dwarf_Unsigned mod_time;
  Dwarf_Unsigned file_size;
} file_info;
#endif /* cgdwarf_INCLUDED */
