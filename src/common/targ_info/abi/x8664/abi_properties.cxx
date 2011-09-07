/*
 *  Copyright (C) 2008 PathScale, LLC.  All Rights Reserved.
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


//  
//  Generate ABI information
///////////////////////////////////////


//  $Revision: 1.19 $
//  $Date: 05/05/05 17:40:09-07:00 $
//  $Author: gautam@jacinth.keyresearch $
//  $Source: common/targ_info/abi/x8664/SCCS/s.abi_properties.cxx $

#include <stddef.h>
#include "abi_properties_gen.h"
#include "targ_isa_registers.h"

static ABI_PROPERTY
  allocatable,
  callee,
  caller,
  func_arg,
  func_val,
  stack_ptr,
  frame_ptr,
  static_link;


static void x86_32_abi(void)
{
  /* The order is assigned to favor the parameter passing. */
  static const char* integer_names[] = {
    "%eax", "%ebx", "%ebp", "%esp", "%edi", "%esi", "%edx", "%ecx"
  };
  enum { EAX=0, EBX, EBP, ESP, EDI, ESI, EDX, ECX };
  enum { XMM0=0, XMM1, XMM2,  XMM3,  XMM4,  XMM5,  XMM6,  XMM7 };
  enum { ST0=0, ST1, ST2, ST3, ST4, ST5, ST6, ST7 };
  enum { MMX0=0, MMX1, MMX2, MMX3, MMX4, MMX5, MMX6, MMX7 };

  // ISA_REGISTER_CLASS_integer

  Reg_Names( ISA_REGISTER_CLASS_integer, 0, 7, integer_names );

  Reg_Property( allocatable, ISA_REGISTER_CLASS_integer,
		EAX, EBX, ECX, EDX, EBP, ESI, EDI,
		-1 );
  Reg_Property( callee, ISA_REGISTER_CLASS_integer,
		EBP, EBX, EDI, ESI, ESP,
	       -1 );
  Reg_Property( caller, ISA_REGISTER_CLASS_integer,
		EAX, ECX, EDX,
		-1 );
  #ifdef _WIN32
  Reg_Property( func_arg, ISA_REGISTER_CLASS_integer,
  		ECX, EDX,
		-1 );
  #else
  Reg_Property( func_arg, ISA_REGISTER_CLASS_integer,
		-1 );
  #endif
  Reg_Property( func_val, ISA_REGISTER_CLASS_integer,
		EAX, EDX,
		-1 );
  Reg_Property( stack_ptr, ISA_REGISTER_CLASS_integer, 
		ESP,
		-1 );
  Reg_Property( frame_ptr, ISA_REGISTER_CLASS_integer, 
		EBP,
	       -1 );
  #ifdef _WIN32
  Reg_Property( static_link, ISA_REGISTER_CLASS_integer, 
		EAX,
		-1 );
  #else
  Reg_Property( static_link, ISA_REGISTER_CLASS_integer, 
  		ECX,
	       -1 );
  #endif

  // ISA_REGISTER_CLASS_float

  Reg_Property( allocatable, ISA_REGISTER_CLASS_float,
		XMM0, XMM1, XMM2, XMM3, XMM4, XMM5, XMM6, XMM7,
		-1 );
  Reg_Property( callee, ISA_REGISTER_CLASS_float,
		-1 );
  Reg_Property( caller, ISA_REGISTER_CLASS_float,
		XMM0, XMM1, XMM2, XMM3, XMM4, XMM5, XMM6, XMM7,
		-1 );
  Reg_Property( func_arg, ISA_REGISTER_CLASS_float,
		XMM0, XMM1, XMM2, XMM3, XMM4, XMM5, XMM6, XMM7,
		-1 );
#ifdef _WIN32
  Reg_Property( func_val, ISA_REGISTER_CLASS_float,
		XMM0,
		-1 );
#else
  Reg_Property( func_val, ISA_REGISTER_CLASS_float,
		XMM0, XMM1,
		-1 );
#endif

  // ISA_REGISTER_CLASS_x87

  Reg_Property( allocatable, ISA_REGISTER_CLASS_x87,
		ST0, ST1, ST2, ST3, ST4, ST5, ST6, ST7,
		-1 );
  Reg_Property( callee, ISA_REGISTER_CLASS_x87,
		-1 );
  Reg_Property( caller, ISA_REGISTER_CLASS_x87,
		ST0, ST1, ST2, ST3, ST4, ST5, ST6, ST7,
		-1 );
  Reg_Property( func_arg, ISA_REGISTER_CLASS_x87,
		-1 );
#ifdef _WIN32
  Reg_Property( func_val, ISA_REGISTER_CLASS_x87,
		ST0,
		-1 );
#else
  Reg_Property( func_val, ISA_REGISTER_CLASS_x87,
		ST0, ST1,
		-1 );
#endif

  // ISA_REGISTER_CLASS_mmx

  Reg_Property( allocatable, ISA_REGISTER_CLASS_mmx,
                MMX0, MMX1, MMX2, MMX3, MMX4, MMX5, MMX6, MMX7,
		-1 );
  Reg_Property( callee, ISA_REGISTER_CLASS_mmx,
                -1 );
  Reg_Property( caller, ISA_REGISTER_CLASS_mmx,
                MMX0, MMX1, MMX2, MMX3, MMX4, MMX5, MMX6, MMX7,
		-1 );
#ifdef _WIN32
  Reg_Property( func_arg, ISA_REGISTER_CLASS_mmx,
		MMX0, MMX1, MMX2, MMX3, MMX4, MMX5, MMX6, MMX7,
		-1 );
  Reg_Property( func_val, ISA_REGISTER_CLASS_mmx,
  		MMX0,
		-1 );
#else
  Reg_Property( func_arg, ISA_REGISTER_CLASS_mmx,
                -1 );
  Reg_Property( func_val, ISA_REGISTER_CLASS_mmx,
                -1 );
#endif
}



static void x86_64_abi(void)
{
  /* The order is assigned to favor the parameter passing. */
  static const char* integer_names[16] = {
    "%rax", "%rbx", "%rbp", "%rsp", "%rdi", "%rsi", "%rdx", "%rcx",
    "%r8",  "%r9",  "%r10", "%r11", "%r12", "%r13", "%r14", "%r15"
  };
  enum { RAX=0, RBX, RBP, RSP, RDI, RSI, RDX, RCX,
	 R8, R9, R10, R11, R12, R13, R14, R15 };
  enum { XMM0=0, XMM1, XMM2,  XMM3,  XMM4,  XMM5,  XMM6,  XMM7,
	 XMM8,   XMM9, XMM10, XMM11, XMM12, XMM13, XMM14, XMM15 };
  enum { ST0=0, ST1, ST2, ST3, ST4, ST5, ST6, ST7 };
  enum { MMX0=0, MMX1, MMX2, MMX3, MMX4, MMX5, MMX6, MMX7 };

  // ISA_REGISTER_CLASS_integer

  Reg_Names( ISA_REGISTER_CLASS_integer, 0, 15, integer_names );

  Reg_Property( allocatable, ISA_REGISTER_CLASS_integer,
		RAX, RBX, RCX, RDX, RBP, RSI, RDI,
		R8, R9, R10, R11, R12, R13, R14, R15,
		-1 );
#ifdef _WIN32
  Reg_Property( callee, ISA_REGISTER_CLASS_integer,
  		RBX, RBP, RSI, RDI, R12, R13, R14, R15,
		-1 );
  Reg_Property( caller, ISA_REGISTER_CLASS_integer,
		RAX, RCX, RDX, R8, R9, R10, R11,
		-1 );
  Reg_Property( func_arg, ISA_REGISTER_CLASS_integer,
		RCX, RDX, R8, R9,
		-1 );
  Reg_Property( func_val, ISA_REGISTER_CLASS_integer,
		RAX,
		-1 );
#else
  Reg_Property( callee, ISA_REGISTER_CLASS_integer,
		RBX, RBP, R12, R13, R14, R15,
		-1 );
  Reg_Property( caller, ISA_REGISTER_CLASS_integer,
		RAX, RCX, RDX, RSI, RDI, R8, R9, R10, R11,
		-1 );
  Reg_Property( func_arg, ISA_REGISTER_CLASS_integer,
		RDI, RSI, RDX, RCX, R8, R9,
		-1 );
  Reg_Property( func_val, ISA_REGISTER_CLASS_integer,
		RAX, RDX,
		-1 );
#endif
  Reg_Property( stack_ptr, ISA_REGISTER_CLASS_integer, 
		RSP,
		-1 );
  Reg_Property( frame_ptr, ISA_REGISTER_CLASS_integer, 
		RBP,
	       -1 );
  Reg_Property( static_link, ISA_REGISTER_CLASS_integer, 
		R10,
	       -1 );

  // ISA_REGISTER_CLASS_float

  Reg_Property( allocatable, ISA_REGISTER_CLASS_float,
		XMM0, XMM1, XMM2,  XMM3,  XMM4,  XMM5,  XMM6,  XMM7,
		XMM8, XMM9, XMM10, XMM11, XMM12, XMM13, XMM14, XMM15,
		-1 );
 #ifdef _WIN32
  // According to Win64 calling convention, XMM6-XMM15's low 64 bits are
  // callee-save. 
  // Now, treat these XMM registers as caller-save for simplicy, which
  // will impact performance when calling Windows library functions.
#if 0
  Reg_Property( callee, ISA_REGISTER_CLASS_float,
		XMM6,  XMM7, XMM8, XMM9, XMM10, XMM11, XMM12, XMM13,
		XMM14, XMM15,
	       -1 );
  Reg_Property( caller, ISA_REGISTER_CLASS_float,
	       XMM0, XMM1, XMM2, XMM3, XMM4, XMM5,
	       -1 );
#else
  Reg_Property( callee, ISA_REGISTER_CLASS_float,
		-1 );
  Reg_Property( caller, ISA_REGISTER_CLASS_float,
		XMM0, XMM1, XMM2,  XMM3,  XMM4,  XMM5,  XMM6,  XMM7,
		XMM8, XMM9, XMM10, XMM11, XMM12, XMM13, XMM14, XMM15,
		-1 );
#endif
  Reg_Property( func_arg, ISA_REGISTER_CLASS_float,
	       XMM0, XMM1, XMM2, XMM3,
	       -1 );
  Reg_Property( func_val, ISA_REGISTER_CLASS_float,
	       XMM0,
	       -1 );
 #else
  Reg_Property( callee, ISA_REGISTER_CLASS_float,
		-1 );
  Reg_Property( caller, ISA_REGISTER_CLASS_float,
		XMM0, XMM1, XMM2,  XMM3,  XMM4,  XMM5,  XMM6,  XMM7,
		XMM8, XMM9, XMM10, XMM11, XMM12, XMM13, XMM14, XMM15,
		-1 );
  Reg_Property( func_arg, ISA_REGISTER_CLASS_float,
		XMM0, XMM1, XMM2, XMM3, XMM4, XMM5, XMM6, XMM7,
		-1 );
  Reg_Property( func_val, ISA_REGISTER_CLASS_float,
		XMM0, XMM1,
		-1 );

#endif
  // ISA_REGISTER_CLASS_x87

  Reg_Property( allocatable, ISA_REGISTER_CLASS_x87,
		ST0, ST1, ST2, ST3, ST4, ST5, ST6, ST7,
		-1 );
  Reg_Property( callee, ISA_REGISTER_CLASS_x87,
		-1 );
  Reg_Property( caller, ISA_REGISTER_CLASS_x87,
		ST0, ST1, ST2, ST3, ST4, ST5, ST6, ST7,
		-1 );
  Reg_Property( func_arg, ISA_REGISTER_CLASS_x87,
		-1 );

 #ifdef _WIN32
  Reg_Property( func_val, ISA_REGISTER_CLASS_x87,
		-1 );

 #else
  Reg_Property( func_val, ISA_REGISTER_CLASS_x87,
		ST0, ST1,
		-1 );
 #endif

  // ISA_REGISTER_CLASS_mmx

  Reg_Property( allocatable, ISA_REGISTER_CLASS_mmx,
                MMX0, MMX1, MMX2, MMX3, MMX4, MMX5, MMX6, MMX7,
		-1 );
  Reg_Property( callee, ISA_REGISTER_CLASS_mmx,
                -1 );
  Reg_Property( caller, ISA_REGISTER_CLASS_mmx,
                MMX0, MMX1, MMX2, MMX3, MMX4, MMX5, MMX6, MMX7,
		-1 );
  Reg_Property( func_arg, ISA_REGISTER_CLASS_mmx,
                -1 );
  Reg_Property( func_val, ISA_REGISTER_CLASS_mmx,
                -1 );
}


int main()
{
  ABI_Properties_Begin( "x8664" );

  allocatable = Create_Reg_Property( "allocatable" );
  callee = Create_Reg_Property( "callee" );
  caller = Create_Reg_Property( "caller" );
  func_arg = Create_Reg_Property( "func_arg" );
  func_val = Create_Reg_Property( "func_val" );
  stack_ptr = Create_Reg_Property( "stack_ptr" );
  frame_ptr = Create_Reg_Property( "frame_ptr" );
  static_link = Create_Reg_Property( "static_link" );

  Begin_ABI( "n32" );
  x86_32_abi();

  Begin_ABI( "n64" );
  x86_64_abi();

  ABI_Properties_End();
}
