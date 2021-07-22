/* -*- C++ -*- */
// Handle_Set.i,v 4.17 1999/12/07 03:35:49 irfan Exp

// Handle_Set.i

// Initialize the bitmask to all 0s and reset the associated fields.

#if defined (PDL_HAS_WINCE)
PDL_INLINE
PDL_Handle_Set::~PDL_Handle_Set (void)
{
  PDL_TRACE ("PDL_Handle_Set::~PDL_Handle_Set");
}
#endif /* PDL_HAS_WINCE */

PDL_INLINE void
PDL_Handle_Set::reset (void)
{
  PDL_TRACE ("PDL_Handle_Set::reset");
  this->max_handle_ =
    PDL_INVALID_SOCKET;
#if defined (PDL_HAS_BIG_FD_SET)
  this->min_handle_ =
    NUM_WORDS * WORDSIZE;
#endif /* PDL_HAS_BIG_FD_SET */
  this->size_ = 0;
// PR-8886  
//#if !defined (PDL_HAS_BIG_FD_SET)
  FD_ZERO (&this->mask_);
//#endif /* PDL_HAS_BIG_FD_SET */
}

#if defined (PDL_HAS_BIG_FD_SET)
PDL_INLINE PDL_Handle_Set &
PDL_Handle_Set::operator= (const PDL_Handle_Set &rhs)
{
  PDL_TRACE ("PDL_Handle_Set::reset");

  if (rhs.size_ > 0)
    {
      this->size_ =
        rhs.size_;
      this->max_handle_ =
        rhs.max_handle_;
      this->min_handle_ =
        rhs.min_handle_;
      this->mask_ =
        rhs.mask_;
    }
  else
    this->reset ();

  return *this;
}
#endif /* PDL_HAS_BIG_FD_SET */

// Returns the number of the large bit.

PDL_INLINE PDL_SOCKET
PDL_Handle_Set::max_set (void) const
{
  PDL_TRACE ("PDL_Handle_Set::max_set");
  return this->max_handle_;
}

// Checks whether handle is enabled.

PDL_INLINE int
PDL_Handle_Set::is_set (PDL_SOCKET handle) const
{
  PDL_TRACE ("PDL_Handle_Set::is_set");
#if defined (PDL_HAS_BIG_FD_SET)
  return FD_ISSET (handle,
                   &this->mask_)
    && this->size_ > 0;
#else
// bug : in aix, this return type is long, so type casting to boolean.
// by gamestar
  return (FD_ISSET (handle,
                   &this->mask_)) != 0;
#endif /* PDL_HAS_BIG_FD_SET */
}

// Enables the handle.

PDL_INLINE void
PDL_Handle_Set::set_bit (PDL_SOCKET handle)
{
  PDL_TRACE ("PDL_Handle_Set::set_bit");
  if (!this->is_set (handle))
    {
#if defined (PDL_WIN32)
      FD_SET ((SOCKET) handle,
              &this->mask_);
      this->size_++;
#else /* PDL_WIN32 */
#if defined (PDL_HAS_BIG_FD_SET)
// PR-8886      
//       if (this->size_ == 0)
//         FD_ZERO (&this->mask_);

      if (handle < this->min_handle_)
        this->min_handle_ = handle;
#endif /* PDL_HAS_BIG_FD_SET */

      FD_SET (handle,
              &this->mask_);
      this->size_++;

      if (handle > this->max_handle_)
        this->max_handle_ = handle;
#endif /* PDL_WIN32 */
    }
}

// Disables the handle.

PDL_INLINE void
PDL_Handle_Set::clr_bit (PDL_SOCKET handle)
{
  PDL_TRACE ("PDL_Handle_Set::clr_bit");

  if (this->is_set (handle))
    {
      FD_CLR ((PDL_SOCKET) handle,
              &this->mask_);
      this->size_--;

#if !defined (PDL_WIN32)
      if (handle == this->max_handle_)
        this->set_max (this->max_handle_);
#endif /* !PDL_WIN32 */
    }
}

// Returns a count of the number of enabled bits.

PDL_INLINE int
PDL_Handle_Set::num_set (void) const
{
  PDL_TRACE ("PDL_Handle_Set::num_set");
#if defined (PDL_WIN32)
  return this->mask_.fd_count;
#else /* !PDL_WIN32 */
  return this->size_;
#endif /* PDL_WIN32 */
}

// Returns a pointer to the underlying fd_set.

PDL_INLINE
PDL_Handle_Set::operator fd_set *()
{
  PDL_TRACE ("PDL_Handle_Set::operator PDL_FD_SET_TYPE *");

  if (this->size_ > 0)
    return (fd_set *) &this->mask_;
  else
    return (fd_set *) NULL;
}

// Returns a pointer to the underlying fd_set.

PDL_INLINE fd_set *
PDL_Handle_Set::fdset (void)
{
  PDL_TRACE ("PDL_Handle_Set::operator PDL_FD_SET_TYPE *");

  if (this->size_ > 0)
    return (fd_set *) &this->mask_;
  else
    return (fd_set *) NULL;
}

PDL_INLINE fd_set *
PDL_Handle_Set::getmask (void)
{
  return (fd_set *) &this->mask_;
}

PDL_INLINE
PDL_Handle_Set_Iterator::~PDL_Handle_Set_Iterator (void)
{
}
