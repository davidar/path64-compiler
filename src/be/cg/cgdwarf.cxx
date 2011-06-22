/*
 * Copyright (C) 2007, 2008, 2009 PathScale, LLC.  All Rights Reserved.
 */

/*
 *  Copyright (C) 2006, 2007. QLogic Corporation. All Rights Reserved.
 */

/*
 * Copyright 2002, 2003, 2004, 2005, 2006 PathScale, Inc.  All Rights Reserved.
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


/* ====================================================================
 * ====================================================================
 * Module: cgdwarf.c
 * $Revision: 1.77 $
 * $Date: 05/11/30 16:49:43-08:00 $
 * $Author: gautam@jacinth.keyresearch $
 * $Source: be/cg/SCCS/s.cgdwarf.cxx $
 *
 * Description:
 *
 * Interface between cgemit and em_dwarf.c 
 *
 * ====================================================================
 * ====================================================================
 */

#define LIBDWARF_SORTS_RELOCS

#if !defined(LIBDWARF_SORTS_RELOCS)
#include <stdlib.h>		// qsort
#endif

#include <limits.h>
#include <stdio.h>
#include "elf_defines.h"
#include <elfaccess.h>
#ifdef KEY /* Bug 3507 */
#include <ctype.h>
#endif /* KEY Bug 3507 */

#define	USE_STANDARD_TYPES 1
#include "defs.h"
#include "erglob.h"
#include "glob.h"
#include "flags.h"
#include "tracing.h"
#include "util.h"
#include "config.h"
#include "config_asm.h"
#include "config_list.h"
#include "config_debug.h"
#include "cgir.h"
#include "mempool.h"
#include "tn_set.h"
#include "srcpos.h"
#include "em_elf.h"
#include "em_dwarf.h"
#include "targ_const.h"
#include "calls.h"
#include "stblock.h"
#include "data_layout.h"
#include "dwarf_DST.h"
#include "dwarf_DST_mem.h"
#include "const.h"
#include "cg.h"
#include "cgtarget.h"
#include "cgemit.h"
#include "cgdwarf.h"
#include "label_util.h"
#include "vstring.h"
#include "cgemit_targ.h"
#include "cgdwarf_targ.h"
#ifdef KEY
#include "config_debug.h" // for DEBUG_Emit_Ehframe.
#include <strings.h>
#include <string.h>
#endif
#ifdef _WIN32
#include <alloca.h>
#endif

#if HAVE_ALLOCA_H
#include <alloca.h>
#endif

extern "C" {
#include "libdwarf.h"
}

BOOL Trace_Dwarf;

#define cast_to_LABEL(x) ((LABEL_IDX) (uintptr_t) (void *)x)
static Dwarf_P_Debug dw_dbg;
static Dwarf_Error dw_error;
static BOOL Disable_DST = FALSE;
static DST_INFO_IDX cu_idx;
static Elf64_Word cur_text_index;
static DST_language Dwarf_Language;
//static INT Current_Tree_Level;

/* used as array to hold pointers to enclosing procedure's DIEs */
static Dwarf_P_Die *CGD_enclosing_proc = NULL;
static mINT32 CGD_enclosing_proc_max = 0;

#define GLOBAL_LEVEL 0
#define LOCAL_LEVEL 1

struct CGD_SYMTAB_ENTRY {
  CGD_SYMTAB_ENTRY_TYPE type;
  Dwarf_Unsigned        index;
  union {
    struct {
      PU_IDX         pu_idx;	// PU in which the label occurs
      STR_IDX        name_idx;  // use global Str_Table index,
				// (using Index_To_Str())
				// not string, as global strings
				// table can move, but index safe
      Dwarf_Unsigned offset;	// offset within ELF section
      Elf64_Word     base_sym;	// ELF symbol index for label's ELF section
    } label_info;
    struct {
      // I think the ELF symbol index tells us everything we need to know.
    } elfsym_info;
  };
#ifdef KEY
  Dwarf_Unsigned   next;	// CGD_Symtab index of next entry from same PU
  Dwarf_Unsigned   CGD_Symtab_offset;	// index in CGD_Symtab
#endif

  CGD_SYMTAB_ENTRY(CGD_SYMTAB_ENTRY_TYPE entry_type, Dwarf_Unsigned idx) :
    type(entry_type), index(idx)
#ifdef KEY
    , next(~0), CGD_Symtab_offset(~0)
#endif
      {}
#ifdef KEY
  bool next_is_null() { return next == ~0; }
#endif
};

vector <CGD_SYMTAB_ENTRY, mempool_allocator<CGD_SYMTAB_ENTRY> > CGD_Symtab;

#ifdef KEY
BOOL CG_prune_unused_debug_types = FALSE;

static BOOL Should_Emit (DST_INFO *info);
static void preorder_visit_mark_emit (DST_INFO_IDX idx, BOOL visit_children);
static void Add_To_Revisit_List (DST_INFO_IDX idx);
static void Traverse_Revisit_List ();

// Map PU to CGD_Symtab entries specific to that PU.
hash_map<PU_IDX, Dwarf_Unsigned> PU_IDX_To_CGD_SYMTAB_OFFSET;

static DST_INFO_IDX Current_Top_Level_Idx = DST_INVALID_IDX;
#endif
#if defined(BUILD_OS_DARWIN)
/* Darwin asm doesn't like subtraction of labels when one of the labels occurs
 * at the end of a section; assigning the difference to a temp symbol fixes
 * the problem */
static void
emit_difference(FILE *asm_file, const char *reloc_name, const char *sym1,
  const char *sym2) {
#define DIFF_SYMS ".DIFF.%s.%s"
  fprintf(asm_file, "\t.set " DIFF_SYMS ", %s - %s\n", sym1, sym2, sym1, sym2);
  fprintf(asm_file, "\t%s\t" DIFF_SYMS "\n", reloc_name, sym1, sym2);
}

/*
 * Generate section header and label for "debug_line" section, but only once
 * asm_file		Assembly output file
 * section_name		Name of debug_line section
 */
static void
gen_debug_line_section_header(FILE *asm_file, const char *section_name) {
  static int done;
  if (!done) {
    done = 1;
    fprintf(asm_file, "\n\t%s %s\n%s:\n", AS_SECTION,
      map_section_name(section_name), section_name);
  }
}
#endif /* defined(BUILD_OS_DARWIN) */

Dwarf_Unsigned Cg_Dwarf_Symtab_Entry(CGD_SYMTAB_ENTRY_TYPE  type,
				     Dwarf_Unsigned         index,
				     Dwarf_Unsigned         pu_base_sym_idx,
				     PU_IDX                 pu,
				     char		   *label_name,
				     Dwarf_Unsigned         last_offset)
{
  if (type == CGD_LABIDX && pu == (PU_IDX) 0) {
    pu = ST_pu(Get_Current_PU_ST());
  }
#ifdef KEY
  // Speed up search by looking at CGD_Symtab entries specific to the PU.
  // Bug 8756.
  CGD_SYMTAB_ENTRY *p;
  Dwarf_Unsigned idx = PU_IDX_To_CGD_SYMTAB_OFFSET[pu];
  for (p = &CGD_Symtab[idx]; !p->next_is_null(); p = &CGD_Symtab[p->next])
#else
  vector <CGD_SYMTAB_ENTRY,
	  mempool_allocator<CGD_SYMTAB_ENTRY> >::iterator p;
  for (p = CGD_Symtab.begin(); p != CGD_Symtab.end(); ++p)
#endif
  {
    if (p->type == type &&
	p->index == index) {
      switch (type) {
      case CGD_LABIDX:
	if (p->label_info.pu_idx == pu) {
#ifdef KEY
	  return p->CGD_Symtab_offset;
#else
	  return p - CGD_Symtab.begin();
#endif
	}
	break;
      case CGD_ELFSYM:
#ifdef KEY
	return p->CGD_Symtab_offset;
#else
	return p - CGD_Symtab.begin();
#endif
      default:
	Fail_FmtAssertion("Illegal CGD_Symtab entry type");
	break;
      }
    }
  }
#ifdef KEY
  CGD_SYMTAB_ENTRY new_entry = CGD_SYMTAB_ENTRY(type, index);
  new_entry.next = PU_IDX_To_CGD_SYMTAB_OFFSET[pu];
  new_entry.CGD_Symtab_offset = CGD_Symtab.size();
  PU_IDX_To_CGD_SYMTAB_OFFSET[pu] = new_entry.CGD_Symtab_offset;
  CGD_Symtab.push_back(new_entry);
#else
  CGD_Symtab.push_back(CGD_SYMTAB_ENTRY(type, index));
#endif

  if (Trace_Dwarf) {
    fprintf(TFile,
#if defined(BUILD_OS_DARWIN)
	    "New CGD_Symtab entry: %lu --> (CGD_%s,%llu)\n",
	    (long unsigned int) (CGD_Symtab.size() - 1),
#else /* defined(BUILD_OS_DARWIN) */
	    "New CGD_Symtab entry: %" PRIuPTR " --> (CGD_%s,%llu)\n",
	    CGD_Symtab.size() - 1,
#endif /* defined(BUILD_OS_DARWIN) */
	    (type == CGD_LABIDX ? "LABIDX" : "ELFSYM"),
	    index);
  }
  Dwarf_Unsigned handle = CGD_Symtab.size() - 1;
  if (type == CGD_LABIDX) {
    // Put label-specific fields in place
    CGD_Symtab[handle].label_info.pu_idx = pu;
    if (label_name == NULL) {
#ifdef KEY
      if (LABEL_IDX_level(index) == 0)
	index = make_LABEL_IDX(index, CURRENT_SYMTAB);
#endif
      CGD_Symtab[handle].label_info.name_idx = LABEL_name_idx(index);
      CGD_Symtab[handle].label_info.offset = Get_Label_Offset(index);
    }
    else {
      STR_IDX lstr = Save_Str(label_name);
      CGD_Symtab[handle].label_info.name_idx = lstr;
      CGD_Symtab[handle].label_info.offset = last_offset;
    }
    CGD_Symtab[handle].label_info.base_sym = pu_base_sym_idx;
    if (Trace_Dwarf) {
	STR_IDX sidx = CGD_Symtab[handle].label_info.name_idx;
      fprintf(TFile,
	      "pu_idx: %d; label: %d %s; ofst: %llu; base: %u\n",
	      CGD_Symtab[handle].label_info.pu_idx,
	      (int)sidx,
	      Index_To_Str(sidx),
	      CGD_Symtab[handle].label_info.offset,
	      CGD_Symtab[handle].label_info.base_sym);
    }
    FmtAssert(pu_base_sym_idx != 0,
	      ("ELF symbol for base of label's section must be specified"));
  }
  else if (type == CGD_ELFSYM) {
    // Put elfsym-specific fields in place
  }
  else {
    Fail_FmtAssertion("Illegal CGD_Symtab entry type");
  }
  return handle;
}


Dwarf_Unsigned Cg_Dwarf_Enter_Elfsym(Elf64_Word index)
{
  return Cg_Dwarf_Symtab_Entry(CGD_ELFSYM, index);
}

Elf64_Word Cg_Dwarf_Translate_Symidx(Dwarf_Unsigned idx_from_sym_reloc)
{
  Is_True(CGD_Symtab.size() > idx_from_sym_reloc,
	  ("Cg_Dwarf_Translate_Symidx: Index %llu out of bounds (%lu)",
	   idx_from_sym_reloc, CGD_Symtab.size()));
  if (Trace_Dwarf) {
    fprintf(TFile, "Translating %llu ", idx_from_sym_reloc);
    fflush(TFile);
    fprintf(TFile, "to %llu\n", CGD_Symtab[idx_from_sym_reloc].index);
  }
  Is_True(CGD_Symtab[idx_from_sym_reloc].type == CGD_ELFSYM,
	  ("Cg_Dwarf_Translate_Symidx: Unexpected entry type"));
  return CGD_Symtab[idx_from_sym_reloc].index;
}

Elf64_Word Cg_Dwarf_Translate_Offset(Dwarf_Unsigned idx_from_sym_reloc)
{
  Is_True(CGD_Symtab.size() > idx_from_sym_reloc,
	  ("Cg_Dwarf_Translate_Offset: Index %llu out of bounds (%lu)",
	   idx_from_sym_reloc, CGD_Symtab.size()));
  if (Trace_Dwarf) {
    fprintf(TFile, "Translating %llu ", idx_from_sym_reloc);
    fflush(TFile);
    fprintf(TFile, "through index %llu ",
	    CGD_Symtab[idx_from_sym_reloc].index);
    fprintf(TFile, "to %" SCNd64 "\n",
	    Get_Label_Offset(CGD_Symtab[idx_from_sym_reloc].index));
  }
  Is_True(CGD_Symtab[idx_from_sym_reloc].type == CGD_LABIDX,
	  ("Cg_Dward_Translate_Offset: Unexpected entry type"));
  return Get_Label_Offset(CGD_Symtab[idx_from_sym_reloc].index);
}

// The string returned is not valid across
// reallocs of the global string table,
// so callers must not save the string pointer returned.
char *
Cg_Dwarf_Name_From_Handle(Dwarf_Unsigned idx)
{
  Is_True(CGD_Symtab.size() > idx,
	  ("Cg_Dwarf_Name_From_Handle: Index %llu out of bounds (%lu)",
	   idx, CGD_Symtab.size()));
  if (CGD_Symtab[idx].type == CGD_ELFSYM) {
    return Em_Get_Symbol_Name(CGD_Symtab[idx].index);
  }
  else {
	// is CGD_LABEL
    STR_IDX sidx = CGD_Symtab[idx].label_info.name_idx;
    return Index_To_Str(sidx);
  }
}


#define put_flag(flag, die) dwarf_add_AT_flag(dw_dbg, die, flag, 1, &dw_error)

static ST *
Get_ST_From_DST (DST_ASSOC_INFO assoc_info)
{
	ST *st;
	st = &St_Table(
		DST_ASSOC_INFO_st_level(assoc_info),
		DST_ASSOC_INFO_st_index(assoc_info) );
	FmtAssert ((st != NULL), 
		("Get_ST_From_DST: bad dst info from fe?  assoc_info = (%d,%d)",
			DST_ASSOC_INFO_st_level(assoc_info), 
			DST_ASSOC_INFO_st_index(assoc_info) ));
	return st;
}

/* Given a 'ref_idx', return the dieptr for that die. */
static Dwarf_P_Die get_ref_die (DST_INFO_IDX ref_idx)
{
  DST_INFO *info;
  Dwarf_P_Die ref_die;

  info = DST_INFO_IDX_TO_PTR(ref_idx);
  FmtAssert(DST_INFO_tag(info) != 0, 
	("get_ref_die found 0 tag for idx %d,%d", 
	ref_idx.block_idx, ref_idx.byte_idx));
  ref_die = (Dwarf_P_Die) DST_INFO_dieptr (info);
  if (ref_die == NULL) {
    ref_die = dwarf_new_die (dw_dbg, DST_INFO_tag(info), NULL, NULL, 
			NULL, NULL, &dw_error);
    DST_INFO_dieptr(info) = ref_die;
    if (Trace_Dwarf) {
      fprintf (TFile,"NEW ref die for [%d,%d]: %p, tag:%d\n",
	ref_idx.block_idx, ref_idx.byte_idx, ref_die, DST_INFO_tag(info));
    }
  }
  return ref_die;
}

/* Given a 'ref_idx', write it out as the 'ref_attr' attribute for 'die'. */
static void
put_reference (DST_INFO_IDX ref_idx, Dwarf_Half ref_attr, Dwarf_P_Die die)
{
  Dwarf_P_Die ref_die;
    
  if (DST_IS_NULL(ref_idx)) return;


  FmtAssert ( ! DST_IS_FOREIGN_OBJ(ref_idx), 
	("Dwarf reference to foreign object"));
  ref_die = get_ref_die (ref_idx);
  dwarf_add_AT_reference (dw_dbg, die, ref_attr, ref_die, &dw_error);

#ifdef KEY
  // The DST info needs to be emitted because it is referenced.
  DST_INFO *info = DST_INFO_IDX_TO_PTR(ref_idx);
  if (!Should_Emit(info)) {
    preorder_visit_mark_emit(ref_idx, TRUE);
    // The should-emit flag might not have been set during a previous visit of
    // the node.  Revisit the node.
    DST_RESET_info_mark(DST_INFO_flag(info));
    // Make sure to emit the subtree.
    Add_To_Revisit_List(ref_idx);
  }
#endif
}

/* enumeration to decide which pubnames section to add the name to. */
typedef enum {
  pb_none,
  pb_pubname,
  pb_funcname,
  pb_weakname,
  pb_varname,
  pb_typename
} which_pb;

/* write out the name to one of the public-names sections if needed.
   If -g is not specified, do not write out names to the varname 
   and typename sections.
*/
static void 
put_pubname (char *name, Dwarf_P_Die die, which_pb pb_type)
{
    switch (pb_type) {
    case pb_pubname:
      dwarf_add_pubname (dw_dbg, die, name, &dw_error);
      break;
    case pb_funcname:
      dwarf_add_funcname (dw_dbg, die, name, &dw_error);
      break;
    case pb_weakname:
      dwarf_add_weakname (dw_dbg, die, name, &dw_error);
      break;
    case pb_varname:
      if (Debug_Level > 0) dwarf_add_varname (dw_dbg, die, name, &dw_error);
      break;
    case pb_typename:
      if (Debug_Level > 0) dwarf_add_typename (dw_dbg, die, name, &dw_error);
      break;
    default:
      break;
    }
}

/* Write out 'str_idx' as a 'str_attr' attribute. */
static void
put_string (DST_STR_IDX str_idx, Dwarf_Half str_attr, Dwarf_P_Die die)
{
  char *name;

  if (DST_IS_NULL(str_idx)) return;

  name = DST_STR_IDX_TO_PTR (str_idx);
  dwarf_add_AT_string (dw_dbg, die, str_attr, name, &dw_error);
}

/* Given a 'str_idx', write it out as the AT_name attribute for 'die'. */
static void
put_name (DST_STR_IDX str_idx, Dwarf_P_Die die, which_pb pb_type)
{
  char *name;

  if (DST_IS_NULL(str_idx)) return;

  name = DST_STR_IDX_TO_PTR (str_idx);
  put_string (str_idx, DW_AT_name, die);
  put_pubname (name, die, pb_type);
  if (Trace_Dwarf) {
    fprintf (TFile,"AT_name attribute: %s\n", DST_STR_IDX_TO_PTR (str_idx));
  }
}


/* write out the source position attributes for a declaration. */
static void
put_decl(USRCPOS decl, Dwarf_P_Die die)
{
   if (USRCPOS_filenum(decl) != 0)
//DevWarn("file # %d", USRCPOS_filenum(decl));
     dwarf_add_AT_unsigned_const (dw_dbg, die, DW_AT_decl_file, 
			   (UINT32)USRCPOS_filenum(decl), &dw_error);
   if (USRCPOS_linenum(decl) != 0)
     dwarf_add_AT_unsigned_const (dw_dbg, die, DW_AT_decl_line, 
			   (UINT32)USRCPOS_linenum(decl), &dw_error);
  if (USRCPOS_column(decl) != 0)
    dwarf_add_AT_unsigned_const (dw_dbg, die, DW_AT_decl_column, 
			   (UINT32)USRCPOS_column(decl), &dw_error);
}

static void
put_const_attribute (DST_CONST_VALUE cval, Dwarf_Half ref_attr, Dwarf_P_Die die)
{
  switch (DST_CONST_VALUE_form(cval)) {
    case DST_FORM_STRING:
      put_string (DST_CONST_VALUE_form_string(cval), ref_attr, die);
      break;
    // Bug 1188 - reverted for now
    case DST_FORM_DATA1:
      dwarf_add_AT_const_value_unsignedint(die, DST_CONST_VALUE_form_data1(cval),
                                           &dw_error);
#if 0
      dwf_add_AT_unsigned_const_ext (dw_dbg, die, ref_attr,
				     DST_CONST_VALUE_form_data1(cval),
				     &dw_error, 1);
#endif
      break;
    case DST_FORM_DATA2:
      dwarf_add_AT_const_value_unsignedint(die, DST_CONST_VALUE_form_data2(cval),
                                           &dw_error);
#if 0
      dwf_add_AT_unsigned_const_ext (dw_dbg, die, ref_attr,
				     DST_CONST_VALUE_form_data2(cval),
				     &dw_error, 2);
#endif
      break;
    case DST_FORM_DATA4:
      dwarf_add_AT_const_value_unsignedint(die, DST_CONST_VALUE_form_data4(cval),
                                           &dw_error);
#if 0
      dwf_add_AT_unsigned_const_ext (dw_dbg, die, ref_attr,
				     DST_CONST_VALUE_form_data4(cval),
				     &dw_error, 4);
#endif
      break;
    case DST_FORM_DATA8:
      dwarf_add_AT_const_value_unsignedint(die, DST_CONST_VALUE_form_data8(cval),
                                           &dw_error);
#if 0
      dwf_add_AT_unsigned_const_ext (dw_dbg, die, ref_attr,
				     DST_CONST_VALUE_form_data8(cval),
				     &dw_error, 8);
#endif
      break;
      // TODO: how to  do this in bsd libdwarf?
#if 0
    case DST_FORM_DATAC4:
      dwf_add_AT_complex_const (dw_dbg, die, ref_attr,
				DST_CONST_VALUE_form_crdata4(cval), 
				DST_CONST_VALUE_form_cidata4(cval), 
				&dw_error, 4);
      break;
    case DST_FORM_DATAC8:
      dwf_add_AT_complex_const (dw_dbg, die, ref_attr,
				DST_CONST_VALUE_form_crdata8(cval), 
				DST_CONST_VALUE_form_cidata8(cval), 
				&dw_error, 8);
      break;
#endif
  }
}

// output dopetype info if it exists
static void
put_dopetype (DST_INFO_IDX dope_idx, DST_flag flag, Dwarf_P_Die die)
{
#ifdef TARG_X8664
  return;
#else
   if (DST_IS_f90_pointer(flag))
	put_reference (dope_idx, DW_AT_MIPS_ptr_dopetype, die);
   if (DST_IS_allocatable(flag))
	put_reference (dope_idx, DW_AT_MIPS_allocatable_dopetype, die);
   if (DST_IS_assumed_shape(flag))
	put_reference (dope_idx, DW_AT_MIPS_assumed_shape_dopetype, die);
#endif
}

   /*----------------------------------
    * One put routine for each DW_TAG
    *----------------------------------*/


static void
put_compile_unit(DST_COMPILE_UNIT *attr, Dwarf_P_Die die)
{
   put_name (DST_COMPILE_UNIT_name(attr), die, pb_none);
   put_string (DST_COMPILE_UNIT_comp_dir(attr), DW_AT_comp_dir, die);
   if (DEBUG_Optimize_Space && Debug_Level == 0)
	// don't emit producer string if not debug and want to save space
	dwarf_add_AT_string (dw_dbg, die, DW_AT_producer, "", &dw_error);
   else
  	put_string (DST_COMPILE_UNIT_producer(attr), DW_AT_producer, die);
   dwarf_add_AT_unsigned_const (dw_dbg, die, DW_AT_language, 
	     DST_COMPILE_UNIT_language(attr), &dw_error);
   dwarf_add_AT_unsigned_const (dw_dbg, die, DW_AT_identifier_case, 
	     DST_COMPILE_UNIT_identifier_case(attr), &dw_error);

   if (FILE_INFO_has_inlines (File_info))
	put_flag (DW_AT_MIPS_has_inlines, die);
}

static BOOL
subprogram_def_is_inlined (DST_SUBPROGRAM *attr)
{
    switch (DST_SUBPROGRAM_def_inline(attr)) {
    case DW_INL_inlined:
    case DW_INL_declared_inlined:
	return TRUE;
    default:
	return FALSE;
    }
}

static void
put_subprogram(DST_flag flag,
	       DST_SUBPROGRAM *attr,
	       Dwarf_P_Die die)
{
  Dwarf_P_Expr expr;

  if (DST_IS_memdef(flag))  /* Not yet supported */ {
    ErrMsg (EC_Unimplemented, 
	" put_subprogram: a class member with AT_specification!");
  }
  else if (DST_IS_declaration(flag)) {
    /* If the origin field is not NULL, then this declaration is really
       a weakname and the origin points to the DST entry for the strong
       symbol. Emit an entry in the weaknames section pointing to the
       original subprogram die.
    */
    if (!DST_IS_NULL(DST_SUBPROGRAM_decl_origin(attr))) {
      if (!DST_IS_NULL(DST_SUBPROGRAM_decl_name(attr))) {
        put_pubname (DST_STR_IDX_TO_PTR(DST_SUBPROGRAM_decl_name(attr)),
		     get_ref_die (DST_SUBPROGRAM_decl_origin(attr)),
		     pb_weakname);
      }
    }
    put_decl(DST_SUBPROGRAM_decl_decl(attr), die);
    put_name (DST_SUBPROGRAM_decl_name(attr), die, pb_none);
    put_reference (DST_SUBPROGRAM_decl_type(attr), DW_AT_type, die);
    put_flag (DW_AT_declaration, die);
    if (DST_IS_external(flag)) put_flag (DW_AT_external, die);
    if (DST_IS_prototyped(flag)) put_flag (DW_AT_prototyped, die);
    if (DST_SUBPROGRAM_decl_virtuality(attr) != DW_VIRTUALITY_none) {
       dwarf_add_AT_unsigned_const(dw_dbg, die, DW_AT_virtuality,
			           DST_SUBPROGRAM_decl_virtuality(attr),
				   &dw_error);
       expr = dwarf_new_expr(dw_dbg, &dw_error);
       dwarf_add_expr_gen(expr,
              	 	  DW_OP_const2u,
			  DST_SUBPROGRAM_decl_vtable_elem_location(attr),
			  0,
			  &dw_error);
       dwarf_add_AT_location_expr(dw_dbg, die, DW_AT_vtable_elem_location,
				  expr, &dw_error);
    }
    put_string (DST_SUBPROGRAM_decl_linkage_name(attr),
	    DW_AT_MIPS_linkage_name, die);

    switch (DST_SUBPROGRAM_decl_inline(attr)) {
    case DW_INL_inlined:
    case DW_INL_declared_inlined:
   	dwarf_add_AT_unsigned_const (dw_dbg, die, DW_AT_inline, 
			DST_SUBPROGRAM_decl_inline(attr), &dw_error);
	break;
    }
  }
  else /* definition */ {
    char *pubname;

    put_decl(DST_SUBPROGRAM_def_decl(attr), die);
    /* check if there is a pubnames name for subroutine (for member fns) */
    if (!DST_IS_NULL(DST_SUBPROGRAM_def_pubname(attr))) {
      put_name (DST_SUBPROGRAM_def_name(attr), die, pb_none);
      pubname = DST_STR_IDX_TO_PTR (DST_SUBPROGRAM_def_pubname(attr));
      dwarf_add_pubname (dw_dbg, die, pubname, &dw_error);
    }
    else {
      put_name (DST_SUBPROGRAM_def_name(attr), 
	        die, 
	        DST_IS_external(flag) ? pb_pubname : pb_funcname);
    }
#ifdef KEY
    // Bug 4311 - The front-end does not set up the correct type information
    // in the DST tree for the subprogram. In such cases, we use the top-level
    // DST tree to derive the return type from a variable of same name as the
    // subroutine. We check only the first child entry of the top level node
    // that is either a decalaration or not a subprogram itself.
    // The reason for following this method is there is no link back
    // from the ST table to the DST table. So, given a ST_pu_type, we are not
    // able to get a DST entry for that type.
    if (DST_IS_NULL(DST_SUBPROGRAM_def_type(attr)) &&
	(Dwarf_Language == DW_LANG_Fortran77 ||
	 Dwarf_Language == DW_LANG_Fortran90) &&
	Current_Top_Level_Idx != DST_INVALID_IDX) {
      DST_INFO_IDX child_idx;
      for (child_idx = DST_first_child (Current_Top_Level_Idx);
	   !DST_IS_NULL(child_idx); 
	   child_idx = DST_INFO_sibling(DST_INFO_IDX_TO_PTR(child_idx))) {
	if (child_idx != DST_INVALID_IDX) {
	  DST_INFO *child_info = DST_INFO_IDX_TO_PTR (child_idx);
	  if (child_info && 
	      DST_INFO_tag(child_info) == DW_TAG_variable) {
	    DST_ATTR_IDX child_attr_idx = DST_INFO_attributes(child_info);
	    DST_VARIABLE *child_attr = 
	      DST_ATTR_IDX_TO_PTR(child_attr_idx, DST_VARIABLE);
	    if (child_attr) {
	      DST_STR_IDX child_str_idx = DST_VARIABLE_def_name(child_attr);
	      if (child_str_idx != DST_INVALID_IDX) {
		ST *pu_st = Get_ST_From_DST(DST_SUBPROGRAM_def_st(attr));
		char *pu_name = ST_name(pu_st);
		char *variable_name = DST_STR_IDX_TO_PTR(child_str_idx);
		if (strncasecmp(variable_name, pu_name, 
				strlen(variable_name)) == 0)
		  put_reference (DST_VARIABLE_decl_type(child_attr), 
				 DW_AT_type, die);
	      }
	    }
	    break;
	  }
	}
      }
    } else
#endif
    put_reference (DST_SUBPROGRAM_def_type(attr), DW_AT_type, die);
    if (DST_IS_external(flag))  put_flag (DW_AT_external, die);
    if (DST_IS_prototyped(flag)) put_flag (DW_AT_prototyped, die);
    /* check if we have a pointer from def to decl (for member function) */
    put_reference (DST_SUBPROGRAM_def_specification(attr),
		      DW_AT_specification, die);
    if (DST_SUBPROGRAM_def_virtuality(attr) != DW_VIRTUALITY_none) {
       dwarf_add_AT_unsigned_const(dw_dbg, die, DW_AT_virtuality,
			           DST_SUBPROGRAM_def_virtuality(attr),
				   &dw_error);
       expr = dwarf_new_expr(dw_dbg, &dw_error);
       dwarf_add_expr_gen(expr,
              	 	  DW_OP_const2u,
			  DST_SUBPROGRAM_def_vtable_elem_location(attr),
			  0,
			  &dw_error);
       dwarf_add_AT_location_expr(dw_dbg, die, DW_AT_vtable_elem_location,
				  expr, &dw_error);
    }
       
    put_string (DST_SUBPROGRAM_def_linkage_name(attr),
		      DW_AT_MIPS_linkage_name, die);
    if (!DST_IS_NULL(DST_SUBPROGRAM_def_clone_origin(attr))) {
	put_reference (DST_SUBPROGRAM_def_clone_origin(attr),
			DW_AT_MIPS_clone_origin, die);
   	dwarf_add_AT_unsigned_const (dw_dbg, die, DW_AT_calling_convention, 
			DW_CC_nocall, &dw_error);
    }
    if (subprogram_def_is_inlined(attr)) {
   	dwarf_add_AT_unsigned_const (dw_dbg, die, DW_AT_inline, 
			DST_SUBPROGRAM_def_inline(attr), &dw_error);
    }
    if (PU_has_inlines(Get_Current_PU()))
	put_flag (DW_AT_MIPS_has_inlines, die);
  }
}


static Elf64_Word 
get_elfindex_from_ASSOC_INFO (DST_ASSOC_INFO assoc_info)
{
  ST *st;
  st = Get_ST_From_DST (assoc_info);
  return EMT_Put_Elf_Symbol(st);
}

static mINT32 
get_ofst_from_ASSOC_INFO (DST_ASSOC_INFO assoc_info)
{
  ST *st;
  st = Get_ST_From_DST (assoc_info);
  return (st != NULL) ? ST_ofst(st) : 0;
}

/* TEMPORARY:  when we combine index field between labels and vars,
 * then can get rid of this, but for now need duplicate routine. */
static mINT32 
get_ofst_from_label_ASSOC_INFO (DST_ASSOC_INFO assoc_info)
{
  ST *st;
  if (DST_ASSOC_INFO_st_index(assoc_info) == 0) {
	/* Fortran format statements create dw_tag_labels that point to
	 * constant strings.  If index is 0, this must be such a constant */
	st = &St_Table(
		DST_ASSOC_INFO_st_level(assoc_info),
		DST_ASSOC_INFO_st_index(assoc_info) );
  } else {
	LABEL_IDX lab;
	lab = DST_ASSOC_INFO_st_index(assoc_info);
  	FmtAssert ((lab > 0 && lab <= LABEL_Table_Size(DST_ASSOC_INFO_st_level(assoc_info))), 
	    ("get_ofst_from_label_ASSOC_INFO: bad dst info from fe? (%d,%d)",
		DST_ASSOC_INFO_st_level(assoc_info), 
		DST_ASSOC_INFO_st_index(assoc_info) ));
	return Get_Label_Offset(lab);
  }
  FmtAssert ((st != NULL), 
	("get_ofst_from_label_ASSOC_INFO: bad dst info from fe? (%d,%d)",
		DST_ASSOC_INFO_st_level(assoc_info), 
		DST_ASSOC_INFO_st_index(assoc_info) ));
  return (st != NULL) ? ST_ofst(st) : 0;
}

extern INT
Offset_from_FP (ST *st)
{
  ST *base_st;
  INT64 base_ofst;
  Allocate_Object(st);
  Base_Symbol_And_Offset (st, &base_st, &base_ofst);

  if (base_st == SP_Sym) {
    return (base_ofst - Frame_Len);
  }
  else if (base_st == FP_Sym) {
    return base_ofst;
  }
  else {
    Is_True(FALSE, ("symbol %s is not allocated", ST_name(st)));
    return 0;
  }
}

/* Generate a location expression for the symbol represented by 'assoc_info'.
 * Only symbols that have a memory location have a location expression. For 
 * other cases, we return without doing anything.
 */
static void
put_location (
  DST_ASSOC_INFO assoc_info, 
  INT offs, 
  DST_flag flag,
  Dwarf_P_Die die,
  Dwarf_Half loc_attr)
{
  Dwarf_P_Expr expr;
  ST *st;
  ST *base_st;
  INT64 base_ofst;
  BOOL deref;

  st = Get_ST_From_DST (assoc_info);
  if (ST_sclass(st) == SCLASS_FORMAL_REF)
	st =	Get_ST_formal_ref_base(st);
  if (st == NULL) return;
  if (ST_is_not_used(st)) return;
#ifdef KEY	// Ignore asm string names.  Bug 7604.
  if (ST_sym_class(st) == CLASS_NAME) return;
#endif

  Base_Symbol_And_Offset (st, &base_st, &base_ofst);

  deref = FALSE;
  if (DST_IS_deref(flag))  /* f90 formals, dope, etc */
	deref = TRUE;

  expr = dwarf_new_expr (dw_dbg, &dw_error);

  if (st == base_st && ST_class(st) != CLASS_BLOCK 
	&& ST_sclass(st) != SCLASS_COMMON && ST_sclass(st) != SCLASS_EXTERN) 
  {
	/* symbol was not allocated, so doesn't have dwarf location */
#if !defined( KEY)
	return;
#else
	// Bug 4723 - Allocate any object not allocated because the rest of 
	// compilation never encountered that object. Note that ST_is_not_used 
	// is not set for this object.
	Allocate_Object(st);
	Base_Symbol_And_Offset (st, &base_st, &base_ofst);
#endif
  }

  switch (ST_sclass(st)) {
    case SCLASS_FORMAL:
	if (base_st != SP_Sym && base_st != FP_Sym) {
                //printf ("[1] base_offset: %"PRId64", ofst: %"PRId64", offs: %d\n", base_ofst, ST_ofst(st), offs) ;
		dwarf_add_expr_addr_b (expr,
#if defined( KEY)
                                       base_ofst + offs,               // need to add base offset because if the symbols has
                                                                    // a base, the offset is not necessarily set
#else
				       ST_ofst(st) + offs,
#endif
				       Cg_Dwarf_Symtab_Entry(CGD_ELFSYM,
							     EMT_Put_Elf_Symbol(base_st)),
				       &dw_error);
		if (Trace_Dwarf) {
	  		fprintf (TFile,"LocExpr: symbol = %s, offset = %" SCNd64 "\n", 
#if defined( KEY)
			      ST_name(base_st), base_ofst + ST_ofst(st) + offs);
#else
			      ST_name(base_st), ST_ofst(st) + offs);
#endif
		}
		break;
	}
	/* else fall through to the case of stack variables. */

    case SCLASS_AUTO:

        if (DST_IS_base_deref(flag)) { /* f90 formal dope  */

	  dwarf_add_expr_gen (expr, DW_OP_fbreg, Offset_from_FP(st), 
			    0, &dw_error);

	  dwarf_add_expr_gen (expr, DW_OP_deref, 0,0, &dw_error);
	  dwarf_add_expr_gen (expr, DW_OP_plus_uconst, offs, 0, &dw_error);

	} else {

	  dwarf_add_expr_gen (expr, DW_OP_fbreg, Offset_from_FP(st) + offs, 
			      0, &dw_error);
	}
	if (Trace_Dwarf) {
	  fprintf (TFile,"LocExpr: DW_OP_fbreg,  offset = %d\n", 
		   Offset_from_FP(st) + offs);
	}
	break;

    case SCLASS_CPLINIT:
    case SCLASS_EH_REGION:
    case SCLASS_EH_REGION_SUPP:
    case SCLASS_DGLOBAL:
    case SCLASS_UGLOBAL:
    case SCLASS_FSTATIC:
    case SCLASS_PSTATIC:
      if (ST_is_thread_local(st)) {
        dwarf_add_expr_gen (expr, DW_OP_const8u, base_ofst, 0, &dw_error);
        dwarf_add_expr_gen (expr, DW_OP_GNU_push_tls_address, 0, 0, &dw_error);
      } else if (base_st != NULL) {
         //printf ("[2] %s (%s): real base_ofst: %llx, calc base_offset: %"PRId64", ofst: %"PRId64", offs: %d, base_st: %p, st: %p\n", ST_name(st), ST_name(base_st), ST_ofst(base_st), base_ofst, ST_ofst(st), offs, base_st, st) ;
	dwarf_add_expr_addr_b (expr,
#if defined( KEY)
			           base_ofst + offs,               // need to add base offset because if the symbols has
                                                                    // a base, the offset is not necessarily set
#else
			       ST_ofst(st) + offs, 
#endif
			       Cg_Dwarf_Symtab_Entry(CGD_ELFSYM,
						     EMT_Put_Elf_Symbol(base_st)),
			       &dw_error);
	if (Trace_Dwarf) {
	  fprintf (TFile,"LocExpr: symbol = %s, offset = %" SCNd64 "\n", 
#if defined( KEY)
			      ST_name(base_st), base_ofst + ST_ofst(st) + offs); 
#else
			      ST_name(base_st), ST_ofst(st) + offs); 
#endif
	}
      }
      else {
	dwarf_add_expr_addr_b (expr, offs,
			       Cg_Dwarf_Symtab_Entry(CGD_ELFSYM,
						     EMT_Put_Elf_Symbol(st)),
			       &dw_error);
	if (Trace_Dwarf) {
	  fprintf (TFile,"LocExpr: symbol = %s, offset = %d\n", ST_name(st), offs);
	}
      }
      break;

    case SCLASS_COMMON:
    case SCLASS_EXTERN:
      dwarf_add_expr_addr_b (expr, offs,
			     Cg_Dwarf_Symtab_Entry(CGD_ELFSYM,
						   EMT_Put_Elf_Symbol(st)),
			     &dw_error);

      if (Trace_Dwarf) {
	fprintf (TFile,"LocExpr: symbol = %s, offset = %d\n", ST_name(st), offs);
      }
      break;
    default:
#ifdef KEY
      // Treat unknown sclass variables as SCLASS_AUTO if 
      // the base points to the stack. Fix bug #380, also bug 11302 
      if ((base_st == FP_Sym || base_st == SP_Sym) && ST_sclass(st) == SCLASS_UNKNOWN) {
        if (DST_IS_base_deref(flag)) { /* f90 formal dope  */
	  
	  dwarf_add_expr_gen (expr, DW_OP_fbreg, Offset_from_FP(st), 
			      0, &dw_error);
	  
	  dwarf_add_expr_gen (expr, DW_OP_deref, 0,0, &dw_error);
	  dwarf_add_expr_gen (expr, DW_OP_plus_uconst, offs, 0, &dw_error);
	  
	} else {

	  dwarf_add_expr_gen (expr, DW_OP_fbreg, Offset_from_FP(st) + offs, 
			      0, &dw_error);
	}
	break;
      }
#endif
      ErrMsg (EC_Unimplemented, "put_location: sclass");
      return;
  }
  if (deref) {
    dwarf_add_expr_gen (expr, DW_OP_deref, 0, 0, &dw_error);
    if (Trace_Dwarf) {
      fprintf (TFile,"LocExpr: DW_OP_deref\n");
    }
  }
  dwarf_add_AT_location_expr (dw_dbg, die, loc_attr, expr, &dw_error);
  return;
}

static void
put_pc_value_symbolic (Dwarf_Unsigned pc_attr,
		       Dwarf_Unsigned pc_label,
		       Dwarf_Addr     pc_offset,
		       Dwarf_P_Die    die)
{
  dwarf_add_AT_targ_address_b(dw_dbg,
			      die,
			      pc_attr,
			      pc_offset,
			      pc_label,
			      &dw_error);
}

#if 0
static void
put_pc_value (Dwarf_Unsigned pc_attr, INT32 pc_value, Dwarf_P_Die die)
{
   dwarf_add_AT_targ_address(
      dw_dbg,
      die,
      pc_attr,
      pc_value,
      cur_text_index,
      &dw_error);
}
#endif

static void
put_lexical_block(DST_flag flag, DST_LEXICAL_BLOCK *attr, Dwarf_P_Die die)
{
  put_name (DST_LEXICAL_BLOCK_name(attr), die, pb_none);
#if 1
  put_pc_value_symbolic (DW_AT_low_pc,
			 Cg_Dwarf_Symtab_Entry(CGD_LABIDX,
					       (LABEL_IDX) DST_ASSOC_INFO_st_index(DST_LEXICAL_BLOCK_low_pc(attr)),
					       cur_text_index),
			 (Dwarf_Addr) 0,
			 die);
  put_pc_value_symbolic (DW_AT_high_pc,
			 Cg_Dwarf_Symtab_Entry(CGD_LABIDX,
					       (LABEL_IDX) DST_ASSOC_INFO_st_index(DST_LEXICAL_BLOCK_high_pc(attr)),
					       cur_text_index),
			 (Dwarf_Addr) 0,
			 die);
#else
  put_pc_value (DW_AT_low_pc, 
	get_ofst_from_label_ASSOC_INFO(DST_LEXICAL_BLOCK_low_pc(attr)),
	die);
  put_pc_value (DW_AT_high_pc,
	get_ofst_from_label_ASSOC_INFO(DST_LEXICAL_BLOCK_high_pc(attr)),
	die);
#endif
}

static void
put_inlined_subroutine(DST_INLINED_SUBROUTINE *attr, Dwarf_P_Die die)
{
#if 1
  put_pc_value_symbolic (DW_AT_low_pc,
			 Cg_Dwarf_Symtab_Entry(CGD_LABIDX,
					       (LABEL_IDX) DST_ASSOC_INFO_st_index(DST_LEXICAL_BLOCK_low_pc(attr)),
					       cur_text_index),
			 (Dwarf_Addr) 0,
			 die);
  put_pc_value_symbolic (DW_AT_high_pc,
			 Cg_Dwarf_Symtab_Entry(CGD_LABIDX,
					       (LABEL_IDX) DST_ASSOC_INFO_st_index(DST_LEXICAL_BLOCK_high_pc(attr)),
					       cur_text_index),
			 (Dwarf_Addr) 0,
			 die);
#else
   put_pc_value (DW_AT_low_pc,
	get_ofst_from_label_ASSOC_INFO(DST_INLINED_SUBROUTINE_low_pc(attr)),
	die);
   put_pc_value (DW_AT_high_pc,
	get_ofst_from_label_ASSOC_INFO(DST_INLINED_SUBROUTINE_high_pc(attr)),
	die);
#endif

   if (DST_IS_FOREIGN_OBJ (DST_INLINED_SUBROUTINE_abstract_origin(attr))) {
	/* cross file inlining */
	put_string (
		DST_INLINED_SUBROUTINE_abstract_name(attr), 
		DW_AT_MIPS_abstract_name, 
		die);
	put_decl (DST_INLINED_SUBROUTINE_decl(attr), die);
   }
   else {
	/* same-file inlining */
	put_reference(
		DST_INLINED_SUBROUTINE_abstract_origin(attr),
		DW_AT_abstract_origin,
		die);
   }
}

static void
put_concrete_subprogram (DST_INFO_IDX abstract_idx,
			 INT32        low_pc,
			 INT32        high_pc,
			 Dwarf_P_Die  die)
{
#if 0
   put_pc_value (DW_AT_low_pc, low_pc, die);
   put_pc_value (DW_AT_high_pc, high_pc, die);
#endif
   put_reference( abstract_idx, DW_AT_abstract_origin, die);
}

static void
put_label(DST_flag flag, DST_LABEL *attr, Dwarf_P_Die die)
{
  put_name (DST_LABEL_name(attr), die, pb_none);
  dwarf_add_AT_targ_address_b (dw_dbg, die, DW_AT_low_pc,
	     get_ofst_from_label_ASSOC_INFO(DST_LABEL_low_pc(attr)), 
	     cur_text_index, &dw_error);
}


static void
put_variable(DST_flag flag, DST_VARIABLE *attr, Dwarf_P_Die die)
{
  if (DST_IS_const(flag)) {
#ifndef KEY
    ErrMsg (EC_Unimplemented, "put_variable: DST_IS_const");
#else
    DST_CONSTANT *attr_tmp = (DST_CONSTANT *)attr;
    FmtAssert (!DST_IS_declaration(flag), ("put_constant of non-def"));
    put_decl(DST_CONSTANT_def_decl(attr_tmp), die);
    put_name (DST_CONSTANT_def_name(attr_tmp), die, pb_none);
    put_reference (DST_CONSTANT_def_type(attr_tmp), DW_AT_type, die);
    put_const_attribute (DST_CONSTANT_def_cval(attr_tmp), DW_AT_const_value, die);    
#endif
  }
  else if (DST_IS_memdef(flag))  /* Not yet supported */ {
    ErrMsg (EC_Unimplemented, 
	"put_variable: a class member with AT_specification");
  }
  else if (DST_IS_declaration(flag)) {
    put_decl(DST_VARIABLE_decl_decl(attr), die);
    put_name (DST_VARIABLE_decl_name(attr), die, pb_none);
    put_reference (DST_VARIABLE_decl_type(attr), DW_AT_type, die);
    put_flag (DW_AT_declaration, die);
    if (DST_IS_external(flag)) put_flag (DW_AT_external, die);
#if defined(KEY)
    if (!DST_IS_NULL (DST_VARIABLE_decl_linkage_name(attr))) {
        put_string (DST_VARIABLE_decl_linkage_name(attr), DW_AT_MIPS_linkage_name, die) ;
    }
#endif
  }
  else if (DST_IS_comm(flag)) { /* definition of a common block variable. */
    put_decl(DST_VARIABLE_comm_decl(attr), die);
    put_name (DST_VARIABLE_comm_name(attr), die, pb_varname);
    put_reference (DST_VARIABLE_comm_type(attr), DW_AT_type, die);
    put_dopetype (DST_VARIABLE_comm_dopetype(attr), flag, die);

    put_location (DST_VARIABLE_comm_st(attr), 
	          (INT) DST_VARIABLE_comm_offs(attr),
		  flag, die, DW_AT_location);
    if (DST_IS_external(flag))  put_flag (DW_AT_external, die);
    if (DST_IS_assumed_size(flag)) put_flag (DW_AT_MIPS_assumed_size, die);
  }
  else /* definition */ {
    which_pb pbtype;

    put_decl(DST_VARIABLE_def_decl(attr), die);
    if (DST_IS_external(flag)) 
      pbtype = pb_pubname;
    else if (!DST_IS_automatic(flag))
      pbtype = pb_varname;
    else
      pbtype = pb_none;
    put_name (DST_VARIABLE_def_name(attr), die, pbtype);
    put_reference (DST_VARIABLE_def_type(attr), DW_AT_type, die);
    put_location (DST_VARIABLE_def_st(attr), 
		  (INT) DST_VARIABLE_def_offs(attr),
		  flag, die, DW_AT_location);
    if (DST_IS_external(flag))  put_flag (DW_AT_external, die);
    if (DST_IS_assumed_size(flag)) put_flag (DW_AT_MIPS_assumed_size, die);
    put_dopetype (DST_VARIABLE_def_dopetype(attr), flag, die);
    /* check if we have a pointer from def to decl (for static data member) */
    if (!DST_IS_NULL(DST_VARIABLE_def_specification(attr)))
       put_reference (DST_VARIABLE_def_specification(attr),
                      DW_AT_specification, die);
    if ( ! DST_IS_FOREIGN_OBJ(DST_VARIABLE_def_abstract_origin(attr))) {
	put_reference(
		DST_VARIABLE_def_abstract_origin(attr),
		DW_AT_abstract_origin,
		die);
    }
    /* else if is cross-file inlined, will use name for matching */
#if defined(KEY)
    if (!DST_IS_NULL (DST_VARIABLE_decl_linkage_name(attr))) {
        put_string (DST_VARIABLE_def_linkage_name(attr), DW_AT_MIPS_linkage_name, die) ;
    }
#endif
  }
}

static void
put_constant (DST_flag flag, DST_CONSTANT *attr, Dwarf_P_Die die)
{
   FmtAssert (!DST_IS_declaration(flag), ("put_constant of non-def"));
   put_decl(DST_CONSTANT_def_decl(attr), die);
   put_name (DST_CONSTANT_def_name(attr), die, pb_none);
   put_reference (DST_CONSTANT_def_type(attr), DW_AT_type, die);
   put_const_attribute (DST_CONSTANT_def_cval(attr), DW_AT_const_value, die);
}

static void
put_formal_parameter(DST_flag flag, DST_FORMAL_PARAMETER *attr, Dwarf_P_Die die)
{
  if (DST_IS_declaration(flag)) {
   put_decl(DST_FORMAL_PARAMETER_decl(attr), die);
   put_name (DST_FORMAL_PARAMETER_name(attr), die, pb_none);
   put_reference (DST_FORMAL_PARAMETER_default_val(attr),
		  DW_AT_default_value, 
	          die);
   put_reference (DST_FORMAL_PARAMETER_type(attr), DW_AT_type, die);            // bug 1735: need the type for the formal paras
   put_flag (DW_AT_declaration, die);
  } else {

   put_decl(DST_FORMAL_PARAMETER_decl(attr), die);
   put_name (DST_FORMAL_PARAMETER_name(attr), die, pb_none);
   put_reference (DST_FORMAL_PARAMETER_type(attr), DW_AT_type, die);
   put_location (DST_FORMAL_PARAMETER_st(attr), 0, flag, die, DW_AT_location);
   if (DST_IS_optional_parm(flag)) put_flag (DW_AT_is_optional, die);
   if (DST_IS_variable_parm(flag)) put_flag (DW_AT_variable_parameter, die);
   if (DST_IS_assumed_size(flag)) put_flag (DW_AT_MIPS_assumed_size, die);
   put_reference (DST_FORMAL_PARAMETER_default_val(attr),
		  DW_AT_default_value, 
	          die);
   put_dopetype (DST_FORMAL_PARAMETER_dopetype(attr), flag, die);
   if ( ! DST_IS_FOREIGN_OBJ(DST_FORMAL_PARAMETER_abstract_origin(attr))) {
	put_reference(
		DST_FORMAL_PARAMETER_abstract_origin(attr),
		DW_AT_abstract_origin,
		die);
   }
  }
   /* else if is cross-file inlined, will use name for matching */
}


static void
put_unspecified_parameters(
    DST_flag flag, 
    DST_UNSPECIFIED_PARAMETERS *attr,
    Dwarf_P_Die die)
{
  put_decl(DST_UNSPECIFIED_PARAMETERS_decl(attr), die);
  /* TODO abstarct origin. */
}


static void
put_basetype(DST_flag flag, DST_BASETYPE *attr, Dwarf_P_Die die)
{
  put_name (DST_BASETYPE_name(attr), die, pb_none);
  dwarf_add_AT_unsigned_const (dw_dbg, die, DW_AT_encoding,
		DST_BASETYPE_encoding(attr), &dw_error);
  dwarf_add_AT_unsigned_const (dw_dbg, die, DW_AT_byte_size,
		DST_BASETYPE_byte_size(attr), &dw_error);
}


static void
put_const_type(DST_flag flag, DST_CONST_TYPE *attr, Dwarf_P_Die die)
{
  put_reference (DST_CONST_TYPE_type(attr), DW_AT_type, die);
}


static void
put_volatile_type(DST_flag flag, DST_VOLATILE_TYPE *attr, Dwarf_P_Die die)
{
  put_reference (DST_VOLATILE_TYPE_type(attr), DW_AT_type, die);
}


static void
put_pointer_type(DST_flag flag, DST_POINTER_TYPE *attr, Dwarf_P_Die die)
{
  put_reference (DST_POINTER_TYPE_type(attr), DW_AT_type, die);
  dwarf_add_AT_unsigned_const (dw_dbg, die, DW_AT_byte_size,
	      DST_POINTER_TYPE_byte_size(attr), &dw_error);
  dwarf_add_AT_unsigned_const (dw_dbg, die, DW_AT_address_class,
	      DST_POINTER_TYPE_address_class(attr), &dw_error);
}


static void
put_reference_type(DST_flag flag, DST_REFERENCE_TYPE *attr, Dwarf_P_Die die)
{
  put_reference (DST_REFERENCE_TYPE_type(attr), DW_AT_type, die);
  dwarf_add_AT_unsigned_const (dw_dbg, die, DW_AT_byte_size,
	      DST_REFERENCE_TYPE_byte_size(attr), &dw_error);
  dwarf_add_AT_unsigned_const (dw_dbg, die, DW_AT_address_class,
	      DST_REFERENCE_TYPE_address_class(attr), &dw_error);
}


static void
put_typedef (DST_flag flag, DST_TYPEDEF *attr, Dwarf_P_Die die)
{
  put_decl(DST_TYPEDEF_decl(attr), die);
  put_name (DST_TYPEDEF_name(attr), die, pb_typename);
  put_reference (DST_TYPEDEF_type(attr), DW_AT_type, die);
   /* TODO: 
   DST_put_idx_attribute(" abstract_origin",
			 DST_TYPEDEF_abstract_origin(attr), FALSE);
   DST_write_line();
   */
}

static void
put_ptr_to_member_type(DST_flag flag,
		       DST_PTR_TO_MEMBER_TYPE *attr,
		       Dwarf_P_Die die)
{
  put_name(DST_PTR_TO_MEMBER_TYPE_name(attr), die, pb_typename);
  put_reference(DST_PTR_TO_MEMBER_TYPE_type(attr), DW_AT_type, die);
  put_reference(DST_PTR_TO_MEMBER_TYPE_class_type(attr), 
		DW_AT_containing_type, die);
}

static void
put_array_type(DST_flag flag, DST_ARRAY_TYPE *attr, Dwarf_P_Die die)
{
  put_decl(DST_ARRAY_TYPE_decl(attr), die);
  put_name (DST_ARRAY_TYPE_name(attr), die, pb_typename);
  put_reference (DST_ARRAY_TYPE_type(attr), DW_AT_type, die);
  if (DST_ARRAY_TYPE_byte_size(attr) != 0) 
    dwarf_add_AT_unsigned_const (dw_dbg, die, DW_AT_byte_size,
		        DST_ARRAY_TYPE_byte_size(attr), &dw_error);
  if (DST_IS_declaration(flag)) put_flag (DW_AT_declaration, die);
#ifdef TARG_X8664
  if (DST_IS_GNU_vector(flag)) put_flag (DW_AT_GNU_vector, die);
#endif
  /* TODO: 
   DST_put_idx_attribute(" abstract_origin",
			 DST_ARRAY_TYPE_abstract_origin(attr), FALSE);
  */
}


/* write out the AT_lower_bound attribute. If the bound is a constant and
   is a default value for the language, do not emit the attribute.
*/
static void
put_lower_bound (DST_flag flag, DST_SUBRANGE_TYPE *attr, Dwarf_P_Die die)
{
  DST_bounds_t cval;

  if (DST_IS_lb_cval(flag)) {
    cval = DST_SUBRANGE_TYPE_lower_cval(attr);
#ifndef TARG_X8664 
    // Bug 2706 - was this meant for the debuggers on the MIPS platform? -
    // - For us, we will emit lower bound no matter what the bound is.
    switch (Dwarf_Language) {
      case DW_LANG_C89:
      case DW_LANG_C:
      case DW_LANG_C_plus_plus:
	if (cval == 0) return; 
	break;
      case DW_LANG_Fortran77:
      case DW_LANG_Fortran90:
	if (cval == 1) return;
	break;
      default:
	break;
    }
#endif
    dwarf_add_AT_signed_const (dw_dbg, die, DW_AT_lower_bound, cval, &dw_error);
  }
  else {
    put_reference (DST_SUBRANGE_TYPE_lower_ref(attr), DW_AT_lower_bound, die);
  }
}


static void
put_subrange_type(DST_flag flag, DST_SUBRANGE_TYPE *attr, Dwarf_P_Die die)
{

  Dwarf_Half att ;

  put_lower_bound (flag, attr, die);

  att = DW_AT_upper_bound;
  if (DST_IS_count(flag))
    att = DW_AT_count;

  if (DST_IS_ub_cval(flag)) {
    dwarf_add_AT_signed_const (dw_dbg, die, att,
		       DST_SUBRANGE_TYPE_upper_cval(attr), &dw_error);
  }
  else {
    put_reference (DST_SUBRANGE_TYPE_upper_ref(attr), att, die);
  }

  /* stride provided if descriptor of (possibly) non-contiguous object */

  if (DST_IS_stride_1byte(flag)) 
    att = DW_AT_MIPS_stride_byte;

  else if (DST_IS_stride_2byte(flag))
    att = DW_AT_MIPS_stride_elem;

  else
    att = DW_AT_MIPS_stride;

  put_reference (DST_SUBRANGE_TYPE_stride_ref(attr),att, die);

}


static void
put_structure_type(DST_flag flag, DST_STRUCTURE_TYPE *attr, Dwarf_P_Die die)
{
  put_decl(DST_STRUCTURE_TYPE_decl(attr), die);
  put_name (DST_STRUCTURE_TYPE_name(attr), die, pb_typename);
  dwarf_add_AT_unsigned_const (dw_dbg, die, DW_AT_byte_size,
		  DST_STRUCTURE_TYPE_byte_size(attr), &dw_error);
  if (DST_IS_declaration(flag)) put_flag (DW_AT_declaration, die);
/* TODO: 
   DST_put_idx_attribute(" abstract_origin",
			 DST_STRUCTURE_TYPE_abstract_origin(attr), FALSE);
*/
}


static void
put_class_type(DST_flag flag, DST_CLASS_TYPE *attr, Dwarf_P_Die die)
{
  put_decl(DST_CLASS_TYPE_decl(attr), die);
  put_name (DST_CLASS_TYPE_name(attr), die, 
      DST_IS_declaration(flag) ? pb_none : pb_typename);
  dwarf_add_AT_unsigned_const (dw_dbg, die, DW_AT_byte_size,
		  DST_CLASS_TYPE_byte_size(attr), &dw_error);
  if (DST_IS_declaration(flag)) put_flag (DW_AT_declaration, die);
/* TODO: 
   DST_put_idx_attribute(" abstract_origin",
			 DST_CLASS_TYPE_abstract_origin(attr), FALSE);
*/
}


static void
put_union_type(DST_flag flag, DST_UNION_TYPE *attr, Dwarf_P_Die die)
{
  put_decl(DST_UNION_TYPE_decl(attr), die);
  put_name (DST_UNION_TYPE_name(attr), die, pb_typename);
  dwarf_add_AT_unsigned_const (dw_dbg, die, DW_AT_byte_size,
		  DST_UNION_TYPE_byte_size(attr), &dw_error);
  if (DST_IS_declaration(flag)) put_flag (DW_AT_declaration, die);
/* TODO:
   DST_put_idx_attribute(" abstract_origin",
			 DST_UNION_TYPE_abstract_origin(attr), FALSE);
*/
}


static void
put_member(DST_flag flag, DST_MEMBER *attr, Dwarf_P_Die die)
{
  Dwarf_P_Expr expr;

  put_decl(DST_MEMBER_decl(attr), die);
  put_name (DST_MEMBER_name(attr), die, pb_none);
  put_reference (DST_MEMBER_type(attr), DW_AT_type, die);
  if (DST_IS_bitfield(flag)) {
    dwarf_add_AT_unsigned_const (dw_dbg, die, DW_AT_byte_size,
		    DST_MEMBER_byte_size(attr), &dw_error);
    dwarf_add_AT_unsigned_const (dw_dbg, die, DW_AT_bit_offset,
		    DST_MEMBER_bit_offset(attr), &dw_error);
    dwarf_add_AT_unsigned_const (dw_dbg, die, DW_AT_bit_size,
		    DST_MEMBER_bit_size(attr), &dw_error);
  }
  put_dopetype (DST_MEMBER_dopetype(attr), flag, die);
  if (DST_IS_declaration(flag)) {
	put_flag (DW_AT_declaration, die);
  }
  else {
	/* C++ static members don't have location;
	 * decl and static flags are set at same time,
	 * so use decl flag. */
  	/* For now, assume that the member location is always a constant. */
  	expr = dwarf_new_expr (dw_dbg, &dw_error);
#ifdef KEY
        // the dwarf spec says that the location expression for a structure member
        // assumes that the location of the struct itself is on the stack.  This
        // implies that we need to add a constant, not just push one
        // (DWARF2 page 41)
  	dwarf_add_expr_gen (expr, DW_OP_plus_uconst, DST_MEMBER_memb_loc(attr), 0, 
		&dw_error);
#else
  	dwarf_add_expr_gen (expr, DW_OP_consts, DST_MEMBER_memb_loc(attr), 0, 
		&dw_error);
#endif
#if pv292951
	/* according to the spec, we should do this, but dbx doesn't like it */
	dwarf_add_expr_gen (expr, DW_OP_plus, 0, 0, &dw_error);
#endif

  	if (expr != NULL) {

	  if (Dwarf_Language == DW_LANG_Fortran90)
	    if (DST_IS_deref(flag)) 
	      dwarf_add_expr_gen (expr, DW_OP_deref, 0, 0, &dw_error);	  

	  dwarf_add_AT_location_expr (dw_dbg, die, 
				      DW_AT_data_member_location, expr, &dw_error);
	}
  }
#ifdef KEY
  // Bug 1419 - add information about accessibility
  if (Dwarf_Language == DW_LANG_C_plus_plus &&
      DST_MEMBER_accessibility(attr) != 0)
    dwarf_add_AT_unsigned_const(dw_dbg, die, DW_AT_accessibility, 
				DST_MEMBER_accessibility(attr), &dw_error);
#endif

}


static void
put_template_type_param(DST_flag flag, DST_TEMPLATE_TYPE_PARAMETER *attr,
			Dwarf_P_Die die)
{
  put_name (DST_TEMPLATE_TYPE_PARAMETER_name(attr), die, pb_none);
  put_reference (DST_TEMPLATE_TYPE_PARAMETER_type(attr), DW_AT_type, die);
}

static void
put_template_value_param(DST_flag flag, DST_TEMPLATE_VALUE_PARAMETER *attr,
			 Dwarf_P_Die die)
{
  put_reference (DST_TEMPLATE_VALUE_PARAMETER_type(attr), DW_AT_type, die);
  put_name (DST_TEMPLATE_VALUE_PARAMETER_name(attr), die, pb_none);
  put_const_attribute (
	  DST_TEMPLATE_VALUE_PARAMETER_cval(attr), DW_AT_const_value, die);
}


static void
put_inheritance(DST_flag flag, DST_INHERITANCE *attr, Dwarf_P_Die die)
{
  Dwarf_P_Expr expr;

  put_reference (DST_INHERITANCE_type(attr), DW_AT_type, die);
  expr = dwarf_new_expr (dw_dbg, &dw_error);
#if defined( KEY)
  // Bug 3107
  // Quote from Dave's last fix:
  // the dwarf spec says that the location expression for a structure member
  // assumes that the location of the struct itself is on the stack.  This
  // implies that we need to add a constant, not just push one
  // (DWARF2 page 41)
  if (DST_INHERITANCE_virtuality(attr) == DW_VIRTUALITY_none) 
    dwarf_add_expr_gen (expr, DW_OP_plus_uconst, DST_INHERITANCE_memb_loc(attr), 
			0, &dw_error);
  else {
    // Bug 1737 - handle virtual base class
    dwarf_add_expr_gen (expr, DW_OP_dup, 0, 0, &dw_error);
    dwarf_add_expr_gen (expr, DW_OP_deref, 0, 0, &dw_error);
    dwarf_add_expr_gen (expr, DW_OP_lit0+DST_INHERITANCE_memb_loc(attr), 
			0, 0, &dw_error);
    dwarf_add_expr_gen (expr, DW_OP_minus, 0, 0, &dw_error);
    dwarf_add_expr_gen (expr, DW_OP_deref, 0, 0, &dw_error);
    dwarf_add_expr_gen (expr, DW_OP_plus, 0, 0, &dw_error);
  }
#else
  dwarf_add_expr_gen (expr, DW_OP_consts, DST_INHERITANCE_memb_loc(attr), 0, 
		      &dw_error);
#endif
  if (expr != NULL) {
    dwarf_add_AT_location_expr (dw_dbg, die, DW_AT_data_member_location, 
	expr, &dw_error);
  if (DST_INHERITANCE_virtuality(attr) != DW_VIRTUALITY_none)
    dwarf_add_AT_unsigned_const(dw_dbg, die, DW_AT_virtuality,
			        DST_INHERITANCE_virtuality(attr),
				&dw_error);
  }
#if defined(KEY)
  // Bug 1419 - add information about accessibility
  dwarf_add_AT_unsigned_const(dw_dbg, die, DW_AT_accessibility, 
  			      DST_INHERITANCE_accessibility(attr), &dw_error);
#endif
}

static void
put_enumeration_type(DST_flag flag, DST_ENUMERATION_TYPE *attr, Dwarf_P_Die die)
{
  put_decl(DST_ENUMERATION_TYPE_decl(attr), die);
  put_name (DST_ENUMERATION_TYPE_name(attr), die, pb_typename);
  dwarf_add_AT_unsigned_const (dw_dbg, die, DW_AT_byte_size,
		  DST_ENUMERATION_TYPE_byte_size(attr), &dw_error);
  if (DST_IS_declaration(flag)) put_flag (DW_AT_declaration, die);
/* TODO: 
   DST_put_idx_attribute(" abstract_origin",
			 DST_ENUMERATION_TYPE_abstract_origin(attr), FALSE);
*/
}


static void
put_enumerator(DST_flag flag, DST_ENUMERATOR *attr, Dwarf_P_Die die)
{
  put_decl(DST_ENUMERATOR_decl(attr), die);
  put_name (DST_ENUMERATOR_name(attr), die, pb_typename);
  put_const_attribute (DST_ENUMERATOR_cval(attr), DW_AT_const_value, die);
}


static void
put_subroutine_type(DST_flag flag, DST_SUBROUTINE_TYPE *attr, Dwarf_P_Die die)
{
  put_decl(DST_SUBROUTINE_TYPE_decl(attr), die);
  put_name (DST_SUBROUTINE_TYPE_name(attr), die, pb_typename);
  put_reference (DST_SUBROUTINE_TYPE_type(attr), DW_AT_type, die);
  if (DST_IS_prototyped(flag)) put_flag (DW_AT_prototyped, die);
/* TODO: 
   DST_put_idx_attribute(" abstract_origin",
			 DST_SUBROUTINE_TYPE_abstract_origin(attr), FALSE);
*/
}

static void
put_entry_point (DST_flag flag, DST_ENTRY_POINT *attr, Dwarf_P_Die die)
{
  put_decl (DST_ENTRY_POINT_decl(attr), die);
  put_name (DST_ENTRY_POINT_name(attr), die, pb_pubname);
  put_reference (DST_ENTRY_POINT_type(attr), DW_AT_type, die);
  dwarf_add_AT_targ_address_b (dw_dbg, die, DW_AT_low_pc, 0,
			       Cg_Dwarf_Symtab_Entry(CGD_ELFSYM,
						     get_elfindex_from_ASSOC_INFO (DST_ENTRY_POINT_st(attr))),
			       &dw_error);
}

static void
put_common_block (DST_flag flag, DST_COMMON_BLOCK *attr, Dwarf_P_Die die)
{
  put_name (DST_COMMON_BLOCK_name(attr), die, pb_none);
  put_location (DST_COMMON_BLOCK_st(attr), 0, flag, die, DW_AT_location);
}

static void
put_common_inclusion (DST_flag flag, DST_COMMON_INCL *attr, Dwarf_P_Die die)
{
  put_decl (DST_COMMON_INCL_decl(attr), die);
  put_reference (DST_COMMON_INCL_com_blk(attr), DW_AT_common_reference, die);
}

#ifdef KEY /* Bug 3507 */
static void
put_imported_decl (DST_flag flag, DST_IMPORTED_DECL *attr, Dwarf_P_Die die)
{
  put_name (DST_IMPORTED_DECL_name(attr), die, pb_none);
  /* DW_AT_import is handled specially inside function
   Cg_Dwarf_Output_Asm_Bytes_Sym_Relocs */
}

static void
put_module (DST_flag flag, DST_MODULE *attr, Dwarf_P_Die die)
{
  put_name (DST_MODULE_name(attr), die, pb_none);
  put_decl (DST_MODULE_decl(attr), die);
}
#endif /* KEY Bug 3507 */

static void 
put_string_type (DST_flag flag, DST_STRING_TYPE *attr, Dwarf_P_Die die)
{
  DST_INFO_IDX ref_idx;
  DST_INFO *info;
  DST_VARIABLE *vattr;

  put_decl (DST_STRING_TYPE_decl(attr), die);
  put_name (DST_STRING_TYPE_name(attr), die, pb_none);
  if (DST_IS_cval (flag)) {
    dwarf_add_AT_unsigned_const (dw_dbg, die, DW_AT_byte_size,
		DST_STRING_TYPE_len_cval(attr), &dw_error);
  }
  else {
    /* In this case, the string length is in a variable, whose location
       expression is to be emitted as the string_length attribute. */
    ref_idx = DST_STRING_TYPE_len_ref(attr);
    if (DST_IS_NULL(ref_idx)) return;
    info = DST_INFO_IDX_TO_PTR(ref_idx);
    vattr = DST_ATTR_IDX_TO_PTR(DST_INFO_attributes(info), DST_VARIABLE);
    put_location (DST_VARIABLE_def_st(vattr), 0, flag, die, DW_AT_string_length);
  }
}

#ifdef KEY
static void
put_namelist (DST_NAMELIST *attr, Dwarf_P_Die die)
{
  put_name (DST_NAMELIST_name(attr), die, pb_none);
}

static void
put_namelist_item (DST_NAMELIST_ITEM *attr, Dwarf_P_Die die)
{
  put_name (DST_NAMELIST_ITEM_name(attr), die, pb_none);
}
#endif

static void
Write_Attributes (
    DST_DW_tag   tag,
    DST_flag     flag,
    DST_ATTR_IDX iattr,
    Dwarf_P_Die  die)
{
  switch (tag) {
    case DW_TAG_compile_unit:
      put_compile_unit(DST_ATTR_IDX_TO_PTR(iattr, DST_COMPILE_UNIT), die);
      dwarf_add_die_to_debug (dw_dbg, die, &dw_error);
      break;
    case DW_TAG_subprogram:
      put_subprogram(flag,
		     DST_ATTR_IDX_TO_PTR(iattr, DST_SUBPROGRAM),
		     die);
      break;
    case DW_TAG_lexical_block:
      put_lexical_block(flag, 
			DST_ATTR_IDX_TO_PTR(iattr, DST_LEXICAL_BLOCK), 
			die);
      break;
    case DW_TAG_inlined_subroutine:
      put_inlined_subroutine(DST_ATTR_IDX_TO_PTR(iattr, DST_INLINED_SUBROUTINE),
			     die);
      break;
    case DW_TAG_label:
      put_label(flag, DST_ATTR_IDX_TO_PTR(iattr, DST_LABEL), die);
      break;
    case DW_TAG_variable:
      put_variable(flag, DST_ATTR_IDX_TO_PTR(iattr, DST_VARIABLE), die);
      break;
    case DW_TAG_formal_parameter:
      put_formal_parameter(
         flag, DST_ATTR_IDX_TO_PTR(iattr, DST_FORMAL_PARAMETER), die);
      break;
    case DW_TAG_constant:
      put_constant( flag, DST_ATTR_IDX_TO_PTR(iattr, DST_CONSTANT), die);
      break;
    case DW_TAG_unspecified_parameters:
      put_unspecified_parameters(
	 flag, 
	 DST_ATTR_IDX_TO_PTR(iattr, DST_UNSPECIFIED_PARAMETERS), die);
      break;
    case DW_TAG_base_type:
      put_basetype(flag, DST_ATTR_IDX_TO_PTR(iattr, DST_BASETYPE), die);
      break;
    case DW_TAG_const_type:
      put_const_type(flag, DST_ATTR_IDX_TO_PTR(iattr, DST_CONST_TYPE), die);
      break;
    case DW_TAG_volatile_type:
      put_volatile_type(flag, 
		DST_ATTR_IDX_TO_PTR(iattr, DST_VOLATILE_TYPE), die);
      break;
    case DW_TAG_pointer_type:
      put_pointer_type(flag, DST_ATTR_IDX_TO_PTR(iattr, DST_POINTER_TYPE), die);
      break;
    case DW_TAG_reference_type:
      put_reference_type(flag, 
			 DST_ATTR_IDX_TO_PTR(iattr, DST_REFERENCE_TYPE) ,die);
      break;
    case DW_TAG_typedef:
      put_typedef(flag, DST_ATTR_IDX_TO_PTR(iattr, DST_TYPEDEF), die);
      break;
    case DW_TAG_ptr_to_member_type:
      put_ptr_to_member_type(flag,
			     DST_ATTR_IDX_TO_PTR(iattr,
						 DST_PTR_TO_MEMBER_TYPE),
			     die);
      break;
    case DW_TAG_array_type:
      put_array_type(flag, DST_ATTR_IDX_TO_PTR(iattr, DST_ARRAY_TYPE), die);
      break;
    case DW_TAG_subrange_type:
      put_subrange_type(flag, 
			DST_ATTR_IDX_TO_PTR(iattr, DST_SUBRANGE_TYPE), die);
      break;
    case DW_TAG_structure_type:
      put_structure_type(flag, 
			 DST_ATTR_IDX_TO_PTR(iattr, DST_STRUCTURE_TYPE), die);
      break;
    case DW_TAG_class_type:
      put_class_type(flag, 
			 DST_ATTR_IDX_TO_PTR(iattr, DST_CLASS_TYPE), die);
      break;
    case DW_TAG_union_type:
      put_union_type(flag, DST_ATTR_IDX_TO_PTR(iattr, DST_UNION_TYPE), die);
      break;
    case DW_TAG_member:
      put_member(flag, DST_ATTR_IDX_TO_PTR(iattr, DST_MEMBER), die);
      break;
    case DW_TAG_inheritance:
      put_inheritance(flag, DST_ATTR_IDX_TO_PTR(iattr, DST_INHERITANCE), die);
      break;
    case DW_TAG_template_type_param:
      put_template_type_param (
	 flag,
	 DST_ATTR_IDX_TO_PTR(iattr, DST_TEMPLATE_TYPE_PARAMETER), die);
      break;
    case DW_TAG_template_value_param:
      put_template_value_param (
	 flag,
	 DST_ATTR_IDX_TO_PTR(iattr, DST_TEMPLATE_VALUE_PARAMETER), die);
      break;
    case DW_TAG_enumeration_type:
      put_enumeration_type(
         flag, 
	 DST_ATTR_IDX_TO_PTR(iattr, DST_ENUMERATION_TYPE), die);
      break;
    case DW_TAG_enumerator:
      put_enumerator(flag, DST_ATTR_IDX_TO_PTR(iattr, DST_ENUMERATOR), die);
      break;
    case DW_TAG_subroutine_type:
      put_subroutine_type(flag, 
	  DST_ATTR_IDX_TO_PTR(iattr, DST_SUBROUTINE_TYPE), die);
      break;
    case DW_TAG_entry_point:
      put_entry_point (flag, 
	  DST_ATTR_IDX_TO_PTR(iattr, DST_ENTRY_POINT), die);
      break;
    case DW_TAG_common_block:
      put_common_block (flag, 
	  DST_ATTR_IDX_TO_PTR(iattr, DST_COMMON_BLOCK), die);
      break;
    case DW_TAG_common_inclusion:
      put_common_inclusion (flag, 
	  DST_ATTR_IDX_TO_PTR(iattr, DST_COMMON_INCL), die);
      break;
    case DW_TAG_string_type:
      put_string_type (flag, 
	  DST_ATTR_IDX_TO_PTR(iattr, DST_STRING_TYPE), die);
    break;
#ifdef KEY
    // Bug 3704 - handle namelist and namelist items for Fortran.
    case DW_TAG_namelist:
      put_namelist (DST_ATTR_IDX_TO_PTR(iattr, DST_NAMELIST), die);
      break;
    case DW_TAG_namelist_item:
      put_namelist_item (DST_ATTR_IDX_TO_PTR(iattr, DST_NAMELIST_ITEM), die);
      break;
#ifdef KEY /* Bug 3507 */
    case DW_TAG_imported_declaration:
      put_imported_decl(flag, 
                        DST_ATTR_IDX_TO_PTR(iattr, DST_IMPORTED_DECL), die);
      break;
    case DW_TAG_module:
      put_module(flag, DST_ATTR_IDX_TO_PTR(iattr, DST_MODULE), die);
      break;
#endif /* KEY Bug 3507 */
#endif
      
   default:
      ErrMsg (EC_Unimplemented, "Write_Attributes: TAG not handled");
      break;
  }
  if (DST_IS_artificial(flag)) 
      put_flag(DW_AT_artificial, die);
}


/* Add 'die' to the CGD_enclosing_proc array at position 'level'. */
static void Set_Enclosing_Die (Dwarf_P_Die die, mINT32 level)
{
  /* see if we need to increase the size of our enclosing-proc table.  */
  if ( level >= CGD_enclosing_proc_max ) {
    /* get some minimal # more than necessary */
    CGD_enclosing_proc_max = level + 8;

    if ( CGD_enclosing_proc == NULL ) {
      CGD_enclosing_proc = 
	  (Dwarf_P_Die *) calloc( CGD_enclosing_proc_max, 
				  sizeof(*CGD_enclosing_proc) );
    }
    else {
      CGD_enclosing_proc = 
	  (Dwarf_P_Die *) realloc( CGD_enclosing_proc,
	    CGD_enclosing_proc_max * sizeof(*CGD_enclosing_proc) );
    }
  }
  CGD_enclosing_proc[level] = die;
}


#ifdef KEY
// Return TRUE if the DST info should be emitted.
static BOOL
Should_Emit (DST_INFO *info)
{
  if (CG_prune_unused_debug_types &&
      !DST_IS_should_emit(DST_INFO_flag(info))) {
    switch (DST_INFO_tag(info)) {
      case DW_TAG_base_type:
      case DW_TAG_const_type:
      case DW_TAG_volatile_type:
      case DW_TAG_pointer_type:
      case DW_TAG_reference_type:
      case DW_TAG_typedef:
      case DW_TAG_ptr_to_member_type:
      case DW_TAG_array_type:
      case DW_TAG_subrange_type:
      case DW_TAG_structure_type:
      case DW_TAG_class_type:
      case DW_TAG_union_type:
      case DW_TAG_enumeration_type:
      case DW_TAG_subroutine_type:
      case DW_TAG_string_type:
	return FALSE;
    }
  }
  return TRUE;
}

// Do a preorder traversal of the DST node 'idx' and all its children.  Mark
// them as needed to be emitted.
static void
preorder_visit_mark_emit (
  DST_INFO_IDX idx, 
  BOOL visit_children)
{
  DST_INFO *info;
  DST_INFO_IDX child_idx;
  DST_DW_tag tag;
  DST_flag flag;


  info = DST_INFO_IDX_TO_PTR(idx);
  tag = DST_INFO_tag(info);
  flag = DST_INFO_flag(info);

  // Return if node is already marked as "should emit".
  if (DST_IS_should_emit(DST_INFO_flag(info)))
    return;

  // Mark the node as "should emit".  Reset info_mark to make preorder_visit
  // (re)visit the node.
  DST_SET_should_emit(DST_INFO_flag(info));
  DST_RESET_info_mark(DST_INFO_flag(info));

  // Visit the types referenced by INFO.
  switch (tag) {
    case DW_TAG_typedef:
      {
	DST_ATTR_IDX iattr = DST_INFO_attributes(info);
	DST_TYPEDEF *attr = DST_ATTR_IDX_TO_PTR(iattr, DST_TYPEDEF);
	DST_INFO_IDX type_idx = DST_TYPEDEF_type(attr);
        if (!DST_IS_NULL(type_idx)) {	// bug 14844
	  DST_INFO *type_info = DST_INFO_IDX_TO_PTR(type_idx);
	  if (!Should_Emit(type_info)) {
	    preorder_visit_mark_emit(type_idx, TRUE);
	    // Add to revisit list because preorder_visit doesn't visit
	    // DST_TYPEDEF_type.
	    Add_To_Revisit_List(type_idx);
	  }
        }
      }
      break;
    case DW_TAG_pointer_type:
      {
	DST_ATTR_IDX iattr = DST_INFO_attributes(info);
	DST_POINTER_TYPE *attr = DST_ATTR_IDX_TO_PTR(iattr, DST_POINTER_TYPE);
	DST_INFO_IDX type_idx = DST_POINTER_TYPE_type(attr);
        if (!DST_IS_NULL(type_idx)) {	// bug 14844
	  DST_INFO *type_info = DST_INFO_IDX_TO_PTR(type_idx);
	  if (!Should_Emit(type_info)) {
	    preorder_visit_mark_emit(type_idx, TRUE);
	    // Add to revisit list because preorder_visit doesn't visit
	    // DST_POINTER_TYPE_type.
	    Add_To_Revisit_List(type_idx);
	  }
        }
      }
      break;
  }

  if (tag == DW_TAG_inlined_subroutine && visit_children) {
	/* if inlined and no body left, then don't visit children */
	ST *st;
	DST_INFO_IDX ref_idx = DST_INLINED_SUBROUTINE_abstract_origin(
		DST_ATTR_IDX_TO_PTR(
		    DST_INFO_attributes(info), DST_INLINED_SUBROUTINE));
        if (!DST_IS_NULL(ref_idx) && !DST_IS_FOREIGN_OBJ (ref_idx)) {
  		info = DST_INFO_IDX_TO_PTR (ref_idx);
  		st = Get_ST_From_DST(DST_SUBPROGRAM_def_st(
			DST_ATTR_IDX_TO_PTR(
			    DST_INFO_attributes(info), DST_SUBPROGRAM)));
  		if (st != NULL && ST_is_not_used(st)) {
			visit_children = FALSE;
		}
	}
  }
 
  if (!visit_children)
    return;

  /* Now, visit each child */
  for (child_idx = DST_first_child (idx);
       !DST_IS_NULL(child_idx); 
       child_idx = DST_INFO_sibling(DST_INFO_IDX_TO_PTR(child_idx)))
  {
    info = DST_INFO_IDX_TO_PTR (child_idx);
    if ((DST_INFO_tag(info) != DW_TAG_subprogram &&
         DST_INFO_tag(info) != DW_TAG_entry_point)
	|| DST_IS_declaration(DST_INFO_flag(info)))
    {
      preorder_visit_mark_emit(child_idx, visit_children);
    }
  }
}
#endif


/* Do a preorder traversal of the DST node 'idx' and all its children. The
 * 'parent' and/or 'left_sibling' parameters indicate where in the 
 * debug_info tree the die for idx should be attached. The 'tree_level'
 * parameter indicates the level for the 'idx' node in the DST tree.
 */
static Dwarf_P_Die
preorder_visit (
  DST_INFO_IDX idx, 
  Dwarf_P_Die parent, 
  Dwarf_P_Die left_sibling,
  INT tree_level,
  BOOL visit_children)
{
  DST_INFO *info;
  DST_INFO_IDX child_idx;
  Dwarf_P_Die die;
  DST_DW_tag tag;
  DST_flag flag;

  info = DST_INFO_IDX_TO_PTR (idx);
  tag = DST_INFO_tag (info);
  flag = DST_INFO_flag (info);
  die = (Dwarf_P_Die) DST_INFO_dieptr (info);

  /* check if a die has already been generated for the DST. This can 
   * happen, if it was a type die that was referenced earlier. 
   * However, we still need to link in the die at the right place and 
   * write out its attributes.
   */
  if (die == NULL) {
    die = dwarf_new_die (dw_dbg, tag, parent, NULL, left_sibling, NULL, 
			    &dw_error);
    DST_INFO_dieptr(info) = die;
    if (Trace_Dwarf) {
      fprintf (TFile,"NEW die for [%d,%d]: %p, parent: %p, left_sibling:%p, tag:%d\n",
	idx.block_idx, idx.byte_idx, die, parent, left_sibling, tag);
    }
  }
  #ifdef KEY /* Bug 15029 */
  else if (DST_INFO_tag(info) == DW_TAG_base_type && 0) {
    /* What a mess. Function Traverse_Global_DST() calls
     * DST_SET_info_mark() to indicate that it has already linked a DIE
     * into the Dwarf output tree, preventing it from being linked a second
     * time.
     *
     * However, if a program unit in the DST_INFO tree has alternate entry
     * points, the function Traverse_DST_Mark_Emit() calls
     * preorder_visit_mark_emit() to force the emission of any sibling which
     * is a DW_TAG_entry_point or a DW_TAG_subprogram; part of that is ading
     * a "do_emit" mark and part of it is removing the "info_mark". It's not
     * clear why alternate entries wouldn't get emitted in due course anyway,
     * but never mind.
     *
     * Unfortunately, Traverse_DST_Mark_Emit() also removes the mark from a
     * subsequent sibling which is a DW_TAG_base_type. A long-ago comment in
     * Traverse_DST_Mark_Emit() questions why it does so. I observe that this
     * can cause a base type to be linked into the output tree twice,
     * corrupting the tree: the first link occurs when Traverse_Global_DST()
     * traverses the list of siblings ignoring subprograms, and the second link
     * occurs when it travels the list again, processing subprograms.
     *
     * I'm afraid to remove the test for DW_TAG_base_type inside
     * Traverse_DST_Mark_Emit() since I suspect there's some good but
     * unknown reason for it, so as a band-aid I'm simply doing nothing
     * when a node has already been linked in. Regardless of the tag, it can't
     * ever be valid to put the same node into the output tree twice.
     */
    return die;
  }
  #endif /* KEY Bug 15029 */
  else {
    dwarf_die_link (die, parent, NULL, left_sibling, NULL, &dw_error);
    if (Trace_Dwarf) {
      fprintf (TFile,"link die for [%d,%d]: %p, parent: %p, left_sibling:%p, tag:%d\n",
	idx.block_idx, idx.byte_idx, die, parent, left_sibling, tag);
    }
  }

#ifdef KEY
  Current_Top_Level_Idx = idx;
#endif
  //Current_Tree_Level = tree_level;
  Write_Attributes (tag,
		   flag,
		   DST_INFO_attributes(info),
		   die);
  if (tag == DW_TAG_inlined_subroutine && visit_children) {
	/* if inlined and no body left, then don't visit children */
	ST *st;
	DST_INFO_IDX ref_idx = DST_INLINED_SUBROUTINE_abstract_origin(
		DST_ATTR_IDX_TO_PTR(
		    DST_INFO_attributes(info), DST_INLINED_SUBROUTINE));
        if (!DST_IS_NULL(ref_idx) && !DST_IS_FOREIGN_OBJ (ref_idx)) {
  		info = DST_INFO_IDX_TO_PTR (ref_idx);
  		st = Get_ST_From_DST(DST_SUBPROGRAM_def_st(
			DST_ATTR_IDX_TO_PTR(
			    DST_INFO_attributes(info), DST_SUBPROGRAM)));
  		if (st != NULL && ST_is_not_used(st)) {
			visit_children = FALSE;
		}
	}
  }
 
  if (!visit_children) return die;
  /* Now, visit each child */
  parent = die;
  left_sibling = NULL;
  for (child_idx = DST_first_child (idx);
       !DST_IS_NULL(child_idx); 
       child_idx = DST_INFO_sibling(DST_INFO_IDX_TO_PTR(child_idx)))
  {
    info = DST_INFO_IDX_TO_PTR (child_idx);
#ifdef KEY /* Bug 3507 */
    if ((DST_INFO_tag(info) != DW_TAG_subprogram &&
          DST_INFO_tag(info) != DW_TAG_entry_point)
	|| DST_IS_declaration(DST_INFO_flag(info)) )
#else /* KEY Bug 3507 */
    if (DST_INFO_tag(info) != DW_TAG_subprogram
	|| DST_IS_declaration(DST_INFO_flag(info)) )
#endif /* KEY Bug 3507 */
    {
      left_sibling = 
        preorder_visit (child_idx, parent, left_sibling, tree_level + 1, visit_children);

      /* after the first child is visited, the parent is set to NULL */
      parent = NULL;
    }
  }
  return die;
}

#ifdef KEY /* Bug 3507 */
#include <map>
using namespace std;
/*
 * The code generator traverses the top-level procedures without regard
 * to whether they belong to any module. We use the DST_INFO constructed
 * by the front end to create a mapping from each procedure which is an
 * immediate child of the module, onto the module itself, so that we can
 * attach those procedures to the module DIE rather than the compilation
 * unit DIE as we otherwise would.
 *
 * Procedures nested inside procedures get handled correctly by the code
 * generator's normal traversal, without any special treatment.
 */
typedef map<DST_INFO *, Dwarf_P_Die> DST_INFO_map_t;
static DST_INFO_map_t DST_INFO_map;

/*
 * For the specified parent, add to DST_INFO_map a mapping from the
 * DST_INFO for each of its children onto the corresponding
 * parent_p_die.
 */
static void
build_map(DST_INFO_IDX parent_idx, Dwarf_P_Die parent_p_die)
{
  for (DST_INFO_IDX child_idx = DST_first_child(parent_idx);
       !DST_IS_NULL(child_idx); 
       child_idx = DST_INFO_sibling(DST_INFO_IDX_TO_PTR(child_idx)))
  {
    DST_INFO *child_info = DST_INFO_IDX_TO_PTR (child_idx);
    int tag = DST_INFO_tag(child_info);
    if (tag == DW_TAG_subprogram || tag == DW_TAG_entry_point) {
      DST_INFO_map[child_info] = parent_p_die;
    }
  }
}
#endif /* KEY Bug 3507 */


/* traverse all DSTs and handle the non-pu info */
static void
Traverse_Global_DST (void)
{
  DST_INFO_IDX idx;
  DST_INFO *info;
  Dwarf_P_Die parent;

  if (Trace_Dwarf) {
	fprintf(TFile, "Trace Global DST\n");
  }

  /* visit subsequent siblings at this level */
  for (idx = DST_first_child (cu_idx);	/* start at beginning */
       !DST_IS_NULL(idx);
       idx = DST_INFO_sibling(DST_INFO_IDX_TO_PTR(idx)))
  {
    info = DST_INFO_IDX_TO_PTR (idx);

#if defined(KEY)
    if (!Should_Emit(info))
	continue;
    if (DST_IS_info_mark(DST_INFO_flag(info)))
	continue;				// already traversed
#endif

    /* only traverse non-subp's */
    if (DST_INFO_tag(info) == DW_TAG_subprogram
      || DST_INFO_tag(info) == DW_TAG_entry_point)
	continue;	/* will traverse later */

    /* All DST entries other than subprograms are attached as children
     * of the compile_unit die.
     */
    parent = CGD_enclosing_proc[GLOBAL_LEVEL];
#if defined( KEY) /* Bug 3507 */
    Dwarf_P_Die die = preorder_visit (idx, parent, NULL, LOCAL_LEVEL,
      TRUE /* visit children */);
    if (DST_INFO_tag(info) == DW_TAG_module) {
      build_map(idx, die);
    }
#else /* KEY Bug 3507 */
    (void) preorder_visit (idx, parent, NULL, LOCAL_LEVEL, TRUE /* visit children */);
#endif /* KEY Bug 3507 */
    DST_SET_info_mark(DST_INFO_flag(info));	/* mark has been traversed */
  }
}

static void
Emit_One_DST (DST_INFO_IDX idx)
{
  Dwarf_P_Die parent;
  BOOL inlined_subp;
  BOOL visit_children;
  DST_INFO *info = DST_INFO_IDX_TO_PTR(idx);

#ifdef KEY
  // Don't emit DSTs that are not really used.
  if (!Should_Emit(info))
    return;
#endif

  if (DST_IS_info_mark(DST_INFO_flag(info)))
    return;	/* already traversed */

  /* check for inlined subprogram that hasn't been traversed yet;
   * this implies that the PU was optimized away, so we can't access
   * any of the info below the subprogram, but we still want the basic
   * subprogram info to be available to dbx. */
  if (DST_INFO_tag(info) == DW_TAG_subprogram
      && ! DST_IS_declaration(DST_INFO_flag(info)) )
  {
      DST_SUBPROGRAM *PU_attr = DST_ATTR_IDX_TO_PTR(DST_INFO_attributes(info), DST_SUBPROGRAM);
      inlined_subp = subprogram_def_is_inlined(PU_attr);
      visit_children = FALSE;
  }
  else {
      visit_children = TRUE;
      inlined_subp = FALSE;
  }
  /* only traverse inlined subp's or non-subp's */
  if ((DST_INFO_tag(info) != DW_TAG_subprogram
       && DST_INFO_tag(info) != DW_TAG_entry_point)
      || DST_IS_declaration(DST_INFO_flag(info))
      || inlined_subp )
    {
      /* All DST entries other than subprograms are attached as children
       * of the compile_unit die.
       */
      parent = CGD_enclosing_proc[GLOBAL_LEVEL];
      (void) preorder_visit (idx, parent, NULL, LOCAL_LEVEL, visit_children);
      DST_SET_info_mark(DST_INFO_flag(info));	/* mark has been traversed */
    }
}

/* traverse all DSTs and handle anything we missed */
static void
Traverse_Extra_DST (void)
{
  DST_INFO_IDX idx;
  DST_INFO *info;

  if (Trace_Dwarf) {
	fprintf(TFile, "Trace Extra DST\n");
  }
  /* visit subsequent siblings at this level */
  for (idx = DST_first_child (cu_idx);	/* start at beginning */
       !DST_IS_NULL(idx);
       idx = DST_INFO_sibling(DST_INFO_IDX_TO_PTR(idx)))
  {
    Emit_One_DST(idx);
  }
}

/* ====================================================================
 *
 * Traverse the DST from for the pu idx.
 *
 * ====================================================================
 */
static void
Traverse_DST (ST *PU_st, DST_IDX pu_idx)
{
  DST_INFO *info;
  DST_SUBPROGRAM *PU_attr;
  Dwarf_P_Die die;
  Dwarf_P_Die parent;
  INT nestlevel;
  DST_ASSOC_INFO assoc_info;

  info = DST_INFO_IDX_TO_PTR (pu_idx);
  FmtAssert ( (DST_INFO_tag(info) == DW_TAG_subprogram)
	&& !DST_IS_declaration(DST_INFO_flag(info)),
	("Traverse_DST:  pu_idx is not a subprogram def?"));
  PU_attr = DST_ATTR_IDX_TO_PTR(DST_INFO_attributes(info), DST_SUBPROGRAM);
  assoc_info = DST_SUBPROGRAM_def_st(PU_attr);
#ifndef KEY
  FmtAssert ( (DST_ASSOC_INFO_st_level(assoc_info) == ST_level(PU_st))
	&& (DST_ASSOC_INFO_st_index(assoc_info) == ST_index(PU_st)),
	("Traverse_DST:  pu_idx doesn't match PU_st"));
#else
  // Bug 6679 - when a PU is declared extern and later defined in 
  // the current file with an attribute, then there will be two ST entries 
  // corresponding to that PU. If that is the case, then the assoc_info may 
  // point to a different st_index than the PU_st. In such cases, we make use 
  // of the fact that there can be utmost one PU with a given name at a given 
  // level.
  FmtAssert ( (DST_ASSOC_INFO_st_level(assoc_info) == ST_level(PU_st))
	      && (strcmp(ST_name(Get_ST_From_DST (assoc_info)), 
			 ST_name(PU_st)) == 0),
	("Traverse_DST:  pu_idx doesn't match PU_st"));  
#endif
  if (Trace_Dwarf)
    fprintf ( TFile, "Traverse_DST:  found idx [%d,%d] for %s\n",
	pu_idx.block_idx, pu_idx.byte_idx, ST_name(PU_st));

  DST_SET_info_mark(DST_INFO_flag(info));	/* mark has been traversed */
  // in new symtab, local level starts at 2.
  // so reset that to 1, and keep globals combined into 0.
  nestlevel = CURRENT_SYMTAB - 1;
  parent = CGD_enclosing_proc[nestlevel-1];
#ifdef KEY /* Bug 3507 */
  /* Special case: procedure is an immediate child of a module */
  if (DST_INFO_map.find(info) != DST_INFO_map.end()) {
    parent = DST_INFO_map[info];
  }
#endif /* KEY Bug 3507 */
  die = preorder_visit (pu_idx, parent, NULL, nestlevel, TRUE /*visit_children*/);
  Set_Enclosing_Die (die, nestlevel);

  if (PU_has_altentry(Get_Current_PU())) {
	/* also process any sibling entries at this time */
	DST_INFO_IDX idx;
	idx = DST_INFO_sibling(DST_INFO_IDX_TO_PTR(pu_idx));
	while ( !DST_IS_NULL(idx) ) {
		info = DST_INFO_IDX_TO_PTR(idx);
		// why do we have base_type and subroutine_type here?
		// I assume cause can be in middle of sibling list
		// and want to keep searching. 
		// but then need to later check whether has been 
		// already traversed.
		if ( ! (DST_INFO_tag(info) == DW_TAG_entry_point
		     || DST_INFO_tag(info) == DW_TAG_base_type
		     || DST_INFO_tag(info) == DW_TAG_subroutine_type
		     || (DST_INFO_tag(info) == DW_TAG_subprogram
			&& DST_IS_declaration(DST_INFO_flag(info)) ) ))
		{
			break;	/* non-entry found, so stop */
		}
    		if ( ! DST_IS_info_mark(DST_INFO_flag(info))) {
			DST_SET_info_mark(DST_INFO_flag(info));	/* mark has been traversed */
			die = preorder_visit (idx, NULL, die /*sibling*/, nestlevel, TRUE);
		}
		idx = DST_INFO_sibling(info);
	}
  }
}


#ifdef KEY
/* ====================================================================
 *
 * Traverse the DST from the pu idx.  Mark them as "should emit".
 *
 * ====================================================================
 */
static void
Traverse_DST_mark_emit (ST *PU_st, DST_IDX pu_idx)
{
  DST_INFO *info;
  DST_SUBPROGRAM *PU_attr;
  DST_ASSOC_INFO assoc_info;

  info = DST_INFO_IDX_TO_PTR (pu_idx);
  FmtAssert((DST_INFO_tag(info) == DW_TAG_subprogram)
	     && !DST_IS_declaration(DST_INFO_flag(info)),
	    ("Traverse_DST_mark_emit:  pu_idx is not a subprogram def?"));
  PU_attr = DST_ATTR_IDX_TO_PTR(DST_INFO_attributes(info), DST_SUBPROGRAM);
  assoc_info = DST_SUBPROGRAM_def_st(PU_attr);

  // Bug 6679 - when a PU is declared extern and later defined in 
  // the current file with an attribute, then there will be two ST entries 
  // corresponding to that PU. If that is the case, then the assoc_info may 
  // point to a different st_index than the PU_st. In such cases, we make use 
  // of the fact that there can be utmost one PU with a given name at a given 
  // level.
  FmtAssert((DST_ASSOC_INFO_st_level(assoc_info) == ST_level(PU_st))
	     && (!strcmp(ST_name(Get_ST_From_DST(assoc_info)), ST_name(PU_st))),
	    ("Traverse_DST_mark_emit:  pu_idx doesn't match PU_st"));  

  preorder_visit_mark_emit(pu_idx, TRUE /*visit_children*/);

  if (PU_has_altentry(Get_Current_PU())) {
	/* also process any sibling entries at this time */
	DST_INFO_IDX idx;
	idx = DST_INFO_sibling(DST_INFO_IDX_TO_PTR(pu_idx));
	while ( !DST_IS_NULL(idx) ) {
		info = DST_INFO_IDX_TO_PTR(idx);
		// why do we have base_type and subroutine_type here?
		// I assume cause can be in middle of sibling list
		// and want to keep searching. 
		// but then need to later check whether has been 
		// already traversed.
		if ( ! (DST_INFO_tag(info) == DW_TAG_entry_point
		     || DST_INFO_tag(info) == DW_TAG_base_type
		     || DST_INFO_tag(info) == DW_TAG_subroutine_type
		     || (DST_INFO_tag(info) == DW_TAG_subprogram
			&& DST_IS_declaration(DST_INFO_flag(info)) ) ))
		{
			break;	/* non-entry found, so stop */
		}
    		if (!DST_IS_should_emit(DST_INFO_flag(info))) {
			preorder_visit_mark_emit(idx, TRUE);
		}
		idx = DST_INFO_sibling(info);
	}
  }
}

/* ====================================================================
 *
 * Revisit list to keep track of DSTs that need to be emitted.
 *
 * ====================================================================
 */

static std::vector<DST_INFO_IDX> revisit_list;

static void
Clear_Revisit_List ()
{
  revisit_list.clear();
}

static void
Add_To_Revisit_List (DST_INFO_IDX idx)
{
  revisit_list.push_back(idx);
}

// Emit all the DSTs in the revisit list.
static void
Traverse_Revisit_List ()
{
  int i;

  // Emit_One_DST may add more DSTs to the list.  Keep on traversing the list
  // until no more DST is added.
  for (i = 0; i < revisit_list.size(); i++) {
    DST_INFO_IDX idx = revisit_list[i];
    Emit_One_DST(idx);
  }
}
#endif

void 
Cg_Dwarf_Process_PU (Elf64_Word	scn_index,
		     LABEL_IDX  begin_label,
		     LABEL_IDX  end_label,
#ifdef TARG_X8664
		     LABEL_IDX *eh_pushbp_label,
		     LABEL_IDX *eh_movespbp_label,
		     LABEL_IDX *eh_adjustsp_label,
		     LABEL_IDX *eh_callee_saved_reg,
		     LABEL_IDX *first_bb_labels,
		     LABEL_IDX *last_bb_labels,
		     INT32     pu_entries,
#elif defined(TARG_MIPS)
		     LABEL_IDX *eh_adjustsp_label,
		     LABEL_IDX *eh_callee_saved_reg,
		     INT32 new_cfa_offset,
#endif // TARG_X8664 || TARG_MIPS
		     INT32      end_offset,
		     ST        *PU_st,
		     DST_IDX    pu_dst,
		     Elf64_Word eh_index,
		     INT        eh_offset,
		     // The following two arguments need to go away
		     // once libunwind provides an interface that lets
		     // us specify ranges symbolically.
		     INT        low_pc,
		     INT        high_pc)
{
  DST_INFO *info;
  Dwarf_P_Die PU_die;
  Dwarf_P_Expr expr;
  DST_SUBPROGRAM *PU_attr;
  static BOOL processed_globals = FALSE;
  
  cur_text_index = scn_index;
  Trace_Dwarf = Get_Trace ( TP_EMIT, 8 );	// for each pu

  /* turn off generation of dwarf information from DSTs. */
  if (Disable_DST) return;

#ifdef KEY
  // Traverse the PU to mark the global DSTs that are actually referenced, then
  // emit only those DSTs.
  if (!DST_IS_NULL(pu_dst)) {	// cloning may create subp with no DST info
    Traverse_DST_mark_emit(PU_st, pu_dst);
    Traverse_Global_DST();	// emit global types before PUs
  }
#else
  static BOOL processed_globals = FALSE;
  // [CL] No longer process globals first: In order to generate only
  // 'necessary' info, we generate all PUs first, then remaining
  // globals, and only needed types.  This is to avoid generating type
  // pointers to types with only local scope where local scope has
  // been discarded by DFE
  if ( ! processed_globals) {
	// do this once, before PU info.
	// we do this here rather than in Begin routine
	// cause have to wait until globals are allocated.
	Traverse_Global_DST ();	// emit global types before PUs
	processed_globals = TRUE;
  }
#endif

  if (Trace_Dwarf)
	fprintf(TFile, "dwarf for %s:\n", ST_name(PU_st));

  if (DST_IS_NULL(pu_dst)) {
    /* cloning may create subp with no DST info */
    if (strncmp(ST_name(PU_st), "*clone*", 7) != 0)
      DevWarn("NULL DST passed to CG for %s", ST_name(PU_st));
    return;
  }

  Traverse_DST (PU_st, pu_dst);

  if (DST_IS_NULL(pu_dst)) return;

  info = DST_INFO_IDX_TO_PTR (pu_dst);
  /* if current die is not for the subprogram, we cannot add any attributes. */
  if (DST_INFO_tag(info) != DW_TAG_subprogram) return;

  PU_attr = DST_ATTR_IDX_TO_PTR(DST_INFO_attributes(info), DST_SUBPROGRAM);
  PU_die = (Dwarf_P_Die) DST_INFO_dieptr(info);

  if (subprogram_def_is_inlined(PU_attr)) {
	/* don't generate frame info for inlined subp,
	 * but do generate a new die for concrete out-of-line instance
	 * that does have frame info. */
	Dwarf_P_Die idie = dwarf_new_die (dw_dbg, DW_TAG_subprogram, NULL, NULL, PU_die, NULL, &dw_error);
	if (Trace_Dwarf) {
		fprintf (TFile,"NEW subprogram concrete instance: die: %p, left_sibling:%p\n",
		idie, PU_die);
	}
	/* has same attributes as inlined_subprogram,
	 * though a different tag. */
	put_concrete_subprogram(pu_dst,
				0 /* low_pc */,
				0 /* high_pc */,
				idie);
	PU_die = idie;
  }

  /* setup the frame_base attribute. */
  expr = dwarf_new_expr (dw_dbg, &dw_error);
#ifndef TARG_X8664
  if (Current_PU_Stack_Model != SMODEL_SMALL)
    dwarf_add_expr_gen (expr, DW_OP_bregx,
	REGISTER_machine_id (TN_register_class(FP_TN), TN_register(FP_TN)),
        0, &dw_error);
  else
    dwarf_add_expr_gen (expr, DW_OP_bregx,
	REGISTER_machine_id (TN_register_class(SP_TN), TN_register(SP_TN)),
        Frame_Len, &dw_error);
#else
  // seems that gdb 5.x doesn't like bregx for AT_frame_base.
  if (Current_PU_Stack_Model != SMODEL_SMALL)
    dwarf_add_expr_gen (expr, 
    			Is_Target_64bit() ? DW_OP_reg6 : DW_OP_reg5, 
			0, 0, &dw_error) ;
  else
    dwarf_add_expr_gen (expr, 
                        Is_Target_64bit() ? DW_OP_breg7 : DW_OP_breg4, 
			Frame_Len, 0, &dw_error) ;           
#if 0
  /* isa_registers.cxx order of registers is different from that of gdb 
   * Try "info registers".  
   * If we change the order in isa_registers now, it is total chaos.
   */
  if (Current_PU_Stack_Model != SMODEL_SMALL)
    dwarf_add_expr_gen (expr, DW_OP_bregx,
	6 /* REGISTER_machine_id (TN_register_class(FP_TN), TN_register(FP_TN)) */,
	0, &dw_error);
  else
    dwarf_add_expr_gen (expr, DW_OP_bregx,
	7 /* REGISTER_machine_id (TN_register_class(SP_TN), TN_register(SP_TN)) */,
	Frame_Len, &dw_error);
#endif 
#endif /* TARG_X8664 */

  dwarf_add_AT_location_expr(dw_dbg, PU_die, DW_AT_frame_base, expr, &dw_error);
  if (PU_is_mainpu(ST_pu(PU_st))) {
   	dwarf_add_AT_unsigned_const (dw_dbg, PU_die, DW_AT_calling_convention, 
	     DW_CC_program, &dw_error);
  }

  Dwarf_Unsigned begin_entry = Cg_Dwarf_Symtab_Entry(CGD_LABIDX,
						     begin_label,
						     scn_index);
  Dwarf_Unsigned end_entry   = Cg_Dwarf_Symtab_Entry(CGD_LABIDX,
						     end_label,
						     scn_index);

  Dwarf_Unsigned callee_saved_reg;
  INT num_callee_saved_regs;
  if (num_callee_saved_regs = Cgdwarf_Num_Callee_Saved_Regs())
    callee_saved_reg = Cg_Dwarf_Symtab_Entry(CGD_LABIDX,
					     eh_callee_saved_reg[0],
					     scn_index);      

  // Generate .eh_frame FDE only for C++ or when -DEBUG:eh_frame=on
  if (Dwarf_Language == DW_LANG_C_plus_plus || DEBUG_Emit_Ehframe) {
    const char *personality = "__gxx_personality_v0";
    unsigned personality_encoding = 3;
    unsigned lsda_encoding = 3;
    if (Gen_PIC_Call_Shared || Gen_PIC_Shared) {
      personality = "DW.ref.__gxx_personality_v0";
      lsda_encoding = 0x1b;
      personality_encoding = 0x9b;
    }
    if (eh_offset != DW_DLX_NO_EH_OFFSET) {
      fprintf(Asm_File, ".cfi_lsda %u, .range_table.%s\n", lsda_encoding, ST_name(PU_st));
      fprintf(Asm_File, ".cfi_personality %u, %s\n", personality_encoding, personality);
    }
  }

  Dwarf_Unsigned eh_handle;

  if (eh_offset != DW_DLX_NO_EH_OFFSET) {
    eh_handle = Cg_Dwarf_Symtab_Entry(CGD_ELFSYM, eh_index);
  }
  else {
    eh_handle = eh_index;
  }

  Em_Dwarf_Process_PU (begin_entry,
		       end_entry,
		       0,	// begin_offset
		       end_offset,
		       PU_die);
#ifdef TARG_X8664
  if (pu_entries > 1) {
    FmtAssert(Dwarf_Language == DW_LANG_Fortran77 ||
              Dwarf_Language == DW_LANG_Fortran90, 
	      ("Dwarf handler: > 1 PU entry in a non-Fortran language"));
    for (INT pu_entry = 1; pu_entry <= pu_entries; pu_entry ++) {
      if (num_callee_saved_regs = Cgdwarf_Num_Callee_Saved_Regs())
	callee_saved_reg = Cg_Dwarf_Symtab_Entry(CGD_LABIDX,
						 eh_callee_saved_reg[pu_entry],
						 scn_index);      
      // XXX: .cfi_startproc and .cfi_endproc in this case?
      Em_Dwarf_Add_PU_Entries (begin_entry,
			       end_entry,
			       0,	// begin_offset
			       end_offset,
			       PU_die);
    }
  }
#endif

}

static DST_language
Get_Dwarf_Language (DST_INFO *cu_info)
{
	return DST_COMPILE_UNIT_language( DST_ATTR_IDX_TO_PTR(
		DST_INFO_attributes(cu_info), DST_COMPILE_UNIT));
}

/* Initialize generation of dwarf information. The file table is emitted
 * in this routine. The compile_unit die is also emitted here.
 */
void
Cg_Dwarf_Begin (BOOL is_64bit)
{
  DST_INFO *   cu_info;
  Dwarf_P_Die  cu_die;

  Trace_Dwarf = Get_Trace ( TP_EMIT, 8 );
  Disable_DST = Get_Trace ( TP_EMIT, 0x200 );

  cu_idx = DST_get_compile_unit ();
  cu_info = DST_INFO_IDX_TO_PTR (cu_idx);
  Dwarf_Language = Get_Dwarf_Language (cu_info);

#ifdef KEY
  if (!CG_emit_unwind_info)
  dw_dbg = Em_Dwarf_Begin(is_64bit, Trace_Dwarf, 
			  0,
			  Cg_Dwarf_Enter_Elfsym);
  else
#endif
  dw_dbg = Em_Dwarf_Begin(is_64bit, Trace_Dwarf, 
			  (Dwarf_Language == DW_LANG_C_plus_plus),
			  Cg_Dwarf_Enter_Elfsym);

  /* Read in the compile unit entry */
  cu_die = get_ref_die (cu_idx);
  Set_Enclosing_Die (cu_die, GLOBAL_LEVEL);
  Write_Attributes (DST_INFO_tag(cu_info), 
		    DST_INFO_flag (cu_info), 
		    DST_INFO_attributes(cu_info), 
		    cu_die);

  // Invalid entry up front to keep from using the zero index.
  CGD_Symtab.push_back(CGD_SYMTAB_ENTRY(CGD_ELFSYM, (Dwarf_Unsigned) -1));

#ifdef KEY
  Clear_Revisit_List();
#endif
}


/* go through any remaining DST entries after the last subprogram. This
   also handles the case of a file with no PUs.
*/
void Cg_Dwarf_Finish (pSCNINFO text_scninfo)
{
  if (Disable_DST) return;

   Traverse_Extra_DST();	/* do final pass for any info not emitted yet */
#ifdef KEY
  Traverse_Revisit_List();
#endif
}

static file_info *file_table;
static include_info *incl_table;
static INT cur_file_index = 0;

void Cg_Dwarf_Gen_Asm_File_Table (void)
{
  INT count;
  DST_IDX idx;
  DST_INCLUDE_DIR *incl;
  DST_FILE_NAME *file;
  const char *name;
  INT incl_table_size;
  INT file_table_size;
  INT new_size;

  incl_table_size = 0;
  incl_table = NULL;
  file_table_size = 0;
  file_table = NULL;
  count = 1;
  for (idx = DST_get_include_dirs (); 
       !DST_IS_NULL(idx); 
       idx = DST_INCLUDE_DIR_next(incl))
  {
    incl = DST_DIR_IDX_TO_PTR (idx);
    name = DST_STR_IDX_TO_PTR (DST_INCLUDE_DIR_path(incl));
    if (count >= incl_table_size) {
      new_size = count + 10;
      if (incl_table == NULL)
        incl_table = (include_info *) malloc (new_size * sizeof (include_info));
      else 
	incl_table = (include_info *) realloc (incl_table, new_size * sizeof (include_info));
      if (incl_table == NULL) ErrMsg (EC_No_Mem, "Cg_Dwarf_Gen_Asm_File_Table");
      incl_table_size = new_size;
    }
    incl_table[count].path_name = name;
    incl_table[count].already_processed = FALSE;
    count++;
  }

  count = 1;
  for (idx = DST_get_file_names (); 
       !DST_IS_NULL(idx); 
       idx = DST_FILE_NAME_next(file))
  {
    file = DST_FILE_IDX_TO_PTR (idx);
    if (DST_IS_NULL(DST_FILE_NAME_name(file))) {
      name = "NULLNAME";
    }
    else {
      name = DST_STR_IDX_TO_PTR (DST_FILE_NAME_name(file));
    }
    if (count >= file_table_size) {
      new_size = count + 10;
      if (file_table == NULL)
        file_table = (file_info *) malloc (new_size * sizeof (file_info));
      else 
	file_table = (file_info *) realloc (file_table, 
					    new_size * sizeof (file_info));
      if (file_table == NULL) ErrMsg (EC_No_Mem, "Cg_Dwarf_Gen_Asm_File_Table");
      file_table_size = new_size;
    }
    file_table[count].filename = name;
    file_table[count].incl_index = DST_FILE_NAME_dir(file);
    file_table[count].fileptr = NULL;
    file_table[count].max_line_printed = 0;
    file_table[count].already_processed = FALSE;
    file_table[count].mod_time = DST_FILE_NAME_modt(file);
    file_table[count].file_size = DST_FILE_NAME_size(file);
    count++;

#ifdef TEMPORARY_STABS_FOR_GDB
    // This is an ugly hack to enable basic debugging for IA-32 target
    if (Debug_Level > 0 && count == 3) {
      fprintf(Asm_File, ".stabs \"%s/\",100,0,0,.Ltext0\n", 
              incl_table[DST_FILE_NAME_dir(file)].path_name);
      fprintf(Asm_File, ".stabs \"%s\",100,0,0,.Ltext0\n", name);
    }
#endif

  }

}


#ifdef KEY
void Cg_Dwarf_Gen_Macinfo (void)
{
  DST_IDX idx;
  DST_MACR *macr;
  char *macro, *macname, *macvalue;
  INT fileindex, linenumber;
  BOOL processed_start_file = FALSE;
  BOOL processed_end_file = FALSE;

  for (idx = DST_get_macro_info();
       !DST_IS_NULL(idx);
       idx = DST_MACR_next(macr)) {
    macr = DST_MACR_IDX_TO_PTR(idx);
    switch (DST_MACR_tag(macr)) {
    case DW_MACINFO_define:
      if (!processed_start_file) {
	dwarf_start_macro_file(dw_dbg, 
			       1 /* fileindex */, 0 /* linenumber */, &dw_error);
	processed_start_file = TRUE;
      }	
      macro  = DST_STR_IDX_TO_PTR(DST_MACR_macro(macr));
      dwarf_def_macro(dw_dbg, DST_MACR_lineno(macr), 
		      macro, NULL, &dw_error);
      break;
    case DW_MACINFO_undef:
      if (!processed_start_file) {
	dwarf_start_macro_file(dw_dbg, 
			       1 /* fileindex */, 0 /* linenumber */, &dw_error);
	processed_start_file = TRUE;
      }	
      macname  = DST_STR_IDX_TO_PTR(DST_MACR_macro(macr));
      macvalue = dwarf_find_macro_value_start(macname);
      macname[strlen(macname) - strlen(macvalue) - 1] = '\0';
      dwarf_undef_macro(dw_dbg, DST_MACR_lineno(macr), 
			macname, &dw_error);
      break;
    case DW_MACINFO_start_file:
      fileindex = DST_MACR_fileno(macr);
      linenumber = DST_MACR_lineno(macr);
      dwarf_start_macro_file(dw_dbg, fileindex, linenumber, &dw_error);
      processed_start_file = TRUE;
      break;
    case DW_MACINFO_end_file:
      dwarf_end_macro_file(dw_dbg, &dw_error);
      processed_end_file = TRUE;
      break;
    default:
      break;
    }
  }
  if (processed_start_file && !processed_end_file)
    // End the macinfo section manually.
    dwarf_end_macro_file(dw_dbg, &dw_error);
}

void
Print_Directives_For_All_Files(void) {
  DST_IDX idx;
  DST_FILE_NAME *file;
  INT count = 1;
#if ! defined(BUILD_OS_DARWIN)
  for (idx = DST_get_file_names(); 
       !DST_IS_NULL(idx);
       idx = DST_FILE_NAME_next(file)) {
    file = DST_FILE_IDX_TO_PTR(idx);
    fprintf(Asm_File, "\t%s\t%d\t\"%s/%s\"\n", AS_FILE, count, 
	    incl_table[file_table[count].incl_index].path_name, 
	    file_table[count].filename);
    count++;
  }
  fputc ('\n', Asm_File);
#endif /* defined(BUILD_OS_DARWIN) */
}
#endif


static void
print_source (SRCPOS srcpos)
{
  USRCPOS usrcpos;
  char srcfile[1024];
  char text[1024];
  file_info *cur_file;
  INT i;
  INT newmax;

  USRCPOS_srcpos(usrcpos) = srcpos;

  /* TODO: we don't handle this yet. */
  if (USRCPOS_filenum(usrcpos) == 0) return;

  cur_file = &file_table[USRCPOS_filenum(usrcpos)];
  if (USRCPOS_filenum(usrcpos) != cur_file_index) {
    if (cur_file_index != 0) {
      /* close the previous file. */
      file_info *prev_file = &file_table[cur_file_index];
      fclose (prev_file->fileptr);
      prev_file->fileptr = NULL;
    }
    cur_file_index = USRCPOS_filenum(usrcpos);
    cur_file = &file_table[cur_file_index];
    /* open the new file. */
    sprintf (srcfile, "%s/%s", incl_table[cur_file->incl_index].path_name,
				cur_file->filename);
    cur_file->fileptr = fopen (srcfile, "r");
    if (cur_file->fileptr == NULL) {
      cur_file_index = 0;	/* indicate invalid cur_file */
      return;
    }
    for (i = 0; i < cur_file->max_line_printed; i++) {
      fgets (text, sizeof(text), cur_file->fileptr);
    }
  }
  newmax = USRCPOS_linenum(usrcpos) - 5;
  if (cur_file->max_line_printed < newmax) {
    for (i = cur_file->max_line_printed; i < newmax; i++) {
      fgets (text, sizeof(text), cur_file->fileptr);
    }
    cur_file->max_line_printed = newmax;
  }
  if (cur_file->max_line_printed < USRCPOS_linenum(usrcpos)) {
    for (i = cur_file->max_line_printed; i < USRCPOS_linenum(usrcpos); i++) {
      if (fgets (text, sizeof(text), cur_file->fileptr) != NULL) {
	// check for really long line
	if (strlen(text) >= 1023) text[1022] = '\n';
        fprintf (Asm_File, "%s%4d  %s", ASM_CMNT_LINE, i+1, text);
      }
    }
    cur_file->max_line_printed = USRCPOS_linenum(usrcpos);
  }
}

#ifdef KEY
BOOL Cg_Dwarf_First_Op_After_Preamble_End = FALSE;
BOOL Cg_Dwarf_BB_First_Op = FALSE;
#endif

// THis adds line info and, as a side effect,
// builds tables in dwarf2 for the file numbers

// For linux IA-64, we should emit a series of .file directives
// on first call in the order used in tables internaly such
// as references from macro info and declarations.
// This series would have the side effect of registering
// file names with the right file number.
// Because the gnu assembler actually searches on *name*
// before number in building its file tables
// to create .debug_line.

//
//
void
Cg_Dwarf_Add_Line_Entry (INT code_address, 
                         SRCPOS srcpos)
{
  static SRCPOS last_srcpos = 0;
  USRCPOS usrcpos;

  if (srcpos == 0 && last_srcpos == 0)
	DevWarn("no valid srcpos at PC %d\n", code_address);
#ifndef KEY

  if (srcpos == 0 || srcpos == last_srcpos) return;
#else
  if (srcpos == 0 || 
      (!Cg_Dwarf_First_Op_After_Preamble_End && !Cg_Dwarf_BB_First_Op &&
       srcpos == last_srcpos))
    return;
#endif

#ifdef TARG_IA64
  // TODO:  figure out what to do about line changes in middle of bundle ???
  // For assembly, can put .loc in middle of bundle.
  // But can't generate object code with that,
  // because libdwarf expects addresses to be aligned with instructions,
  // so ignore such cases.
  if ((code_address % INST_BYTES) != 0) {
#if defined(BUILD_OS_DARWIN)
        return;
#endif /* defined(BUILD_OS_DARWIN) */
  }
#endif

  USRCPOS_srcpos(usrcpos) = srcpos;

  // only emit file changes when happen, so don't bloat objects
  // with unused header files.
  USRCPOS last_usrcpos;
  USRCPOS_srcpos(last_usrcpos) = last_srcpos;

  // now do line number:
#if defined(BUILD_OS_DARWIN)
  /* Mach-O assembler doesn't emit Dwarf for .line directive */
  Em_Dwarf_Add_Line_Entry (code_address, srcpos);
#endif /* defined(BUILD_OS_DARWIN) */
  
  if (Assembly) {
    CGEMIT_Prn_Line_Dir_In_Asm(usrcpos);
    if (List_Source)
    	print_source (srcpos);
  }
  last_srcpos = srcpos;
}

static inline Elf64_Word
reloc_offset(char *reloc, BOOL is_64bit)
{
  if (is_64bit) {
    return REL_offset(*((Elf64_Rel *) reloc));
  }
  else {
    return REL32_offset(*((Elf32_Rel *) reloc));
  }
}

static inline Elf64_Word
reloc_sym_index(char *reloc, BOOL is_64bit)
{
  if (is_64bit) {
    return REL64_sym(*((Elf64_AltRel *) reloc));
  }
  else {
    return REL32_sym(*((Elf32_Rel *) reloc));
  }
}

static void check_reloc_fmt_and_size(Elf64_Word     reloc_scn_type,
				     Dwarf_Ptr      reloc_buffer,
				     Dwarf_Unsigned reloc_buffer_size,
				     Dwarf_Unsigned scn_buffer_size)
{
}

#if defined(TARG_LINUX)
struct UINT32_unaligned {
  UINT32 val;
} __attribute__ ((aligned(1)));

struct UINT64_unaligned {
  UINT64 val;
} __attribute__ ((aligned(1)));
#else
#pragma pack(1)
struct UINT32_unaligned {
  UINT32 val;
};

struct UINT64_unaligned {
  UINT64 val;
};
#pragma pack(0)
#endif /* linux */



// These are intended to be file-local, and the unnamed namespace
// tells c++ to make them file-local

// See the comments at
// Cg_Dwarf_Output_Asm_Bytes_Sym_Relocs
// for the motivation for this virtual_section_position
// complexity.

// The following allows multiple sections
namespace {
  // scn_handles are buffer holders for section buffers.

  // The 'buffer' pointer does not own the space, but
  // simply points to space owned by other code.

  struct scn_handle {
    Dwarf_Unsigned sc_bufsize;
    Dwarf_Signed   sc_scndx;
    Dwarf_Ptr      sc_buffer;
    pSCNINFO       sc_cursection;
    BOOL           sc_output;
  };

  struct virtual_section_position {
    Dwarf_Unsigned vsp_virtpos; // byte offset in
			// a 'virtual buffer' which
			// is the same size as the total of
			// the buffers->bufsize fields.

    //Dwarf_Unsigned vsp_base; // could be used
		// for consistency check, but seems
		// unnecessary at the moment.

    Dwarf_Unsigned vsp_curbufpos;// byte offset in the current
			// real buffer

    Dwarf_Signed   vsp_remaining_buffercount; // number of remaining
			// buffers. Usually 1.

    scn_handle **  vsp_buffers; // pointer to the array of 
		// pointers to  buffers.
			

    virtual_section_position(Dwarf_Signed ct,scn_handle **buffers):
      //vsp_base(0), 
      vsp_virtpos(0),vsp_curbufpos(0),
      vsp_remaining_buffercount(ct),
      vsp_buffers(buffers)  {}

    ~virtual_section_position() {}

   
    Dwarf_Unsigned vsp_get_bytes(Dwarf_Unsigned offset, int size)  
    {
	// We insist that offset march in step
	// with vsp_virtpos to ensure there is no
	// uncaught bug in the calling-client.
      FmtAssert((vsp_virtpos == offset), 
		("Error in vsp location (dwarf output)"));

      if(vsp_curbufpos >= vsp_buffers[0]->sc_bufsize) {
        // ran off the end of a buffer.


        if(vsp_remaining_buffercount < 2) {
		Fail_FmtAssertion("Ran off end of vsp buffer generating dwarf");
        }
        // we used up one buffer, proceed to the next.
        --vsp_remaining_buffercount;
      	//vsp_base += vsp_buffers[0]->sc_bufsize;
        vsp_buffers++;
      	vsp_curbufpos = 0;
      }
      // Should be pointing at valid buffer now
      if((vsp_curbufpos + size) > vsp_buffers[0]->sc_bufsize) {
	   // No field should cross buffers!
	   Fail_FmtAssertion("Impossible vsp buffer configuration");
      }
      Dwarf_Unsigned value;
      char *loc = ((char *)vsp_buffers[0]->sc_buffer) + vsp_curbufpos;

      switch(size) {
      case 1:
      	  value = loc[0];
      	  break;
      case 4:
      	    value =  ((UINT32_unaligned *)loc)->val;
      	    break;
      case 8:
      	    value =  ((UINT64_unaligned *)loc)->val;
      	    break;
      default:
      	   Fail_FmtAssertion("Impossible vsp buffer vsp_get_bytes size %d",
			(int)size);
      }
      vsp_curbufpos += size;
      vsp_virtpos += size;

      return value;
    }
    Dwarf_Unsigned vsp_print_bytes(
		FILE * asm_file,
		Dwarf_Unsigned current_reloc_target,
                Dwarf_Unsigned cur_byte_in
                );

  };
}


// print the assembly form.
// return the number of bytes printed
Dwarf_Unsigned 
virtual_section_position::vsp_print_bytes(
		FILE * asm_file,
	        Dwarf_Unsigned cur_reloc_target,
		Dwarf_Unsigned cur_byte_in
                )
{
    Dwarf_Unsigned cur_byte = cur_byte_in;

#ifdef KEY
    /* The efficiency of this routine turns out to be more important
     * than you might first suspect.  In particular, when compiling
     * with "-O0 -g" a lot of time is spent doing file IO, and the
     * debug info is typically the single largest data source.  To
     * conserve space this routine packs data into words which take
     * about half as much space as bytes in the ASCII intermediate.
     */

    const int bytes_per_word_line = 16; /* must be multiple of 4 */
    const int bytes_per_byte_line = 8;  /* can be anything you want */
    int i, nlines_this_reloc;

    /* Write byte stream in word-sized chunks, by line  */
    nlines_this_reloc = (cur_reloc_target - cur_byte) / bytes_per_word_line;
    for (i = 0; i < nlines_this_reloc; i++) {

      fputs("\t" AS_WORD_UNALIGNED "\t", asm_file);

      int j;
      for (j = 0; j < bytes_per_word_line; j += 4) {
        UINT32 val = 0;

        if (!Target_Is_Little_Endian) {
          int k;
          for (k = 3*CHAR_BIT; k >= 0; k-=CHAR_BIT) {
            val |= vsp_get_bytes(cur_byte,1) << k;
            cur_byte++;
          }
        } else {
          int k;
          for (k = 0; k < 4*CHAR_BIT; k+=CHAR_BIT) {
            val |= vsp_get_bytes(cur_byte,1) << k;
            cur_byte++;
          }
        }

        if (j != 0) {
          fputs(", ", asm_file);
        }

        fprintf(asm_file, "0x%08x", (int)val);
      }

      fputc('\n', asm_file);
    }

    /* Write byte stream in byte-sized chunks, by line  */
    nlines_this_reloc = (cur_reloc_target - cur_byte) / bytes_per_byte_line;
    for (i = 0; i < nlines_this_reloc; i++) {
      fputs("\t" AS_BYTE "\t", asm_file);

      fprintf(asm_file, "0x%02x",
        (int)vsp_get_bytes(cur_byte,1));
      cur_byte++;

      int j;
      for (j = 1; j < bytes_per_byte_line; j++) {
        fprintf(asm_file, ", 0x%02x",
          (int)vsp_get_bytes(cur_byte,1));
        cur_byte++;
      }

      fputc('\n', asm_file);
    }

    /* Write byte stream in byte-sized chunks, remainder */
    if (cur_byte != cur_reloc_target) {
      fputs("\t" AS_BYTE "\t", asm_file);

      fprintf(asm_file, "0x%02x",
        (int)vsp_get_bytes(cur_byte,1));
      cur_byte++;

      for (; cur_byte < cur_reloc_target; cur_byte++) {
        fprintf(asm_file, ", 0x%02x",
          (int)vsp_get_bytes(cur_byte,1));
      }

      fputc('\n', asm_file);
    } 
#else
    const int bytes_per_line = 8;
    int nlines_this_reloc = (cur_reloc_target - cur_byte) / bytes_per_line;

    int i;
    for (i = 0; i < nlines_this_reloc; ++i) {
      fprintf(asm_file, "\t%s\t", AS_BYTE);
      int j;
      for (j = 1; j < bytes_per_line; ++j) {
        fprintf(asm_file, "0x%02x, ",
		(int)vsp_get_bytes(cur_byte,1));
	++cur_byte;
      }
      fprintf(asm_file, "0x%02x\n", 
		(int)vsp_get_bytes(cur_byte,1));
      ++cur_byte;
    }
    if (cur_byte != cur_reloc_target) {
      fprintf(asm_file, "\t%s\t", AS_BYTE);
      for (; cur_byte != cur_reloc_target - 1; ) {
        fprintf(asm_file, "0x%02x, ", 
		(int)vsp_get_bytes(cur_byte,1));
	++cur_byte;
      }
      fprintf(asm_file, "0x%02x\n", 
		(int)vsp_get_bytes(cur_byte,1));
      ++cur_byte;
    }
#endif
    return cur_byte - cur_byte_in;
}

//
// Find the list of sections that are section secnum
// caller must pass in array scn_bufferp, and it must
// be large enough.
static Dwarf_Signed 
match_scndx(Elf64_Word scndx,
                scn_handle * scn_handles /* input */,
                Dwarf_Unsigned section_count /* input */,
                scn_handle** scn_bufferp /* output info */)
{
  for (Dwarf_Signed i = 0; i < section_count; i++) {
    if (scn_handles[i].sc_scndx == scndx) {
      Dwarf_Signed k = 0;
      scn_bufferp[k] =  scn_handles +i;
      ++i;
      ++k;
      while( i < section_count && scn_handles[i].sc_scndx == scndx  ){
         scn_bufferp[k] = scn_handles +i;
	 ++k;
	 ++i;
      }
      return k;
    }
  }
  Fail_FmtAssertion("Bogus section index %ld\n", scndx);
  /*NOTREACHED*/
}


// We need the size of a virtual buffer
// so we can use it to check the validity of
// relocation offsets.
static Dwarf_Unsigned
compute_buffer_net_size(Dwarf_Signed   buffer_cnt,
  scn_handle **  buffers)
{
	if(buffer_cnt < 1) {
		return 0;
	}
	Dwarf_Unsigned next = 0;
	Dwarf_Unsigned totsize = 0;
	for( ; next < buffer_cnt; ++next) {
		totsize += buffers[next]->sc_bufsize;
	}
	return totsize;
}


// Emit assembly code using elf-relocations
// created by libdwarf.

// At present, this is only used  in a limited way 
// for MIPS/SGI for assembly output (if ever used), and
// is really a holdover approach.
// The more general code is Cg_Dwarf_Output_Asm_Bytes_Sym_Relocs.

// See Cg_Dwarf_Output_Asm_Bytes_Sym_Relocs
// for comments on the reason for
// the virtual_section_position class.
static void
Cg_Dwarf_Output_Asm_Bytes_Elf_Relocs (FILE          *asm_file,
				      const char    *section_name,
				      Elf64_Word     section_type,
				      Elf64_Word     section_flags,
				      Elf64_Word     section_entsize,
				      Elf64_Word     section_align,
				      Dwarf_Signed   buffer_cnt,
				      scn_handle **  buffers,
				      Elf64_Word     reloc_scn_type,
				      Dwarf_Ptr      reloc_buffer,
				      Dwarf_Unsigned reloc_buffer_size,
				      BOOL           is_64bit)
{

  const Dwarf_Unsigned buffer = 0; // Leave at 0: is virtual buffer

  CGEMIT_Prn_Scn_In_Asm(asm_file, section_name, section_type,
			section_flags, section_entsize,
			section_align, NULL);


  Dwarf_Unsigned bufsize =  // compute size of virtual buffer
	compute_buffer_net_size(buffer_cnt, buffers);

  check_reloc_fmt_and_size(reloc_scn_type,
			   reloc_buffer,
			   reloc_buffer_size,
			   bufsize);

  Dwarf_Unsigned cur_byte =  buffer;

  // initialize position
  virtual_section_position vsp(buffer_cnt, buffers);

  char * current_reloc = (char *) reloc_buffer;

	// With MIPS binary relocation output, the size is 4 or 8,
	// never the cygnus semi-64-bit, so we do not need to support
	// that here  (and don't have the data to do so).
  UINT current_reloc_size = (is_64bit ? 8 : 4);

  do {
    Dwarf_Unsigned current_reloc_target;
    if (current_reloc != ((char *) reloc_buffer) + reloc_buffer_size) {
      current_reloc_target = (buffer) + reloc_offset(current_reloc,
							      is_64bit);
    }
    else {
      current_reloc_target = buffer + bufsize;
    }
#if defined(Is_True_On)
    if ((UINT64) current_reloc_target < (UINT64) cur_byte) {
      fprintf(stderr, "ERROR: relocation records not sorted\n");
      exit(-1);
    }
#endif

    cur_byte += vsp.vsp_print_bytes(asm_file,current_reloc_target,cur_byte);

    if (cur_byte != (buffer) + bufsize) {
#if defined(Is_True_On)
      // Check that if the final bytes are relocated, the relocation
      // doesn't overrun the end of the buffer.
      if ((UINT64) current_reloc_target + current_reloc_size >
	  (UINT64) buffer + bufsize) {
	fprintf(stderr, "ERROR: relocation record overruns end of section data\n");
	exit(-1);
      }
#endif

      //
      // Now put out the symbol reference plus offset for the relocation.
      //
      Elf64_Word current_reloc_sym_index = reloc_sym_index(current_reloc,
							   is_64bit);
      char *current_reloc_sym_name = Em_Get_Symbol_Name(current_reloc_sym_index);
      Dwarf_Unsigned ofst;

#ifdef TARG_MIPS
      if (CG_emit_non_gas_syntax)
	fprintf(asm_file, "\t%s\t%s", Use_32_Bit_Pointers ? ".word" : ".dword",
	        current_reloc_sym_name);
      else
#endif
      fprintf(asm_file, "\t%s\t%s", AS_ADDRESS,
	      current_reloc_sym_name);

      switch (reloc_scn_type) {
      case SHT_REL:
	if (is_64bit) {
	  Check_Dwarf_Rel(*((Elf64_AltRel *) current_reloc));
          FmtAssert(current_reloc_size == sizeof(UINT64),
			("reloc size error"));
	  ofst = vsp.vsp_get_bytes(cur_byte,sizeof(UINT64));
	
	  current_reloc += sizeof(Elf64_Rel);
	}
	else {
	  Check_Dwarf_Rel(*((Elf32_Rel *) current_reloc));
          FmtAssert(current_reloc_size == sizeof(UINT32),
			("reloc size error"));
	  ofst = vsp.vsp_get_bytes(cur_byte,sizeof(UINT32));

	  current_reloc += sizeof(Elf32_Rel);
	}
	break;
      case SHT_RELA:
	if (is_64bit) {
	  Check_Dwarf_Rela(*((Elf64_AltRela *) current_reloc));

	  // position vsp to match cur_byte, discard value
	  vsp.vsp_get_bytes(cur_byte,sizeof(current_reloc_size));

	  ofst = ((Elf64_Rela *) current_reloc)->r_addend;
	  current_reloc += sizeof(Elf64_Rela);
	}
	else {
	  Check_Dwarf_Rela(*((Elf32_Rela *) current_reloc));

	  // position vsp to match cur_byte, discard value
	  vsp.vsp_get_bytes(cur_byte,sizeof(current_reloc_size));

	  ofst = ((Elf32_Rela *) current_reloc)->r_addend;
	  current_reloc += sizeof(Elf32_Rela);
	}
	break;
      default:
	fprintf(stderr,
		"ERROR: unrecognized relocation section type: %u\n",
		reloc_scn_type);
	exit (-1);
      }
      if (ofst != 0) {
	fprintf(asm_file, " + 0x%llx", (unsigned long long)ofst);
      }
      fputc ('\n', asm_file);
      cur_byte += current_reloc_size;
    }
  } while (cur_byte != ( buffer) + bufsize);
  fflush(asm_file);
}

// Symbolic assembler output where the input
// is symbolic relocations created by libdwarf
// (as opposed to the elf binary relocations
// processed by Cg_Dwarf_Output_Asm_Bytes_Elf_Relocs).

// Each relocation section is complete (only one buffer).
// However, the data bytes can be split across
// multiple data buffers!
// Note that for binary Elf output, the bytes were just added
// to the end of an elf section, so having multiple buffers
// was no problem.
// Here, we are working with the bytes, and
// we have to carefully recognize that the relocations
// apply to the mythical 'complete' buffer.
// To make that work in the simplest way, 
// we do most of the work in terms
// of a mythical 'complete' buffer, and the
// virtual_section_position class does the messy bits.
//
static void
Cg_Dwarf_Output_Asm_Bytes_Sym_Relocs (FILE                 *asm_file,
				      const char           *section_name,
				      Elf64_Word            section_type,
				      Elf64_Word            section_flags,
				      Elf64_Word            section_entsize,
				      Elf64_Word            section_align,
				      Dwarf_Signed          buffer_cnt,
                                      scn_handle        **  buffers,
				      Dwarf_Relocation_Data reloc_buffer,
				      Dwarf_Unsigned        reloc_count,
				      BOOL                  is_64bit)
{

  // [re]establish the section this is part of.
  //
  CGEMIT_Prn_Scn_In_Asm(asm_file, section_name, section_type,
			section_flags, section_entsize,
			section_align, NULL);

  const Dwarf_Unsigned buffer   = 0; // virtual buffer, so 0 ok.
  Dwarf_Unsigned cur_byte =  buffer;
  Dwarf_Unsigned bufsize =  compute_buffer_net_size(buffer_cnt,buffers);

  virtual_section_position vsp(buffer_cnt, buffers);

#ifdef KEY
#define buflen 30
  char dwarf_begin[buflen], dwarf_end[buflen];
  const int alignment = Use_32_Bit_Pointers ? 4 : 8;
#endif

  Dwarf_Unsigned k = 0;
  while (k <= reloc_count) {
    // The last time thru, k == reloc_count which
    // emits the final part of the buffer (after the last reloc).
    // There is no relocation record for k == reloc_count as
    // the relocation records are numbered 0 thru  (reloc_count-1).

    Dwarf_Unsigned current_reloc_target;
    if (k != reloc_count) {
      current_reloc_target = ( buffer) + reloc_buffer[k].drd_offset;

    }
    else {
      current_reloc_target = ( buffer) + bufsize;
    }
#if defined(Is_True_On)
    Is_True((UINT64) current_reloc_target >= (UINT64) cur_byte,
	    ("Relocation records not sorted\n"));
#endif

    UINT current_reloc_size = is_64bit?8:4; 

    if((reloc_count > 0) && reloc_buffer && k < reloc_count) {
	// For cygnus semi-64-bit dwarf.
	// targets of relocs in semi-64-bit vary.
	// Some sections have no relocs and reloc_buffer can be null
	// in such cases.
      current_reloc_size = reloc_buffer[k].drd_length;
    }

    cur_byte += vsp.vsp_print_bytes(asm_file,current_reloc_target,cur_byte);
    
    if (cur_byte != (buffer) + bufsize) {
#if defined(Is_True_On)
      // Check that if the final bytes are relocated, the relocation
      // doesn't overrun the end of the buffer.
      Is_True((UINT64) current_reloc_target + current_reloc_size <=
	      (UINT64) buffer + bufsize,
	      ("Relocation record overruns end of section data\n"));
#endif

      //
      // Now put out the symbol reference plus offset for the relocation.
      //
      // If pointer-length reloc, use data8.ua, else dwarf offset
      // size to be relocated, use data4.ua

      const char *reloc_name = 
#ifdef TARG_MIPS
	   CG_emit_non_gas_syntax ? (Use_32_Bit_Pointers ? ".word" : ".dword") :
#endif
	  	(reloc_buffer[k].drd_length == 8)?
			AS_ADDRESS_UNALIGNED: AS_WORD_UNALIGNED;
#ifdef TARG_X8664
      // don't want to affect other sections, although they may also need
      // to be updated under fPIC
      bool gen_pic = ((Gen_PIC_Call_Shared || Gen_PIC_Shared) &&
      		       !strcmp (section_name, EH_FRAME_SECTNAME));
#endif
      switch (reloc_buffer[k].drd_type) {
      case dwarf_drt_none:
	break;
      case dwarf_drt_data_reloc:
	fprintf(asm_file, "\t%s\t%s", reloc_name,
		Cg_Dwarf_Name_From_Handle(reloc_buffer[k].drd_symbol_index));
#ifdef TARG_X8664
	if (gen_pic)
	    fputs ("-.", asm_file);
#endif
	break;

      case dwarf_drt_segment_rel:
        {
	// need unaligned AS_ADDRESS for dwarf, so add .ua
	fprintf(asm_file, "\t%s\t%s", reloc_name,
		Cg_Dwarf_Name_From_Handle(reloc_buffer[k].drd_symbol_index));
#ifdef TARG_X8664
	if (gen_pic)
	    fprintf(asm_file, "-.");
#endif
	break;
        }
      case dwarf_drt_first_of_length_pair:
	Is_True(k + 1 < reloc_count, ("unpaired first_of_length_pair"));
	Is_True((reloc_buffer[k + 1].drd_type ==
		 dwarf_drt_second_of_length_pair),
		("unpaired first_of_length_pair"));
	// need unaligned AS_ADDRESS for dwarf, so add .ua
#if defined(BUILD_OS_DARWIN)
	emit_difference(asm_file, reloc_name,
	  Cg_Dwarf_Name_From_Handle(reloc_buffer[k + 1].drd_symbol_index),
	  Cg_Dwarf_Name_From_Handle(reloc_buffer[k].drd_symbol_index));
#else /* defined(BUILD_OS_DARWIN) */
	fprintf(asm_file, "\t%s\t%s - %s", reloc_name,
		Cg_Dwarf_Name_From_Handle(reloc_buffer[k + 1].drd_symbol_index),
		Cg_Dwarf_Name_From_Handle(reloc_buffer[k].drd_symbol_index));
#endif /* defined(BUILD_OS_DARWIN) */
	++k;
	break;
      case dwarf_drt_second_of_length_pair:
	Fail_FmtAssertion("unpaired first/second_of_length_pair");
	break;
      default:
	break;
      }

      Dwarf_Unsigned ofst;
      if (current_reloc_size == 8) {
	// 64 bit target of relocation.
	ofst = vsp.vsp_get_bytes(cur_byte,8);
      }
      else if(current_reloc_size == 4){
	// 32 bit target of relocation, which can happen
	// with 32 bit target or with
	// cygnus semi-64-bit dwarf.
	ofst = vsp.vsp_get_bytes(cur_byte,4);
      } else {
	Fail_FmtAssertion("current_reloc_size %ld, 4 or 8 required!\n", 
		(long)current_reloc_size);
      }
#ifdef KEY
      // Bug 5357 - we emit 0x0 for the CIE_ID for eh_frame (in accordance with
      // g++). The following code will skip that 4byte if that is the case.
      if (ofst == 0 && cur_byte == 4 && strcmp(section_name, EH_FRAME_SECTNAME) == 0) 
	fprintf(asm_file, "\t%s 0x%llx", reloc_name, (unsigned long long)ofst);
#endif
      if (ofst != 0) {
#if defined(KEY)
	if (reloc_buffer[k].drd_type == dwarf_drt_none)
	    fprintf(asm_file, "\t%s 0x%llx", reloc_name, (unsigned long long)ofst);
	else
#endif // KEY
	fprintf(asm_file, " + 0x%llx", (unsigned long long)ofst);
      }
      fputc ('\n', asm_file);
      cur_byte += current_reloc_size;
    }
    ++k;
  }

#ifdef KEY
  if (!strcmp (section_name, EH_FRAME_SECTNAME)){
    // End preceding frame/cie
#if defined(BUILD_OS_DARWIN)
    fprintf (asm_file, "\t%s %d\n", AS_ALIGN, logtwo(alignment));
#else /* defined(BUILD_OS_DARWIN) */
    fprintf (asm_file, "\t%s %d\n", AS_ALIGN, alignment);
#endif /* defined(BUILD_OS_DARWIN) */
    fprintf (asm_file, "%s:\n", dwarf_end);
  }
#endif

  fflush(asm_file);
}


#if !defined(LIBDWARF_SORTS_RELOCS)
static int compare_rel64(const void *const a, const void *const b)
{
  if (REL_offset(*((Elf64_Rel *) a)) < REL_offset(*((Elf64_Rel *) b))) {
    return -1;
  }
  else if (REL_offset(*((Elf64_Rel *) a)) > REL_offset(*((Elf64_Rel *) b))) {
    return 1;
  }
  else {
    return 0;
  }
}

static int compare_rela64(const void *const a, const void *const b)
{
  if (REL_offset(*((Elf64_Rela *) a)) < REL_offset(*((Elf64_Rela *) b))) {
    return -1;
  }
  else if (REL_offset(*((Elf64_Rela *) a)) > REL_offset(*((Elf64_Rela *) b))) {
    return 1;
  }
  else {
    return 0;
  }
}

static int compare_rel32(const void *const a, const void *const b)
{
  if (REL32_offset(*((Elf32_Rel *) a)) < REL32_offset(*((Elf32_Rel *) b))) {
    return -1;
  }
  else if (REL32_offset(*((Elf32_Rel *) a)) > REL32_offset(*((Elf32_Rel *) b))) {
    return 1;
  }
  else {
    return 0;
  }
}

static int compare_rela32(const void *const a, const void *const b)
{
  if (REL32_offset(*((Elf32_Rela *) a)) < REL32_offset(*((Elf32_Rela *) b))) {
    return -1;
  }
  else if (REL32_offset(*((Elf32_Rela *) a)) > REL32_offset(*((Elf32_Rela *) b))) {
    return 1;
  }
  else {
    return 0;
  }
}
#endif

void
Cg_Dwarf_Write_Assembly_From_Elf (FILE *asm_file,
				  INT   section_count,
				  BOOL  is_64bit)
{
  Dwarf_Signed i;
  Dwarf_Ptr buffer;
  Dwarf_Signed scndx;
  Dwarf_Unsigned bufsize;
  pSCNINFO cursection;
  scn_handle *scn_handles = 
	(scn_handle *) malloc(section_count * sizeof(scn_handle));
  FmtAssert(scn_handles != 0,
	("malloc space for scn_handles failed"));
  scn_handle *s = scn_handles;

  scn_handle ** scn_buffers =
	(scn_handle **) malloc(section_count * sizeof(scn_handle *));
  FmtAssert(scn_buffers != 0,
	("malloc space for scn_buffers failed"));

  dwarf_reset_section_bytes(dw_dbg);

  for (i = 0; i < section_count; i++) {
    s = scn_handles + i;
    buffer = dwarf_get_section_bytes (dw_dbg, i, &scndx, &bufsize, &dw_error);
    cursection = Em_Dwarf_Find_Dwarf_Scn (scndx);
    if (cursection != NULL) {
      s->sc_buffer     = buffer;
      s->sc_bufsize    = bufsize;
      s->sc_scndx      = scndx;
      s->sc_cursection = cursection;
      s->sc_output     = FALSE;
    }
    else {
      fprintf (stderr, "ERROR No such section index: %d\n", (int)scndx);
      exit(-1);
    }
  }

  // BUG in the following loop: We don't output sections for which
  // there is no corresponding relocation section!
  for (i = 0; i < section_count; i++) {
    pSCNINFO relsection = scn_handles[i].sc_cursection;
    Elf64_Word relsection_type = Em_Get_Section_Type(relsection);

    if (relsection_type == SHT_REL || relsection_type == SHT_RELA) {
      // Sort the relocation records by offset
#if defined(LIBDWARF_SORTS_RELOCS)
      char *reloc_sorted = (char *) scn_handles[i].sc_buffer;
#else
      char *reloc_sorted = (char *) malloc(scn_handles[i].sc_bufsize);
      reloc_sorted = (char *) memcpy((void *) reloc_sorted,
				     (void *) scn_handles[i].sc_buffer,
				     (size_t) scn_handles[i].sc_bufsize);
      size_t n_relocs;
      if (relsection_type == SHT_REL) {
	if (is_64bit) {
	  n_relocs = scn_handles[i].sc_bufsize / sizeof(Elf64_Rel);
	  qsort(reloc_sorted, n_relocs, sizeof(Elf64_Rel), compare_rel64);
	}
	else {
	  n_relocs = scn_handles[i].sc_bufsize / sizeof(Elf32_Rel);
	  qsort(reloc_sorted, n_relocs, sizeof(Elf32_Rel), compare_rel32);
	}
      }
      else {
	if (is_64bit) {
	  n_relocs = scn_handles[i].sc_bufsize / sizeof(Elf64_Rela);
	  qsort(reloc_sorted, n_relocs, sizeof(Elf64_Rela), compare_rela64);
	}
	else {
	  qsort(reloc_sorted, n_relocs, sizeof(Elf32_Rela), compare_rela32);
	}
      }
#endif

      // Get the sections that are the target of the relocations
      // (cursection).

      Elf64_Word relsection_info = Em_Get_Section_Info(relsection);

      Dwarf_Signed scn_count  = match_scndx(relsection_info, 
		scn_handles, 
		section_count,
		scn_buffers);
      cursection                 = scn_buffers[0]->sc_cursection;

      char *section_name = Em_Get_Section_Name(cursection);
      Elf64_Word section_type  = Em_Get_Section_Type(cursection);
      Elf64_Word section_flags = Em_Get_Section_Flags(cursection);
      Elf64_Word section_entsize = Em_Get_Section_Entsize(cursection);
      Elf64_Word section_align = Em_Get_Section_Align(cursection);
      Cg_Dwarf_Output_Asm_Bytes_Elf_Relocs (asm_file, section_name,
					    section_type, section_flags,
					    section_entsize, section_align,
					    scn_count,
					    scn_buffers,
					    relsection_type,
					    reloc_sorted,
					    scn_handles[i].sc_bufsize,
					    is_64bit);
#if !defined(LIBDWARF_SORTS_RELOCS)
      free(reloc_sorted);
#endif
    }
  }
  free(scn_handles);
  scn_handles = NULL;
  free(scn_buffers);
  scn_buffers = 0;
}

void
Cg_Dwarf_Write_Assembly_From_Symbolic_Relocs (FILE *asm_file,
					      INT   section_count,
					      BOOL  is_64bit)
{
  Dwarf_Signed i;
  Dwarf_Ptr buffer;
  Dwarf_Signed scndx;
  Dwarf_Unsigned bufsize;
  pSCNINFO cursection;
  scn_handle *scn_handles;
  scn_handle *s;

  scn_handles = (scn_handle *) malloc(section_count * sizeof(scn_handle));
  FmtAssert(scn_handles != 0,
	("malloc space for scn_handles failed"));
  scn_handle ** scn_buffers =
        (scn_handle **) malloc(section_count * sizeof(scn_handle *));
  FmtAssert(scn_buffers != 0,
	("malloc space for scn_buffers failed"));


  dwarf_reset_section_bytes(dw_dbg);

  for (i = 0; i < section_count; i++) {
    s = scn_handles + i;

    buffer = dwarf_get_section_bytes (dw_dbg, i, &scndx, &bufsize, &dw_error);
    cursection = Em_Dwarf_Find_Dwarf_Scn (scndx);
    if (cursection != NULL) {
      s->sc_buffer     = buffer;
      s->sc_bufsize    = bufsize;
      s->sc_scndx      = scndx;
      s->sc_cursection = cursection;
      s->sc_output     = FALSE;
    } else {
      fprintf (stderr, "ERROR No such section index: %d\n", (int)scndx);
      exit(-1);
    }
  }

  // Now get the relocation buffers, translate each one into an ELF
  // relocation section, and write the ELF section to the output file.
  Dwarf_Unsigned relocation_section_count;
  int            reloc_buffer_version;
  int result = dwarf_get_relocation_info_count(dw_dbg,
					       &relocation_section_count,
					       &reloc_buffer_version,
					       &dw_error);
  FmtAssert(result == DW_DLV_OK,
	    ("Failure to get relocation info count"));
  Is_True(reloc_buffer_version == 2,
	  ("Symbolic relocation format mismatch"));

  for (i = 0; i < relocation_section_count; i++) {
    Dwarf_Relocation_Data reloc_buf;
    Dwarf_Unsigned entry_count;
    Dwarf_Signed link_scn;

    result = dwarf_get_relocation_info(dw_dbg, &scndx, &link_scn,
				       &entry_count, &reloc_buf,
				       &dw_error);
    FmtAssert(result == DW_DLV_OK,
	      ("Failure to get relocation info"));
    // Get the section that is the target of the relocations
    // (cursection).

    Dwarf_Signed scn_count  = match_scndx(link_scn,
                scn_handles,
                section_count,
                scn_buffers);


    cursection = scn_buffers[0]->sc_cursection;

    char *section_name = Em_Get_Section_Name(cursection);
    // Assembler generates .debug_line from directives itself, so we
    // don't output it or others not needed.
    if(TRUE == Is_Dwarf_Section_To_Emit(section_name)) {
      Elf64_Word section_type  = Em_Get_Section_Type(cursection);
      Elf64_Word section_flags = Em_Get_Section_Flags(cursection);
      Elf64_Word section_entsize = Em_Get_Section_Entsize(cursection);
      Elf64_Word section_align = Em_Get_Section_Align(cursection);
      Cg_Dwarf_Output_Asm_Bytes_Sym_Relocs (asm_file, section_name,
					    section_type, section_flags,
					    section_entsize, section_align,
					    scn_count,
					    scn_buffers,
					    reloc_buf,
					    entry_count, is_64bit);
    }
#ifdef TARG_X8664 
    // We will just emit the .debug_line section header but no bytes because
    // there is a reference to .debug_line in .debug_info.
    // We could remove the reference to .debug_line from .debug_info but
    // we will just match Gcc's output.
    // The .debug_line will be generated from the .loc directives by the 
    // assembler. - bug 2779 (why did it get exposed now?)
    else if (strcmp(section_name, DEBUG_LINE_SECTNAME) == 0)
#if defined(BUILD_OS_DARWIN)
      gen_debug_line_section_header(asm_file, section_name);
#else /* defined(BUILD_OS_DARWIN) */
      fputs("\n\t.section " DEBUG_LINE_SECTNAME ", \"\"\n", asm_file);
#endif /* defined(BUILD_OS_DARWIN) */
#endif
    for(int i = 0; i < scn_count; ++i) {
	scn_buffers[i]->sc_output = TRUE;
    }
  }

  for (i = 0; i < section_count; i++) {
    if (!scn_handles[i].sc_output) {
      cursection = scn_handles[i].sc_cursection;
      char *section_name = Em_Get_Section_Name(cursection);
      scn_handle *one_han = scn_handles +i;
      // Assembler generates .debug_line from directives itself, so we
      // don't output it.
      if(TRUE == Is_Dwarf_Section_To_Emit(section_name)) {
	Elf64_Word section_type  = Em_Get_Section_Type(cursection);
	Elf64_Word section_flags = Em_Get_Section_Flags(cursection);
	Elf64_Word section_entsize = Em_Get_Section_Entsize(cursection);
	Elf64_Word section_align = Em_Get_Section_Align(cursection);
	Cg_Dwarf_Output_Asm_Bytes_Sym_Relocs (asm_file, section_name,
					      section_type, section_flags,
					      section_entsize, section_align,
					      /* list of one entry */1,
					      /* the list entry */ &one_han,
					      /* reloc buff= */ NULL, 
						/* reloc count = */0, 
						is_64bit);
      }
#ifdef TARG_X8664
      // We will just emit the .debug_line section header but no bytes because
      // there is a reference to .debug_line in .debug_info.
      // We could remove the reference to .debug_line from .debug_info but
      // we will just match Gcc's output.
      // The .debug_line will be generated from the .loc directives by the 
      // assembler. - bug 2779 (why did it get exposed now?)
      else if (strcmp(section_name, DEBUG_LINE_SECTNAME) == 0)
#if defined(BUILD_OS_DARWIN)
	gen_debug_line_section_header(asm_file, section_name);
#else /* defined(BUILD_OS_DARWIN) */
	fputs("\n\t.section " DEBUG_LINE_SECTNAME ", \"\"\n", asm_file);
#endif /* defined(BUILD_OS_DARWIN) */
#endif
    }
  }

  free(scn_handles);
  scn_handles = NULL;
  free(scn_buffers);
  scn_buffers = 0;
}
