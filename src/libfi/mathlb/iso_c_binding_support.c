/*
 * Copyright (C) 2008. PathScale Inc. All Rights Reserved.

   Path64 is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation version 3

   Path64 is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with Path64; see the file COPYING.  If not, write to the Free
   Software Foundation, 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

 */

/* Support for procedures in ISO_C_BINDING intrinsic module */

#include <stdint.h>
#include "cray/dopevec.h"

/*
 * Make fptr have the same target as cptr, with the shape of "shape"
 *
 * cptr		type(c_ptr) by reference
 * fptr		Fortran array pointer by reference
 * shape	Fortran rank-1 integer array by reference
 */

/* We don't actually know whether the target was allocated via a pointer
 * (versus being an allocatable variable, or even a variable which wasn't
 * dynamically allocated at all.) If we implemented association status
 * by means of a list of addresses, rather than by means of bits inside
 * the dope vector, then we would know. The issue is whether the user can
 * pass this pointer to "deallocate". To be safe, we say "yes", although
 * we can't conform completely to the standard (if the user passes the
 * pointer to "deallocate" and the target wasn't dynamically allocated,
 * we may crash inside "free" instead of setting the stat= variable.)
 *
 * The same issue arises in a case like this:
 *
 *     integer, pointer:: pa
 *     integer :: i
 *     allocate(pa)
 *     call s(pa)
 *     call s(i)
 *   contains
 *     subroutine s(a)
 *       integer, target :: a
 *       integer, pointer :: spa
 *       spa => a
 *       deallocate(spa, stat=i)
 *       print *, i .eq. 0
 *     end subroutine s
 *   end
 *
 * The program ought to print "T" then "F", but it is likely to crash
 * instead of printing "F"
 *
 * Note that we don't have a problem if the user tries to deallocate a
 * pointer whose target is an allocatable variable: the standard says the
 * user shall not do that, and doesn't specifically say that the
 * implementation must detect it (though one would like to.)
 */


void
_C_f_pointera1(void **cptr, DopeVectorType *fptr, int8_t *shape) {

  fptr->base_addr.a.ptr = *cptr;
  fptr->orig_base = *cptr;
  fptr->orig_size = 0; /* Alas, we don't know */

  /* As I read the standard, cptr shouldn't be a null pointer, but other
   * compilers allow this. */
  fptr->assoc = !!*cptr;

  fptr->ptr_alloc = 1;
  fptr->p_or_a = POINTTR;
  fptr->a_contig = 1;
  /* fptr->alloc_cpnt,
   * fptr->type_lens: fptr is supposed to be type-compatible with
   * target of cptr, so hopefully these are already set correctly */
  if (fptr->n_dim) {
    /* shape could be assumed-size, so no way to check that its length
     * matches rank of fptr */
    /* stride multiplier is in units of 32bits */
    /* this probably fails for derived types and char sequences */
    unsigned stride_mult = (fptr->type_lens.int_len > 32 ? fptr->type_lens.int_len / 32 : 1);
    int i = 0;
    for (; i < fptr->n_dim; i += 1) {
      int32_t extent = shape[i];
      fptr->dimension[i].low_bound = 1;
      fptr->dimension[i].extent = extent;
      fptr->dimension[i].stride_mult = stride_mult;
      stride_mult = stride_mult * extent;
    }
  }
}


void
_C_f_pointera2(void **cptr, DopeVectorType *fptr, int16_t *shape) {

  fptr->base_addr.a.ptr = *cptr;
  fptr->orig_base = *cptr;
  fptr->orig_size = 0; /* Alas, we don't know */

  /* As I read the standard, cptr shouldn't be a null pointer, but other
   * compilers allow this. */
  fptr->assoc = !!*cptr;

  fptr->ptr_alloc = 1;
  fptr->p_or_a = POINTTR;
  fptr->a_contig = 1;
  /* fptr->alloc_cpnt,
   * fptr->type_lens: fptr is supposed to be type-compatible with
   * target of cptr, so hopefully these are already set correctly */
  if (fptr->n_dim) {
    /* shape could be assumed-size, so no way to check that its length
     * matches rank of fptr */
    /* stride multiplier is in units of 32bits */
    /* this probably fails for derived types and char sequences */
    unsigned stride_mult = (fptr->type_lens.int_len > 32 ? fptr->type_lens.int_len / 32 : 1);
    int i = 0;
    for (; i < fptr->n_dim; i += 1) {
      int32_t extent = shape[i];
      fptr->dimension[i].low_bound = 1;
      fptr->dimension[i].extent = extent;
      fptr->dimension[i].stride_mult = stride_mult;
      stride_mult = stride_mult * extent;
    }
  }
}


void
_C_f_pointera4(void **cptr, DopeVectorType *fptr, int32_t *shape) {

  fptr->base_addr.a.ptr = *cptr;
  fptr->orig_base = *cptr;
  fptr->orig_size = 0; /* Alas, we don't know */

  /* As I read the standard, cptr shouldn't be a null pointer, but other
   * compilers allow this. */
  fptr->assoc = !!*cptr;

  fptr->ptr_alloc = 1;
  fptr->p_or_a = POINTTR;
  fptr->a_contig = 1;
  /* fptr->alloc_cpnt,
   * fptr->type_lens: fptr is supposed to be type-compatible with
   * target of cptr, so hopefully these are already set correctly */
  if (fptr->n_dim) {
    /* shape could be assumed-size, so no way to check that its length
     * matches rank of fptr */
    /* stride multiplier is in units of 32bits */
    /* this probably fails for derived types and char sequences */
    unsigned stride_mult = (fptr->type_lens.int_len > 32 ? fptr->type_lens.int_len / 32 : 1);
    int i = 0;
    for (; i < fptr->n_dim; i += 1) {
      int32_t extent = shape[i];
      fptr->dimension[i].low_bound = 1;
      fptr->dimension[i].extent = extent;
      fptr->dimension[i].stride_mult = stride_mult;
      stride_mult = stride_mult * extent;
    }
  }
}


void
_C_f_pointera8(void **cptr, DopeVectorType *fptr, int64_t *shape) {

  fptr->base_addr.a.ptr = *cptr;
  fptr->orig_base = *cptr;
  fptr->orig_size = 0; /* Alas, we don't know */

  /* As I read the standard, cptr shouldn't be a null pointer, but other
   * compilers allow this. */
  fptr->assoc = !!*cptr;

  fptr->ptr_alloc = 1;
  fptr->p_or_a = POINTTR;
  fptr->a_contig = 1;
  /* fptr->alloc_cpnt,
   * fptr->type_lens: fptr is supposed to be type-compatible with
   * target of cptr, so hopefully these are already set correctly */
  if (fptr->n_dim) {
    /* shape could be assumed-size, so no way to check that its length
     * matches rank of fptr */
    /* stride multiplier is in units of 32bits */
    /* this probably fails for derived types and char sequences */
    unsigned stride_mult = (fptr->type_lens.int_len > 32 ? fptr->type_lens.int_len / 32 : 1);
    int i = 0;
    for (; i < fptr->n_dim; i += 1) {
      int32_t extent = shape[i];
      fptr->dimension[i].low_bound = 1;
      fptr->dimension[i].extent = extent;
      fptr->dimension[i].stride_mult = stride_mult;
      stride_mult = stride_mult * extent;
    }
  }
}



/*
 * Like _C_f_pointera_, but for scalar pointers
 */
void
_C_f_pointers(void **cptr, DopeVectorType *fptr) {
  fptr->base_addr.a.ptr = *cptr;
  fptr->orig_base = *cptr;
  fptr->orig_size = 0; /* Alas, we don't know */
  fptr->assoc = !!*cptr;
  fptr->ptr_alloc = 1;
  fptr->p_or_a = POINTTR;
  fptr->a_contig = 1;
}

/* Keep around for backwards compatibility after pathf90 learns to emit "loc"
  operator for this */
void *
_C_loc(void *p) {
  return p;
  }

/* Keep around for backwards compatibility after pathf90 learns to emit "loc"
  operator for this */
void *
_C_funloc(void *p) {
  return p;
  }

int
_C_associated_ptr(void *p, void **q) {
  return q ? (p && (p == *q)) : !!p;
  }

int
_C_associated_funptr(void *p, void **q) {
  return q ? (p && (p == *q)) : !!p;
  }

