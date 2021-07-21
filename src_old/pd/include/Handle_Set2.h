/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 
/* -*- C++ -*- */
// Handle_Set.h,v 4.21 1999/12/07 03:35:49 irfan Exp

// ============================================================================
//
// = LIBRARY
//    pdl
//
// = FILENAME
//    Handle_Set.h
//
// = AUTHOR
//    Douglas C. Schmidt <schmidt@cs.wustl.edu>
//
// ============================================================================

#ifndef PDL_HANDLE_SET_H
#define PDL_HANDLE_SET_H

#include "PDL2.h"

#if !defined (PDL_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* PDL_LACKS_PRAGMA_ONCE */

class PDL_Export PDL_Handle_Set
{
  // = TITLE
  //     C++ wrapper facade for the socket <fd_set> abstraction.
  //
  // = DESCRIPTION
  //     This abstraction is a very efficient wrapper facade over
  //     <fd_set>.  In particular, no range checking is performed, so
  //     it's important not to set or clear bits that are outside the
  //     <PDL_DEFAULT_SELECT_REACTOR_SIZE>.
public:
  friend class PDL_Handle_Set_Iterator;

  // = Initialization and termination.

  enum
  {
    MAXSIZE = PDL_DEFAULT_SELECT_REACTOR_SIZE
  };

  // = Initialization methods.
  PDL_Handle_Set (void);
  // Constructor, initializes the bitmask to all 0s.

  PDL_Handle_Set (const PDL_FD_SET_TYPE &mask);

#if defined (PDL_HAS_WINCE)
  ~PDL_Handle_Set (void);
  // Default dtor.
#endif /* PDL_HAS_WINCE */

  // = Methods for manipulating bitsets.
  void reset (void);
  // Initialize the bitmask to all 0s and reset the associated fields.

  int is_set (PDL_SOCKET handle) const;
  // Checks whether <handle> is enabled.  No range checking is
  // performed so <handle> must be less than
  // <PDL_DEFAULT_SELECT_REACTOR_SIZE>.

  void set_bit (PDL_SOCKET handle);
  // Enables the <handle>.  No range checking is performed so <handle>
  // must be less than <PDL_DEFAULT_SELECT_REACTOR_SIZE>.

  void clr_bit (PDL_SOCKET handle);
  // Disables the <handle>.  No range checking is performed so
  // <handle> must be less than <PDL_DEFAULT_SELECT_REACTOR_SIZE>.

  int num_set (void) const;
  // Returns a count of the number of enabled bits.

  PDL_SOCKET max_set (void) const;
  // Returns the number of the large bit.

  void sync (PDL_SOCKET max);
  // Synchronize the underlying <fd_set> with the <max_handle> and the
  // <size>.

  operator fd_set *();
  // Returns a pointer to the underlying <fd_set>.  Returns 0 if
  // <size_> == 0.

  fd_set *fdset (void);
  // Returns a pointer to the underlying <fd_set>.  Returns 0 if
  // <size_> == 0.

  fd_set *getmask(void); // by gamestar
    
#if defined (PDL_HAS_BIG_FD_SET)
  PDL_Handle_Set & operator= (const PDL_Handle_Set &);
  // Assignment operator optimizes for cases where <size_> == 0.
#endif /* PDL_HAS_BIG_FD_SET */

  void dump (void) const;
  // Dump the state of an object.

  PDL_ALLOC_HOOK_DECLARE;
  // Declare the dynamic allocation hooks.

private:
  int size_;
  // Size of the set, i.e., a count of the number of enabled bits.

  PDL_SOCKET max_handle_;
  // Current max handle.

#if defined (PDL_HAS_BIG_FD_SET)
  PDL_SOCKET min_handle_;
  // Current min handle.
#endif /* PDL_HAS_BIG_FD_SET */

  fd_set mask_;
  // Bitmask.

  enum
  {
    WORDSIZE = NFDBITS,
#if !defined (PDL_WIN32)
    NUM_WORDS = howmany (MAXSIZE, NFDBITS),
#endif /* PDL_WIN32 */
    NBITS = 256
  };

  static int count_bits (u_long n);
  // Counts the number of bits enabled in N.  Uses a table lookup to
  // speed up the count.

#if defined (PDL_HAS_BIG_FD_SET)
  static int bitpos (u_long bit);
  // Find the position of the bit counting from right to left.
#endif /* PDL_HAS_BIG_FD_SET */

  void set_max (PDL_SOCKET max);
  // Resets the <max_handle_> after a clear of the original
  // <max_handle_>.

  static const char nbits_[NBITS];
  // Table that maps bytes to counts of the enabled bits in each value
  // from 0 to 255.
};

class PDL_Export PDL_Handle_Set_Iterator
{
  // = TITLE
  //     Iterator for the <PDL_Handle_Set> abstraction.
public:
  PDL_Handle_Set_Iterator (const PDL_Handle_Set &hs);
  // Constructor.

  ~PDL_Handle_Set_Iterator (void);
  // Default dtor.

  PDL_SOCKET operator () (void);
  // "Next" operator.  Returns the next unseen <PDL_HANDLE> in the
  // <Handle_Set> up to <handle_set_.max_handle_>).  When all the
  // handles have been seen returns <PDL_INVALID_HANDLE>.  Advances
  // the iterator automatically, so you need not call <operator++>
  // (which is now obsolete).

  void operator++ (void);
  // This is a no-op and no longer does anything.  It's only here for
  // backwards compatibility.

  void dump (void) const;
  // Dump the state of an object.

  PDL_ALLOC_HOOK_DECLARE;
  // Declare the dynamic allocation hooks.

private:
  const PDL_Handle_Set &handles_;
  // The <Handle_Set> we are iterating through.

#if defined (PDL_WIN32)
  u_int handle_index_;
#elif !defined (PDL_HAS_BIG_FD_SET)
  int handle_index_;
#elif defined (PDL_HAS_BIG_FD_SET)
  int handle_index_;
  u_long oldlsb_;
#endif /* PDL_WIN32 */
  // Index of the bit we're examining in the current <word_num_> word.

  int word_num_;
  // Number of the word we're iterating over (typically between 0..7).

#if defined (PDL_HAS_BIG_FD_SET)
  int word_max_;
  // Number max of the words with a possible bit on.
#endif /* PDL_HAS_BIG_FD_SET */

#if !defined (PDL_WIN32) && !defined (PDL_HAS_BIG_FD_SET)
  fd_mask word_val_;
  // Value of the bits in the word we're iterating on.
#elif !defined (PDL_WIN32) && defined (PDL_HAS_BIG_FD_SET)
  u_long word_val_;
  // Value of the bits in the word we're iterating on.
#endif /* !PDL_WIN32 && !PDL_HAS_BIG_FD_SET */
};

#if defined (__PDL_INLINE__)
#include "Handle_Set2.i"
#endif /* __PDL_INLINE__ */

#endif /* PDL_HANDLE_SET */
