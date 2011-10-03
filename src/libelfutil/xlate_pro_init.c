/*

  Copyright (C) 2000, 2001 Silicon Graphics, Inc.  All Rights Reserved.

   Path64 is free software; you can redistribute it and/or modify it
   under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation version 2.1

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



/*
   xlate_pro_init.c
   $Revision$

   This is the initialization and release of data 
   involved in production of a translate section.

   The calling app must do the section adding: this simply
   produces a byte stream that the app can use to produce
   (typically) one or 2 elf sections.

   There will be one 'section' (byte stream)
   output if there is no input table to 
   merge with. There will be two 'sections' output if there
   is an input table to merge with.

   The fully merged output stream is call the 'normal' or 'standard'
   stream.  The current input is called the 'debug' stream in the
   output calls.

*/

#include "xlateincl.h"

#ifdef _LIBELF_XTND_EXPANDED_DATA
#pragma weak xlate_pro_init_xtnd = _xlate_pro_init_xtnd
#pragma weak xlate_pro_finish_xtnd = _xlate_pro_finish_xtnd
#elif defined(XLATE_DISABLE_PRAGMA_WEAK)
#else
#pragma weak xlate_pro_init = _xlate_pro_init
#pragma weak xlate_pro_finish = _xlate_pro_finish
#endif
static
int  determine_func(int is64bit,xlate_tablekind tablekind, 
	add_range_func *retfunc)
{
  add_range_func localfunc;
  switch(tablekind) {
  case xlate_tk_general:
	if(is64bit) {
	  localfunc = _xlate_pro_add_range_ge64;
	} else {
	  localfunc = _xlate_pro_add_range_ge32;
	}
	break;

  case xlate_tk_preserve_order:
	if(is64bit) {
	  localfunc = _xlate_pro_add_range_po64;
	} else {
	  localfunc = _xlate_pro_add_range_po32;
	}
	break;
  case xlate_tk_preserve_size:
	if(is64bit) {
	  localfunc = _xlate_pro_add_range_ps64;
	} else {
	  localfunc = _xlate_pro_add_range_ps32;
	}
	break;
  default:
	return XLATE_TB_STATUS_BAD_TABLEKIND;
  }
  
  *retfunc = localfunc;
  return XLATE_TB_STATUS_NO_ERROR;
}

int xlate_pro_init(xlate_table_pro *    ret_table,
    xlate_tablekind                     tablekind,
    xlate_table_con                     compose_with_table,
    int    	                      is64Bit)
{
   int retstatus  = XLATE_TB_STATUS_NO_ERROR;
   int com64bit;
   xlate_tablekind tabkind;
   xlate_table_pro newtab;

   newtab = (xlate_table_pro)calloc(1, sizeof(struct xlate_table_pro_s));
   if(newtab == NULL) {
        return XLATE_TB_STATUS_ALLOC_FAIL;
   }

   newtab->tb_con_table = compose_with_table;
   newtab->tb_is64bit = is64Bit;
   newtab->tb_std_highwater.hw_lowNewAddr = INITIAL_LOW_ADDR;
   newtab->tb_std_highwater.hw_lowOldAddr = INITIAL_LOW_ADDR;

   if(compose_with_table ) {
        newtab->tb_debug_highwater.hw_lowNewAddr = INITIAL_LOW_ADDR;
        newtab->tb_debug_highwater.hw_lowOldAddr = INITIAL_LOW_ADDR;
	newtab->tb_debug_tablekind = tablekind;
	retstatus = xlate_get_info(compose_with_table,
	   /*dataMoved=*/0,/*new_low=*/0,/*old_low=*/0,
	   /*new_high=*/0,/*old_high=*/0,
	   /* startup fwa=*/0,/*startuplwa=*/0,
	   /*number_of_ranges=*/0,
	   /* oldtextexists=*/0,/*oldtextalloc=*/0,
	   &com64bit,&tabkind, /* tableversion */0);
	if(retstatus != XLATE_TB_STATUS_NO_ERROR) {
		return retstatus;
	}
	if(is64Bit != com64bit) {
	  return XLATE_TB_STATUS_PRO_CON_TABLE_MISMATCH;
	}
	if(tabkind == tablekind && tabkind != xlate_tk_general) {
	  newtab->tb_std_tablekind = tablekind;
	} else {
	  newtab->tb_std_tablekind = xlate_tk_general;
	}
	retstatus = determine_func(is64Bit,
		newtab->tb_std_tablekind,&newtab->tb_std_add_range);
	if(retstatus != XLATE_TB_STATUS_NO_ERROR) {
		return retstatus;
	}
	retstatus = determine_func(is64Bit,
		newtab->tb_debug_tablekind,&newtab->tb_debug_add_range);
   }else {
	newtab->tb_std_tablekind = tablekind;
	retstatus = determine_func(is64Bit,
		newtab->tb_std_tablekind,&newtab->tb_std_add_range);
   }

   newtab->tb_magic = PRO_MAGIC_VAL;

   *ret_table = newtab;
   return retstatus;
}

int 
xlate_pro_finish(xlate_table_pro table)
{
   Block_s *blk;
   Block_s *blkn;
   int retstatus  = XLATE_TB_STATUS_NO_ERROR;
   if(table->tb_magic != PRO_MAGIC_VAL) {
	return XLATE_TB_STATUS_INVALID_TABLE;
   }

   if(table->tb_regInfo) {
	free(table->tb_regInfo);
   }
   blk = table->tb_std_block_head;
   for( ; blk; blk = blkn) {
	blkn = blk->bk_next;
	blk->bk_next = 0;
	free(blk);
   }
   blk = table->tb_debug_block_head;
   for( ; blk; blk = blkn) {
	blkn = blk->bk_next;
	blk->bk_next = 0;
	free(blk);
   }
	
   table->tb_magic = 0;
   free(table);
   return retstatus;
}


