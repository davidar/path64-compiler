/*
  Copyright (C) 2006, STMicroelectronics, All Rights Reserved.

  This program is free software; you can redistribute it and/or modify it
  under the terms of version 2 of the GNU General Public License as
  published by the Free Software Foundation.

  This program is distributed in the hope that it would be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

  Further, this software is distributed without any warranty that it is
  free of the rightful claim of any third person regarding infringement 
  or the like.  Any license provided herein, whether implied or 
  otherwise, applies only to this software file.  Patent licenses, if 
  any, provided herein do not apply to combinations of this program with 
  other software, or any other product whatsoever.  

  You should have received a copy of the GNU General Public License along
  with this program; if not, write the Free Software Foundation, Inc., 59
  Temple Place - Suite 330, Boston MA 02111-1307, USA.
*/

/* ====================================================================
 * ====================================================================
 *
 * Module: brange.cxx
 *
 * Description:
 *
 * Implementation of bit-value range type
 *
 * ====================================================================
 * ====================================================================
 */

#include "defs.h"
#include "errors.h"
#include "clz.h"
#include "lbitvalue.h"



const LBitValue
LBitValue::Top ()
{
  LBitValue result;
  // Represent Top by 'all proved zero' and 'all proved one' that should not happen
  // We don't use the ctor since it is built to assert when creating such a value
  result.zeromask_= UINT64_MAX;
  result.onemask_= UINT64_MAX;
  return result;
}

const LBitValue
LBitValue::Bottom ()
{
  return LBitValue(0ULL, 0ULL);
}

const LBitValue Meet (const LBitValue &a, const LBitValue &b)
{
  if (b.isTop ())
    return a;
  else if (a.isTop ())
    return b;
  else {
    UINT64 zeromask, onemask;
    zeromask = a.zeromask_ & b.zeromask_;
    onemask = a.onemask_ & b.onemask_;
    return LBitValue (zeromask, onemask);
  }
}

const LBitValue Join (const LBitValue &a, const LBitValue &b)
{
  if (a.isTop ())
    return a;
  else if (b.isTop ())
    return b;
  else {
    UINT64 zeromask, onemask;
    zeromask = a.zeromask_ | b.zeromask_;
    onemask = a.onemask_ | b.onemask_;
    return LBitValue (zeromask, onemask);
  }
}


BOOL
LBitValue::StrictlyContains (const LBitValue &a) const
{
  if (isTop ())
    return FALSE;
  else if (a.isTop ())
    return TRUE;
  else {
    UINT64 valmask, avalmask;
    avalmask = a.zeromask_ ^ a.onemask_;
    valmask = zeromask_ ^ onemask_;
    return ((valmask & avalmask) == valmask
	    && ! Equal (a));
  }
}

BOOL
LBitValue::Equal (const LBitValue &a) const
{
  // If both are Top, they are equal
  // Else if both masks are equal, they are equal
  // Meaning we need to test masks only
  return (zeromask_ == a.zeromask_ && onemask_ == a.onemask_);
}

/* ====================================================================
 *
 * Support functions.
 *
 * ====================================================================
 */
static int byteperm(int mask, int val){
  // perform a 32-bit byte permutation, 
  // defined by the 8-bit value val
  int resmask = 0;
  for (int i = 0; i < 4; i++)
    resmask |= ((mask >> (8*(val >> (2*i) & 0x3))) & 0xff) << (8*i);
  return resmask;
}

// Queries

INT 
LBitValue::getlzcnt() const
{  
  if (isTop ())
    return 0;
  else {
    return countLeadingZeros64(~zeromask_);
  }
}

INT 
LBitValue::getlocnt() const
{  
  if (isTop ())
    return 0;
  else {
    return countLeadingZeros64(~onemask_);
  }
}

  

INT
LBitValue::bits () const
{
  if (isTop ())
    return 0;
  else {
    return 64 - countLeadingZeros64(~zeromask_);
  }
}

INT
LBitValue::gettzcnt () const
{
  if (isTop ())
    return 0;
  else {
    return countTrailingZeros64(~zeromask_);
  }
}

INT
LBitValue::gettocnt () const
{
  if (isTop ())
    return 0;
  else {
    return countTrailingZeros64(~onemask_);
  }
}

UINT64
LBitValue::getonemask () const
{
    return onemask_;
}

UINT64
LBitValue::getzeromask () const
{
    return zeromask_;
}

UINT64
LBitValue::getvalmask () const
{
    return zeromask_ ^ onemask_;
}

BOOL
LBitValue::hasValue () const
{
  // Recall: comparison returns false if either is non-finite.
  return ! isTop () && (~(zeromask_ ^ onemask_) == 0);
}

INT64
LBitValue::getValue () const
{
  FmtAssert (hasValue (), ("Called LiteralValue on non-literal Range"));
  return onemask_;
}


// Operators

const LBitValue MakeUnsigned(const LBitValue &a, INT width)
{
  if (width > 64) return LBitValue::Bottom(); // Don't support more
  if (a.isTop ())
    return a;
  else {
    UINT64 bmask_width = (width == 64) ? UINT64_MAX : ((UINT64)1 << width) - (UINT64)1;
    return LBitValue ((a.zeromask_ & bmask_width) | ~bmask_width, a.onemask_ & bmask_width);
  }
}


const LBitValue MakeSigned (const LBitValue &a, INT width)
{
  if (width > 64) return LBitValue::Bottom(); // Don't support more
  if (a.isTop ())
    return a;
  else {
    UINT64 bmask_width = (width == 64) ? UINT64_MAX :((UINT64)1 << width) - (UINT64)1;
    INT rwidth = 64 - width;
    UINT64 zeromask = (INT64)((a.zeromask_ & bmask_width) << rwidth) >> rwidth;
    UINT64 onemask = (INT64)((a.onemask_ & bmask_width) << rwidth) >> rwidth;
    return LBitValue (zeromask, onemask);
  }
}

const LBitValue SignExtend (const LBitValue &a, INT width)
{
  return MakeSigned (a, width);
}

const LBitValue ZeroExtend (const LBitValue &a, INT width)
{
  return MakeUnsigned (a, width);
}

const LBitValue LeftShift (const LBitValue &a, INT width)
{
  if (a.isTop ())
    return a;
  else if (width < 0)
    return LBitValue::Bottom ();
  else {
    UINT64 zeromask = (width >= 64) ? UINT64_MAX : (a.zeromask_ << width) | ~(UINT64_MAX << width);
    UINT64 onemask = (width >= 64) ? 0 : a.onemask_ << width;
    return LBitValue (zeromask, onemask);
  }
}


const LBitValue LeftShiftRange (const LBitValue &a, const LBitValue &b)
{
  if (a.isTop () || b.isTop ())
    return LBitValue::Top ();
  else if (!b.hasValue ()) 
    return LBitValue::Bottom ();
  else { // b is a literal
    return LeftShift (a, b.getValue ());
  }
}

const LBitValue RightShift (const LBitValue &a, INT width)
{
  if (a.isTop ())
    return a;
  else {
    UINT64 zeromask = ((UINT)width >= 64) ? ((INT64)a.zeromask_ >> 63) :((INT64)a.zeromask_ >> width);
    UINT64 onemask = ((UINT)width >= 64) ? ((INT64)a.onemask_ >> 63) :((INT64)a.onemask_ >> width);
    return LBitValue (zeromask, onemask);
  }
}


const LBitValue RightShiftRange (const LBitValue &a, const LBitValue &b)
{
  if (a.isTop () || b.isTop ())
    return LBitValue::Top ();
  else if (!b.hasValue ()) 
    return LBitValue::Bottom ();
  else { // b is a literal
    return RightShift (a, b.getValue ());
  }
}


const LBitValue BitOr (const LBitValue &a, const LBitValue &b)
{
  if (a.isTop ())
    return b;
  else  if (b.isTop ())
    return a;
  else {
    UINT64 zeromask =  a.zeromask_ & b.zeromask_;
    UINT64 onemask = a.onemask_ | b.onemask_;
    return LBitValue (zeromask, onemask);
  }
}

const LBitValue BitAnd (const LBitValue &a, const LBitValue &b)
{
  if (a.isTop ())
    return a;
  else  if (b.isTop ())
    return b;
  else {
    UINT64 zeromask =  a.zeromask_ | b.zeromask_;
    UINT64 onemask = a.onemask_ & b.onemask_;
    return LBitValue (zeromask, onemask);
  }
}

const LBitValue BitXor (const LBitValue &a, const LBitValue &b)
{
  if (a.isTop ())
    return b;
  else  if (b.isTop ())
    return a;
  else {
    UINT64 zeromask = (a.zeromask_ & b.zeromask_) | (a.onemask_ & b.onemask_);
    UINT64 onemask = (a.zeromask_ & b.onemask_) | (a.onemask_ & b.zeromask_);
    return LBitValue (zeromask, onemask);
  }
}

const LBitValue BitNot (const LBitValue &a)
{
  if (a.isTop ())
    return a;
  else {
    UINT64 zeromask =  a.onemask_;
    UINT64 onemask = a.zeromask_;
    return LBitValue (zeromask, onemask);
  }
}


const LBitValue Extract (const LBitValue &a, INT start, INT width)
{
  if (start + width > 64) return LBitValue::Bottom(); // Don't support more
  UINT64 mask = ((UINT64)1 << width) - 1;
  return BitAnd (RightShift (a, start),
		    LBitValue ((INT64)mask));
}      

const LBitValue Insert (const LBitValue &a, INT start, INT width,
		    const LBitValue &b)
{
  if (start + width > 64) return LBitValue::Bottom(); // Don't support more
  UINT64 mask = ((UINT64)1 << width) - 1;
  return BitOr (LeftShift (BitAnd (b, LBitValue ((INT64)mask)),
			   start),
		a);
}


const LBitValue BytePermute(const LBitValue &a, INT mask)
{
  FmtAssert ((mask & 0xff) == mask, ("Attempt to use a non-valid byte permutation mask value"));  
  if (a.isTop ())
    return a;
  else {
    UINT64 zeromask = byteperm(a.zeromask_, mask);
    UINT64 onemask = byteperm(a.onemask_, mask) ;
    return LBitValue (zeromask, onemask);
  }
}


// Tracing helper

void
LBitValue::Print (FILE *f) const
{
  if (isTop ())
    fputs ("<top>", f);
  else if (isBottom ())
    fputs ("<bottom>", f);
  else {
    fprintf (f, "\t zeromask: %#llx \t onemask: %#llx", zeromask_, onemask_);
  }
}

// Constructors
LBitValue::LBitValue (RangeSign sign, INT bitwidth)
{
  FmtAssert (bitwidth >= 0, ("Attempt to construct a range with negative bitwidth"));
  if (bitwidth >= 64) {
    zeromask_= (UINT64)0;
    onemask_= (UINT64)0;
  }
  else {
    if (sign == Unsigned){
      zeromask_ = UINT64_MAX << bitwidth;
      onemask_= (UINT64)0;
    }
    else if (sign == Signed) {
      zeromask_ = (UINT64)0;
      onemask_= (UINT64)0;      
    }
    else {
      FmtAssert (FALSE, ("Unknown sign value"));
    }
  }
}

