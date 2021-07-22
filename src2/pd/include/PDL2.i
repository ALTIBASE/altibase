/* -*- C++ -*- */
// PDL.i,v 4.25 1999/12/07 03:35:49 irfan Exp

// Wrappers for methods that have been moved to PDL_OS.

ASYS_INLINE ssize_t
PDL::read_n (PDL_HANDLE handle,
             void *buf,
             size_t len)
{
  return PDL_OS::read_n (handle, buf, len);
}

ASYS_INLINE ssize_t
PDL::write_n (PDL_HANDLE handle,
              const void *buf,
              size_t len)
{
  return PDL_OS::write_n (handle, buf, len);
}

ASYS_INLINE ssize_t
PDL::send_i (PDL_SOCKET handle, const void *buf, size_t len)
{
#if defined (PDL_WIN32) || defined (PDL_PSOS)
  return PDL_OS::send (handle, (const char *) buf, len);
#else
  return PDL_OS::write (handle, (const char *) buf, len);
#endif /* PDL_WIN32 */
}

ASYS_INLINE ssize_t
PDL::recv_i (PDL_SOCKET handle, void *buf, size_t len)
{
#if defined (PDL_WIN32) || defined (PDL_PSOS)
    return PDL_OS::recv (handle, (char *) buf, len);
#else
    return PDL_OS::read (handle, (char *) buf, len);
#endif /* PDL_WIN32 */
}

ASYS_INLINE int
PDL::handle_read_ready (PDL_SOCKET handle,
                        const PDL_Time_Value *timeout)
{
  return PDL::handle_ready (handle,
                            timeout,
                            1,
                            0,
                            0);
}

ASYS_INLINE int
PDL::handle_write_ready (PDL_SOCKET handle,
                         const PDL_Time_Value *timeout)
{
  return PDL::handle_ready (handle,
                            timeout,
                            0,
                            1,
                            0);
}

ASYS_INLINE int
PDL::handle_exception_ready (PDL_SOCKET handle,
                             const PDL_Time_Value *timeout)
{
  return PDL::handle_ready (handle,
                            timeout,
                            0,
                            0,
                            1);
}

ASYS_INLINE char *
PDL::strecpy (char *s, const char *t)
{
  return PDL_OS::strecpy (s, t);
}

#if defined (PDL_HAS_UNICODE)
ASYS_INLINE wchar_t *
PDL::strecpy (wchar_t *s, const wchar_t *t)
{
  return PDL_OS::strecpy (s, t);
}
#endif /* PDL_HAS_UNICODE */

ASYS_INLINE void
PDL::unique_name (const void *object,
                  LPTSTR      name,
                  size_t length)
{
  PDL_OS::unique_name (object, name, length);
}

// Return flags currently associated with handle.

ASYS_INLINE int
PDL::get_flags (PDL_SOCKET handle)
{
  PDL_TRACE ("PDL::get_flags");

#if defined (PDL_LACKS_FCNTL)
  // PDL_OS::fcntl is not supported, e.g., on VxWorks.  It
  // would be better to store PDL's notion of the flags
  // associated with the handle, but this works for now.
  PDL_UNUSED_ARG (handle);
  return 0;
#else
  return PDL_OS::fcntl (handle, F_GETFL, 0);
#endif /* PDL_LACKS_FCNTL */
}

//#define log2(x) (log (x) / 0.693147180559945309417)
#ifdef log2
# undef log2
#endif

ASYS_INLINE u_long
PDL::log2(u_long num)
{
  u_long log = 0;

  for (;
       num > 0;
       log++)
    num >>= 1;

  return log;
}

ASYS_INLINE char
PDL::nibble2hex (u_int n)
{
  return PDL::hex_chars_[n & 0x0f];
}

ASYS_INLINE u_char
PDL::hex2byte (char c)
{
  if (isdigit (c))
    return (u_char) (c - '0');
  else if (islower (c))
    return (u_char) (10 + c - 'a');
  else
    return (u_char) (10 + c - 'A');
}

ASYS_INLINE char
PDL::debug (void)
{
  return PDL::debug_;
}

ASYS_INLINE void
PDL::debug (char c)
{
  PDL::debug_ = c;
}

ASYS_INLINE char *
PDL::strnew (const char *s)
{
  // PDL_TRACE ("PDL::strnew"); BUGBUG : new
  char *t = new char [::strlen(s) + 1];
  if (t == 0)
    return 0;
  else
    return PDL_OS::strcpy (t, s);
}

#if defined (PDL_WIN32) && defined (PDL_HAS_UNICODE)
ASYS_INLINE wchar_t *
PDL::strnew (const wchar_t *s)
{
  // PDL_TRACE ("PDL_OS::strnew");
  wchar_t *t = new wchar_t[::wcslen (s) + 1];
  if (t == 0)
    return 0;
  else
    return PDL_OS::strcpy (t, s);
}
#endif /* PDL_WIN32 && PDL_HAS_UNICODE */
