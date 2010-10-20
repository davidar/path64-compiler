
/*
 * Copyright (C) 2007, 2008 PathScale, LLC.  All Rights Reserved.
 */

/*
 * Copyright (C) 2008 Advanced Micro Devices, Inc.  All Rights Reserved.
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


/* ====================================================================
 * ====================================================================
 *
 * Module: config_opt.h
 * $Revision: 1.22 $
 * $Date: 05/12/02 16:59:45-08:00 $
 * $Author: fchow@fluorspar.internal.keyresearch.com $
 * $Source: common/com/SCCS/s.config_opt.h $
 *
 * Revision history:
 *  05-May-96 - Extracted from be/opt/opt_config.h.
 *
 * Description:
 *
 * Declare global flag variables for -OPT group options.
 * This file is included in common/com/config.c.
 *
 * Declarations of -OPT flags should be put here, instead of in
 * config.h.  The intent is to allow updates of the -OPT group
 * without forcing recompilation of everything that includes config.h.
 * (However, the transfer of the flags' definitions here from config.h
 * is not yet complete, so most of the old ones still require
 * config.h.)
 *
 * ====================================================================
 * WARNING: WHENEVER A NEW FLAG IS ADDED:
 * ###	- Add the flag variable declaration to config_opt.h (here) .
 * ###	- Add the flag variable definition to config_opt.cxx .
 * ###	- Add the option to the group description in config_opt.cxx .
 * ====================================================================
 *
 * ====================================================================
 * ====================================================================
 */

#ifndef config_opt_INCLUDED
#define config_opt_INCLUDED

#ifndef flags_INCLUDED
#include "flags.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Incomplete types to prevent unnecessary inclusion: */
struct skiplist;

/*********************************************************************
 ***
 *** Flag variable declarations:
 ***
 *********************************************************************
 */

/***** Optimization Warning Messages *****/
extern BOOL Show_OPT_Warnings;		/* Display OPT warning messages */

/***** Aliasing control *****/
extern BOOL Alias_Pointer_Parms;	/* Reference parms indep? */
extern BOOL Alias_Pointer_Types;	/* Ptrs to distinct basic types indep? */
extern BOOL Alias_Not_In_Union;	/* Ptrs point to non-union types */
extern BOOL Alias_Pointer_Strongly_Typed; /* Ptrs to distinct types indep? */
extern BOOL Alias_Pointer_Named_Data;	/* No pointers to named data? */
extern BOOL Alias_Pointer_Restricted;	/* *p and *q not aliased */
extern BOOL Alias_Pointer_Disjoint;     /* **p and **q not aliased */
extern BOOL Alias_Pointer_Cray;         /* Cray pointer semantics? */
extern BOOL Alias_Common_Scalar;        /* Distinguish scalar from other array
                                           in a common block */
extern BOOL  Alias_F90_Pointer_Unaliased;  /* Are F90 pointers unaliased? */

/***** Expression folding options *****/
extern BOOL Enable_Cfold_Float;		/* FP constant folding? */
extern BOOL Enable_Cfold_Reassociate;	/* Re-association allowed? */
extern BOOL Enable_Cfold_Intrinsics;	/* Intrinsic constant folding? */
extern BOOL Cfold_Intrinsics_Set;	/* ... option seen? */
extern BOOL CIS_Allowed;	/* sin(x) and cos(x) => cis(x) ? */
extern BOOL Div_Split_Allowed;	/* Change a/b --> a*1/b ? */
#ifdef TARG_ST
BE_EXPORTED extern BOOL Float_Eq_Simp;	/* change a==b (float cmp) --> a==b (integer cmp if a or b is cst)) ? */
BE_EXPORTED extern BOOL OPT_fb_div_simp;	// Apply division simplification with feedback info 
BE_EXPORTED extern BOOL OPT_fb_mpy_simp;	// Apply multiply simplification with feedback info 
#endif

#ifdef KEY
extern UINT32 Div_Exe_Counter;	  /* Change a/b --> a/N if b==N ?             */
extern UINT32 Div_Exe_Ratio;	  /* Change a/b --> a/N if b has high ratio   */
extern UINT32 Div_Exe_Candidates; /* The top entries that will be taken care. */
extern UINT32 Mpy_Exe_Counter;	/* Change a*b to a if b==N or 0.0 if b == 0.0 */
extern UINT32 Mpy_Exe_Ratio;	/* Change a*b to a if b==N or 0.0 if b == 0.0 */
#endif
#ifdef TARG_ST
BE_EXPORTED extern UINT32 Freq_Threshold_For_Space;      /* If the PU is executed less than this, OPT_Space is set to true. */
BE_EXPORTED extern UINT32 Size_Threshold_For_Space;      /* If the PU is bigger than this, OPT_Space is set to true. */
BE_EXPORTED extern BOOL FB_CodeSize_Perf_Ratio;	 /* Optimize for size when freq < Freq_Threshold_For_Space or when size > Size_Threshold_For_Space */
#endif
extern BOOL Fast_Exp_Allowed;	/* Avoid exp() calls? */
extern BOOL Fast_IO_Allowed;	/* Fast printf/scanf/printw */
extern BOOL Fast_Sqrt_Allowed;	/* Change sqrt(x) --> x * rsqrt(x) ? */
extern BOOL Optimize_CVTL_Exp;	/* Optimize expansion of CVTL operators */
extern BOOL Enable_CVT_Opt;	/* Optimize expansion of CVT operators */
extern BOOL Force_IEEE_Comparisons;	/* IEEE NaN comparisons? */
extern BOOL Inline_Intrinsics_Early;    /* Inline intrinsics just after VHO */
extern BOOL Enable_extract_bits;     /* Enable use of the extract whirl op */
extern BOOL Enable_compose_bits;     /* Enable use of the compose whirl op */
#ifdef TARG_ST
BE_EXPORTED extern BOOL Enable_Rotate;     /* Enable use of the rotate whirl ops */
BE_EXPORTED extern BOOL Enable_Rotate_overriden;     /* ... option seen? */
#endif
 extern BOOL Enable_extract;     /* Enable use of the extract whirl ops */
 extern BOOL Enable_extract_overriden;     /* ... option seen? */
 extern BOOL Enable_compose;     /* Enable use of the compose whirl ops */
 extern BOOL Enable_compose_overriden;     /* ... option seen? */



#ifdef TARG_ST
  /* Enable optimisation of comparisons using minmax */
BE_EXPORTED extern BOOL Enable_simplify_comparisons_per_minmax;

/***** Floating point optimizations options *****/
BE_EXPORTED extern BOOL No_Math_Errno;  /* Do not set ERRNO ? */
BE_EXPORTED extern BOOL No_Math_Errno_Set;  /* ... option seen? */
BE_EXPORTED extern BOOL Finite_Math;  /* Finite math optimizations ? */
BE_EXPORTED extern BOOL Finite_Math_Set;  /* ... option seen? */
BE_EXPORTED extern BOOL No_Rounding;  /*  ? */
BE_EXPORTED extern BOOL No_Rounding_Set;  /* ... option seen? */
BE_EXPORTED extern BOOL No_Trapping;  /* No trapping math ? */
BE_EXPORTED extern BOOL No_Trapping_Set;  /* ... option seen? */
BE_EXPORTED extern BOOL Unsafe_Math;  /* Unsafe math allowed ? */
BE_EXPORTED extern BOOL Unsafe_Math_Set;  /* ... option seen? */
BE_EXPORTED extern BOOL Fused_FP;  /* Fused FP ops allowed ? */
BE_EXPORTED extern BOOL Fused_FP_Set;  /* ... option seen? */
BE_EXPORTED extern BOOL Fused_Madd;  /* Fused madd allowed ? */
BE_EXPORTED extern BOOL Fused_Madd_Set;  /* ... option seen? */
BE_EXPORTED extern BOOL No_Denormals;  /* No denormals support  ? */
BE_EXPORTED extern BOOL No_Denormals_Set;  /* ... option seen? */
BE_EXPORTED extern BOOL OPT_Expand_Assume; /* Expand __builtin_assume ? */
BE_EXPORTED extern BOOL OPT_Expand_Assume_Set; /* ... option seen? */
// FdF 20080305: Emit warning on unsupported expressions in __builtin_assume
BE_EXPORTED extern BOOL OPT_Enable_Warn_Assume;
// TB: 20081020 Check that non void function always return a value
BE_EXPORTED extern BOOL OPT_Enable_Warn_ReturnVoid;
typedef enum {
  REASSOC_NONE,	/* No roundoff-inducing transformations */
  REASSOC_SIMPLE,	/* Simple roundoff transformations */
  REASSOC_ASSOC,	/* Reassociation transformations */
  REASSOC_ANY		/* Anything goes */
} REASSOC;
BE_EXPORTED extern REASSOC Reassoc_Level; /* reassociations level */
BE_EXPORTED extern BOOL Reassoc_Set;  /* ... option seen? */
#endif


/***** Miscellaneous optimization options *****/
extern BOOL OPT_Pad_Common;	/* Do internal common block padding? */
extern BOOL OPT_Reorg_Common;	/* Do common block reorganization (split)? */
extern BOOL OPT_Reorg_Common_Set;	/* ... option seen? */
extern BOOL OPT_Unroll_Analysis;	/* Enable unroll limitations? */
extern BOOL OPT_Unroll_Analysis_Set;	/* ... option seen? */
extern BOOL GCM_Speculative_Ptr_Deref;   /* allow load speculation of a memory
                                          reference that differs by a small
                                          offset from some reference location*/
extern BOOL GCM_Speculative_Ptr_Deref_Set;   /* ... option seen? */
extern BOOL Early_MP_Processing; /* Do mp lowerering before lno/preopt */
extern BOOL Implied_Do_Io_Opt;	/* Do implied-do loop opt for I/O */
extern BOOL Cray_Ivdep;		/* Use Cray meaning for Ivdep */
extern BOOL Liberal_Ivdep;	/* Use liberal meaning for ivdep */
extern BOOL Inhibit_EH_opt;     /* Don't remove calless EH regions */
extern BOOL Allow_Overflow_Opt;   /* Allow strength reduction of unsigned types possibly losing overflow */
extern BOOL OPT_recompute_addr_flags; /* recompute addr saved */
extern BOOL OPT_IPA_addr_analysis; /* enable the use of IPA addr analysis result */ 
extern BOOL Delay_U64_Lowering;/* Delay unsigned 64-bit lowering to after wopt*/
extern BOOL OPT_shared_memory;	// assume use of shared memory

/***** Instrumentation related options *****/
extern INT32 Instrumentation_Phase_Num;
extern INT32 Instrumentation_Type_Num;
extern BOOL Instrumentation_Enabled;
extern UINT32 Instrumentation_Actions;
extern BOOL Instrumentation_Unique_Output;
extern INT32 Feedback_Phase_Num;
extern OPTION_LIST* Feedback_Option;
#ifdef KEY
extern INT32 OPT_Cyg_Instrument;
extern BOOL profile_arcs;
extern BOOL Asm_Memory;
extern BOOL Align_Unsafe;
extern INT32 Enable_WN_Simp_Expr_Limit;
extern BOOL OPT_Lower_To_Memlib;
extern INT32 OPT_Threshold_To_Memlib;
extern INT32 OPT_Enable_Lower_To_Memlib_Limit;
extern BOOL OPT_Enable_Memlib_Aggressive;
extern BOOL OPT_Enable_Simp_Fold;

extern BOOL OPT_Fast_Math;
extern BOOL OPT_Fast_Stdlib;
extern BOOL OPT_MP_Barrier_Opt;
extern BOOL OPT_Icall_Instr;
extern BOOL OPT_Int_Value_Instr;
extern BOOL OPT_FP_Value_Instr;
extern BOOL OPT_Ffast_Math;
extern BOOL OPT_Funsafe_Math_Optimizations;

extern BOOL OPT_Float_Via_Int;
extern UINT32 OPT_Malloc_Alg;
extern INT32 OPT_Hugepage_Heap_Limit;
extern BOOL OPT_Hugepage_Heap_Set;
extern INT32 OPT_Hugepage_Attr;
extern BOOL OPT_Malloc_Alg_Set;
extern BOOL Early_Goto_Conversion;
extern BOOL Early_Goto_Conversion_Set;
extern INT32 OPT_Madd_Height;
extern INT32 OPT_Madd_Chains;
extern BOOL OPT_Freestanding;
extern BOOL OPT_Reassoc_For_Cse;;
#endif
#ifdef TARG_ST
BE_EXPORTED extern char* disable_instrument;
BE_EXPORTED extern char* enable_instrument;
#endif

#ifdef __cplusplus
}
#endif
#endif /* config_opt_INCLUDED */
