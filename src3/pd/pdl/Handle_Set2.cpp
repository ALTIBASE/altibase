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
 
// Handle_Set.cpp
// Handle_Set.cpp,v 4.30 1999/10/19 17:55:40 othman Exp

#define PDL_BUILD_DLL
#include "Handle_Set2.h"

#if !defined (__PDL_INLINE__)
#include "Handle_Set2.i"
#endif /* __PDL_INLINE__ */

PDL_RCSID(pdl, Handle_Set, "Handle_Set.cpp,v 4.30 1999/10/19 17:55:40 othman Exp")

PDL_ALLOC_HOOK_DEFINE(PDL_Handle_Set)

#if defined (linux) && __GLIBC__ > 1 && __GLIBC_MINOR__ >= 1 && !defined (_XOPEN_SOURCE)
  // XPG4.2 requires the fds_bits member name, so it is not enabled by
  // default on Linux/glibc-2.1.x systems.  Instead use "__fds_bits."
  // Ugly, but "what are you going to do?" 8-)
#define fds_bits __fds_bits
#endif  /* linux && __GLIBC__ > 1 && __GLIBC_MINOR__ >= 1 && !_GNU_SOURCE */

    
void
PDL_Handle_Set::dump (void) const
{
  PDL_TRACE ("PDL_Handle_Set::dump");

  PDL_OS::fprintf(stderr, ASYS_TEXT("\nsize_ = %d"), this->size_);
  PDL_OS::fprintf(stderr, ASYS_TEXT("\nmax_handle_ = %d \n"), this->max_handle_);

#if defined (PDL_WIN32)
  for (size_t i = 0; i < (size_t) this->mask_.fd_count + 1; i++) 
    PDL_DEBUG ((LM_DEBUG, ASYS_TEXT(" %x "), this->mask_.fd_array[i]));
#else /* !PDL_WIN32 */
  for (PDL_SOCKET i = 0; i < this->max_handle_ + 1; i++)
  {
    if (this->is_set (i))
    {
        PDL_OS::fprintf(stderr, " %d ", i);
    }
  }
#endif /* PDL_WIN32 */
  PDL_OS::fprintf(stderr, ASYS_TEXT(" \n")); PDL_OS::fflush(stderr);
}

// Table that maps bytes to counts of the enabled bits in each value
// from 0 to 255,
// 
// nbits_[0] == 0
//
// because there are no bits enabled for the value 0.
//
// nbits_[5] == 2
//
// because there are 2 bits enabled in the value 5, i.e., it's
// 101 in binary.

const char PDL_Handle_Set::nbits_[256] = 
{
  0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
  1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
  1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
  1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
  3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
  1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
  3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
  3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
  3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
  4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8,
};

// Constructor, initializes the bitmask to all 0s.

PDL_Handle_Set::PDL_Handle_Set (void)
{
  PDL_TRACE ("PDL_Handle_Set::PDL_Handle_Set");
  this->reset ();
}

PDL_Handle_Set::PDL_Handle_Set (const PDL_FD_SET_TYPE &fd_mask)
{
  PDL_TRACE ("PDL_Handle_Set::PDL_Handle_Set");
  this->reset ();
  PDL_OS::memcpy ((void *) &this->mask_,
		  (void *) &fd_mask, 
		  sizeof this->mask_);  
#if !defined (PDL_WIN32)
  this->sync (PDL_Handle_Set::MAXSIZE);
#if defined (PDL_HAS_BIG_FD_SET)
  this->min_handle_ = 0;
#endif /* PDL_HAS_BIG_FD_SET */
#endif /* !PDL_WIN32 */
}

// Counts the number of bits enabled in N.  Uses a table lookup to
// speed up the count.

int
PDL_Handle_Set::count_bits (u_long n)
{

 PDL_TRACE ("PDL_Handle_Set::count_bits");
#if defined (PDL_HAS_HANDLE_SET_OPTIMIZED_FOR_SELECT)
  register int rval = 0;

  // Count the number of enabled bits in <n>.  This algorithm is very
  // fast, i.e., O(enabled bits in n).

  for (register u_long m = n;
       m != 0;
       m &= m - 1)
    rval++;

  return rval;
#else
   return (PDL_Handle_Set::nbits_[n & 0xff] 
 	  + PDL_Handle_Set::nbits_[(n >> 8) & 0xff] 
 	  + PDL_Handle_Set::nbits_[(n >> 16) & 0xff] 
  	  + PDL_Handle_Set::nbits_[(n >> 24) & 0xff]);
#endif /* PDL_HAS_HANDLE_SET_OPTIMIZED_FOR_SELECT */
}

#if defined (PDL_HAS_BIG_FD_SET)
// Find the bit position counting from right to left worst case
// (1<<31) is 8.

int 
PDL_Handle_Set::bitpos (u_long bit)
{
  register int l = 0;
  register u_long n = bit - 1;

  // This is a fast count method when have the most significative bit.

  while (n >> 8) 
    {
      n >>= 8;
      l += 8;
    }

  // Is greater than 15?
  if (n & 16) 
    {
      n >>= 4;
      l += 4;
    }

  // Count number remaining bits.
  while (n != 0) 
    {
      n &= n - 1;
      l++;
    }
  return l;
}
#endif /* PDL_HAS_BIG_FD_SET */

// Synchronize the underlying FD_SET with the MAX_FD and the SIZE.

#if defined (PDL_USE_SHIFT_FOR_EFFICIENCY)
// These don't work because shifting right 3 bits is not the same as
// dividing by 3, e.g., dividing by 8 requires shifting right 3 bits.
// In order to do the shift, we need to calculate the number of bits
// at some point.
#define PDL_DIV_BY_WORDSIZE(x) ((x) >> (PDL_Handle_Set::WORDSIZE))
#define PDL_MULT_BY_WORDSIZE(x) ((x) << (PDL_Handle_Set::WORDSIZE))
#else
#define PDL_DIV_BY_WORDSIZE(x) ((x) / (PDL_Handle_Set::WORDSIZE))
#define PDL_MULT_BY_WORDSIZE(x) ((x) * (PDL_Handle_Set::WORDSIZE))
#endif /* PDL_USE_SHIFT_FOR_EFFICIENCY */

void
PDL_Handle_Set::sync (PDL_SOCKET max)
{
  PDL_TRACE ("PDL_Handle_Set::sync");
#if !defined (PDL_WIN32)
  fd_mask *maskp = (fd_mask *)(this->mask_.fds_bits);
  this->size_ = 0;

  for (int i = PDL_DIV_BY_WORDSIZE (max - 1);
       i >= 0; 
       i--)
    this->size_ += PDL_Handle_Set::count_bits (maskp[i]);

  this->set_max (max);
#else
  PDL_UNUSED_ARG (max);
#endif /* !PDL_WIN32 */
}

// Resets the MAX_FD after a clear of the original MAX_FD.

void
PDL_Handle_Set::set_max (PDL_SOCKET current_max)
{
  PDL_TRACE ("PDL_Handle_Set::set_max");
#if !defined(PDL_WIN32)
  fd_mask * maskp = (fd_mask *)(this->mask_.fds_bits);

  if (this->size_ == 0)
    this->max_handle_ = PDL_INVALID_SOCKET;
  else
    {
      int i;

      for (i = PDL_DIV_BY_WORDSIZE(current_max - 1);
	   maskp[i] == 0; 
	   i--)
	continue;

#if 1 /* !defined(PDL_HAS_BIG_FD_SET) */
      this->max_handle_ = PDL_MULT_BY_WORDSIZE(i);
      for (fd_mask val = maskp[i]; 
	   (val & ~1) != 0; // This obscure code is needed since "bit 0" is in location 1...
	   val = (val >> 1) & PDL_MSB_MASK)
	this->max_handle_++;
#else
      register u_long val = this->mask_.fds_bits[i];
      this->max_handle_ = PDL_MULT_BY_WORDSIZE(i)
	+ PDL_Handle_Set::bitpos(val & ~(val - 1));
#endif /* 1 */
    }

  // Do some sanity checking...
  if (this->max_handle_ >= PDL_Handle_Set::MAXSIZE)
    this->max_handle_ = PDL_Handle_Set::MAXSIZE - 1;
#else
  PDL_UNUSED_ARG (current_max);
#endif /* !PDL_WIN32 */
}

PDL_ALLOC_HOOK_DEFINE(PDL_Handle_Set_Iterator)

void
PDL_Handle_Set_Iterator::dump (void) const
{
  PDL_TRACE ("PDL_Handle_Set_Iterator::dump");

  PDL_DEBUG ((LM_DEBUG, PDL_BEGIN_DUMP, this));
#if defined(PDL_WIN32) || !defined(PDL_HAS_BIG_FD_SET)
  PDL_DEBUG ((LM_DEBUG, ASYS_TEXT("\nhandle_index_ = %d"), this->handle_index_));
#elif defined(PDL_HAS_BIG_FD_SET)
  PDL_DEBUG ((LM_DEBUG, ASYS_TEXT("\nword_max_ = %d"), this->word_max_));
  PDL_DEBUG ((LM_DEBUG, ASYS_TEXT("\nword_val_ = %d"), this->word_val_));
#endif
  PDL_DEBUG ((LM_DEBUG, ASYS_TEXT("\nword_num_ = %d"), this->word_num_));
  PDL_DEBUG ((LM_DEBUG, PDL_END_DUMP));
}

PDL_SOCKET
PDL_Handle_Set_Iterator::operator () (void)
{
  PDL_TRACE ("PDL_Handle_Set_Iterator::operator");
#if defined (PDL_WIN32)
  if (this->handle_index_ < this->handles_.mask_.fd_count)
    // Return the handle and advance the iterator.
    return (PDL_SOCKET) this->handles_.mask_.fd_array[this->handle_index_++];
  else 
    return PDL_INVALID_SOCKET;

#elif !defined (PDL_HAS_BIG_FD_SET) /* !PDL_WIN32 */
  // No sense searching further than the max_handle_ + 1;
  PDL_SOCKET maxhandlep1 = this->handles_.max_handle_ + 1;

  // HP-UX 11 plays some games with the fd_mask type - fd_mask is
  // defined as an int_32t, but the fds_bits is an array of longs.
  // This makes plainly indexing through the array by hand tricky,
  // since the FD_* macros treat the array as int32_t.  So the bits
  // are in the right place for int32_t, even though the array is
  // long.  This, they say, is to preserve the same in-memory layout
  // for 32-bit and 64-bit processes.  So, we play the same game as
  // the FD_* macros to get the bits right.  On all other systems,
  // this amounts to practically a NOP, since this is what would have
  // been done anyway, without all this type jazz.
  fd_mask * maskp = (fd_mask *)(this->handles_.mask_.fds_bits);

  if (this->handle_index_ >= maxhandlep1)
    // We've seen all the handles we're interested in seeing for this
    // iterator.
    return PDL_INVALID_SOCKET;
  else
    {
      PDL_SOCKET result = this->handle_index_;

      // Increment the iterator and advance to the next bit in this
      // word.
      this->handle_index_++;
      this->word_val_ = (this->word_val_ >> 1) & PDL_MSB_MASK;

      // If we've examined all the bits in this word, we'll go onto
      // the next word.

      if (this->word_val_ == 0) 
	{
	  // Start the handle_index_ at the beginning of the next word
	  // and then loop until we've found the first non-zero bit or
	  // we run past the <maxhandlep1> of the bitset.

	  for (this->handle_index_ = PDL_MULT_BY_WORDSIZE(++this->word_num_);
	       this->handle_index_ < maxhandlep1
		 && maskp[this->word_num_] == 0;
	       this->word_num_++)
	    this->handle_index_ += PDL_Handle_Set::WORDSIZE;
	  
	  // If the bit index becomes >= the maxhandlep1 that means
	  // there weren't any more bits set that we want to consider.
	  // Therefore, we'll just store the maxhandlep1, which will
	  // cause <operator()> to return <PDL_INVALID_HANDLE>
	  // immediately next time it's called.
	  if (this->handle_index_ >= maxhandlep1)
	    {
	      this->handle_index_ = maxhandlep1;
	      return result;
	    }
	  else
	    // Load the bits of the next word.
	    this->word_val_ = maskp[this->word_num_];
	}

      // Loop until we get <word_val_> to have its least significant
      // bit enabled, keeping track of which <handle_index> this
      // represents (this information is used by subsequent calls to
      // <operator()>).

      for (;
	   PDL_BIT_DISABLED (this->word_val_, 1);
	   this->handle_index_++)
	this->word_val_ = (this->word_val_ >> 1) & PDL_MSB_MASK;

      return result;
    }
#else /* !PDL_HAS_BIG_FD_SET */
   // Find the first word in fds_bits with bit on
   register u_long lsb = this->word_val_;

   if (lsb == 0)
     {
       do
	 {
	   // We have exceeded the word count in Handle_Set?
	   if (++this->word_num_ >= this->word_max_)
	     return PDL_INVALID_SOCKET;

	   lsb = this->handles_.mask_.fds_bits[this->word_num_];
	 }
       while (lsb == 0);

       // Set index to word boundary.
       this->handle_index_ = PDL_MULT_BY_WORDSIZE(this->word_num_);

       // Put new word_val.
       this->word_val_ = lsb;

       // Find the least significative bit.
       lsb &= ~(lsb - 1);

       // Remove least significative bit.
       this->word_val_ ^= lsb;

       // Save to calculate bit distance.
       this->oldlsb_ = lsb;

       // Move index to least significative bit.
       while (lsb >>= 1)
	 this->handle_index_++;
     }
    else 
      {
	// Find the least significative bit.
	lsb &= ~(lsb - 1);

	// Remove least significative bit.
	this->word_val_ ^= lsb;

	register u_long n = lsb - this->oldlsb_;

	// Move index to bit distance between new lsb and old lsb.
	do
	  {
	    this->handle_index_++;
	    n &= n >> 1;
	  }
	while (n != 0);

	this->oldlsb_ = lsb;
      }

   return this->handle_index_;
#endif /* PDL_WIN32 */
}

void
PDL_Handle_Set_Iterator::operator++ (void)
{
  PDL_TRACE ("PDL_Handle_Set_Iterator::operator++");

  // This is now a no-op.
}

PDL_Handle_Set_Iterator::PDL_Handle_Set_Iterator (const PDL_Handle_Set &hs)
  : handles_ (hs), 
#if !defined (PDL_HAS_BIG_FD_SET) || defined (PDL_WIN32)
    handle_index_ (0),
    word_num_ (-1)
#elif defined (PDL_HAS_BIG_FD_SET)
    oldlsb_ (0),
    word_max_ (hs.max_handle_ == PDL_INVALID_SOCKET 
	       ? 0 
               : ((PDL_DIV_BY_WORDSIZE (hs.max_handle_)) + 1))
#endif /* PDL_HAS_BIG_FD_SET */
{
  PDL_TRACE ("PDL_Handle_Set_Iterator::PDL_Handle_Set_Iterator");
#if !defined (PDL_WIN32) && !defined (PDL_HAS_BIG_FD_SET)
  // No sense searching further than the max_handle_ + 1;
  PDL_SOCKET maxhandlep1 =
    this->handles_.max_handle_ + 1;

  fd_mask *maskp =
    (fd_mask *)(this->handles_.mask_.fds_bits);

  // Loop until we've found the first non-zero bit or we run past the
  // <maxhandlep1> of the bitset.
  while (this->handle_index_ < maxhandlep1
	 && maskp[++this->word_num_] == 0)
    this->handle_index_ += PDL_Handle_Set::WORDSIZE;

  // If the bit index becomes >= the maxhandlep1 that means there
  // weren't any bits set.  Therefore, we'll just store the
  // maxhandlep1, which will cause <operator()> to return
  // <PDL_INVALID_HANDLE> immediately.
  if (this->handle_index_ >= maxhandlep1)
    this->handle_index_ = maxhandlep1;
  else
    // Loop until we get <word_val_> to have its least significant bit
    // enabled, keeping track of which <handle_index> this represents
    // (this information is used by <operator()>).
    for (this->word_val_ = maskp[this->word_num_];
	 PDL_BIT_DISABLED (this->word_val_, 1)
	   && this->handle_index_ < maxhandlep1;
	 this->handle_index_++)
      this->word_val_ = (this->word_val_ >> 1) & PDL_MSB_MASK;
#elif !defined (PDL_WIN32) && defined (PDL_HAS_BIG_FD_SET)
    if (this->word_max_==0) 
      {
	this->word_num_ = -1;
	this->word_val_ = 0;
      }
    else
      {
	this->word_num_ =
          PDL_DIV_BY_WORDSIZE (this->handles_.min_handle_) - 1;
	this->word_val_ = 0;
      }
#endif /* !PDL_WIN32 && !PDL_HAS_BIG_FD_SET */
}
