// -*- C++ -*-
// OS.i,v 4.846 2000/02/03 19:35:32 schmidt Exp
#if defined (PDL_HAS_WINCE)
#include "inc_user_config.h"
#include <assert.h>
#endif /* PDL_HAS_WINCE*/
# ifdef PDL_WIN32
#  if !defined (PDL_HAS_WINCE)
#  include <conio.h>
#  endif /* !PDL_HAS_WINCE */
// global objects
extern struct rlimit win32_rlimit;
# endif

#include <idConfig.h>

#if defined(VXWORKS)
#define RLIMIT_CPU      0   /* limit on CPU time per process */
#define RLIMIT_FSIZE    1   /* limit on file size */
#define RLIMIT_DATA     2   /* limit on data segment size */
#define RLIMIT_STACK    3   /* limit on process stack size */
#define RLIMIT_CORE     4   /* limit on size of core dump file */
#define RLIMIT_NOFILE   5   /* limit on number of open files */
#define RLIMIT_AS       6   /* limit on process total address space size */
#define RLIMIT_VMEM     RLIMIT_AS
#define RLIM_NLIMITS    7

typedef unsigned long long rlim_t;
struct rlimit
{
    rlim_t    rlim_cur;     // current (soft) limit
    rlim_t    rlim_max;     // hard limit
};

extern struct rlimit vxworks_rlimit;
#endif

#if defined(SYMBIAN)
extern struct rlimit symbian_rlimit;
#endif

#if !defined (PDL_HAS_INLINED_OSCALLS)
# undef PDL_INLINE
# define PDL_INLINE
#endif /* PDL_HAS_INLINED_OSCALLS */

#if defined (PDL_LACKS_RLIMIT_PROTOTYPE)
int getrlimit (int resource, struct rlimit *rlp);
int setrlimit (int resource, const struct rlimit *rlp);
#endif /* PDL_LACKS_RLIMIT_PROTOTYPE */

#if !defined (PDL_HAS_STRERROR)
# if defined (PDL_HAS_SYS_ERRLIST)
extern char *sys_errlist[];
#   define strerror(err) sys_errlist[err]
# else
#   define strerror(err) "strerror is unsupported"
# endif /* PDL_HAS_SYS_ERRLIST */
#endif /* !PDL_HAS_STERROR */

#if defined (PDL_HAS_SYS_SIGLIST)
# if !defined (_sys_siglist)
#   define _sys_siglist sys_siglist
# endif /* !defined (sys_siglist) */
//extern char **_sys_siglist;
#endif /* PDL_HAS_SYS_SIGLIST */

#if defined (PDL_HAS_SOCKLEN_T) || defined (_SOCKLEN_T)
typedef socklen_t PDL_SOCKET_LEN;
#elif defined (PDL_HAS_SIZET_SOCKET_LEN)
typedef size_t PDL_SOCKET_LEN;
#else
typedef int PDL_SOCKET_LEN;
#endif /* PDL_HAS_SIZET_SOCKET_LEN */

#if defined (PDL_LACKS_CONST_STRBUF_PTR)
typedef struct strbuf *PDL_STRBUF_TYPE;
#else
typedef const struct strbuf *PDL_STRBUF_TYPE;
#endif /* PDL_LACKS_CONST_STRBUF_PTR */

#if defined (PDL_HAS_VOIDPTR_SOCKOPT)
typedef void *PDL_SOCKOPT_TYPE1;
#elif defined (PDL_HAS_CHARPTR_SOCKOPT)
typedef char *PDL_SOCKOPT_TYPE1;
#else
typedef const char *PDL_SOCKOPT_TYPE1;
#endif /* PDL_HAS_VOIDPTR_SOCKOPT */

#if defined (PDL_LACKS_SETREUID_PROTOTYPE)
extern "C" {
extern int setreuid (uid_t ruid, uid_t euid);
}
#endif /* PDL_LACKS_SETREUID_PROTOTYPE */

#if defined (PDL_LACKS_SETREGID_PROTOTYPE)
extern "C" {
extern int setregid (gid_t rgid, gid_t egid);
}
#endif /* PDL_LACKS_SETREGID_PROTOTYPE */

#if defined (PDL_LACKS_WRITEV)
extern "C" PDL_Export int writev (PDL_HANDLE handle, PDL_WRITEV_TYPE *iov, int iovcnt);
#endif /* PDL_LACKS_WRITEV */

#if defined (PDL_LACKS_READV)
extern "C" PDL_Export ssize_t readv (PDL_HANDLE handle, PDL_READV_TYPE *iov, int iovcnt);
#endif /* PDL_LACKS_READV */

#if defined (PDL_NEEDS_FTRUNCATE)
extern "C" PDL_Export int ftruncate (PDL_HANDLE handle, PDL_OFF_T len);
#endif /* PDL_NEEDS_FTRUNCATE */

#if defined (PDL_HAS_VOIDPTR_MMAP)
// Needed for some odd OSs (e.g., SGI).
typedef void *PDL_MMAP_TYPE;
#else
typedef char *PDL_MMAP_TYPE;
#endif /* PDL_HAS_VOIDPTR_MMAP */

#if defined (PDL_HAS_XLI)
# include /**/ <xliuser.h>
#endif /* PDL_HAS_XLI */

#if defined (_M_UNIX)
extern "C" int _dlclose (void *);
extern "C" char *_dlerror (void);
extern "C" void *_dlopen (const char *, int);
extern "C" void * _dlsym (void *, const char *);
#endif /* _M_UNIX */

#if !defined (PDL_HAS_CPLUSPLUS_HEADERS)
# include /**/ <libc.h>
# include /**/ <osfcn.h>
#endif /* PDL_HAS_CPLUSPLUS_HEADERS */

#if defined (PDL_HAS_SYSENT_H)
# include /**/ <sysent.h>
#endif /* PDL_HAS_SYSENT_H_*/

#if defined (PDL_HAS_SVR4_GETTIMEOFDAY)
# if !defined (m88k) && !defined (SCO)
extern "C" int gettimeofday (struct timeval *tp, void * = 0);
# else
extern "C" int gettimeofday (struct timeval *tp);
# endif  /*  !m88k && !SCO */
#elif defined (PDL_HAS_OSF1_GETTIMEOFDAY)
extern "C" int gettimeofday (struct timeval *tp, struct timezone * = 0);
#elif defined (PDL_HAS_SUNOS4_GETTIMEOFDAY)
# define PDL_HAS_SVR4_GETTIMEOFDAY
#endif /* PDL_HAS_SVR4_GETTIMEOFDAY */

#if defined (PDL_LACKS_CONST_TIMESPEC_PTR)
typedef struct timespec * PDL_TIMESPEC_PTR;
#else
typedef const struct timespec * PDL_TIMESPEC_PTR;
#endif /* HPUX */

#if defined (PDL_HAS_WINCE_BROKEN_ERRNO)

PDL_INLINE PDL_CE_Errno *
PDL_CE_Errno::instance ()
{
  // This should be inlined.
  return PDL_CE_Errno::instance_;
}

PDL_INLINE
PDL_CE_Errno::operator int (void) const
{
  return (int) TlsGetValue (PDL_CE_Errno::errno_key_);
}

PDL_INLINE int
PDL_CE_Errno::operator= (int x)
{
  // error checking?
  TlsSetValue (PDL_CE_Errno::errno_key_, (void *) x);
  return x;
}

#endif /* PDL_HAS_WINCE_BROKEN_ERRNO */

#if defined (UNICODE)
PDL_INLINE
PDL_Wide_To_Ascii::~PDL_Wide_To_Ascii (void)
{
  delete [] this->s_;
}

PDL_INLINE char *
PDL_Wide_To_Ascii::char_rep (void)
{
  return this->s_;
}

PDL_INLINE char *
PDL_Wide_To_Ascii::convert (const wchar_t *wstr)
{
  // Short circuit null pointer case
  if (wstr == 0)
    return 0;

# if defined (PDL_WIN32)
  int len = ::WideCharToMultiByte (CP_OEMCP,
                                   0,
                                   wstr,
                                   -1,
                                   0,
                                   0,
                                   0,
                                   0);
# elif defined (PDL_LACKS_WCSLEN)
  const wchar_t *wtemp = wstr;
  while (wtemp != 0)
    ++wtemp;

  int len = wtemp - wstr + 1;
# else  /* PDL_WIN32 */
  int len = ::wcslen (wstr) + 1;
# endif /* PDL_WIN32 */

  char *str = new char[len];

# if defined (PDL_WIN32)
  ::WideCharToMultiByte (CP_OEMCP, 0, wstr, -1, str, len, 0, 0);
# elif defined (VXWORKS)
  ::wcstombs (str, wstr, len);
# else /* PDL_WIN32 */
  for (int i = 0; i < len; i++)
    {
      wchar_t *t = PDL_const_cast (wchar_t *, wstr);
      str[i] = PDL_static_cast (char, *(t + i));
    }
# endif /* PDL_WIN32 */
  return str;
}

PDL_INLINE
PDL_Wide_To_Ascii::PDL_Wide_To_Ascii (const wchar_t *s)
  : s_ (PDL_Wide_To_Ascii::convert (s))
{
}

PDL_INLINE
PDL_Ascii_To_Wide::~PDL_Ascii_To_Wide (void)
{
  delete [] this->s_;
}

PDL_INLINE wchar_t *
PDL_Ascii_To_Wide::wchar_rep (void)
{
  return this->s_;
}

PDL_INLINE wchar_t *
PDL_Ascii_To_Wide::convert (const char *str)
{
  // Short circuit null pointer case
  if (str == 0)
    return 0;

# if defined (PDL_WIN32)
  int len = ::MultiByteToWideChar (CP_OEMCP, 0, str, -1, 0, 0);
# else /* PDL_WIN32 */
  int len = strlen (str) + 1;
# endif /* PDL_WIN32 */

  wchar_t *wstr = new wchar_t[len];

# if defined (PDL_WIN32)
  ::MultiByteToWideChar (CP_OEMCP, 0, str, -1, wstr, len);
# elif defined (VXWORKS)
  ::mbstowcs (wstr, str, len);
# else /* PDL_WIN32 */
  for (int i = 0; i < len; i++)
    {
      char *t = PDL_const_cast (char *, str);
      wstr[i] = PDL_static_cast (wchar_t, *(t + i));
    }
# endif /* PDL_WIN32 */
  return wstr;
}

PDL_INLINE
PDL_Ascii_To_Wide::PDL_Ascii_To_Wide (const char *s)
: s_ (PDL_Ascii_To_Wide::convert (s))
{
}

#endif /* UNICODE */

/*BUGBUG_NT*/
#if defined (PDL_WIN32)
# define pdlPrintWin32Error(x) ;
#endif /* PDL_WIN32 */
/*BUGBUG_NT ADD*/

PDL_INLINE
PDL_Errno_Guard::PDL_Errno_Guard (PDL_ERRNO_TYPE &errno_ref,
                                  int error)
  :
#if defined (PDL_MT_SAFE)
    errno_ptr_ (&errno_ref),
#endif /* PDL_MT_SAFE */
    error_ (error)
{
#if !defined(PDL_MT_SAFE)
  PDL_UNUSED_ARG (errno_ref);
#endif /* PDL_MT_SAFE */
}

PDL_INLINE
PDL_Errno_Guard::PDL_Errno_Guard (PDL_ERRNO_TYPE &errno_ref)
  :
#if defined (PDL_MT_SAFE)
    errno_ptr_ (&errno_ref),
#endif /* PDL_MT_SAFE */
    error_ (errno_ref)
{
}

PDL_INLINE
PDL_Errno_Guard::~PDL_Errno_Guard (void)
{
#if defined (PDL_MT_SAFE)
  *errno_ptr_ = this->error_;
#else
  errno = this->error_;
#endif /* PDL_MT_SAFE */
}

#if defined (PDL_HAS_WINCE_BROKEN_ERRNO)
PDL_INLINE PDL_Errno_Guard&
PDL_Errno_Guard::operator= (const PDL_ERRNO_TYPE &error)
{
   this->error_ = error;
   return *this;
}
#endif /* PDL_HAS_WINCE_BROKEN_ERRNO */

PDL_INLINE PDL_Errno_Guard&
PDL_Errno_Guard::operator= (int error)
{
   this->error_ = error;
   return *this;
}

PDL_INLINE int
PDL_Errno_Guard::operator== (int error)
{
  return this->error_ == error;
}

PDL_INLINE int
PDL_Errno_Guard::operator!= (int error)
{
  return this->error_ != error;
}

// Returns the value of the object as a timeval.

PDL_INLINE
PDL_Time_Value::operator timeval () const
{
  PDL_TRACE ("PDL_Time_Value::operator timeval");
  return this->tv_;
}

// Returns a pointer to the object as a timeval.

PDL_INLINE
PDL_Time_Value::operator const timeval * () const
{
  PDL_TRACE ("PDL_Time_Value::operator timeval");
  return (const timeval *) &this->tv_;
}

PDL_INLINE void
PDL_Time_Value::set (long sec, long usec)
{
  // PDL_TRACE ("PDL_Time_Value::set");
  this->tv_.tv_sec = sec;
  this->tv_.tv_usec = usec;
  this->normalize ();
}

PDL_INLINE void
PDL_Time_Value::set (double d)
{
  // PDL_TRACE ("PDL_Time_Value::set");
  long l = (long) d;
  this->tv_.tv_sec = l;
  this->tv_.tv_usec = (long) ((d - (double) l) * PDL_ONE_SECOND_IN_USECS);
  this->normalize ();
}

// Initializes a timespec_t.  Note that this approach loses precision
// since it converts the nano-seconds into micro-seconds.  But then
// again, do any real systems have nano-second timer precision?!

PDL_INLINE void
PDL_Time_Value::set (const timespec_t &tv)
{
  // PDL_TRACE ("PDL_Time_Value::set");
#if ! defined(PDL_HAS_BROKEN_TIMESPEC_MEMBERS)
  this->tv_.tv_sec = tv.tv_sec;
  // Convert nanoseconds into microseconds.
  this->tv_.tv_usec = tv.tv_nsec / 1000;
#else
  this->tv_.tv_sec = tv.ts_sec;
  // Convert nanoseconds into microseconds.
  this->tv_.tv_usec = tv.ts_nsec / 1000;
#endif /* PDL_HAS_BROKEN_TIMESPEC_MEMBERS */

  this->normalize ();
}

PDL_INLINE void
PDL_Time_Value::set (const timeval &tv)
{
  // PDL_TRACE ("PDL_Time_Value::set");
  this->tv_.tv_sec = tv.tv_sec;
  this->tv_.tv_usec = tv.tv_usec;

  this->normalize ();
}

PDL_INLINE void
PDL_Time_Value::initialize (const timeval &tv)
  // : tv_ ()
{
  // PDL_TRACE ("PDL_Time_Value::initialize");
  this->set (tv);
}

PDL_INLINE void
PDL_Time_Value::initialize (void)
  // : tv_ ()
{
  // PDL_TRACE ("PDL_Time_Value::initialize");
  this->set (0, 0);

  // Don't need to normalize time value of (0, 0).
}

PDL_INLINE void
PDL_Time_Value::initialize (long sec, long usec)
{
  // PDL_TRACE ("PDL_Time_Value::initialize");
  this->set (sec, usec);
  this->normalize ();
}

// Returns number of seconds.

PDL_INLINE long
PDL_Time_Value::sec (void) const
{
  PDL_TRACE ("PDL_Time_Value::sec");
  return this->tv_.tv_sec;
}

// Sets the number of seconds.

PDL_INLINE void
PDL_Time_Value::sec (long sec)
{
  PDL_TRACE ("PDL_Time_Value::sec");
  this->tv_.tv_sec = sec;
}

// Converts from Time_Value format into milli-seconds format.

PDL_INLINE long
PDL_Time_Value::msec (void) const
{
  PDL_TRACE ("PDL_Time_Value::msec");
  return this->tv_.tv_sec * 1000 + this->tv_.tv_usec / 1000;
}

// Converts from <Time_Value> format into micro-seconds format.

PDL_INLINE long
PDL_Time_Value::microsec (void) const  /* BUG-46183 */
{
  PDL_TRACE ("PDL_Time_Value::microsec");
  return this->tv_.tv_sec * PDL_U_ONE_SECOND_IN_USECS + this->tv_.tv_usec;
}

// Converts from milli-seconds format into Time_Value format.

PDL_INLINE void
PDL_Time_Value::msec (long milliseconds)
{
  PDL_TRACE ("PDL_Time_Value::msec");
  // Convert millisecond units to seconds;
  this->tv_.tv_sec = milliseconds / 1000;
  // Convert remainder to microseconds;
  this->tv_.tv_usec = (milliseconds - (this->tv_.tv_sec * 1000)) * 1000;
}

/* Converts from micro-seconds format into Time_Value format. */

PDL_INLINE void
PDL_Time_Value::microsec (long microseconds)
{
  PDL_TRACE ("PDL_Time_Value::microsec");
  /* Convert microseconds units to seconds; */
  this->tv_.tv_sec = microseconds / 1000000;
  /* Convert remainder to microseconds; */
  this->tv_.tv_usec = ( microseconds % 1000000 );
}

// Returns number of micro-seconds.

PDL_INLINE long
PDL_Time_Value::usec (void) const
{
  PDL_TRACE ("PDL_Time_Value::usec");
  return this->tv_.tv_usec;
}

// Sets the number of micro-seconds.

PDL_INLINE void
PDL_Time_Value::usec (long usec)
{
  PDL_TRACE ("PDL_Time_Value::usec");
  this->tv_.tv_usec = usec;
}

// True if tv1 > tv2.

PDL_INLINE int
operator > (const PDL_Time_Value &tv1,
            const PDL_Time_Value &tv2)
{
  PDL_TRACE ("operator >");
  if (tv1.sec () > tv2.sec ())
    return 1;
  else if (tv1.sec () == tv2.sec ()
           && tv1.usec () > tv2.usec ())
    return 1;
  else
    return 0;
}

// True if tv1 >= tv2.

PDL_INLINE int
operator >= (const PDL_Time_Value &tv1,
             const PDL_Time_Value &tv2)
{
  PDL_TRACE ("operator >=");
  if (tv1.sec () > tv2.sec ())
    return 1;
  else if (tv1.sec () == tv2.sec ()
           && tv1.usec () >= tv2.usec ())
    return 1;
  else
    return 0;
}

// Returns the value of the object as a timespec_t.

PDL_INLINE
PDL_Time_Value::operator timespec_t () const
{
  PDL_TRACE ("PDL_Time_Value::operator timespec_t");
  timespec_t tv;
#if ! defined(PDL_HAS_BROKEN_TIMESPEC_MEMBERS)
  tv.tv_sec = this->sec ();
  // Convert microseconds into nanoseconds.
  tv.tv_nsec = this->tv_.tv_usec * 1000;
#else
  tv.ts_sec = this->sec ();
  // Convert microseconds into nanoseconds.
  tv.ts_nsec = this->tv_.tv_usec * 1000;
#endif /* PDL_HAS_BROKEN_TIMESPEC_MEMBERS */
  return tv;
}

PDL_INLINE void
PDL_Time_Value::initialize (const timespec_t &tv)
  // : tv_ ()
{
  // PDL_TRACE ("PDL_Time_Value::initialize");
  this->set (tv);
}

// True if tv1 < tv2.

PDL_INLINE int
operator < (const PDL_Time_Value &tv1,
            const PDL_Time_Value &tv2)
{
  PDL_TRACE ("operator <");
  return tv2 > tv1;
}

// True if tv1 >= tv2.

PDL_INLINE int
operator <= (const PDL_Time_Value &tv1,
             const PDL_Time_Value &tv2)
{
  PDL_TRACE ("operator <=");
  return tv2 >= tv1;
}

// True if tv1 == tv2.

PDL_INLINE int
operator == (const PDL_Time_Value &tv1,
             const PDL_Time_Value &tv2)
{
  // PDL_TRACE ("operator ==");
  return tv1.sec () == tv2.sec ()
    && tv1.usec () == tv2.usec ();
}

// True if tv1 != tv2.

PDL_INLINE int
operator != (const PDL_Time_Value &tv1,
             const PDL_Time_Value &tv2)
{
  // PDL_TRACE ("operator !=");
  return !(tv1 == tv2);
}

// Add TV to this.

PDL_INLINE void
PDL_Time_Value::operator+= (const PDL_Time_Value &tv)
{
  PDL_TRACE ("PDL_Time_Value::operator+=");
  this->sec (this->sec () + tv.sec ());
  this->usec (this->usec () + tv.usec ());
  this->normalize ();
}

// Subtract TV to this.

PDL_INLINE void
PDL_Time_Value::operator-= (const PDL_Time_Value &tv)
{
  PDL_TRACE ("PDL_Time_Value::operator-=");
  this->sec (this->sec () - tv.sec ());
  this->usec (this->usec () - tv.usec ());
  this->normalize ();
}

// Adds two PDL_Time_Value objects together, returns the sum.

PDL_INLINE PDL_Time_Value
operator + (const PDL_Time_Value &tv1,
            const PDL_Time_Value &tv2)
{
  PDL_TRACE ("operator +");
  PDL_Time_Value sum;

  sum.initialize(tv1.sec () + tv2.sec (), tv1.usec () + tv2.usec ());
  sum.normalize ();
  return sum;
}

// Subtracts two PDL_Time_Value objects, returns the difference.

PDL_INLINE PDL_Time_Value
operator - (const PDL_Time_Value &tv1,
            const PDL_Time_Value &tv2)
{
  PDL_TRACE ("operator -");
  PDL_Time_Value delta;
  delta.initialize (tv1.sec () - tv2.sec (), tv1.usec () - tv2.usec ());
  delta.normalize ();
  return delta;
}

PDL_INLINE int
PDL_OS::fcntl (PDL_HANDLE handle, int cmd, long arg)
{
  PDL_TRACE ("PDL_OS::fcntl");
# if defined (PDL_LACKS_FCNTL)
  PDL_UNUSED_ARG (handle);
  PDL_UNUSED_ARG (cmd);
  PDL_UNUSED_ARG (arg);
  PDL_NOTSUP_RETURN (-1);
# else
  PDL_OSCALL_RETURN (::fcntl (handle, cmd, arg), int, -1);
# endif /* PDL_LACKS_FCNTL */
}

#if !defined (PDL_WIN32)

// Matthew Stevens 7-10-95 Fix GNU GCC 2.7 for memchr() problem.
# if defined (PDL_HAS_GNU_CSTRING_H)
// Define this file to keep /usr/include/memory.h from being included.
#   include /**/ <cstring>
# else
#   if defined (PDL_LACKS_MEMORY_H)
#     if !defined (PDL_PSOS_DIAB_MIPS)
#       include /**/ <string.h>
#     endif /* PDL_PSOS_DIAB_MIPS */
#   else
#     include /**/ <memory.h>
#   endif /* VXWORKS */
# endif /* PDL_HAS_GNU_CSTRING_H */

// These prototypes are chronically lacking from many versions of
// UNIX.
extern "C" int t_getname (int, struct netbuf *, int);
extern "C" int isastream (int);
# if !defined (PDL_HAS_GETRUSAGE_PROTO)
extern "C" int getrusage (int who, struct rusage *rusage);
# endif /* ! PDL_HAS_GETRUSAGE_PROTO */

# if defined (PDL_LACKS_SYSCALL)
extern "C" int syscall (int, PDL_HANDLE, struct rusage *);
# endif /* PDL_LACKS_SYSCALL */

// The following are #defines and #includes that must be visible for
// PDL to compile it's OS wrapper class implementation correctly.  We
// put them inside of here to reduce compiler overhead if we're not
// inlining...

# if defined (PDL_HAS_REGEX)
#   include /**/ <regexpr.h>
# endif /* PDL_HAS_REGEX */

# if defined (PDL_HAS_SYSINFO)
#   include /**/ <sys/systeminfo.h>
# endif /* PDL_HAS_SYS_INFO */

# if defined (PDL_HAS_SYSCALL_H)
#   include /**/ <sys/syscall.h>
# endif /* PDL_HAS_SYSCALL_H */

# if defined (UNIXWARE)  /* See strcasecmp, below */
#   include /**/ <ctype.h>
# endif /* UNIXWARE */

// Adapt the weird threading and synchronization routines (which
// return errno rather than -1) so that they return -1 and set errno.
// This is more consistent with the rest of PDL_OS and enables use to
// use the PDL_OSCALL* macros.
# if defined (VXWORKS)
#   define PDL_ADAPT_RETVAL(OP,RESULT) ((RESULT = (OP)) != OK ? (errno = RESULT, -1) : 0)
# else
#   define PDL_ADAPT_RETVAL(OP,RESULT) ((RESULT = (OP)) != 0 ? (errno = RESULT, -1) : 0)
# endif /* VXWORKS */

# if !defined (PDL_HAS_MOSTLY_UNICODE_APIS)
PDL_INLINE int
PDL_OS::chdir (const char *path)
{
  PDL_TRACE ("PDL_OS::chdir");
#   if defined (VXWORKS)
  PDL_OSCALL_RETURN (::chdir (PDL_const_cast (char *, path)), int, -1);

#elif defined (PDL_PSOS_LACKS_PHILE)
  PDL_UNUSED_ARG (path);
  PDL_NOTSUP_RETURN (-1);

#elif defined (PDL_PSOS)
    PDL_OSCALL_RETURN (PDL_ADAPT_RETVAL (::change_dir ((char *) path), pdl_result_),
                     int, -1);
#elif defined(ITRON)
  /* empty */
  PDL_NOTSUP_RETURN (-1);
#else
  PDL_OSCALL_RETURN (::chdir (path), int, -1);
#   endif /* VXWORKS */
}
# endif /* PDL_HAS_MOSTLY_UNICODE_APIS */

PDL_INLINE int
PDL_OS::fstat(PDL_HANDLE handle, PDL_stat *stp)
{
  PDL_TRACE ("PDL_OS::fstat");
#if defined (PDL_PSOS_LACKS_PHILE)
  PDL_UNUSED_ARG (handle);
  PDL_UNUSED_ARG (stp);
  PDL_NOTSUP_RETURN (-1);
#elif defined (PDL_PSOS)
  PDL_OSCALL_RETURN (::fstat_f (handle, stp), int, -1);
#elif defined (PDL_WIN32)
  int retval = -1;
  int fd = _open_osfhandle ((long) fildes, 0);
  if (fd != -1) {
      retval = _fstat (fd, (struct _stat *) buf);
    }
  _close (fd);
  /* Remember to close the file handle. */
  return retval;
#else
# if defined (PDL_HAS_X86_STAT_MACROS)
    // Solaris for intel uses an macro for fstat(), this is a wrapper
    // for _fxstat() use of the macro.
    // causes compile and runtime problems.
    PDL_OSCALL_RETURN (::_fxstat (_STAT_VER, handle, stp), int, -1);
//# elif defined (PDL_WIN32)
//    PDL_OSCALL_RETURN (::_fstat (handle, stp), int, -1);
# else  /* !PDL_HAS_X86_STAT_MACROS */
    PDL_OSCALL_RETURN (::fstat (handle, stp), int, -1);
# endif /* !PDL_HAS_X86_STAT_MACROS */
#endif /* defined (PDL_PSOS) */
}

PDL_INLINE int
PDL_OS::lstat (const char *file, PDL_stat *stp)
{
  PDL_TRACE ("PDL_OS::lstat");
# if defined (PDL_LACKS_LSTAT) || \
     defined (PDL_HAS_WINCE) || defined (PDL_WIN32)
  PDL_UNUSED_ARG (file);
  PDL_UNUSED_ARG (stp);
  PDL_NOTSUP_RETURN (-1);
#else
# if defined (PDL_HAS_X86_STAT_MACROS)
   // Solaris for intel uses an macro for lstat(), this macro is a
   // wrapper for _lxstat().
  PDL_OSCALL_RETURN (::_lxstat (_STAT_VER, file, stp), int, -1);
#else /* !PDL_HAS_X86_STAT_MACROS */
  PDL_OSCALL_RETURN (::lstat (file, stp), int, -1);
#endif /* !PDL_HAS_X86_STAT_MACROS */
# endif /* VXWORKS */
}

PDL_INLINE int
PDL_OS::fsync (PDL_HANDLE handle)
{
  PDL_TRACE ("PDL_OS::fsync");
# if defined (PDL_LACKS_FSYNC)
#if defined(VXWORKS)
  //TrueFFS does not support ioctl, but verifies each write operation
  int a = ::ioctl(handle, FIOSYNC, 0);
  return 0;
#else
  PDL_UNUSED_ARG (handle);
  PDL_NOTSUP_RETURN (-1);
#endif
# else
  PDL_OSCALL_RETURN (::fsync (handle), int, -1);
# endif /* PDL_LACKS_FSYNC */
}

PDL_INLINE int
PDL_OS::getopt (int argc, char *const *argv, const char *optstring)
{
  PDL_TRACE ("PDL_OS::getopt");
#if defined (PDL_PSOS)
  PDL_UNUSED_ARG (argc);
  PDL_UNUSED_ARG (argv);
  PDL_UNUSED_ARG (optstring);
  PDL_NOTSUP_RETURN (-1);
# elif defined (PDL_LACKS_GETOPT_PROTO)
  PDL_OSCALL_RETURN (::getopt (argc, (char**) argv, optstring), int, -1);
# elif defined (PDL_LACKS_POSIX_PROTOTYPES)
  PDL_OSCALL_RETURN (::getopt (argc, (const char* const *) argv, optstring), int, -1);
#elif defined(ITRON)
  PDL_NOTSUP_RETURN (-1);
#elif defined(VXWORKS)
  PDL_NOTSUP_RETURN (-1);
# else
  PDL_OSCALL_RETURN (::getopt (argc, argv, optstring), int, -1);
# endif /* VXWORKS */
}

# if !defined (PDL_HAS_MOSTLY_UNICODE_APIS)
PDL_INLINE int
PDL_OS::mkfifo (const char *file, mode_t mode)
{
  PDL_TRACE ("PDL_OS::mkfifo");
#if defined (PDL_LACKS_MKFIFO)
  PDL_UNUSED_ARG (file);
  PDL_UNUSED_ARG (mode);
  PDL_NOTSUP_RETURN (-1);
#else
  PDL_OSCALL_RETURN (::mkfifo (file, mode), int, -1);
#   endif /* PDL_LACKS_MKFIFO */
}

# endif /* !PDL_HAS_MOSTLY_UNICODE_APIS */

PDL_INLINE int
PDL_OS::pipe (PDL_HANDLE fds[])
{
  PDL_TRACE ("PDL_OS::pipe");
#if defined (VXWORKS) || defined (PDL_PSOS)
  PDL_UNUSED_ARG (fds);
  PDL_NOTSUP_RETURN (-1);
#elif defined(ITRON)
  /* empty */
  PDL_NOTSUP_RETURN (-1);
# else
  PDL_OSCALL_RETURN (::pipe (fds), int, -1);
# endif /* VXWORKS */
}

# if defined (DIGITAL_UNIX)
extern "C" {
  extern char *_Pctime_r (const time_t *, char *);
  extern struct tm *_Plocaltime_r (const time_t *, struct tm *);
  extern struct tm *_Pgmtime_r (const time_t *, struct tm *);
  extern char *_Pasctime_r (const struct tm *, char *);
  extern int _Prand_r (unsigned int *seedptr);
  extern int _Pgetpwnam_r (const char *, struct passwd *,
                           char *, size_t, struct passwd **);
}
# endif /* DIGITAL_UNIX */

PDL_INLINE int
PDL_OS::rand_r (PDL_RANDR_TYPE &seed)
{
  PDL_TRACE ("PDL_OS::rand_r");
# if defined (PDL_HAS_REENTRANT_FUNCTIONS)
#   if defined (DIGITAL_UNIX)
  PDL_OSCALL_RETURN (::_Prand_r (&seed), int, -1);
#   elif defined (HPUX_10)
  // rand() is thread-safe on HP-UX 10.  rand_r's signature is not consistent
  // with latest POSIX and will change in a future HP-UX release so that it
  // is consistent.  At that point, this #elif section can be changed or
  // removed, and just call rand_r.
  PDL_UNUSED_ARG (seed);
  PDL_OSCALL_RETURN (::rand(), int, -1);
#   elif defined (PDL_HAS_BROKEN_RANDR)
  PDL_OSCALL_RETURN (::rand_r (seed), int, -1);
#   else
  PDL_OSCALL_RETURN (::rand_r (&seed), int, -1);
#   endif /* DIGITAL_UNIX */
# else
  PDL_UNUSED_ARG (seed);
  PDL_OSCALL_RETURN (::rand (), int, -1);
# endif /* PDL_HAS_REENTRANT_FUNCTIONS */
}

PDL_INLINE pid_t
PDL_OS::setsid (void)
{
  PDL_TRACE ("PDL_OS::setsid");
#if defined (VXWORKS) || defined (CHORUS) || defined (PDL_PSOS)
  PDL_NOTSUP_RETURN (-1);
#elif defined(ITRON)
  /* empty */
  PDL_NOTSUP_RETURN (-1);
# else
  PDL_OSCALL_RETURN (::setsid (), int, -1);
# endif /* VXWORKS */
}

PDL_INLINE mode_t
PDL_OS::umask (mode_t cmask)
{
  PDL_TRACE ("PDL_OS::umask");
#if defined (VXWORKS) || defined (PDL_PSOS)
  PDL_UNUSED_ARG (cmask);
  PDL_NOTSUP_RETURN (-1);
# else
  return ::umask (cmask); // This call shouldn't fail...
# endif /* VXWORKS */
}

#else /* PDL_WIN32 */

// Adapt the Win32 System Calls (which return BOOLEAN values of TRUE
// and FALSE) into int values expected by the PDL_OSCALL macros.
# define PDL_ADAPT_RETVAL(OP,RESULT) ((RESULT = (OP)) == FALSE ? -1 : 0)

// Perform a mapping of Win32 error numbers into POSIX errnos.
# define PDL_FAIL_RETURN(RESULT) do { \
  switch (PDL_OS::set_errno_to_last_error ()) { \
  case ERROR_NOT_ENOUGH_MEMORY: errno = ENOMEM; break; \
  case ERROR_FILE_EXISTS:       errno = EEXIST; break; \
  case ERROR_SHARING_VIOLATION: errno = EACCES; break; \
  case ERROR_PATH_NOT_FOUND:    errno = ENOENT; break; \
  } \
  return RESULT; } while (0)

PDL_INLINE LPSECURITY_ATTRIBUTES
PDL_OS::default_win32_security_attributes (LPSECURITY_ATTRIBUTES sa)
{
#if defined (PDL_DEFINES_DEFAULT_WIN32_SECURITY_ATTRIBUTES)
  if (sa == 0)
    {
      // @@ This is a good plpdl to use pthread_once.
      static SECURITY_ATTRIBUTES default_sa;
      static SECURITY_DESCRIPTOR sd;
      InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
      SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE);
      default_sa.nLength = sizeof(SECURITY_ATTRIBUTES);
      default_sa.lpSecurityDescriptor = &sd;
      default_sa.bInheritHandle       = TRUE;
      sa = &default_sa;
    }
  return sa;
#else /* !PDL_DEFINES_DEFAULT_WIN32_SECURITY_ATTRIBUTES */
  return sa;
#endif /* PDL_DEFINES_DEFAULT_WIN32_SECURITY_ATTRIBUTES */
}

/*BUGBUG_NT*/
PDL_INLINE int
PDL_OS::fsync (PDL_HANDLE handle)
{
  PDL_TRACE ("PDL_OS::fsync");

  if (!FlushFileBuffers(handle)) {
    return -1;
  }
  return 0;
}
/*BUGBUG_NT ADD*/

# if !defined (PDL_HAS_MOSTLY_UNICODE_APIS)
PDL_INLINE int
PDL_OS::chdir (const char *path)
{
  PDL_TRACE ("PDL_OS::chdir");
#if defined (__IBMCPP__) && (__IBMCPP__ >= 400)
  PDL_OSCALL_RETURN (::_chdir (path), int, -1);
#else
  PDL_OSCALL_RETURN (::_chdir ((char *) path), int, -1);
#endif /* defined (__IBMCPP__) && (__IBMCPP__ >= 400) */
}

# endif /* !PDL_HAS_MOSTLY_UNICODE_APIS */

PDL_INLINE int
PDL_OS::getopt (int argc, char *const *argv, const char *optstring)
{
  PDL_UNUSED_ARG (argc);
  PDL_UNUSED_ARG (argv);
  PDL_UNUSED_ARG (optstring);

  PDL_TRACE ("PDL_OS::getopt");
  PDL_NOTSUP_RETURN (-1);
}

# if !defined (PDL_HAS_MOSTLY_UNICODE_APIS)
PDL_INLINE int
PDL_OS::mkfifo (const char *file, mode_t mode)
{
  PDL_UNUSED_ARG (file);
  PDL_UNUSED_ARG (mode);

  PDL_TRACE ("PDL_OS::mkfifo");
  PDL_NOTSUP_RETURN (-1);
}
# endif /* PDL_HAS_MOSTLY_UNICODE_APIS */

PDL_INLINE int
PDL_OS::pipe (PDL_HANDLE fds[])
{
# if !defined (PDL_HAS_WINCE) && !defined (__IBMCPP__) //VisualAge C++ 4.0 does not support this
  PDL_TRACE ("PDL_OS::pipe");
  PDL_OSCALL_RETURN (::_pipe ((int *) fds, PIPE_BUF, 0),
                     int,
                     -1);   // Use default mode
# else
  PDL_NOTSUP_RETURN (-1);
# endif /* PDL_HAS_WINCE */
}

PDL_INLINE int
PDL_OS::rand_r (PDL_RANDR_TYPE& seed)
{
  PDL_UNUSED_ARG (seed);

  PDL_TRACE ("PDL_OS::rand_r");
  PDL_NOTSUP_RETURN (-1);
}

PDL_INLINE pid_t
PDL_OS::setsid (void)
{
  PDL_TRACE ("PDL_OS::setsid");
  PDL_NOTSUP_RETURN (-1);
}

PDL_INLINE mode_t
PDL_OS::umask (mode_t cmask)
{
# if !defined (PDL_HAS_WINCE)
  PDL_TRACE ("PDL_OS::umask");
  PDL_OSCALL_RETURN (::_umask (cmask), int, -1);
# else
  PDL_NOTSUP_RETURN (-1);
# endif /* PDL_HAS_WINCE */
}

PDL_INLINE int
PDL_OS::fstat(PDL_HANDLE handle, PDL_stat *stp)
{
  PDL_TRACE ("PDL_OS::fstat");
# if 1
  BY_HANDLE_FILE_INFORMATION fdata;

  if (::GetFileInformationByHandle (handle, &fdata) == FALSE)
  {
      PDL_OS::set_errno_to_last_error ();
      return -1;
  }
  else
  {
      /* BUG-18734: [Win32] Large File(> MAXDWORD)에 대한 fstat함수에 대한
       * 포팅이 필요합니다.
       * nFileSizeHigh :The high-order DWORD value of the file size,
       * in bytes. This value is zero (0) unless the file size is
       * greater than MAXDWORD. 
       *
       * nFileSizeLow: The low-order DWORD value of the file size,
       * in bytes.
       *
       * The size of the file is equal to
       * (nFileSizeHigh * (MAXDWORD+1)) + nFileSizeLow.
       *  = nFileSizeHigh << 32 | nFileSizeLow
       * */
      stp->st_size = (__int64)fdata.nFileSizeHigh << 32 |
          fdata.nFileSizeLow;

      PDL_Time_Value sLastAccessTime;
      PDL_Time_Value sLastWriteTime;
      sLastAccessTime.initialize(fdata.ftLastAccessTime);
      sLastWriteTime.initialize(fdata.ftLastWriteTime);

#if defined (PDL_HAS_WINCE)
      stp->st_atime.initialize(sLastAccessTime.sec(), 0);
      stp->st_mtime.initialize(sLastWriteTime.sec(), 0);
#else
      stp->st_atime = sLastAccessTime.sec();
      stp->st_mtime = sLastWriteTime.sec();

      PDL_Time_Value sCreationTime;
      sCreationTime.initialize(fdata.ftCreationTime);

      stp->st_ctime = sCreationTime.sec ();
      stp->st_nlink = PDL_static_cast (short, fdata.nNumberOfLinks);
      stp->st_dev = stp->st_rdev = 0; // No equivalent conversion.
      stp->st_mode = S_IXOTH | S_IROTH |
          (fdata.dwFileAttributes & FILE_ATTRIBUTE_READONLY ? 0 : S_IWOTH);
#endif /* PDL_HAS_WINCE */
  }
  return 0;
# else /* 1 */
  // This implementation close the handle.
  int retval = -1;
  int fd = ::_open_osfhandle ((long) handle, 0);
  if (fd != -1)
      retval = ::_fstat (fd, (struct _stat *) stp);

  ::_close (fd);
  // Remember to close the file handle.
  return retval;
# endif /* 1 */
}

#endif /* WIN32 */

PDL_INLINE int
PDL_OS::clock_gettime (clockid_t clockid, struct timespec *ts)
{
  PDL_TRACE ("PDL_OS::clock_gettime");
#if defined (PDL_WIN32) || defined(X86_64_DARWIN)
  // just, clockid == CLOCK_REALTIME
  PDL_Time_Value tv = gettimeofday();
  if (tv.sec() == 0 && tv.usec() == 0)
	return -1;
  ts->tv_sec  = tv.sec();
  ts->tv_nsec = tv.usec() * 1000;
  return 0;
#elif defined (PDL_HAS_CLOCK_GETTIME)
  PDL_OSCALL_RETURN (::clock_gettime (clockid, ts), int, -1);
#elif defined (PDL_PSOS) && ! defined (PDL_PSOS_DIAB_MIPS)
  PDL_UNUSED_ARG (clockid);
  PDL_PSOS_Time_t pt;
  int result = PDL_PSOS_Time_t::get_system_time (pt);
  *ts = PDL_static_cast (struct timespec, pt);
  return result;
#elif defined(ITRON)
  /* empty */
  PDL_NOTSUP_RETURN (-1);
#else
  PDL_UNUSED_ARG (clockid);
  PDL_UNUSED_ARG (ts);
  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_HAS_CLOCK_GETTIME */
}

PDL_INLINE PDL_Time_Value
PDL_OS::gettimeofday (void)
{
  // PDL_TRACE ("PDL_OS::gettimeofday");

  timeval tv;
  int result = 0;
#if defined (PDL_HAS_WINCE)
  SYSTEMTIME tsys;
  FILETIME   tfile;
  ::GetSystemTime (&tsys);
  ::SystemTimeToFileTime (&tsys, &tfile);

  PDL_Time_Value sPDL_Time_Value;
  sPDL_Time_Value.initialize(tfile);
  return sPDL_Time_Value;
#elif defined (PDL_WIN32)
  FILETIME   tfile;
  ::GetSystemTimeAsFileTime (&tfile);

  PDL_Time_Value sPDL_Time_Value;
  sPDL_Time_Value.initialize(tfile);
  return sPDL_Time_Value;
#if 0
  // From Todd Montgomery...
  struct _timeb tb;
  ::_ftime (&tb);
  tv.tv_sec = tb.time;
  tv.tv_usec = 1000 * tb.millitm;
#endif /* 0 */
#elif defined (PDL_HAS_AIX_HI_RES_TIMER)
  timebasestruct_t tb;

  ::read_real_time (&tb, TIMEBASE_SZ);
  ::time_base_to_time (&tb, TIMEBASE_SZ);

  tv.tv_sec = tb.tb_high;
  tv.tv_usec = tb.tb_low / 1000L;
#else
# if defined (PDL_HAS_TIMEZONE_GETTIMEOFDAY) || \
  (defined (PDL_HAS_SVR4_GETTIMEOFDAY) && !defined (m88k) && !defined (SCO))
  PDL_OSCALL (::gettimeofday (&tv, 0), int, -1, result);
# elif defined (VXWORKS) || defined (CHORUS) || defined (PDL_PSOS)
  // Assumes that struct timespec is same size as struct timeval,
  // which assumes that time_t is a long: it currently (VxWorks
  // 5.2/5.3) is.
  struct timespec ts;

  PDL_OSCALL (PDL_OS::clock_gettime (CLOCK_REALTIME, &ts), int, -1, result);
  tv.tv_sec = ts.tv_sec;
  tv.tv_usec = ts.tv_nsec / 1000L;  // timespec has nsec, but timeval has usec
#elif defined(ITRON)
  /* empty */
  PDL_NOTSUP_RETURN (-1);
# else
  PDL_OSCALL (::gettimeofday (&tv), int, -1, result);
# endif /* PDL_HAS_SVR4_GETTIMEOFDAY */
#endif /* PDL_WIN32 */
  if (result == -1)
  {
    PDL_Time_Value sPDL_Time_Value;
    sPDL_Time_Value.initialize(-1);
    return sPDL_Time_Value;
  }
  else
  {
    PDL_Time_Value sPDL_Time_Value;
    sPDL_Time_Value.initialize(tv);
    return sPDL_Time_Value;
  }
}

#if !defined (PDL_HAS_WINCE)
PDL_INLINE int
PDL_OS::stat( const char *file, PDL_stat *stp )
{
  PDL_TRACE ("PDL_OS::stat");
#if defined (VXWORKS)
  PDL_OSCALL_RETURN (::stat ((char *) file, stp), int, -1);
#elif defined (PDL_PSOS_LACKS_PHILE)
  PDL_UNUSED_ARG (file);
  PDL_UNUSED_ARG (stp);
  PDL_NOTSUP_RETURN (-1);
#elif defined (PDL_PSOS)
  PDL_OSCALL_RETURN (::stat_f ((char *) file, stp), int, -1);
#else
# if defined (PDL_HAS_X86_STAT_MACROS)
   // Solaris for intel uses an macro for stat(), this macro is a
   // wrapper for _xstat().5
  PDL_OSCALL_RETURN (::_xstat (_STAT_VER, file, stp), int, -1);
#else /* !PDL_HAS_X86_STAT_MACROS */

#if defined (PDL_WIN32)
  # if (_MSC_VER <= 1200) && !defined( COMPILE_64BIT )
     PDL_OSCALL_RETURN (::stat(file, stp), int, -1);
  # else
     PDL_OSCALL_RETURN (::_stat64(file, stp), int, -1);
  # endif
#else
  PDL_OSCALL_RETURN (::stat(file, stp), int, -1);
#endif /* PDL_WIN32 */

#endif /* !PDL_HAS_X86_STAT_MACROS */
# endif /* VXWORKS */
}
#endif /* PDL_HAS_WINCE */

PDL_INLINE time_t
PDL_OS::time (time_t *tloc)
{
#if !defined (PDL_HAS_WINCE)
  PDL_TRACE ("PDL_OS::time");
#  if defined (PDL_PSOS) && ! defined (PDL_PSOS_HAS_TIME)
        unsigned long d_date, d_time, d_tick;
        tm_get(&d_date, &d_time, &d_tick); // get current time
        if (tloc)
                *tloc = d_time; // set time as time_t
        return d_time;
#  else
  PDL_OSCALL_RETURN (::time (tloc), time_t, (time_t) -1);
#  endif /* PDL_PSOS && ! PDL_PSOS_HAS_TIME */
#else
  time_t retv = PDL_OS::gettimeofday ().sec ();
  if (tloc)
    *tloc = retv;
  return retv;
#endif /* PDL_HAS_WINCE */
}

PDL_INLINE void
PDL_OS::srand (u_int seed)
{
  PDL_TRACE ("PDL_OS::srand");
  ::srand (seed);
}

PDL_INLINE int
PDL_OS::rand (void)
{
  PDL_TRACE ("PDL_OS::rand");
# if defined (PDL_WIN32)
  return ((((int)(::rand()) << 16) | ::rand()) & PDL_INT32_MAX);
# else
  PDL_OSCALL_RETURN (::rand (), int, -1);
# endif
}

#if !defined (PDL_HAS_MOSTLY_UNICODE_APIS)
PDL_INLINE int
PDL_OS::unlink (const char *path)
{
  PDL_TRACE ("PDL_OS::unlink");
# if defined (VXWORKS)
    PDL_OSCALL_RETURN (::unlink (PDL_const_cast (char *, path)), int, -1);
# elif defined (PDL_PSOS) && ! defined (PDL_PSOS_LACKS_PHILE)
    PDL_OSCALL_RETURN (::remove_f ((char *) path), int , -1);
# elif defined (PDL_PSOS) && defined (PDL_PSOS_HAS_C_LIBRARY)
    PDL_OSCALL_RETURN (PDL_ADAPT_RETVAL (::remove ((char *) path),
                                                   pdl_result_),
                       int, -1);
# elif defined (PDL_LACKS_UNLINK)
    PDL_UNUSED_ARG (path);
    PDL_NOTSUP_RETURN (-1);
#elif defined(ITRON)
  /* empty */
  PDL_NOTSUP_RETURN (-1);
# else
  PDL_OSCALL_RETURN (::unlink (path), int, -1);
# endif /* VXWORKS */
}

PDL_INLINE int
PDL_OS::remove (const char *path)
{
  PDL_TRACE ("PDL_OS::remove");
# if defined (VXWORKS)
    PDL_OSCALL_RETURN (::remove (PDL_const_cast (char *, path)), int, -1);
# elif defined (PDL_PSOS) && ! defined (PDL_PSOS_LACKS_PHILE)
    PDL_OSCALL_RETURN (::remove_f ((char *) path), int , -1);
# elif defined (PDL_PSOS) && defined (PDL_PSOS_HAS_C_LIBRARY)
    PDL_OSCALL_RETURN (PDL_ADAPT_RETVAL (::remove ((char *) path),
                                                   pdl_result_),
                       int, -1);
# elif defined (PDL_LACKS_REMOVE)
    PDL_UNUSED_ARG (path);
    PDL_NOTSUP_RETURN (-1);
#elif defined(ITRON)
  /* empty */
  PDL_NOTSUP_RETURN (-1);
# else
  PDL_OSCALL_RETURN (::remove (path), int, -1);
# endif /* VXWORKS */
}

PDL_INLINE int
PDL_OS::rename (const char *old_name, const char *new_name)
{
# if (PDL_LACKS_RENAME)
  PDL_UNUSED_ARG (old_name);
  PDL_UNUSED_ARG (new_name);
  PDL_NOTSUP_RETURN (-1);
# else
  PDL_OSCALL_RETURN (::rename (old_name, new_name), int, -1);
# endif /* PDL_LACKS_RENAME */
}

PDL_INLINE PDL_HANDLE
PDL_OS::shm_open (const char *filename,
                  int mode,
                  int perms,
                  LPSECURITY_ATTRIBUTES sa)
{
  PDL_TRACE ("PDL_OS::shm_open");
# if defined (PDL_HAS_SHM_OPEN)
  PDL_UNUSED_ARG (sa);
  PDL_OSCALL_RETURN (::shm_open (filename, mode, perms), PDL_HANDLE, -1);
# elif defined (PDL_WIN32)
  PDL_UNUSED_ARG (sa);
     PDL_NOTSUP_RETURN ((PDL_HANDLE)-1);
# else  /* ! PDL_HAS_SHM_OPEN */
  // Just use ::open.
  return PDL_OS::open (filename, mode, perms, sa);
# endif /* ! PDL_HAS_SHM_OPEN */
}

PDL_INLINE int
PDL_OS::shm_unlink (const char *path)
{
  PDL_TRACE ("PDL_OS::shm_unlink");
# if defined (PDL_HAS_SHM_OPEN)
  PDL_OSCALL_RETURN (::shm_unlink (path), int, -1);
# else  /* ! PDL_HAS_SHM_OPEN */
  // Just use ::unlink.
  return PDL_OS::unlink (path);
# endif /* ! PDL_HAS_SHM_OPEN */
}
#endif /* !PDL_HAS_MOSTLY_UNICODE_APIS */

// Doesn't need a macro since it *never* returns!

PDL_INLINE void
PDL_OS::_exit (int status)
{
  PDL_TRACE ("PDL_OS::_exit");
#if defined (VXWORKS)
  ::exit (status);
#elif defined (PDL_PSOSIM)
  ::u_exit (status);
#elif defined (PDL_PSOS)
# if defined (PDL_PSOS_LACKS_PREPC)  // pSoS TM does not support exit.
  PDL_UNUSED_ARG (status);
  return;
# else
  ::exit (status);
# endif /* defined (PDL_PSOS_LACKS_PREPC) */
#elif defined(ITRON)
  /* empty */
#elif !defined (PDL_HAS_WINCE)
  ::_exit (status);
#else
  ::TerminateProcess (::GetCurrentProcess (),
                      status);
#endif /* VXWORKS */
}

PDL_INLINE void
PDL_OS::abort (void)
{
#if !defined (PDL_HAS_WINCE)
  ::abort ();
#else
  // @@ CE doesn't support abort?
  ::exit (1);
#endif /* PDL_HAS_WINCE */
}

PDL_INLINE void *
PDL_OS::malloc (size_t nbytes)
{
  PDL_TRACE ("PDL_OS::malloc");

  return ::malloc (nbytes);
}

PDL_INLINE void *
PDL_OS::calloc (size_t elements, size_t sizeof_elements)
{
  PDL_TRACE ("PDL_OS::calloc");
#if defined(ALTIBASE_LIMIT_CHECK)
  void* sPtr = malloc(elements * sizeof_elements);
  if(NULL != sPtr)
  {
    memset(sPtr, 0, elements * sizeof_elements);
  }
  return sPtr;
#else
  return ::calloc (elements, sizeof_elements);
#endif
}

PDL_INLINE void *
PDL_OS::realloc (void *ptr, size_t nbytes)
{
  PDL_TRACE ("PDL_OS::realloc");
  return ::realloc (PDL_MALLOC_T (ptr), nbytes);
}

PDL_INLINE void
PDL_OS::free (void *ptr)
{
  // PDL_TRACE ("PDL_OS::free");
  ::free (PDL_MALLOC_T (ptr));
}

PDL_INLINE int
PDL_OS::memcmp (const void *t, const void *s, size_t len)
{
  PDL_TRACE ("PDL_OS::memcmp");
  return ::memcmp (t, s, len);
}

PDL_INLINE const void *
PDL_OS::memchr (const void *s, int c, size_t len)
{
#if defined (PDL_HAS_MEMCHR)
  PDL_TRACE ("PDL_OS::memchr");
  return ::memchr (s, c, len);
#else
  u_char *t = (u_char *) s;
  u_char *e = (u_char *) s + len;

  while (t < e)
    if (((int) *t) == c)
      return t;
    else
      t++;

  return 0;
#endif /* PDL_HAS_MEMCHR */
}

PDL_INLINE void *
PDL_OS::memcpy (void *t, const void *s, size_t len)
{
  // PDL_TRACE ("PDL_OS::memcpy");
  return ::memcpy (t, s, len);
}

PDL_INLINE void *
PDL_OS::memmove (void *t, const void *s, size_t len)
{
  PDL_TRACE ("PDL_OS::memmove");
  return ::memmove (t, s, len);
}

PDL_INLINE void *
PDL_OS::memset (void *s, int c, size_t len)
{
  // PDL_TRACE ("PDL_OS::memset");
  return ::memset (s, c, len);
}

PDL_INLINE char *
PDL_OS::strcat (char *s, const char *t)
{
  PDL_TRACE ("PDL_OS::strcat");
  return ::strcat (s, t);
}

PDL_INLINE size_t
PDL_OS::strcspn (const char *s, const char *reject)
{
#if !defined (PDL_HAS_WINCE)
  PDL_TRACE ("PDL_OS::strcspn");
  return ::strcspn (s, reject);
#else
  const char *scan;
  const char *rej_scan;
  int count = 0;

  for (scan = s; *scan; scan++)
    {

      for (rej_scan = reject; *rej_scan; rej_scan++)
        if (*scan == *rej_scan)
          return count;

      count++;
    }

  return count;
#endif /* PDL_HAS_WINCE */
}

PDL_INLINE size_t
PDL_OS::strspn (const char *s, const char *t)
{
#if !defined (PDL_HAS_WINCE)
  PDL_TRACE ("PDL_OS::strspn");
  return ::strspn (s, t);
#else
  PDL_UNUSED_ARG (s);
  PDL_UNUSED_ARG (t);
  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_HAS_WINCE */
}

PDL_INLINE char *
PDL_OS::strchr (char *s, int c)
{
#if !defined (PDL_HAS_WINCE)
  PDL_TRACE ("PDL_OS::strchr");
  return ::strchr (s, c);
#else
  for (;;++s)
    {
      if (*s == c)
        return s;
      if (*s == 0)
        return 0;
    }
#endif /* PDL_HAS_WINCE */
}

PDL_INLINE const char *
PDL_OS::strchr (const char *s, int c)
{
#if !defined (PDL_HAS_WINCE)
  PDL_TRACE ("PDL_OS::strchr");
  return (const char *) ::strchr (s, c);
#else
  for (;;++s)
    {
      if (*s == c)
        return s;
      if (*s == 0)
        return 0;
    }
#endif /* PDL_HAS_WINCE */
}

PDL_INLINE const char *
PDL_OS::strnchr (const char *s, int c, size_t len)
{
  for (size_t i = 0; i < len; i++)
    if (s[i] == c)
      return s + i;

  return 0;
}

PDL_INLINE char *
PDL_OS::strnchr (char *s, int c, size_t len)
{
#if defined PDL_PSOS_DIAB_PPC  //Complier problem Diab 4.2b
  const char *const_char_s=s;
  return (char *) PDL_OS::strnchr (const_char_s, c, len);
#else
  return (char *) PDL_OS::strnchr ((const char *) s, c, len);
#endif
}

PDL_INLINE const char *
PDL_OS::strstr (const char *s, const char *t)
{
  PDL_TRACE ("PDL_OS::strstr");
  return (const char *) ::strstr (s, t);
}

PDL_INLINE char *
PDL_OS::strstr (char *s, const char *t)
{
  PDL_TRACE ("PDL_OS::strstr");
  return ::strstr (s, t);
}

PDL_INLINE size_t
PDL_OS::strlen (const char *s)
{
  // PDL_TRACE ("PDL_OS::strlen");
  return ::strlen (s);
}

PDL_INLINE const char *
PDL_OS::strnstr (const char *s1, const char *s2, size_t len2)
{
  // Substring length
  size_t len1 = PDL_OS::strlen (s1);

  // Check if the substring is longer than the string being searched.
  if (len2 > len1)
    return 0;

  // Go upto <len>
  size_t len = len1 - len2;

  for (size_t i = 0; i <= len; i++)
    {
      if (PDL_OS::memcmp (s1 + i, s2, len2) == 0)
        // Found a match!  Return the index.
        return s1 + i;
    }

  return 0;
}

PDL_INLINE char *
PDL_OS::strnstr (char *s, const char *t, size_t len)
{
#if defined PDL_PSOS_DIAB_PPC  //Complier problem Diab 4.2b
  const char *const_char_s=s;
  return (char *) PDL_OS::strnstr (const_char_s, t, len);
#else
  return (char *) PDL_OS::strnstr ((const char *) s, t, len);
#endif
}

PDL_INLINE char *
PDL_OS::strrchr (char *s, int c)
{
  PDL_TRACE ("PDL_OS::strrchr");
#if !defined (PDL_LACKS_STRRCHR)
  return ::strrchr (s, c);
#else
  char *p = s + ::strlen (s);

  while (*p != c)
    if (p == s)
      return 0;
    else
      p--;

  return p;
#endif /* PDL_LACKS_STRRCHR */
}

PDL_INLINE const char *
PDL_OS::strrchr (const char *s, int c)
{
  PDL_TRACE ("PDL_OS::strrchr");
#if !defined (PDL_LACKS_STRRCHR)
  return (const char *) ::strrchr (s, c);
#else
  const char *p = s + ::strlen (s);

  while (*p != c)
    if (p == s)
      return 0;
    else
      p--;

  return p;
#endif /* PDL_LACKS_STRRCHR */
}

PDL_INLINE int
PDL_OS::strcmp (const char *s, const char *t)
{
  PDL_TRACE ("PDL_OS::strcmp");
  return ::strcmp (s, t);
}

PDL_INLINE char *
PDL_OS::strcpy (char *s, const char *t)
{
  // PDL_TRACE ("PDL_OS::strcpy");

  return ::strcpy (s, t);
}

PDL_INLINE char *
PDL_OS::strecpy (char *s, const char *t)
{
  //  PDL_TRACE ("PDL_OS::strecpy");
  register char *dscan = s;
  register const char *sscan = t;

  while ((*dscan++ = *sscan++) != '\0')
    continue;

  return dscan;
}

PDL_INLINE int
PDL_OS::to_lower (int c)
{
  PDL_TRACE ("PDL_OS::to_lower");
  return tolower (c);
}

PDL_INLINE char *
PDL_OS::strpbrk (char *s1, const char *s2)
{
#if !defined (PDL_HAS_WINCE)
  return ::strpbrk (s1, s2);
#else
  PDL_UNUSED_ARG (s1);
  PDL_UNUSED_ARG (s2);
  PDL_NOTSUP_RETURN (0);
#endif /* PDL_HAS_WINCE */
}

PDL_INLINE const char *
PDL_OS::strpbrk (const char *s1, const char *s2)
{
#if !defined (PDL_HAS_WINCE)
  return (const char *) ::strpbrk (s1, s2);
#else
  PDL_UNUSED_ARG (s1);
  PDL_UNUSED_ARG (s2);
  PDL_NOTSUP_RETURN (0);
#endif /* PDL_HAS_WINCE */
}

PDL_INLINE char *
PDL_OS::strdup (const char *s)
{
  // PDL_TRACE ("PDL_OS::strdup");

  // @@ Should we provide this function on WinCE?
#if defined (PDL_HAS_STRDUP_EMULATION)
  char *t = (char *) ::malloc (::strlen (s) + 1);
  if (t == 0)
    return 0;
  else
    return PDL_OS::strcpy (t, s);
#else
  return ::strdup (s);
#endif /* PDL_HAS_STRDUP_EMULATION */
}

#if !defined (PDL_HAS_WINCE)
PDL_INLINE int
PDL_OS::vsprintf (char *buffer, const char *format, va_list argptr)
{
  return PDL_SPRINTF_ADAPTER (::vsprintf (buffer, format, argptr));
}
#endif /* PDL_HAS_WINCE */

PDL_INLINE int
PDL_OS::strcasecmp (const char *s, const char *t)
{
#if !defined (PDL_WIN32) || defined (PDL_HAS_WINCE)
  PDL_TRACE ("PDL_OS::strcasecmp");
# if defined (PDL_LACKS_STRCASECMP)
  const char *scan1 = s;
  const char *scan2 = t;

  while (*scan1 != 0
         && PDL_OS::to_lower (*scan1) == PDL_OS::to_lower (*scan2))
    {
      ++scan1;
      ++scan2;
    }

  // The following case analysis is necessary so that characters which
  // look negative collate low against normal characters but high
  // against the end-of-string NUL.

  if (*scan1 == '\0' && *scan2 == '\0')
    return 0;
  else if (*scan1 == '\0')
    return -1;
  else if (*scan2 == '\0')
    return 1;
  else
    return PDL_OS::to_lower (*scan1) - PDL_OS::to_lower (*scan2);
# else
  return ::strcasecmp (s, t);
# endif /* PDL_LACKS_STRCASECMP */
#else /* PDL_WIN32 */
  return ::_stricmp (s, t);
#endif /* PDL_WIN32 */
}

PDL_INLINE int
PDL_OS::strncasecmp (const char *s,
                     const char *t,
                     size_t len)
{
#if !defined (PDL_WIN32) || defined (PDL_HAS_WINCE)
  PDL_TRACE ("PDL_OS::strncasecmp");
# if defined (PDL_LACKS_STRCASECMP)
  const char *scan1 = s;
  const char *scan2 = t;
  ssize_t count = ssize_t (len);

  while (--count >= 0
         && *scan1 != 0
         && PDL_OS::to_lower (*scan1) == PDL_OS::to_lower (*scan2))
    {
      ++scan1;
      ++scan2;
    }

  if (count < 0)
    return 0;

  // The following case analysis is necessary so that characters which
  // look negative collate low against normal characters but high
  // against the end-of-string NUL.

  if (*scan1 == '\0' && *scan2 == '\0')
    return 0;
  else if (*scan1 == '\0')
    return -1;
  else if (*scan2 == '\0')
    return 1;
  else
    return PDL_OS::to_lower (*scan1) - PDL_OS::to_lower (*scan2);
# else
  return ::strncasecmp (s, t, len);
# endif /* PDL_LACKS_STRCASECMP */
#else /* PDL_WIN32 */
  return ::_strnicmp (s, t, len);
#endif /* PDL_WIN32 */
}

PDL_INLINE int
PDL_OS::strncmp (const char *s, const char *t, size_t len)
{
  PDL_TRACE ("PDL_OS::strncmp");
  return ::strncmp (s, t, len);
}

PDL_INLINE char *
PDL_OS::strncpy (char *s, const char *t, size_t len)
{
  // PDL_TRACE ("PDL_OS::strncpy");
  return ::strncpy (s, t, len);
}

PDL_INLINE char *
PDL_OS::strncat (char *s, const char *t, size_t len)
{
  PDL_TRACE ("PDL_OS::strncat");
  return ::strncat (s, t, len);
}

PDL_INLINE char *
PDL_OS::strtok (char *s, const char *tokens)
{
  PDL_TRACE ("PDL_OS::strtok");
  return ::strtok (s, tokens);
}

PDL_INLINE char *
PDL_OS::strtok_r (char *s, const char *tokens, char **lasts)
{
  PDL_TRACE ("PDL_OS::strtok_r");
#if defined (PDL_HAS_REENTRANT_FUNCTIONS)
  return ::strtok_r (s, tokens, lasts);
#else
  if (s == 0)
    s = *lasts;
  else
    *lasts = s;
  if (*s == 0)                  // We have reached the end
    return 0;
  int l_org = PDL_OS::strlen (s);
  int l_sub = PDL_OS::strlen (s = ::strtok (s, tokens));
  *lasts = s + l_sub;
  if (l_sub != l_org)
    *lasts += 1;
  return s ;
#endif /* (PDL_HAS_REENTRANT_FUNCTIONS) */
}

PDL_INLINE long
PDL_OS::strtol (const char *s, char **ptr, int base)
{
  // @@ We must implement this function for WinCE also.
  // Notice WinCE support wcstol.

  PDL_TRACE ("PDL_OS::strtol");
  return ::strtol (s, ptr, base);
}

PDL_INLINE unsigned long
PDL_OS::strtoul (const char *s, char **ptr, int base)
{
  // @@ We must implement this function for WinCE also.
  // Notice WinCE support wcstoul.
#if !defined (PDL_HAS_WINCE)
  PDL_TRACE ("PDL_OS::strtoul");
# if defined (linux) && defined (__GLIBC__)
  // ::strtoul () appears to be broken on Linux 2.0.30/Alpha w/glibc:
  // it returns 0 for a negative number.
  return (unsigned long) ::strtol (s, ptr, base);
# else  /* ! linux || ! __GLIBC__ */
  return ::strtoul (s, ptr, base);
# endif /* ! linux || ! __GLIBC__ */
#else /* PDL_HAS_WINCE */
  PDL_UNUSED_ARG (s);
  PDL_UNUSED_ARG (ptr);
  PDL_UNUSED_ARG (base);
  PDL_NOTSUP_RETURN (0);
#endif /* PDL_HAS_WINCE */
}

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

#if defined(VXWORKS)
    static double powersOf10[] = {
                        /* Table giving binary powers of 10.  Entry */
        10.,			/* is 10^2^i.  Used to convert decimal */
        100.,			/* exponents into floating-point numbers. */
        1.0e4,
        1.0e8,
        1.0e16,
        1.0e32,
        1.0e64,
        1.0e128,
        1.0e256
    };

    static int maxExponent = 511;
#endif

PDL_INLINE double
PDL_OS::strtod_simple(const char *str1, char **endPtr)
{
#if !defined(VXWORKS)
    static double powersOf10[] = {
                        /* Table giving binary powers of 10.  Entry */
        10.,			/* is 10^2^i.  Used to convert decimal */
        100.,			/* exponents into floating-point numbers. */
        1.0e4,
        1.0e8,
        1.0e16,
        1.0e32,
        1.0e64,
        1.0e128,
        1.0e256
    };

    static int maxExponent = 511;
#endif
    int sign, expSign = FALSE;
    double fraction, dblExp, *d;
    const char *p;
    int c;
    int exp = 0;		/* Exponent read from "EX" field. */
    int fracExp = 0;		/* Exponent that derives from the fractional
				 * part.  Under normal circumstatnces, it is
				 * the negative of the number of digits in F.
				 * However, if I is very long, the last digits
				 * of I get dropped (otherwise a long I with a
				 * large negative exponent could cause an
				 * unnecessary overflow on I alone).  In this
				 * case, fracExp is incremented one for each
				 * dropped digit. */
    int mantSize;		/* Number of digits in mantissa. */
    int decPt;			/* Number of mantissa digits BEFORE decimal
				 * point. */
    const char *pExp;		/* Temporarily holds location of exponent
				 * in str1. */

    /*
     * Strip off leading blanks and check for a sign.
     */

    p = str1;
    while (isspace(*p)) {
	p += 1;
    }
    if (*p == '-') {
	sign = TRUE;
	p += 1;
    } else {
	if (*p == '+') {
	    p += 1;
	}
	sign = FALSE;
    }

    /*
     * Count the number of digits in the mantissa (including the decimal
     * point), and also locate the decimal point.
     */

    decPt = -1;
    for (mantSize = 0; ; mantSize += 1)
    {
	c = *p;
	if (!isdigit(c)) {
	    if ((c != '.') || (decPt >= 0)) {
		break;
	    }
	    decPt = mantSize;
	}
	p += 1;
    }

    /*
     * Now suck up the digits in the mantissa.  Use two integers to
     * collect 9 digits each (this is faster than using floating-point).
     * If the mantissa has more than 18 digits, ignore the extras, since
     * they can't affect the value anyway.
     */

    pExp  = p;
    p -= mantSize;
    if (decPt < 0) {
	decPt = mantSize;
    } else {
	mantSize -= 1;			/* One of the digits was the point. */
    }
    if (mantSize > 18) {
	fracExp = decPt - 18;
	mantSize = 18;
    } else {
	fracExp = decPt - mantSize;
    }
    if (mantSize == 0) {
	fraction = 0.0;
	p = str1;
	goto done;
    } else {
	int frac1, frac2;
	frac1 = 0;
	for ( ; mantSize > 9; mantSize -= 1)
	{
	    c = *p;
	    p += 1;
	    if (c == '.') {
		c = *p;
		p += 1;
	    }
	    frac1 = 10*frac1 + (c - '0');
	}
	frac2 = 0;
	for (; mantSize > 0; mantSize -= 1)
	{
	    c = *p;
	    p += 1;
	    if (c == '.') {
		c = *p;
		p += 1;
	    }
	    frac2 = 10*frac2 + (c - '0');
	}
	fraction = (1.0e9 * frac1) + frac2;
    }

    /*
     * Skim off the exponent.
     */

    p = pExp;
    if ((*p == 'E') || (*p == 'e')) {
	p += 1;
	if (*p == '-') {
	    expSign = TRUE;
	    p += 1;
	} else {
	    if (*p == '+') {
		p += 1;
	    }
	    expSign = FALSE;
	}
	while (isdigit(*p)) {
	    exp = exp * 10 + (*p - '0');
	    p += 1;
	}
    }
    if (expSign) {
	exp = fracExp - exp;
    } else {
	exp = fracExp + exp;
    }

    /*
     * Generate a floating-point number that represents the exponent.
     * Do this by processing the exponent one bit at a time to combine
     * many powers of 2 of 10. Then combine the exponent with the
     * fraction.
     */

    if (exp < 0) {
	expSign = TRUE;
	exp = -exp;
    } else {
	expSign = FALSE;
    }
    if (exp > maxExponent) {
	exp = maxExponent;
    }
    dblExp = 1.0;
    for (d = powersOf10; exp != 0; exp >>= 1, d += 1) {
	if (exp & 01) {
	    dblExp *= *d;
	}
    }
    if (expSign) {
	fraction /= dblExp;
    } else {
	fraction *= dblExp;
    }

done:
    if (endPtr != NULL) {
	*endPtr = (char *) p;
    }

    if (sign) {
	return -fraction;
    }
    return fraction;
}

PDL_INLINE int
PDL_OS::pdl_isspace (const char s)
{
#if !defined (PDL_HAS_WINCE)
  return isspace (s);
#else
  return ( s==' '||s=='\f'||s=='\n'||s=='\r'||s=='\t'||s=='\v') ? 1 : 0 ;
#endif /* PDL_HAS_WINCE */
}

PDL_INLINE long
PDL_OS::sysconf (int name)
{
  PDL_TRACE ("PDL_OS::sysconf");
#if defined (PDL_WIN32) || defined (VXWORKS) || defined (PDL_PSOS)
  PDL_UNUSED_ARG (name);
  PDL_NOTSUP_RETURN (-1);
#elif defined(ITRON)
  /* empty */
  PDL_NOTSUP_RETURN (-1);
#else
  PDL_OSCALL_RETURN (::sysconf (name), long, -1);
#endif /* PDL_WIN32 || VXWORKS */
}

PDL_INLINE int
PDL_OS::mutex_init (PDL_mutex_t *m,
                    int type,
                    LPCTSTR name,
                    void *arg,
                    LPSECURITY_ATTRIBUTES sa)
{
  // PDL_TRACE ("PDL_OS::mutex_init");
#if defined (PDL_HAS_THREADS)
# if defined (PDL_HAS_PTHREADS)
  PDL_UNUSED_ARG (name);
  PDL_UNUSED_ARG (arg);
  PDL_UNUSED_ARG (sa);

  pthread_mutexattr_t attributes;
  int result = -1;

#   if defined (PDL_HAS_PTHREADS_DRAFT4)
  if (::pthread_mutexattr_create (&attributes) == 0
#     if defined (PDL_HAS_PTHREAD_MUTEXATTR_SETKIND_NP)
      && ::pthread_mutexattr_setkind_np (&attributes, type) == 0
#     endif /* PDL_HAS_PTHREAD_MUTEXATTR_SETKIND_NP */
      && ::pthread_mutex_init (m, attributes) == 0)
#   elif defined (PDL_HAS_PTHREADS_DRAFT7) || defined (PDL_HAS_PTHREADS_STD)
    if (PDL_ADAPT_RETVAL(::pthread_mutexattr_init (&attributes), result) == 0
#     if defined (_POSIX_THREAD_PROCESS_SHARED) && !defined (PDL_LACKS_MUTEXATTR_PSHARED)
        && PDL_ADAPT_RETVAL(::pthread_mutexattr_setpshared(&attributes, type),
                            result) == 0
#     endif /* _POSIX_THREAD_PROCESS_SHARED && ! PDL_LACKS_MUTEXATTR_PSHARED */
        && PDL_ADAPT_RETVAL(::pthread_mutex_init (m, &attributes), result)== 0)
#   else // draft 6
    if (::pthread_mutexattr_init (&attributes) == 0
#     if !defined (PDL_LACKS_MUTEXATTR_PSHARED)
      && ::pthread_mutexattr_setpshared (&attributes, type) == 0
#     endif /* PDL_LACKS_MUTEXATTR_PSHARED */
#     if defined (PDL_HAS_PTHREAD_MUTEXATTR_SETKIND_NP)
        && ::pthread_mutexattr_setkind_np (&attributes, type) == 0
#     endif /* PDL_HAS_PTHREAD_MUTEXATTR_SETKIND_NP */
        && ::pthread_mutex_init (m, &attributes) == 0)
#   endif /* PDL_HAS_PTHREADS_DRAFT4 */
      result = 0;
  else
    result = -1;        // PDL_ADAPT_RETVAL used it for intermediate status

#   if (!defined (PDL_HAS_PTHREAD_MUTEXATTR_SETKIND_NP) && !defined (_POSIX_THREAD_PROCESS_SHARED)  ||  defined (PDL_LACKS_MUTEXATTR_PSHARED))
  PDL_UNUSED_ARG (type);
#   endif /* ! PDL_HAS_PTHREAD_MUTEXATTR_SETKIND_NP */

#   if defined (PDL_HAS_PTHREADS_DRAFT4)
  ::pthread_mutexattr_delete (&attributes);
#   else
  ::pthread_mutexattr_destroy (&attributes);
#   endif /* PDL_HAS_PTHREADS_DRAFT4 */

  return result;
# elif defined (PDL_HAS_STHREADS)
  PDL_UNUSED_ARG (name);
  PDL_UNUSED_ARG (sa);
  PDL_OSCALL_RETURN (PDL_ADAPT_RETVAL (::mutex_init (m, type, arg),
                                       pdl_result_),
                     int, -1);
# elif defined (PDL_HAS_WTHREADS)
  m->type_ = type;

  switch (type)
    {
    case USYNC_PROCESS:
      m->proc_mutex_ = ::CreateMutex (PDL_OS::default_win32_security_attributes (sa),
                                      FALSE,
                                      name);

      if (m->proc_mutex_ == 0)
        PDL_FAIL_RETURN (-1);
      else
        return 0;
    case USYNC_THREAD:
      return PDL_OS::thread_mutex_init (&(m->thr_mutex_),
                                        type,
                                        name,
                                        arg);
    default:
      errno = EINVAL;
      return -1;
    }
  /* NOTREACHED */

# elif defined (PDL_PSOS)
  PDL_UNUSED_ARG (type);
  PDL_UNUSED_ARG (arg);
  PDL_UNUSED_ARG (sa);
#   if defined (PDL_PSOS_HAS_MUTEX)

    u_long flags = MU_LOCAL;
    u_long ceiling = 0;

#     if defined (PDL_HAS_RECURSIVE_MUTEXES)
    flags |= MU_RECURSIVE;
#     else /* ! PDL_HAS_RECURSIVE_MUTEXES */
    flags |= MU_NONRECURSIVE;
#     endif /* PDL_HAS_RECURSIVE_MUTEXES */

#     if defined (PDL_PSOS_HAS_PRIO_MUTEX)

    flags |= MU_PRIOR;

#       if defined (PDL_PSOS_HAS_PRIO_INHERIT_MUTEX)
    flags |= MU_PRIO_INHERIT;
#       elif defined (PDL_PSOS_HAS_PRIO_PROTECT_MUTEX)
    ceiling =  PSOS_TASK_MAX_PRIORITY;
    flags |= MU_PRIO_PROTECT;
#       else
    flags |= MU_PRIO_NONE;
#       endif /* PDL_PSOS_HAS_PRIO_INHERIT_MUTEX */

#     else /* ! PDL_PSOS_HAS_PRIO_MUTEX */

    flags |= MU_FIFO | MU_PRIO_NONE;

#     endif

    return (::mu_create (PDL_reinterpret_cast (char *, name),
                         flags, ceiling, m) == 0) ? 0 : -1;

#   else /* ! PDL_PSOS_HAS_MUTEX */
  return ::sm_create ((char *) name,
                      1,
                      SM_LOCAL | SM_PRIOR,
                      m) == 0 ? 0 : -1;
#   endif /* PDL_PSOS_HAS_MUTEX */
# elif defined (VXWORKS)
  PDL_UNUSED_ARG (name);
  PDL_UNUSED_ARG (arg);
  PDL_UNUSED_ARG (sa);

  return (*m = ::semMCreate (type)) == 0 ? -1 : 0;
# endif /* PDL_HAS_PTHREADS */
#else
  PDL_UNUSED_ARG (m);
  PDL_UNUSED_ARG (type);
  PDL_UNUSED_ARG (name);
  PDL_UNUSED_ARG (arg);
  PDL_UNUSED_ARG (sa);
  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_HAS_THREADS */
}

PDL_INLINE int
PDL_OS::mutex_destroy (PDL_mutex_t *m)
{
  PDL_TRACE ("PDL_OS::mutex_destroy");
#if defined (PDL_HAS_THREADS)
# if defined (PDL_HAS_PTHREADS)
#   if (defined (PDL_HAS_PTHREADS_DRAFT4) || defined (PDL_HAS_PTHREADS_DRAFT6))
  PDL_OSCALL_RETURN (::pthread_mutex_destroy (m), int, -1);
#   else
  PDL_OSCALL_RETURN (PDL_ADAPT_RETVAL (::pthread_mutex_destroy (m),
                                       pdl_result_), int, -1);
#   endif /* PDL_HAS_PTHREADS_DRAFT4 || PDL_HAS_PTHREADS_DRAFT6*/
# elif defined (PDL_HAS_STHREADS)
  PDL_OSCALL_RETURN (PDL_ADAPT_RETVAL (::mutex_destroy (m), pdl_result_), int, -1);
# elif defined (PDL_HAS_WTHREADS)
  switch (m->type_)
    {
    case USYNC_PROCESS:
      PDL_WIN32CALL_RETURN (PDL_ADAPT_RETVAL (::CloseHandle (m->proc_mutex_),
                                              pdl_result_),
                            int, -1);
    case USYNC_THREAD:
      return PDL_OS::thread_mutex_destroy (&m->thr_mutex_);
    default:
      errno = EINVAL;
      return -1;
    }
  /* NOTREACHED */
# elif defined (PDL_PSOS)
#   if defined (PDL_PSOS_HAS_MUTEX)
  return (::mu_delete (*m) == 0) ? 0 : -1;
#   else /* ! PDL_PSOS_HAS_MUTEX */
  return (::sm_delete (*m) == 0) ? 0 : -1;
#   endif /* PDL_PSOS_HAS_MUTEX */
# elif defined (VXWORKS)
  return ::semDelete (*m) == OK ? 0 : -1;
# endif /* Threads variety case */
#else
  PDL_UNUSED_ARG (m);
  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_HAS_THREADS */
}

PDL_INLINE int
PDL_OS::mutex_lock (PDL_mutex_t *m)
{
  // PDL_TRACE ("PDL_OS::mutex_lock");
#if defined (PDL_HAS_THREADS)
# if defined (PDL_HAS_PTHREADS)
  // Note, don't use "::" here since the following call is often a macro.
#   if (defined (PDL_HAS_PTHREADS_DRAFT4) || defined (PDL_HAS_PTHREADS_DRAFT6))
  PDL_OSCALL_RETURN (pthread_mutex_lock (m), int, -1);
#   else
  PDL_OSCALL_RETURN (PDL_ADAPT_RETVAL (pthread_mutex_lock (m), pdl_result_),
                     int, -1);
#   endif /* PDL_HAS_PTHREADS_DRAFT4 || PDL_HAS_PTHREADS_DRAFT6 */
# elif defined (PDL_HAS_STHREADS)
  PDL_OSCALL_RETURN (PDL_ADAPT_RETVAL (::mutex_lock (m), pdl_result_), int, -1);
# elif defined (PDL_HAS_WTHREADS)
  switch (m->type_)
    {
    case USYNC_PROCESS:
      // Timeout can't occur, so don't bother checking...

      switch (::WaitForSingleObject (m->proc_mutex_, INFINITE))
        {
        case WAIT_OBJECT_0:
        case WAIT_ABANDONED:
          // We will ignore abandonments in this method
          // Note that we still hold the lock
          return 0;
        default:
          // This is a hack, we need to find an appropriate mapping...
          PDL_OS::set_errno_to_last_error ();
          return -1;
        }
    case USYNC_THREAD:
      return PDL_OS::thread_mutex_lock (&m->thr_mutex_);
    default:
      errno = EINVAL;
      return -1;
    }
  /* NOTREACHED */
# elif defined (PDL_PSOS)
#   if defined (PDL_PSOS_HAS_MUTEX)
  return (::mu_lock (*m, MU_WAIT, 0) == 0) ? 0 : -1;
#   else /* PDL_PSOS_HAS_MUTEX */
  return (::sm_p (*m, SM_WAIT, 0) == 0) ? 0 : -1;
#   endif /* PDL_PSOS_HAS_MUTEX */
# elif defined (VXWORKS)
  return ::semTake (*m, WAIT_FOREVER) == OK ? 0 : -1;
# endif /* Threads variety case */
#else
  PDL_UNUSED_ARG (m);
  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_HAS_THREADS */
}

PDL_INLINE int
PDL_OS::mutex_lock (PDL_mutex_t *m,
                    int &abandoned)
{
  PDL_TRACE ("PDL_OS::mutex_lock");
#if defined (PDL_HAS_THREADS) && defined (PDL_HAS_WTHREADS)
  abandoned = 0;
  switch (m->type_)
    {
    case USYNC_PROCESS:
      // Timeout can't occur, so don't bother checking...

      switch (::WaitForSingleObject (m->proc_mutex_, INFINITE))
        {
        case WAIT_OBJECT_0:
          return 0;
        case WAIT_ABANDONED:
          abandoned = 1;
          return 0;  // something goofed, but we hold the lock ...
        default:
          // This is a hack, we need to find an appropriate mapping...
          PDL_OS::set_errno_to_last_error ();
          return -1;
        }
    case USYNC_THREAD:
      return PDL_OS::thread_mutex_lock (&m->thr_mutex_);
    default:
      errno = EINVAL;
      return -1;
    }
  /* NOTREACHED */
#else
  PDL_UNUSED_ARG (m);
  PDL_UNUSED_ARG (abandoned);
  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_HAS_THREADS and PDL_HAS_WTHREADS */
}

PDL_INLINE int
PDL_OS::mutex_trylock (PDL_mutex_t *m)
{
  PDL_TRACE ("PDL_OS::mutex_trylock");
#if defined (PDL_HAS_THREADS)
# if defined (PDL_HAS_PTHREADS)
  // Note, don't use "::" here since the following call is often a macro.
#   if (defined (PDL_HAS_PTHREADS_DRAFT4) || defined (PDL_HAS_PTHREADS_DRAFT6))
  int status = pthread_mutex_trylock (m);
  if (status == 1)
    status = 0;
  else if (status == 0) {
    status = -1;
    errno = EBUSY;
  }
  return status;
#   else
  PDL_OSCALL_RETURN (PDL_ADAPT_RETVAL (pthread_mutex_trylock (m), pdl_result_),
                     int, -1);
#   endif /* PDL_HAS_PTHREADS_DRAFT4 || PDL_HAS_PTHREADS_DRAFT6 */
# elif defined (PDL_HAS_STHREADS)
  PDL_OSCALL_RETURN (PDL_ADAPT_RETVAL (::mutex_trylock (m), pdl_result_), int, -1);
# elif defined (PDL_HAS_WTHREADS)
  switch (m->type_)
    {
    case USYNC_PROCESS:
      {
        // Try for 0 milliseconds - i.e. nonblocking.
        switch (::WaitForSingleObject (m->proc_mutex_, 0))
          {
          case WAIT_OBJECT_0:
            return 0;
          case WAIT_ABANDONED:
            // We will ignore abandonments in this method.  Note that
            // we still hold the lock.
            return 0;
          case WAIT_TIMEOUT:
            errno = EBUSY;
            return -1;
          default:
            PDL_OS::set_errno_to_last_error ();
            return -1;
          }
      }
    case USYNC_THREAD:
      return PDL_OS::thread_mutex_trylock (&m->thr_mutex_);
    default:
      errno = EINVAL;
      return -1;
    }
  /* NOTREACHED */
# elif defined (PDL_PSOS)
#   if defined (PDL_PSOS_HAS_MUTEX)
   return (::mu_lock (*m, MU_NOWAIT, 0) == 0) ? 0 : -1;
#   else /* ! PDL_PSOS_HAS_MUTEX */
   switch (::sm_p (*m, SM_NOWAIT, 0))
   {
     case 0:
       return 0;
     case ERR_NOSEM:
       errno = EBUSY;
       // intentional fall through
     default:
       return -1;
   }
#   endif /* PDL_PSOS_HAS_MUTEX */

# elif defined (VXWORKS)
  if (::semTake (*m, NO_WAIT) == ERROR)
    if (errno == S_objLib_OBJ_UNAVAILABLE)
      {
        // couldn't get the semaphore
        errno = EBUSY;
        return -1;
      }
    else
      // error
      return -1;
  else
    // got the semaphore
    return 0;
# endif /* Threads variety case */
#else
  PDL_UNUSED_ARG (m);
  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_HAS_THREADS */
}

PDL_INLINE int
PDL_OS::mutex_trylock (PDL_mutex_t *m, int &abandoned)
{
#if defined (PDL_HAS_THREADS) && defined (PDL_HAS_WTHREADS)
  abandoned = 0;
  switch (m->type_)
    {
    case USYNC_PROCESS:
      {
        // Try for 0 milliseconds - i.e. nonblocking.
        switch (::WaitForSingleObject (m->proc_mutex_, 0))
          {
          case WAIT_OBJECT_0:
            return 0;
          case WAIT_ABANDONED:
            abandoned = 1;
            return 0;  // something goofed, but we hold the lock ...
          case WAIT_TIMEOUT:
            errno = EBUSY;
            return -1;
          default:
            PDL_OS::set_errno_to_last_error ();
            return -1;
          }
      }
    case USYNC_THREAD:
      return PDL_OS::thread_mutex_trylock (&m->thr_mutex_);
    default:
      errno = EINVAL;
      return -1;
    }
  /* NOTREACHED */
#else
  PDL_UNUSED_ARG (m);
  PDL_UNUSED_ARG (abandoned);
  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_HAS_THREADS and PDL_HAS_WTHREADS */
}

PDL_INLINE int
PDL_OS::mutex_unlock (PDL_mutex_t *m)
{
  PDL_TRACE ("PDL_OS::mutex_unlock");
#if defined (PDL_HAS_THREADS)
# if defined (PDL_HAS_PTHREADS)
  // Note, don't use "::" here since the following call is often a macro.
#   if (defined (PDL_HAS_PTHREADS_DRAFT4) || defined (PDL_HAS_PTHREADS_DRAFT6))
  PDL_OSCALL_RETURN (pthread_mutex_unlock (m), int, -1);
#   else
  PDL_OSCALL_RETURN (PDL_ADAPT_RETVAL (pthread_mutex_unlock (m), pdl_result_),
                     int, -1);
#   endif /* PDL_HAS_PTHREADS_DRAFT4 || PDL_HAS_PTHREADS_DRAFT6 */
# elif defined (PDL_HAS_STHREADS)
  PDL_OSCALL_RETURN (PDL_ADAPT_RETVAL (::mutex_unlock (m), pdl_result_), int, -1);
# elif defined (PDL_HAS_WTHREADS)
  switch (m->type_)
    {
    case USYNC_PROCESS:
      PDL_WIN32CALL_RETURN (PDL_ADAPT_RETVAL (::ReleaseMutex (m->proc_mutex_),
                                              pdl_result_),
                            int, -1);
    case USYNC_THREAD:
      return PDL_OS::thread_mutex_unlock (&m->thr_mutex_);
    default:
      errno = EINVAL;
      return -1;
    }
  /* NOTREACHED */
# elif defined (PDL_PSOS)
#   if defined (PDL_PSOS_HAS_MUTEX)
  return (::mu_unlock (*m) == 0) ? 0 : -1;
#   else /* ! PDL_PSOS_HAS_MUTEX */
  return (::sm_v (*m) == 0) ? 0 : -1;
#   endif /* PDL_PSOS_HAS_MUTEX */
# elif defined (VXWORKS)
  return ::semGive (*m) == OK ? 0 : -1;
# endif /* Threads variety case */
#else
  PDL_UNUSED_ARG (m);
  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_HAS_THREADS */
}

PDL_INLINE int
PDL_OS::thread_mutex_init (PDL_thread_mutex_t *m,
                           int type,
                           LPCTSTR name,
                           void *arg)
{
  // PDL_TRACE ("PDL_OS::thread_mutex_init");
#if defined(ITRON)
  /* empty */
  return 0;
#else
#if defined (PDL_HAS_THREADS)
# if defined (PDL_HAS_STHREADS) || defined (PDL_HAS_PTHREADS)
  PDL_UNUSED_ARG (type);
  // Force the use of USYNC_THREAD!
  return PDL_OS::mutex_init (m, USYNC_THREAD, name, arg);
# elif defined (PDL_HAS_WTHREADS)
#if defined(PDL_HAS_WINCE)
  m->mutex = ::CreateMutex(NULL, FALSE, NULL);

  if(m->mutex == 0)
  {
    return -1;
  }
  else
  {
    return 0;
  } 
#else
  PDL_UNUSED_ARG (type);
  PDL_UNUSED_ARG (name);
  PDL_UNUSED_ARG (arg);

  __try
  {
    ::InitializeCriticalSection (m);
  }
  __except (::GetExceptionCode() == STATUS_NO_MEMORY)
  {
    return -1;
  }

  return 0;
#endif
# elif defined (VXWORKS) || defined (PDL_PSOS)
  return mutex_init (m, type, name, arg);
# endif /* Threads variety case */
#else
  PDL_UNUSED_ARG (m);
  PDL_UNUSED_ARG (type);
  PDL_UNUSED_ARG (name);
  PDL_UNUSED_ARG (arg);
  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_HAS_THREADS */
#endif
}

PDL_INLINE int
PDL_OS::thread_mutex_destroy (PDL_thread_mutex_t *m)
{
  PDL_TRACE ("PDL_OS::thread_mutex_destroy");
#if defined(ITRON)
  /* empty */
  return 0;
#else
#if defined (PDL_HAS_THREADS)
# if defined (PDL_HAS_STHREADS) || defined (PDL_HAS_PTHREADS)
  return PDL_OS::mutex_destroy (m);
# elif defined (PDL_HAS_WTHREADS)
#ifdef PDL_HAS_WINCE
  PDL_WIN32CALL_RETURN(PDL_ADAPT_RETVAL(::CloseHandle(m->mutex), pdl_result_), int, -1);
#else
  ::DeleteCriticalSection (m);
  return 0;
#endif
# elif defined (VXWORKS) || defined (PDL_PSOS)
  return mutex_destroy (m);
# endif /* PDL_HAS_STHREADS || PDL_HAS_PTHREADS */
#else
  PDL_UNUSED_ARG (m);
  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_HAS_THREADS */
#endif
}

PDL_INLINE int
PDL_OS::thread_mutex_lock (PDL_thread_mutex_t *m)
{
  // PDL_TRACE ("PDL_OS::thread_mutex_lock");
#if defined(ITRON)
  /* empty */
  return 0;
#else
#if defined (PDL_HAS_THREADS)
# if defined (PDL_HAS_STHREADS) || defined (PDL_HAS_PTHREADS)
  return PDL_OS::mutex_lock (m);
# elif defined (PDL_HAS_WTHREADS)
#if defined(PDL_HAS_WINCE)
  switch(::WaitForSingleObject(m->mutex, INFINITE))
  {
    case WAIT_OBJECT_0:
    case WAIT_ABANDONED:
      return 0;
    default:
      PDL_OS::set_errno_to_last_error ();
      return -1;
  }
#else
  __try
  {
    ::EnterCriticalSection (m);
  }
  __except (::GetExceptionCode() == STATUS_INVALID_HANDLE)
  {
    return -1;
  }

  return 0;
#endif
#elif defined (VXWORKS) || defined (PDL_PSOS)
  return mutex_lock (m);
# endif /* PDL_HAS_STHREADS || PDL_HAS_PTHREADS */
#else
  PDL_UNUSED_ARG (m);
  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_HAS_THREADS */
#endif
}

PDL_INLINE int
PDL_OS::thread_mutex_trylock (PDL_thread_mutex_t *m)
{
  PDL_TRACE ("PDL_OS::thread_mutex_trylock");
#if defined (PDL_HAS_WINCE)
  switch(::WaitForSingleObject(m->mutex, 0))
  {
    case WAIT_OBJECT_0:
      return 0;
    case WAIT_ABANDONED:
      return 0;
    case WAIT_TIMEOUT:
      errno = EBUSY;
      return -1;
    default:
      PDL_OS::set_errno_to_last_error();
      return -1;
  }
#else
#if defined (PDL_HAS_THREADS)
# if defined (PDL_HAS_STHREADS) || defined (PDL_HAS_PTHREADS)
  return PDL_OS::mutex_trylock (m);
# elif defined (PDL_HAS_WTHREADS)
#   if defined (PDL_HAS_WIN32_TRYLOCK)
  BOOL result = ::TryEnterCriticalSection (m);
  if (result != 0)
    return 0;
  else
    {
      errno = EBUSY;
      return -1;
    }
#   elif defined (PDL_HAS_WINCE)
  BOOL sResult = ::EnterCriticalSection (m);
    if (result != 0)
    return 0;
  else
    {
      errno = GetLastError();
      return -1;
    }

#   else
  PDL_UNUSED_ARG (m);
  PDL_NOTSUP_RETURN (-1);
#   endif /* PDL_HAS_WIN32_TRYLOCK */
# elif defined (VXWORKS) || defined (PDL_PSOS)
  return PDL_OS::mutex_trylock (m);
# endif /* Threads variety case */
#else
  PDL_UNUSED_ARG (m);
  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_HAS_THREADS */
#endif /* !ITRON  || PDL_HAS_WINCE */
}

PDL_INLINE int
PDL_OS::thread_mutex_unlock (PDL_thread_mutex_t *m)
{
  PDL_TRACE ("PDL_OS::thread_mutex_unlock");
#if defined(ITRON)
  /* empty */
  return 0;
#else
#if defined (PDL_HAS_THREADS)
# if defined (PDL_HAS_STHREADS) || defined (PDL_HAS_PTHREADS)
  return PDL_OS::mutex_unlock (m);
# elif defined (PDL_HAS_WTHREADS)
#ifdef PDL_HAS_WINCE
  PDL_WIN32CALL_RETURN(PDL_ADAPT_RETVAL(::ReleaseMutex(m->mutex), pdl_result_), int, -1);
#else
  ::LeaveCriticalSection (m);
  return 0;
#endif
# elif defined (VXWORKS) || defined (PDL_PSOS)
  return PDL_OS::mutex_unlock (m);
# endif /* Threads variety case */
#else
  PDL_UNUSED_ARG (m);
  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_HAS_THREADS */
#endif
}

#if !defined (PDL_LACKS_COND_T)
// NOTE: The PDL_OS::cond_* functions for Unix platforms are defined
// here because the PDL_OS::sema_* functions below need them.
// However, PDL_WIN32 and VXWORKS define the PDL_OS::cond_* functions
// using the PDL_OS::sema_* functions.  So, they are defined in OS.cpp.

PDL_INLINE int
PDL_OS::cond_destroy (PDL_cond_t *cv)
{
  PDL_TRACE ("PDL_OS::cond_destroy");
# if defined (PDL_HAS_THREADS)
#   if defined (PDL_HAS_PTHREADS)
#     if defined (PDL_HAS_PTHREADS_DRAFT4) || defined (PDL_HAS_PTHREADS_DRAFT6)
  PDL_OSCALL_RETURN (::pthread_cond_destroy (cv), int, -1);
#     else
  PDL_OSCALL_RETURN (PDL_ADAPT_RETVAL (::pthread_cond_destroy (cv), pdl_result_), int, -1);
#     endif /* PDL_HAS_PTHREADS_DRAFT4 || PDL_HAS_PTHREADS_DRAFT6 */
#   elif defined (PDL_HAS_STHREADS)
  PDL_OSCALL_RETURN (PDL_ADAPT_RETVAL (::cond_destroy (cv), pdl_result_), int, -1);
#   elif defined (PDL_PSOS) && defined (PDL_PSOS_HAS_COND_T)
  return (::cv_delete (*cv)) ? 0 : -1;
#   endif /* PDL_HAS_STHREADS */
# else
  PDL_UNUSED_ARG (cv);
  PDL_NOTSUP_RETURN (-1);
# endif /* PDL_HAS_THREADS */
}

PDL_INLINE int
PDL_OS::condattr_init (PDL_condattr_t &attributes,
                       int type)
{
  PDL_UNUSED_ARG (type);
# if defined (PDL_HAS_THREADS)
#   if defined (PDL_HAS_PTHREADS)
  int result = -1;

  if (
#     if defined  (PDL_HAS_PTHREADS_DRAFT4)
      ::pthread_condattr_create (&attributes) == 0
#     elif defined (PDL_HAS_PTHREADS_STD) || defined (PDL_HAS_PTHREADS_DRAFT7)
      PDL_ADAPT_RETVAL(::pthread_condattr_init (&attributes), result) == 0
#       if defined (_POSIX_THREAD_PROCESS_SHARED) && !defined (PDL_LACKS_MUTEXATTR_PSHARED)
      && PDL_ADAPT_RETVAL(::pthread_condattr_setpshared(&attributes, type),
                          result) == 0
#       endif /* _POSIX_THREAD_PROCESS_SHARED && ! PDL_LACKS_MUTEXATTR_PSHARED */
#     else  /* this is draft 6 */
      ::pthread_condattr_init (&attributes) == 0
#       if !defined (PDL_LACKS_CONDATTR_PSHARED)
      && ::pthread_condattr_setpshared (&attributes, type) == 0
#       endif /* PDL_LACKS_CONDATTR_PSHARED */
#       if defined (PDL_HAS_PTHREAD_CONDATTR_SETKIND_NP)
      && ::pthread_condattr_setkind_np (&attributes, type) == 0
#       endif /* PDL_HAS_PTHREAD_CONDATTR_SETKIND_NP */
#     endif /* PDL_HAS_PTHREADS_DRAFT4 */
      )
     result = 0;
  else
     result = -1;       // PDL_ADAPT_RETVAL used it for intermediate status

  return result;
#   elif defined (PDL_HAS_STHREADS)
  attributes.type = type;

  return 0;
#   endif /* PDL_HAS_PTHREADS && PDL_HAS_STHREADS */

# else
  PDL_UNUSED_ARG (attributes);
  PDL_UNUSED_ARG (type);
  PDL_NOTSUP_RETURN (-1);
# endif /* PDL_HAS_THREADS */
}

PDL_INLINE int
PDL_OS::condattr_destroy (PDL_condattr_t &attributes)
{
# if defined (PDL_HAS_THREADS)
#   if defined (PDL_HAS_PTHREADS)

#     if defined (PDL_HAS_PTHREADS_DRAFT4)
  ::pthread_condattr_delete (&attributes);
#     else
  ::pthread_condattr_destroy (&attributes);
#     endif /* PDL_HAS_PTHREADS_DRAFT4 */

#   elif defined (PDL_HAS_STHREADS)
  attributes.type = 0;
#   endif /* PDL_HAS_PTHREADS && PDL_HAS_STHREADS */
  return 0;
# else
  PDL_UNUSED_ARG (attributes);
  return 0;
# endif /* PDL_HAS_THREADS */
}

PDL_INLINE int
PDL_OS::cond_init (PDL_cond_t *cv,
                   PDL_condattr_t &attributes,
                   LPCTSTR name,
                   void *arg)
{
  // PDL_TRACE ("PDL_OS::cond_init");
  PDL_UNUSED_ARG (name);
  PDL_UNUSED_ARG (arg);
# if defined (PDL_HAS_THREADS)
#   if defined (PDL_HAS_PTHREADS)
  int result = -1;

  if (
#     if defined  (PDL_HAS_PTHREADS_DRAFT4)
      ::pthread_cond_init (cv, attributes) == 0
#     elif defined (PDL_HAS_PTHREADS_STD) || defined (PDL_HAS_PTHREADS_DRAFT7)
      PDL_ADAPT_RETVAL(::pthread_cond_init (cv, &attributes), result) == 0
#     else  /* this is draft 6 */
      ::pthread_cond_init (cv, &attributes) == 0
#     endif /* PDL_HAS_PTHREADS_DRAFT4 */
      )
     result = 0;
  else
     result = -1;       // PDL_ADAPT_RETVAL used it for intermediate status

  return result;
#   elif defined (PDL_HAS_STHREADS)
  PDL_OSCALL_RETURN (PDL_ADAPT_RETVAL (::cond_init (cv,
                                                    attributes.type,
                                                    arg),
                                       pdl_result_),
                     int, -1);
#   elif defined (PDL_PSOS) && defined (PDL_PSOS_HAS_COND_T)
#     if defined (PDL_PSOS_HAS_PRIO_MUTEX)
  u_long flags = CV_LOCAL | CV_PRIOR;
#     else /* PDL_PSOS_HAS_PRIO_MUTEX */
  u_long flags = CV_LOCAL | CV_FIFO;
#     endif /* PDL_PSOS_HAS_PRIO_MUTEX */
  return (::cv_create ((char *) name, flags, cv)) ? 0 : -1;
#   endif /* PDL_HAS_PTHREADS && PDL_HAS_STHREADS */
# else
  PDL_UNUSED_ARG (cv);
  PDL_UNUSED_ARG (attributes);
  PDL_UNUSED_ARG (name);
  PDL_UNUSED_ARG (arg);
  PDL_NOTSUP_RETURN (-1);
# endif /* PDL_HAS_THREADS */
}

PDL_INLINE int
PDL_OS::cond_init (PDL_cond_t *cv, short type, LPCTSTR name, void *arg)
{
  PDL_condattr_t attributes;
  if (PDL_OS::condattr_init (attributes, type) == 0
      && PDL_OS::cond_init (cv, attributes, name, arg) == 0)
    {
      (void) PDL_OS::condattr_destroy (attributes);
      return 0;
    }
  return -1;
}

PDL_INLINE int
PDL_OS::cond_signal (PDL_cond_t *cv)
{
PDL_TRACE ("PDL_OS::cond_signal");
# if defined (PDL_HAS_THREADS)
#   if defined (PDL_HAS_PTHREADS)
#     if defined (PDL_HAS_PTHREADS_DRAFT4) || defined (PDL_HAS_PTHREADS_DRAFT6)
  PDL_OSCALL_RETURN (::pthread_cond_signal (cv), int, -1);
#     else
  PDL_OSCALL_RETURN (PDL_ADAPT_RETVAL (::pthread_cond_signal (cv),pdl_result_),
                     int, -1);
#     endif /* PDL_HAS_PTHREADS_DRAFT4 || PDL_HAS_PTHREADS_DRAFT6 */
#   elif defined (PDL_HAS_STHREADS)
  PDL_OSCALL_RETURN (PDL_ADAPT_RETVAL (::cond_signal (cv), pdl_result_), int, -1);
#   elif defined (PDL_PSOS) && defined (PDL_PSOS_HAS_COND_T)
  return (::cv_signal (*cv)) ? 0 : -1;
#   endif /* PDL_HAS_STHREADS */
# else
  PDL_UNUSED_ARG (cv);
  PDL_NOTSUP_RETURN (-1);
# endif /* PDL_HAS_THREADS */
}

PDL_INLINE int
PDL_OS::cond_broadcast (PDL_cond_t *cv)
{
PDL_TRACE ("PDL_OS::cond_broadcast");
# if defined (PDL_HAS_THREADS)
#   if defined (PDL_HAS_PTHREADS)
#     if defined (PDL_HAS_PTHREADS_DRAFT4) || defined (PDL_HAS_PTHREADS_DRAFT6)
  PDL_OSCALL_RETURN (::pthread_cond_broadcast (cv), int, -1);
#     else
  PDL_OSCALL_RETURN (PDL_ADAPT_RETVAL (::pthread_cond_broadcast (cv),
                                       pdl_result_),
                     int, -1);
#     endif /* PDL_HAS_PTHREADS_DRAFT4 || PDL_HAS_PTHREADS_DRAFT6 */
#   elif defined (PDL_HAS_STHREADS)
  PDL_OSCALL_RETURN (PDL_ADAPT_RETVAL (::cond_broadcast (cv),
                                       pdl_result_),
                     int, -1);
#   elif defined (PDL_PSOS) && defined (PDL_PSOS_HAS_COND_T)
  return (::cv_broadcast (*cv)) ? 0 : -1;
#   endif /* PDL_HAS_STHREADS */
# else
  PDL_UNUSED_ARG (cv);
  PDL_NOTSUP_RETURN (-1);
# endif /* PDL_HAS_THREADS */
}

PDL_INLINE int
PDL_OS::cond_wait (PDL_cond_t *cv,
                   PDL_mutex_t *external_mutex)
{
  PDL_TRACE ("PDL_OS::cond_wait");
# if defined (PDL_HAS_THREADS)
#   if defined (PDL_HAS_PTHREADS)
#     if defined (PDL_HAS_PTHREADS_DRAFT4) || defined (PDL_HAS_PTHREADS_DRAFT6)
  PDL_OSCALL_RETURN (::pthread_cond_wait (cv, external_mutex), int, -1);
#     else
  PDL_OSCALL_RETURN (PDL_ADAPT_RETVAL (::pthread_cond_wait (cv, external_mutex), pdl_result_),
                     int, -1);
#     endif /* PDL_HAS_PTHREADS_DRAFT4 || PDL_HAS_PTHREADS_DRAFT6 */
#   elif defined (PDL_HAS_STHREADS)
  PDL_OSCALL_RETURN (PDL_ADAPT_RETVAL (::cond_wait (cv, external_mutex), pdl_result_),
                     int, -1);
#   elif defined (PDL_PSOS) && defined (PDL_PSOS_HAS_COND_T)
  return (::cv_wait (*cv, *external_mutex, 0)) ? 0 : -1;
#   endif /* PDL_HAS_PTHREADS */
# else
  PDL_UNUSED_ARG (cv);
  PDL_UNUSED_ARG (external_mutex);
  PDL_NOTSUP_RETURN (-1);
# endif /* PDL_HAS_THREADS */
}

PDL_INLINE int
PDL_OS::cond_timedwait (PDL_cond_t *cv,
                        PDL_mutex_t *external_mutex,
                        PDL_Time_Value *timeout)
{
  PDL_TRACE ("PDL_OS::cond_timedwait");
# if defined (PDL_HAS_THREADS)
  int result;
  timespec_t ts;

  if (timeout != 0)
    ts = *timeout; // Calls PDL_Time_Value::operator timespec_t().

#   if defined (PDL_HAS_PTHREADS)

#     if defined (PDL_HAS_PTHREADS_DRAFT4) || defined (PDL_HAS_PTHREADS_DRAFT6)
  if (timeout == 0)
    PDL_OSCALL (::pthread_cond_wait (cv, external_mutex),
                int, -1, result);
  else
    {

#     if defined (__Lynx__)
      // Note that we must convert between absolute time (which is
      // passed as a parameter) and relative time (which is what the
      // LynxOS pthread_cond_timedwait expects).  This differs from 1003.4a
      // draft 4.

      timespec_t relative_time = *timeout - PDL_OS::gettimeofday ();

      PDL_OSCALL (::pthread_cond_timedwait (cv, external_mutex,
                                            &relative_time),
                  int, -1, result);
#     else
      PDL_OSCALL (::pthread_cond_timedwait (cv, external_mutex,
                                            (PDL_TIMESPEC_PTR) &ts),
                  int, -1, result);
#     endif /* __Lynx__ */
    }

#     else
  PDL_OSCALL (PDL_ADAPT_RETVAL (timeout == 0
                                ? ::pthread_cond_wait (cv, external_mutex)
                                : ::pthread_cond_timedwait (cv, external_mutex,
                                                            (PDL_TIMESPEC_PTR) &ts),
                                result),
              int, -1, result);
#     endif /* PDL_HAS_PTHREADS_DRAFT4 || PDL_HAS_PTHREADS_DRAFT6*/
  // We need to adjust this to make the POSIX and Solaris return
  // values consistent.  EAGAIN is from Pthreads DRAFT4 (HP-UX 10.20 and
  // down); EINTR is from LynxOS.
  if (result == -1 &&
      (errno == ETIMEDOUT || errno == EAGAIN || errno == EINTR))
    errno = ETIME;

#   elif defined (PDL_HAS_STHREADS)
  PDL_OSCALL (PDL_ADAPT_RETVAL (timeout == 0
                                ? ::cond_wait (cv, external_mutex)
                                : ::cond_timedwait (cv,
                                                    external_mutex,
                                                    (timestruc_t*)&ts),
                                result),
              int, -1, result);
#   endif /* PDL_HAS_STHREADS */
  if (timeout != 0)
    timeout->set (ts); // Update the time value before returning.

  return result;
# else
  PDL_UNUSED_ARG (cv);
  PDL_UNUSED_ARG (external_mutex);
  PDL_UNUSED_ARG (timeout);
  PDL_NOTSUP_RETURN (-1);
# endif /* PDL_HAS_THREADS */
}
#endif /* !PDL_LACKS_COND_T */

PDL_INLINE int
PDL_OS::thr_equal (PDL_thread_t t1, PDL_thread_t t2)
{
#if defined (PDL_HAS_PTHREADS)
# if defined (pthread_equal)
  // If it's a macro we can't say "::pthread_equal"...
  return pthread_equal (t1, t2);
# else
  return ::pthread_equal (t1, t2);
# endif /* pthread_equal */
#elif defined (VXWORKS)
  return ! PDL_OS::strcmp (t1, t2);
#else /* For both STHREADS and WTHREADS... */
  // Hum, Do we need to treat WTHREAD differently?
  // levine 13 oct 98 % I don't think so, PDL_thread_t is a DWORD.
  return t1 == t2;
#endif /* Threads variety case */
}

PDL_INLINE void
PDL_OS::thr_self (PDL_hthread_t &self)
{
  PDL_TRACE ("PDL_OS::thr_self");
#if defined (PDL_HAS_THREADS)
# if defined (PDL_HAS_PTHREADS)
  // Note, don't use "::" here since the following call is often a macro.
  self = pthread_self ();
# elif defined (PDL_HAS_THREAD_SELF)
  self = ::thread_self ();
# elif defined (PDL_HAS_STHREADS)
  self = ::thr_self ();
# elif defined (PDL_HAS_WTHREADS)
  self = ::GetCurrentThread ();
# elif defined (PDL_PSOS)
  t_ident ((char *) 0, 0, &self);
# elif defined (VXWORKS)
  self.id = ::taskIdSelf ();
# endif /* PDL_HAS_STHREADS */
#else
  self = 1; // Might as well make it the main thread ;-)
#endif /* PDL_HAS_THREADS */
}

PDL_INLINE PDL_thread_t
PDL_OS::thr_self (void)
{
  // PDL_TRACE ("PDL_OS::thr_self");
#if defined (PDL_HAS_THREADS)
# if defined (PDL_HAS_PTHREADS)
  // Note, don't use "::" here since the following call is often a macro.
  PDL_OSCALL_RETURN (pthread_self (), int, -1);
# elif defined (PDL_HAS_STHREADS)
  PDL_OSCALL_RETURN (::thr_self (), int, -1);
# elif defined (PDL_HAS_WTHREADS)
  return ::GetCurrentThreadId ();
# elif defined (PDL_PSOS)
  // there does not appear to be a way to get
  // a task's name other than at creation
  return 0;
# elif defined (VXWORKS)
  return ::taskName (::taskIdSelf ());
# endif /* PDL_HAS_STHREADS */
#else
  return 1; // Might as well make it the first thread ;-)
#endif /* PDL_HAS_THREADS */
}

PDL_INLINE int
PDL_OS::recursive_mutex_init (PDL_recursive_thread_mutex_t *m,
                              LPCTSTR name,
                              void *arg,
                              LPSECURITY_ATTRIBUTES sa)
{
  PDL_UNUSED_ARG (sa);
#if defined (PDL_HAS_THREADS)
#if defined (PDL_HAS_RECURSIVE_MUTEXES)
  return PDL_OS::thread_mutex_init (m, 0, name, arg);
#else
  if (PDL_OS::thread_mutex_init (&m->nesting_mutex_, 0, name, arg) == -1)
    return -1;
  else if (PDL_OS::cond_init (&m->lock_available_,
                              (short) USYNC_THREAD,
                              name,
                              arg) == -1)
    return -1;
  else
    {
      m->nesting_level_ = 0;
      m->owner_id_ = PDL_OS::NULL_thread;
      return 0;
    }
#endif /* PDL_HAS_RECURSIVE_MUTEXES */
#else
  PDL_UNUSED_ARG (m);
  PDL_UNUSED_ARG (name);
  PDL_UNUSED_ARG (arg);
  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_HAS_THREADS */
}

PDL_INLINE int
PDL_OS::recursive_mutex_destroy (PDL_recursive_thread_mutex_t *m)
{
#if defined (PDL_HAS_THREADS)
#if defined (PDL_HAS_RECURSIVE_MUTEXES)
  return PDL_OS::thread_mutex_destroy (m);
#else
  if (PDL_OS::thread_mutex_destroy (&m->nesting_mutex_) == -1)
    return -1;
  else if (PDL_OS::cond_destroy (&m->lock_available_) == -1)
    return -1;
  else
    return 0;
#endif /* PDL_HAS_RECURSIVE_MUTEXES */
#else
  PDL_UNUSED_ARG (m);
  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_HAS_THREADS */
}

PDL_INLINE int
PDL_OS::recursive_mutex_lock (PDL_recursive_thread_mutex_t *m)
{
#if defined (PDL_HAS_THREADS)
#if defined (PDL_HAS_RECURSIVE_MUTEXES)
  return PDL_OS::thread_mutex_lock (m);
#else
  PDL_thread_t t_id = PDL_OS::thr_self ();
  int result = 0;

  // Acquire the guard.
  if (PDL_OS::thread_mutex_lock (&m->nesting_mutex_) == -1)
    result = -1;
  else
    {
      // If there's no contention, just grab the lock immediately
      // (since this is the common case we'll optimize for it).
      if (m->nesting_level_ == 0)
        m->owner_id_ = t_id;
      // If we already own the lock, then increment the nesting level
      // and return.
      else if (PDL_OS::thr_equal (t_id, m->owner_id_) == 0)
        {
          // Wait until the nesting level has dropped to zero, at
          // which point we can acquire the lock.
          while (m->nesting_level_ > 0)
            PDL_OS::cond_wait (&m->lock_available_,
                               &m->nesting_mutex_);

              // At this point the nesting_mutex_ is held...
              m->owner_id_ = t_id;
        }

      // At this point, we can safely increment the nesting_level_ no
      // matter how we got here!
      m->nesting_level_++;
    }

  {
    // Save/restore errno.
    PDL_Errno_Guard error ((PDL_ERRNO_TYPE&)errno);
    PDL_OS::thread_mutex_unlock (&m->nesting_mutex_);
  }
  return result;
#endif /* PDL_HAS_RECURSIVE_MUTEXES */
#else
  PDL_UNUSED_ARG (m);
  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_HAS_THREADS */
}

PDL_INLINE int
PDL_OS::recursive_mutex_trylock (PDL_recursive_thread_mutex_t *m)
{
#if defined (PDL_HAS_THREADS)
#if defined (PDL_HAS_RECURSIVE_MUTEXES)
  return PDL_OS::thread_mutex_trylock (m);
#else
  PDL_thread_t t_id = PDL_OS::thr_self ();
  int result = 0;

  // Acquire the guard.
  if (PDL_OS::thread_mutex_lock (&m->nesting_mutex_) == -1)
    result = -1;
  else
    {
      // If there's no contention, just grab the lock immediately.
      if (m->nesting_level_ == 0)
        {
          m->owner_id_ = t_id;
          m->nesting_level_ = 1;
        }
      // If we already own the lock, then increment the nesting level
      // and proceed.
      else if (PDL_OS::thr_equal (t_id, m->owner_id_))
        m->nesting_level_++;
      else
        {
          errno = EBUSY;
          result = -1;
        }
    }

  {
    // Save/restore errno.
    PDL_Errno_Guard error ((PDL_ERRNO_TYPE&)errno);
    PDL_OS::thread_mutex_unlock (&m->nesting_mutex_);
  }
  return result;
#endif /* PDL_HAS_RECURSIVE_MUTEXES */
#else
  PDL_UNUSED_ARG (m);
  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_HAS_THREADS */
}

PDL_INLINE int
PDL_OS::recursive_mutex_unlock (PDL_recursive_thread_mutex_t *m)
{
#if defined (PDL_HAS_THREADS)
#if defined (PDL_HAS_RECURSIVE_MUTEXES)
  return PDL_OS::thread_mutex_unlock (m);
#else
PDL_TRACE ("PDL_Recursive_Thread_Mutex::release");
#if !defined (PDL_NDEBUG)
  PDL_thread_t t_id = PDL_OS::thr_self ();
#endif /* PDL_NDEBUG */
  int result = 0;

  if (PDL_OS::thread_mutex_lock (&m->nesting_mutex_) == -1)
    result = -1;
  else
    {
#if !defined (PDL_NDEBUG)
      if (m->nesting_level_ == 0
          || PDL_OS::thr_equal (t_id, m->owner_id_) == 0)
        {
          errno = EINVAL;
          result = -1;
        }
      else
#endif /* PDL_NDEBUG */
        {
          m->nesting_level_--;
          if (m->nesting_level_ == 0)
            {
              // This may not be strictly necessary, but it does put
              // the mutex into a known state...
              m->owner_id_ = PDL_OS::NULL_thread;

              // Inform waiters that the lock is free.
              if (PDL_OS::cond_signal (&m->lock_available_) == -1)
                result = -1;
            }
        }
    }

  {
    // Save/restore errno.
    PDL_Errno_Guard error ((PDL_ERRNO_TYPE&)errno);
    PDL_OS::thread_mutex_unlock (&m->nesting_mutex_);
  }
  return result;
#endif /* PDL_HAS_RECURSIVE_MUTEXES */
#else
  PDL_UNUSED_ARG (m);
  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_HAS_THREADS */
}

PDL_INLINE int
PDL_OS::sema_destroy (PDL_sema_t *s)
{
  PDL_TRACE ("PDL_OS::sema_destroy");
# if defined (PDL_HAS_POSIX_SEM)
  int result;
#   if defined (PDL_LACKS_NAMED_POSIX_SEM)
  if (s->name_)
    {
      // Only destroy the semaphore if we're the ones who
      // initialized it.
      PDL_OSCALL (::sem_destroy (s->sema_),int, -1, result);
      PDL_OS::shm_unlink (s->name_);
      PDL_OS::free(s->name_);
      return result;
    }
#else
  if (s->name_)
    {
      PDL_OSCALL (::sem_unlink (s->name_), int, -1, result);
      PDL_OS::free ((void *) s->name_);
      PDL_OSCALL_RETURN (::sem_close (s->sema_), int, -1);
    }
#   endif /*  PDL_LACKS_NAMED_POSIX_SEM */
  else
    {
      PDL_OSCALL (::sem_destroy (s->sema_), int, -1, result);
      PDL_OS::free(s->sema_);
      s->sema_ = 0;
      return result;
    }
# elif defined (PDL_HAS_THREADS)
#   if defined (PDL_HAS_STHREADS)
  PDL_OSCALL_RETURN (PDL_ADAPT_RETVAL (::sema_destroy (s), pdl_result_), int, -1);
#   elif defined (PDL_HAS_PTHREADS)
  int r1 = PDL_OS::mutex_destroy (&s->lock_);
  int r2 = PDL_OS::cond_destroy (&s->count_nonzero_);
  return r1 != 0 || r2 != 0 ? -1 : 0;
#   elif defined (PDL_HAS_WTHREADS)
#     if !defined (PDL_USES_WINCE_SEMA_SIMULATION)
  PDL_WIN32CALL_RETURN (PDL_ADAPT_RETVAL (::CloseHandle (*s), pdl_result_), int, -1);
#     else /* PDL_USES_WINCE_SEMA_SIMULATION */
  // Free up underlying objects of the simulated semaphore.
  int r1 = PDL_OS::thread_mutex_destroy (&s->lock_);
  int r2 = PDL_OS::event_destroy (&s->count_nonzero_);
  return r1 != 0 || r2 != 0 ? -1 : 0;
#     endif /* PDL_USES_WINCE_SEMA_SIMULATION */
#   elif defined (PDL_PSOS)
  int result;
  PDL_OSCALL (PDL_ADAPT_RETVAL (::sm_delete (s->sema_), result), int, -1, result);
  s->sema_ = 0;
  return result;
#   elif defined (VXWORKS)
  int result;
  PDL_OSCALL (::semDelete (s->sema_), int, -1, result);
  s->sema_ = 0;
  return result;
#   endif /* PDL_HAS_STHREADS */
# else
  PDL_UNUSED_ARG (s);
  PDL_NOTSUP_RETURN (-1);
# endif /* PDL_HAS_POSIX_SEM */
}

// NOTE: The following four function definitions must appear before
// PDL_OS::sema_init ().

PDL_INLINE int
PDL_OS::close (PDL_HANDLE handle)
{
  PDL_TRACE ("PDL_OS::close");
#if defined (PDL_WIN32)
  PDL_WIN32CALL_RETURN (PDL_ADAPT_RETVAL (::CloseHandle (handle), pdl_result_), int, -1);
#elif defined (PDL_PSOS) && ! defined (PDL_PSOS_LACKS_PHILE)
  u_long result = ::close_f (handle);
  if (result != 0)
    {
      errno = result;
      return PDL_static_cast (int, -1);
    }
  return PDL_static_cast (int, 0);
#elif defined(ITRON)
  /* empty */
  PDL_NOTSUP_RETURN (-1);
#else
  PDL_OSCALL_RETURN (::close (handle), int, -1);
#endif /* PDL_WIN32 */
}

// This function returns the number of bytes in the file referenced by
// FD.
#if defined (PDL_WIN32)
# if (_MSC_VER <= 1200)
extern "C" {
     BOOL WINAPI GetFileSizeEx( HANDLE hFile, PLARGE_INTEGER lpFileSize);
};
# endif
#endif

#if defined (PDL_WIN32)
# if (_MSC_VER <= 1200)
extern "C" {
     BOOL WINAPI GetFileSizeEx( HANDLE hFile, PLARGE_INTEGER lpFileSize);
};
# endif
#endif

PDL_INLINE PDL_OFF_T
PDL_OS::filesize (PDL_HANDLE handle)
{
  PDL_TRACE ("PDL_OS::filesize");
#if defined (PDL_WIN32) && !defined(PDL_HAS_WINCE)
  LARGE_INTEGER li;
  if(0 == GetFileSizeEx(handle, &li))
  {
      PDL_OS::set_errno_to_last_error();
      return -1;
  }
  return li.QuadPart;
#else /* !PDL_WIN32 */
  PDL_stat sb;

  return PDL_OS::fstat (handle, &sb) == -1 ? -1 : (long) sb.st_size;
#endif /* PDL_WIN32 */
}

PDL_INLINE int
PDL_OS::ftruncate (PDL_HANDLE handle, PDL_OFF_T offset)
{
  PDL_TRACE ("PDL_OS::ftruncate");
#if defined (PDL_WIN32)
  LARGE_INTEGER li;
  li.QuadPart = offset;
  if (0xFFFFFFFF == ::SetFilePointer(handle, li.LowPart, &li.HighPart, FILE_BEGIN) &&
      GetLastError() != NO_ERROR)
  {
      PDL_FAIL_RETURN (-1);
  }
  else
  {
      PDL_WIN32CALL_RETURN (PDL_ADAPT_RETVAL (::SetEndOfFile (handle), pdl_result_), int, -1);
  }
  /* NOTREACHED */
#elif defined (PDL_PSOS_LACKS_PHILE)
  PDL_UNUSED_ARG (handle);
  PDL_UNUSED_ARG (offset);
  PDL_NOTSUP_RETURN (-1);
#elif defined (PDL_PSOS)
  PDL_OSCALL_RETURN (::ftruncate_f (handle, offset), int, -1);
#elif defined(ITRON)
  /* empty */
  PDL_NOTSUP_RETURN (-1);
#else
  PDL_OSCALL_RETURN (::ftruncate (handle, offset), int, -1);
#endif /* PDL_WIN32 */
}

PDL_INLINE int
PDL_OS::fallocate ( PDL_HANDLE handle, int mode, PDL_OFF_T offset, PDL_OFF_T size )
{
  PDL_TRACE ("PDL_OS::fallocate");
# if defined (PDL_HAS_FALLOCATE)
  PDL_OSCALL_RETURN (::fallocate (handle, mode, offset, size), int, -1); 
#else
  PDL_UNUSED_ARG (handle);
  PDL_UNUSED_ARG (mode);
  PDL_UNUSED_ARG (offset);
  PDL_UNUSED_ARG (size);

  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_HAS_FALLOCATE */
}

PDL_INLINE void *
PDL_OS::mmap (void *addr,
              size_t len,
              int prot,
              int flags,
              PDL_HANDLE file_handle,
              PDL_OFF_T off,
              const ASYS_TCHAR *file_mapping_name,
              PDL_HANDLE *file_mapping,
              LPSECURITY_ATTRIBUTES sa)
{
#if defined (WIN32_ODBC_CLI2) && defined(PDL_HAS_WINCE)
	return NULL;
#else
  PDL_TRACE ("PDL_OS::mmap");
#if !defined (PDL_WIN32) || defined (PDL_HAS_PHARLAP)
  PDL_UNUSED_ARG (file_mapping_name);
#endif /* !defined (PDL_WIN32) || defined (PDL_HAS_PHARLAP) */

#if defined (PDL_WIN32) && !defined (PDL_HAS_PHARLAP) && !defined (USE_WIN32IPC_DAEMON)

#  if defined(PDL_HAS_WINCE)
  PDL_UNUSED_ARG (addr);
  if (PDL_BIT_ENABLED (flags, MAP_FIXED))     // not supported
  {
    errno = EINVAL;
    return MAP_FAILED;
  }
#  else
  if (!PDL_BIT_ENABLED (flags, MAP_FIXED))
    addr = 0;
  else if (addr == 0)   // can not map to address 0
  {
    errno = EINVAL;
    return MAP_FAILED;
  }
#  endif

  int nt_flags = 0;
  PDL_HANDLE local_handle = PDL_INVALID_HANDLE;

  /*BUGBUG_NT*/
  if (prot & PROT_READ)
    if (prot & PROT_WRITE)
       prot = PROT_RDWR;

  // Ensure that file_mapping is non-zero.
  if (file_mapping == 0)
    file_mapping = &local_handle;

  if (PDL_BIT_ENABLED (flags, MAP_PRIVATE))
    {
#  if !defined(PDL_HAS_WINCE)
      prot = PAGE_WRITECOPY;
#  endif  // PDL_HAS_WINCE
      nt_flags = FILE_MAP_COPY;
    }
  else if (PDL_BIT_ENABLED (flags, MAP_SHARED))
    {
      if (PDL_BIT_ENABLED (prot, PAGE_READONLY))
        nt_flags = FILE_MAP_READ;
      if (PDL_BIT_ENABLED (prot, PAGE_READWRITE))
        nt_flags = FILE_MAP_WRITE;
    }

  // Only create a new handle if we didn't have a valid one passed in.
  if (*file_mapping == PDL_INVALID_HANDLE)
    {
#  if !defined(PDL_HAS_WINCE) && (!defined (PDL_HAS_WINNT4) || (PDL_HAS_WINNT4 == 0))
      int try_create = 1;
      if ((file_mapping_name != 0) && (*file_mapping_name != 0))
        {
          // On Win9x, we first try to OpenFileMapping to
          // file_mapping_name. Only if there is no mapping object
          // with that name, and the desired name is valid, do we try
          // CreateFileMapping.

          *file_mapping = PDL_TEXT_OpenFileMapping (nt_flags,
                                                    0,
                                                    file_mapping_name);
          if (*file_mapping != 0
              || (::GetLastError () == ERROR_INVALID_NAME
                  && ::GetLastError () == ERROR_FILE_NOT_FOUND))
            try_create = 0;
        }

      if (try_create)
#  endif /* !PDL_HAS_WINCE && (PDL_HAS_WINNT4 || PDL_HAS_WINNT4 == 0) */
        {
          const LPSECURITY_ATTRIBUTES attr =
            PDL_OS::default_win32_security_attributes (sa);

#  if !defined (PDL_HAS_WINCE)
          *file_mapping = PDL_TEXT_CreateFileMapping (file_handle,
                                                      attr,
                                                      prot,
                                                      0,
                                                      0,
                                                      file_mapping_name);
#  else
		 *file_mapping = ::CreateFileMapping (file_handle,
                                                      attr,
                                                      prot,
                                                      0,
                                                      0,
                                                      file_mapping_name);
#  endif /* !PDL_HAS_WINCE */

        }
    }

  if (*file_mapping == 0)
    PDL_FAIL_RETURN (MAP_FAILED);

#  if defined (PDL_OS_EXTRA_MMAP_FLAGS)
  nt_flags |= PDL_OS_EXTRA_MMAP_FLAGS;
#  endif /* PDL_OS_EXTRA_MMAP_FLAGS */

#  if !defined (PDL_HAS_WINCE)

  LARGE_INTEGER li;
  li.QuadPart = off;
  void *addr_mapping = ::MapViewOfFileEx (*file_mapping,
                                          nt_flags,
                                          li.HighPart,
                                          li.LowPart,
                                          len,
                                          addr);
#  else

  LARGE_INTEGER li;
  li.QuadPart = off;
  void *addr_mapping = ::MapViewOfFile (*file_mapping,
                                        nt_flags,
                                        li.HighPart,
                                        li.LowPart,
                                        len);
#  endif /* ! PDL_HAS_WINCE */

  // Only close this down if we used the temporary.
  if (file_mapping == &local_handle)
    ::CloseHandle (*file_mapping);

  if (addr_mapping == 0)
    PDL_FAIL_RETURN (MAP_FAILED);
  else
    return addr_mapping;
#elif defined (USE_WIN32IPC_DAEMON) && !defined (WIN32_ODBC_CLI2)
  PDL_UNUSED_ARG (file_mapping_name);
  PDL_UNUSED_ARG (file_mapping);
  PDL_UNUSED_ARG (sa);
# if defined (PDL_WIN64)
  PDL_OSCALL_RETURN ((void *) ::mmap (addr,
                                      len,
                                      prot,
                                      flags,
                                      (intptr_t)file_handle,
                                      off),
                     void *, MAP_FAILED);
# else
  PDL_OSCALL_RETURN ((void *) ::mmap (addr,
                                      len,
                                      prot,
                                      flags,
                                      (int)file_handle,
                                      off),
                     void *, MAP_FAILED);
# endif /* PDL_WIN64*/
#elif defined (__Lynx__)
  // The LynxOS 2.5.0 mmap doesn't allow operations on plain
  // file descriptors.  So, create a shm object and use that.
  PDL_UNUSED_ARG (sa);

  char name [128];
  sprintf (name, "%d", file_handle);

  // Assumes that this was called by PDL_Mem_Map, so &file_mapping != 0.
  // Otherwise, we don't support the incomplete LynxOS mmap implementation.
  // We do support it by creating a hidden shared memory object, and using
  // that for the mapping.
  int shm_handle;
  if (! file_mapping)
    file_mapping = &shm_handle;
  if ((*file_mapping = ::shm_open (name,
                                   O_RDWR | O_CREAT | O_TRUNC,
                                   PDL_DEFAULT_FILE_PERMS)) == -1)
    return MAP_FAILED;
  else
    {
      // The size of the shared memory object must be explicitly set on LynxOS.
      const PDL_OFF_T filesize = PDL_OS::filesize (file_handle);
      if (::ftruncate (*file_mapping, filesize) == -1)
        return MAP_FAILED;
      else
        {
#  if defined (PDL_OS_EXTRA_MMAP_FLAGS)
          flags |= PDL_OS_EXTRA_MMAP_FLAGS;
#  endif /* PDL_OS_EXTRA_MMAP_FLAGS */
          char *map = (char *) ::mmap ((PDL_MMAP_TYPE) addr,
                                       len,
                                       prot,
                                       flags,
                                       *file_mapping,
                                       off);
          if (map == MAP_FAILED)
            return MAP_FAILED;
          else
            // Finally, copy the file contents to the shared memory object.
            return ::read (file_handle, map, (int) filesize) == filesize
              ? map
              : MAP_FAILED;
        }
    }
#elif !defined (PDL_LACKS_MMAP) && !defined (WIN32_ODBC_CLI2)
  PDL_UNUSED_ARG (sa);

#  if defined (PDL_OS_EXTRA_MMAP_FLAGS)
  flags |= PDL_OS_EXTRA_MMAP_FLAGS;
#  endif /* PDL_OS_EXTRA_MMAP_FLAGS */
  PDL_UNUSED_ARG (file_mapping);
  PDL_OSCALL_RETURN ((void *) ::mmap ((PDL_MMAP_TYPE) addr,
                                      len,
                                      prot,
                                      flags,
                                      file_handle,
                                      off),
                     void *, MAP_FAILED);
#else
  PDL_UNUSED_ARG (addr);
  PDL_UNUSED_ARG (len);
  PDL_UNUSED_ARG (prot);
  PDL_UNUSED_ARG (flags);
  PDL_UNUSED_ARG (file_handle);
  PDL_UNUSED_ARG (off);
  PDL_UNUSED_ARG (file_mapping);
  PDL_UNUSED_ARG (sa);
  PDL_NOTSUP_RETURN (MAP_FAILED);
#endif /* PDL_WIN32 && !PDL_HAS_PHARLAP */
#endif /* WIN32_ODBC_CLI2 && PDL_HAS_WINCE */
}

// NOTE: The previous four function definitions must appear before
// PDL_OS::sema_init ().

PDL_INLINE int
PDL_OS::sema_init (PDL_sema_t *s,
                   u_int count,
                   int type,
                   LPCTSTR name,
                   void *arg,
                   int max,
                   LPSECURITY_ATTRIBUTES sa)
{
  PDL_TRACE ("PDL_OS::sema_init");
# if defined (PDL_HAS_POSIX_SEM)
  PDL_UNUSED_ARG (arg);
  PDL_UNUSED_ARG (max);
  PDL_UNUSED_ARG (sa);

  s->name_ = 0;

#   if defined (PDL_LACKS_NAMED_POSIX_SEM)
  if (type == USYNC_PROCESS)
    {
      // Let's see if it already exists.
      PDL_HANDLE fd = PDL_OS::shm_open (name,
                                        O_RDWR | O_CREAT | O_EXCL,
                                        PDL_DEFAULT_FILE_PERMS);
      if (fd == PDL_INVALID_HANDLE)
        {
          if (errno == EEXIST)
            fd = PDL_OS::shm_open (name,
                                   O_RDWR | O_CREAT,
                                   PDL_DEFAULT_FILE_PERMS);
          else
            return -1;
        }
      else
        {
          // We own this shared memory object!  Let's set its
          // size.
          if (PDL_OS::ftruncate (fd,
                                 sizeof (PDL_sema_t)) == -1)
            return -1;
          s->name_ = PDL_OS::strdup (name);
          if (s->name_ == 0)
            return -1;
        }
      if (fd == -1)
        return -1;

      s->sema_ = (sem_t *)
        PDL_OS::mmap (0,
                      sizeof (PDL_sema_t),
                      PROT_RDWR,
                      MAP_SHARED,
                      fd,
                      0);
      PDL_OS::close (fd);
      if (s->sema_ == (sem_t *) MAP_FAILED)
        return -1;
      if (s->name_
          // @@ According UNIX Network Programming V2 by Stevens,
          //    sem_init() is currently not required to return zero on
          //    success, but it *does* return -1 upon failure.  For
          //    this reason, check for failure by comparing to -1,
          //    instead of checking for success by comparing to zero.
          //        -Ossama
          // Only initialize it if we're the one who created it.
          && ::sem_init (s->sema_, type == USYNC_PROCESS, count) == -1)
        return -1;
      return 0;
    }
#else
  if (name)
    {
      PDL_ALLOCATOR_RETURN (s->name_,
                            PDL_OS::strdup (name),
                            -1);
      s->sema_ = ::sem_open (s->name_,
                             O_CREAT,
                             PDL_DEFAULT_FILE_PERMS,
                             count);
      if (s->sema_ == SEM_FAILED)
        return -1;
      else
        return 0;
    }
#   endif /* PDL_LACKS_NAMED_POSIX_SEM */
  else
    {
      PDL_NEW_RETURN (s->sema_,
                      sem_t,
                      sizeof(sem_t),
                      -1);
      PDL_OSCALL_RETURN (::sem_init (s->sema_,
                                     type != USYNC_THREAD,
                                     count), int, -1);
    }
# elif defined (PDL_HAS_THREADS)
#   if defined (PDL_HAS_STHREADS)
  PDL_UNUSED_ARG (name);
  PDL_UNUSED_ARG (max);
  PDL_UNUSED_ARG (sa);
  PDL_OSCALL_RETURN (PDL_ADAPT_RETVAL (::sema_init (s, count, type, arg), pdl_result_),
                     int, -1);
#   elif defined (PDL_HAS_PTHREADS)
  PDL_UNUSED_ARG (max);
  PDL_UNUSED_ARG (sa);
  int result = -1;

  if (PDL_OS::mutex_init (&s->lock_, type, name, arg) == 0
      && PDL_OS::cond_init (&s->count_nonzero_, type, name, arg) == 0
      && PDL_OS::mutex_lock (&s->lock_) == 0)
    {
      s->count_ = count;
      s->waiters_ = 0;

      if (PDL_OS::mutex_unlock (&s->lock_) == 0)
        result = 0;
    }

  if (result == -1)
    {
      PDL_OS::mutex_destroy (&s->lock_);
      PDL_OS::cond_destroy (&s->count_nonzero_);
    }
  return result;
#   elif defined (PDL_HAS_WTHREADS)
#     if ! defined (PDL_USES_WINCE_SEMA_SIMULATION)
  PDL_UNUSED_ARG (type);
  PDL_UNUSED_ARG (arg);
  // Create the semaphore with its value initialized to <count> and
  // its maximum value initialized to <max>.
  *s = ::CreateSemaphore (PDL_OS::default_win32_security_attributes (sa), count, max, name);

  if (*s == 0)
    PDL_FAIL_RETURN (-1);
  /* NOTREACHED */
  else
    return 0;
#     else /* PDL_USES_WINCE_SEMA_SIMULATION */
  int result = -1;

  // Initialize internal object for semaphore simulation.
  // Grab the lock as soon as possible when we initializing
  // the semaphore count.  Notice that we initialize the
  // event object as "manually reset" so we can amortize the
  // cost for singling/reseting the event.
  // @@ I changed the mutex type to thread_mutex.  Notice that this
  // is basically a CriticalSection object and doesn't not has
  // any security attribute whatsoever.  However, since this
  // semaphore implementation only works within a process, there
  // shouldn't any security issue at all.
  if (PDL_OS::thread_mutex_init (&s->lock_, type, name, arg) == 0
      && PDL_OS::event_init (&s->count_nonzero_, 1,
                             count > 0, type, name, arg, sa) == 0
      && PDL_OS::thread_mutex_lock (&s->lock_) == 0)
    {
      s->count_ = count;

      if (PDL_OS::thread_mutex_unlock (&s->lock_) == 0)
        result = 0;
    }

  // Destroy the internal objects if we didn't initialize
  // either of them successfully.  Don't bother to check
  // for errors.
  if (result == -1)
    {
      PDL_OS::thread_mutex_destroy (&s->lock_);
      PDL_OS::event_destroy (&s->count_nonzero_);
    }
  return result;
#     endif /* PDL_USES_WINCE_SEMA_SIMULATION */
#   elif defined (PDL_PSOS)
  u_long result;
  PDL_OS::memcpy (s->name_, name, sizeof (s->name_));
  // default semaphore creation flags to priority based, global across nodes
  u_long flags = 0;
  flags |= (type & SM_LOCAL) ? SM_LOCAL : SM_GLOBAL;
  flags |= (type & SM_FIFO) ? SM_FIFO : SM_PRIOR;
  result = ::sm_create (s->name_, count, flags, &(s->sema_));
  return (result == 0) ? 0 : -1;
#   elif defined (VXWORKS)
  PDL_UNUSED_ARG (name);
  PDL_UNUSED_ARG (arg);
  PDL_UNUSED_ARG (max);
  PDL_UNUSED_ARG (sa);
  s->name_ = 0;
  s->sema_ = ::semCCreate (type, count);
  return s->sema_ ? 0 : -1;
#   endif /* PDL_HAS_STHREADS */
# else
  PDL_UNUSED_ARG (s);
  PDL_UNUSED_ARG (count);
  PDL_UNUSED_ARG (type);
  PDL_UNUSED_ARG (name);
  PDL_UNUSED_ARG (arg);
  PDL_UNUSED_ARG (max);
  PDL_UNUSED_ARG (sa);
  PDL_NOTSUP_RETURN (-1);
# endif /* PDL_HAS_POSIX_SEM */
}

PDL_INLINE int
PDL_OS::sema_post (PDL_sema_t *s)
{
  PDL_TRACE ("PDL_OS::sema_post");
# if defined (PDL_HAS_POSIX_SEM)
  PDL_OSCALL_RETURN (::sem_post (s->sema_), int, -1);
# elif defined (PDL_HAS_THREADS)
#   if defined (PDL_HAS_STHREADS)
  PDL_OSCALL_RETURN (PDL_ADAPT_RETVAL (::sema_post (s), pdl_result_), int, -1);
#   elif defined (PDL_HAS_PTHREADS)
  int result = -1;

  if (PDL_OS::mutex_lock (&s->lock_) == 0)
    {
      // Always allow a waiter to continue if there is one.
      if (s->waiters_ > 0)
        result = PDL_OS::cond_signal (&s->count_nonzero_);
      else
        result = 0;

      s->count_++;
      PDL_OS::mutex_unlock (&s->lock_);
    }
  return result;
#   elif defined (PDL_HAS_WTHREADS)
#     if !defined (PDL_USES_WINCE_SEMA_SIMULATION)
  PDL_WIN32CALL_RETURN (PDL_ADAPT_RETVAL (::ReleaseSemaphore (*s, 1, 0),
                                          pdl_result_),
                        int, -1);
#     else /* PDL_USES_WINCE_SEMA_SIMULATION */
  int result = -1;

  // Since we are simulating semaphores, we need to update semaphore
  // count manually.  Grab the lock to prevent rpdl condition first.
  if (PDL_OS::thread_mutex_lock (&s->lock_) == 0)
    {
      // Check the original state of event object.  Single the event
      // object in transition from semaphore not available to
      // semaphore available.
      if (s->count_++ <= 0)
        result = PDL_OS::event_signal (&s->count_nonzero_);
      else
        result = 0;

      PDL_OS::thread_mutex_unlock (&s->lock_);
    }
  return result;
#     endif /* PDL_USES_WINCE_SEMA_SIMULATION */
#   elif defined (PDL_PSOS)
  int result;
  PDL_OSCALL (PDL_ADAPT_RETVAL (::sm_v (s->sema_), result), int, -1, result);
  return result;
#   elif defined (VXWORKS)
  PDL_OSCALL_RETURN (::semGive (s->sema_), int, -1);
#   endif /* PDL_HAS_STHREADS */
# else
  PDL_UNUSED_ARG (s);
  PDL_NOTSUP_RETURN (-1);
# endif /* PDL_HAS_POSIX_SEM */
}

PDL_INLINE int
PDL_OS::sema_post (PDL_sema_t *s, size_t release_count)
{
#if defined (PDL_WIN32) && !defined (PDL_USES_WINCE_SEMA_SIMULATION)
  // Win32 supports this natively.
  PDL_WIN32CALL_RETURN (PDL_ADAPT_RETVAL (::ReleaseSemaphore (*s, release_count, 0),
                                          pdl_result_), int, -1);
#else
  // On POSIX platforms we need to emulate this ourselves.
  // @@ We can optimize on this implementation.  However,
  // the semaphore promitive on Win32 doesn't allow one
  // to increase a semaphore to more than the count it was
  // first initialized.  Posix and solaris don't seem to have
  // this restriction.  Should we impose the restriction in
  // our semaphore simulation?
  for (size_t i = 0; i < release_count; i++)
    if (PDL_OS::sema_post (s) == -1)
      return -1;

  return 0;
#endif /* PDL_WIN32 */
}

PDL_INLINE int
PDL_OS::sema_trywait (PDL_sema_t *s)
{
  PDL_TRACE ("PDL_OS::sema_trywait");
# if defined (PDL_HAS_POSIX_SEM)
  // POSIX semaphores set errno to EAGAIN if trywait fails
  PDL_OSCALL_RETURN (::sem_trywait (s->sema_), int, -1);
# elif defined (PDL_HAS_THREADS)
#   if defined (PDL_HAS_STHREADS)
  // STHREADS semaphores set errno to EBUSY if trywait fails.
  PDL_OSCALL_RETURN (PDL_ADAPT_RETVAL (::sema_trywait (s),
                                       pdl_result_),
                     int, -1);
#   elif defined (PDL_HAS_PTHREADS)

  int result = -1;

  if (PDL_OS::mutex_lock (&s->lock_) == 0)
    {
      if (s->count_ > 0)
        {
          --s->count_;
          result = 0;
        }
      else
        errno = EBUSY;

      PDL_OS::mutex_unlock (&s->lock_);
    }
  return result;
#   elif defined (PDL_HAS_WTHREADS)
#     if !defined (PDL_USES_WINCE_SEMA_SIMULATION)
  int result = ::WaitForSingleObject (*s, 0);

  if (result == WAIT_OBJECT_0)
    return 0;
  else
    {
      if (result == WAIT_TIMEOUT)
        errno = EBUSY;
      else
        PDL_OS::set_errno_to_last_error ();
      // This is a hack, we need to find an appropriate mapping...
      return -1;
    }
#     else /* PDL_USES_WINCE_SEMA_SIMULATION */
  // Check the status of semaphore first.  Return immediately
  // if the semaphore is not available and avoid grabing the
  // lock.
  int result = ::WaitForSingleObject (s->count_nonzero_, 0);

  if (result == WAIT_OBJECT_0)  // Proceed when it is available.
    {
      PDL_OS::thread_mutex_lock (&s->lock_);

      // Need to double check if the semaphore is still available.
      // The double checking scheme will slightly affect the
      // efficiency if most of the time semaphores are not blocked.
      result = ::WaitForSingleObject (s->count_nonzero_, 0);
      if (result == WAIT_OBJECT_0)
        {
          // Adjust the semaphore count.  Only update the event
          // object status when the state changed.
          s->count_--;
          if (s->count_ <= 0)
            PDL_OS::event_reset (&s->count_nonzero_);
          result = 0;
        }

      PDL_OS::thread_mutex_unlock (&s->lock_);
    }

  // Translate error message to errno used by PDL.
  if (result == WAIT_TIMEOUT)
    errno = EBUSY;
  else
    PDL_OS::set_errno_to_last_error ();
  // This is taken from the hack above. ;)
  return -1;
#     endif /* PDL_USES_WINCE_SEMA_SIMULATION */
#   elif defined (PDL_PSOS)
   switch (::sm_p (s->sema_, SM_NOWAIT, 0))
   {
     case 0:
       return 0;
     case ERR_NOSEM:
       errno = EBUSY;
       // intentional fall through
     default:
       return -1;
   }
#   elif defined (VXWORKS)
  if (::semTake (s->sema_, NO_WAIT) == ERROR)
    if (errno == S_objLib_OBJ_UNAVAILABLE)
      {
        // couldn't get the semaphore
        errno = EBUSY;
        return -1;
      }
    else
      // error
      return -1;
  else
    // got the semaphore
    return 0;
#   endif /* PDL_HAS_STHREADS */
# else
  PDL_UNUSED_ARG (s);
  PDL_NOTSUP_RETURN (-1);
# endif /* PDL_HAS_POSIX_SEM */
}

PDL_INLINE int
PDL_OS::sema_wait (PDL_sema_t *s)
{
  PDL_TRACE ("PDL_OS::sema_wait");
# if defined (PDL_HAS_POSIX_SEM)
  PDL_OSCALL_RETURN (::sem_wait (s->sema_), int, -1);
# elif defined (PDL_HAS_THREADS)
#   if defined (PDL_HAS_STHREADS)
  PDL_OSCALL_RETURN (PDL_ADAPT_RETVAL (::sema_wait (s), pdl_result_), int, -1);
#   elif defined (PDL_HAS_PTHREADS)
  int result = 0;

  if (PDL_OS::mutex_lock (&s->lock_) != 0)
    result = -1;
  else
    {
      // Keep track of the number of waiters so that we can signal
      // them properly in <PDL_OS::sema_post>.
      s->waiters_++;

      // Wait until the semaphore count is > 0.
      while (s->count_ == 0)
        if (PDL_OS::cond_wait (&s->count_nonzero_,
                               &s->lock_) == -1)
          {
            result = -2; // -2 means that we need to release the mutex.
            break;
          }

      --s->waiters_;
    }

  if (result == 0)
    --s->count_;

  if (result != -1)
    PDL_OS::mutex_unlock (&s->lock_);

  return result < 0 ? -1 : result;

#   elif defined (PDL_HAS_WTHREADS)
#     if !defined (PDL_USES_WINCE_SEMA_SIMULATION)
  switch (::WaitForSingleObject (*s, INFINITE))
    {
    case WAIT_OBJECT_0:
      return 0;
    case WAIT_ABANDONED:
    case WAIT_TIMEOUT:
    case WAIT_FAILED:
    default:
      // This is a hack, we need to find an appropriate mapping...
      PDL_OS::set_errno_to_last_error ();
      return -1;
    }
  /* NOTREACHED */
#     else /* PDL_USES_WINCE_SEMA_SIMULATION */
  // Timed wait.
  int result = -1;
  for (;;)
    // Check if the semaphore is avialable or not and wait forever.
    // Don't bother to grab the lock if it is not available (to avoid
    // deadlock.)
    switch (::WaitForSingleObject (s->count_nonzero_, INFINITE))
      {
      case WAIT_OBJECT_0:
        PDL_OS::thread_mutex_lock (&s->lock_);

        // Need to double check if the semaphore is still available.
        // This time, we shouldn't wait at all.
        if (::WaitForSingleObject (s->count_nonzero_, 0) == WAIT_OBJECT_0)
          {
            // Decrease the internal counter.  Only update the event
            // object's status when the state changed.
            s->count_--;
            if (s->count_ <= 0)
              PDL_OS::event_reset (&s->count_nonzero_);
            result = 0;
          }

        PDL_OS::thread_mutex_unlock (&s->lock_);
        // if we didn't get a hold on the semaphore, the result won't
        // be 0 and thus, we'll start from the beginning again.
        if (result == 0)
          return 0;
        break;

      default:
        // Since we wait indefinitely, anything other than
        // WAIT_OBJECT_O indicates an error.
        PDL_OS::set_errno_to_last_error ();
        // This is taken from the hack above. ;)
        return -1;
      }
  /* NOTREACHED */
#     endif /* PDL_USES_WINCE_SEMA_SIMULATION */
#   elif defined (PDL_PSOS)
  int result;
  PDL_OSCALL (PDL_ADAPT_RETVAL (::sm_p (s->sema_, SM_WAIT, 0), result),
                                int, -1, result);
  return result;
#   elif defined (VXWORKS)
  PDL_OSCALL_RETURN (::semTake (s->sema_, WAIT_FOREVER), int, -1);
#   endif /* PDL_HAS_STHREADS */
# else
  PDL_UNUSED_ARG (s);
  PDL_NOTSUP_RETURN (-1);
# endif /* PDL_HAS_POSIX_SEM */
}

PDL_INLINE int
PDL_OS::sema_wait (PDL_sema_t *s, PDL_Time_Value &tv)
{
  PDL_TRACE ("PDL_OS::sema_wait");
# if defined (PDL_HAS_POSIX_SEM)
  PDL_UNUSED_ARG (s);
  PDL_UNUSED_ARG (tv);
  PDL_NOTSUP_RETURN (-1);
# elif defined (PDL_HAS_THREADS)
#   if defined (PDL_HAS_STHREADS)
  PDL_UNUSED_ARG (s);
  PDL_UNUSED_ARG (tv);
  PDL_NOTSUP_RETURN (-1);
#   elif defined (PDL_HAS_PTHREADS)
  int result = 0;
  PDL_Errno_Guard error ((PDL_ERRNO_TYPE&)errno, 0);

  if (PDL_OS::mutex_lock (&s->lock_) != 0)
    result = -1;
  else
    {
      // Keep track of the number of waiters so that we can signal
      // them properly in <PDL_OS::sema_post>.
      s->waiters_++;

      // Wait until the semaphore count is > 0 or until we time out.
      while (s->count_ == 0)
        if (PDL_OS::cond_timedwait (&s->count_nonzero_,
                                    &s->lock_,
                                    &tv) == -1)
          {
            error = errno;
            result = -2; // -2 means that we need to release the mutex.
            break;
          }

      --s->waiters_;
    }

  if (result == 0)
    {
#     if defined (PDL_LACKS_COND_TIMEDWAIT_RESET)
      tv = PDL_OS::gettimeofday ();
#     endif /* PDL_LACKS_COND_TIMEDWAIT_RESET */
      --s->count_;
    }

  if (result != -1)
    PDL_OS::mutex_unlock (&s->lock_);

  return result < 0 ? -1 : result;
#   elif defined (PDL_HAS_WTHREADS)
#     if !defined (PDL_USES_WINCE_SEMA_SIMULATION)
  int msec_timeout;

  if (tv.sec () == 0 && tv.usec () == 0)
    msec_timeout = 0; // Do a "poll."
  else
    {
      // Note that we must convert between absolute time (which is
      // passed as a parameter) and relative time (which is what
      // <WaitForSingleObjects> expects).
      PDL_Time_Value relative_time;
      relative_time = tv - PDL_OS::gettimeofday ();

      // Watchout for situations where a context switch has caused the
      // current time to be > the timeout.
      PDL_Time_Value PDL_Time_Value_zero;
      PDL_Time_Value_zero.initialize();
      if (relative_time < PDL_Time_Value_zero)
        msec_timeout = 0;
      else
        msec_timeout = relative_time.msec ();
    }

  switch (::WaitForSingleObject (*s, msec_timeout))
    {
    case WAIT_OBJECT_0:
      tv = PDL_OS::gettimeofday ();     // Update time to when acquired
      return 0;
    case WAIT_TIMEOUT:
      errno = ETIME;
      return -1;
    default:
      // This is a hack, we need to find an appropriate mapping...
      PDL_OS::set_errno_to_last_error ();
      return -1;
    }
  /* NOTREACHED */
#     else /* PDL_USES_WINCE_SEMA_SIMULATION */
  // Note that in this mode, the acquire is done in two steps, and
  // we may get signaled but cannot grab the semaphore before
  // timeout.  In that case, we'll need to restart the process with
  // updated timeout value.

  // <tv> is an absolute time
  PDL_Time_Value relative_time = tv - PDL_OS::gettimeofday ();
  int result = -1;

  // While we are not timeout yet.
  PDL_Time_Value PDL_Time_Value_zero;
  PDL_Time_Value_zero.initialize();
  while (relative_time > PDL_Time_Value_zero)
    {
      // Wait for our turn to get the object.
      switch (::WaitForSingleObject (s->count_nonzero_, relative_time.msec ()))
        {
        case WAIT_OBJECT_0:
          PDL_OS::thread_mutex_lock (&s->lock_);

          // Need to double check if the semaphore is still available.
          // We can only do a "try lock" styled wait here to avoid
          // blocking threads that want to signal the semaphore.
          if (::WaitForSingleObject (s->count_nonzero_, 0) == WAIT_OBJECT_0)
            {
              // As before, only reset the object when the semaphore
              // is no longer available.
              s->count_--;
              if (s->count_ <= 0)
                PDL_OS::event_reset (&s->count_nonzero_);
              result = 0;
            }

          PDL_OS::thread_mutex_unlock (&s->lock_);

          // Only return when we successfully get the semaphore.
          if (result == 0)
            {
              tv = PDL_OS::gettimeofday ();     // Update to time acquired
              return 0;
            }
          break;

          // We have timed out.
        case WAIT_TIMEOUT:
          errno = ETIME;
          return -1;

          // What?
        default:
          PDL_OS::set_errno_to_last_error ();
          // This is taken from the hack above. ;)
          return -1;
        };

      // Haven't been able to get the semaphore yet, update the
      // timeout value to reflect the remaining time we want to wait.
      relative_time = tv - PDL_OS::gettimeofday ();
    }

  // We have timed out.
  errno = ETIME;
  return -1;
#     endif /* PDL_USES_WINCE_SEMA_SIMULATION */
#   elif defined (PDL_PSOS)
  // Note that we must convert between absolute time (which is
  // passed as a parameter) and relative time (which is what
  // the system call expects).
  PDL_Time_Value relative_time;
  relative_time.initialize(tv - PDL_OS::gettimeofday ());

  u_long ticks = relative_time.sec() * KC_TICKS2SEC +
                 relative_time.usec () * KC_TICKS2SEC /
                   PDL_ONE_SECOND_IN_USECS;
  if(ticks == 0)
    PDL_OSCALL_RETURN (::sm_p (s->sema_, SM_NOWAIT, 0), int, -1); //no timeout
  else
    PDL_OSCALL_RETURN (::sm_p (s->sema_, SM_WAIT, ticks), int, -1);
#   elif defined (VXWORKS)
  // Note that we must convert between absolute time (which is
  // passed as a parameter) and relative time (which is what
  // the system call expects).
  PDL_Time_Value relative_time;
  PDL_Time_Value a = tv - PDL_OS::gettimeofday ();
  relative_time.initialize(a.sec(), a.usec());

  int ticks_per_sec = ::sysClkRateGet ();

  int ticks = relative_time.sec() * ticks_per_sec +
              relative_time.usec () * ticks_per_sec / PDL_ONE_SECOND_IN_USECS;
  if (::semTake (s->sema_, ticks) == ERROR)
    {
      if (errno == S_objLib_OBJ_TIMEOUT || errno == S_objLib_OBJ_UNAVAILABLE )
        // Convert the VxWorks errno to one that's common for to PDL
        // platforms.
        errno = ETIME;
      return -1;
    }
  else
    {
      tv = PDL_OS::gettimeofday ();  // Update to time acquired
      return 0;
    }
#   endif /* PDL_HAS_STHREADS */
# else
  PDL_UNUSED_ARG (s);
  PDL_UNUSED_ARG (tv);
  PDL_NOTSUP_RETURN (-1);
# endif /* PDL_HAS_POSIX_SEM */
}


PDL_INLINE int
PDL_OS::rw_tryrdlock (PDL_rwlock_t *rw)
{
  PDL_TRACE ("PDL_OS::rw_tryrdlock");
#if defined (PDL_HAS_THREADS)
# if !defined (PDL_LACKS_RWLOCK_T) || defined (PDL_HAS_PTHREADS_UNIX98_EXT)
#  if defined (PDL_HAS_PTHREADS_UNIX98_EXT)
  PDL_OSCALL_RETURN (PDL_ADAPT_RETVAL (pthread_rwlock_tryrdlock (rw),
                                       pdl_result_),
                     int, -1);
#  else /* Solaris */
  PDL_OSCALL_RETURN (PDL_ADAPT_RETVAL (::rw_tryrdlock (rw), pdl_result_), int, -1);
#  endif /* PDL_HAS_PTHREADS_UNIX98_EXT */
# else /* NT, POSIX, and VxWorks don't support this natively. */
  int result = -1;

  if (PDL_OS::mutex_lock (&rw->lock_) != -1)
    {
      PDL_Errno_Guard error ((PDL_ERRNO_TYPE&)errno, 0);

      if (rw->ref_count_ == -1 || rw->num_waiting_writers_ > 0)
        {
          error = EBUSY;
          result = -1;
        }
      else
        {
          rw->ref_count_++;
          result = 0;
        }

      PDL_OS::mutex_unlock (&rw->lock_);
    }
  return result;
# endif /* ! PDL_LACKS_RWLOCK_T */
#else
  PDL_UNUSED_ARG (rw);
  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_HAS_THREADS */
}

PDL_INLINE int
PDL_OS::rw_trywrlock (PDL_rwlock_t *rw)
{
  PDL_TRACE ("PDL_OS::rw_trywrlock");
#if defined (PDL_HAS_THREADS)
# if !defined (PDL_LACKS_RWLOCK_T) || defined (PDL_HAS_PTHREADS_UNIX98_EXT)
#  if defined (PDL_HAS_PTHREADS_UNIX98_EXT)
  PDL_OSCALL_RETURN (PDL_ADAPT_RETVAL (pthread_rwlock_trywrlock (rw),
                                       pdl_result_),
                     int, -1);
#  else /* Solaris */
  PDL_OSCALL_RETURN (PDL_ADAPT_RETVAL (::rw_trywrlock (rw), pdl_result_), int, -1);
#  endif /* PDL_HAS_PTHREADS_UNIX98_EXT */
# else /* NT, POSIX, and VxWorks don't support this natively. */
  int result = -1;

  if (PDL_OS::mutex_lock (&rw->lock_) != -1)
    {
      PDL_Errno_Guard error ((PDL_ERRNO_TYPE&)errno, 0);

      if (rw->ref_count_ != 0)
        {
          error = EBUSY;
          result = -1;
        }
      else
        {
          rw->ref_count_ = -1;
          result = 0;
        }

      PDL_OS::mutex_unlock (&rw->lock_);
    }
  return result;
# endif /* ! PDL_LACKS_RWLOCK_T */
#else
  PDL_UNUSED_ARG (rw);
  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_HAS_THREADS */
}

PDL_INLINE int
PDL_OS::rw_rdlock (PDL_rwlock_t *rw)
{
  PDL_TRACE ("PDL_OS::rw_rdlock");
#if defined (PDL_HAS_THREADS)
# if !defined (PDL_LACKS_RWLOCK_T) || defined (PDL_HAS_PTHREADS_UNIX98_EXT)
#  if defined (PDL_HAS_PTHREADS_UNIX98_EXT)
  PDL_OSCALL_RETURN (PDL_ADAPT_RETVAL (pthread_rwlock_rdlock (rw),
                                       pdl_result_),
                     int, -1);
#  else /* Solaris */
  PDL_OSCALL_RETURN (PDL_ADAPT_RETVAL (::rw_rdlock (rw), pdl_result_), int, -1);
#  endif /* PDL_HAS_PTHREADS_UNIX98_EXT */
# else /* NT, POSIX, and VxWorks don't support this natively. */

  int result = 0;
  if (PDL_OS::mutex_lock (&rw->lock_) == -1)
    result = -1; // -1 means didn't get the mutex.
  else
    {
      // Give preference to writers who are waiting.
      while (rw->ref_count_ < 0 || rw->num_waiting_writers_ > 0)
        {
          rw->num_waiting_readers_++;
          if (PDL_OS::cond_wait (&rw->waiting_readers_, &rw->lock_) == -1)
            {
              result = -2; // -2 means that we need to release the mutex.
              break;
            }
          rw->num_waiting_readers_--;
        }
    }
  if (result == 0)
    rw->ref_count_++;
  if (result != -1)
    PDL_OS::mutex_unlock (&rw->lock_);

  return 0;
# endif /* ! PDL_LACKS_RWLOCK_T */
#else
  PDL_UNUSED_ARG (rw);
  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_HAS_THREADS */
}

PDL_INLINE int
PDL_OS::rw_wrlock (PDL_rwlock_t *rw)
{
  PDL_TRACE ("PDL_OS::rw_wrlock");
#if defined (PDL_HAS_THREADS)
# if !defined (PDL_LACKS_RWLOCK_T) || defined (PDL_HAS_PTHREADS_UNIX98_EXT)
#  if defined (PDL_HAS_PTHREADS_UNIX98_EXT)
  PDL_OSCALL_RETURN (PDL_ADAPT_RETVAL (pthread_rwlock_wrlock (rw),
                                       pdl_result_),
                     int, -1);
#  else /* Solaris */
  PDL_OSCALL_RETURN (PDL_ADAPT_RETVAL (::rw_wrlock (rw), pdl_result_), int, -1);
#  endif /* PDL_HAS_PTHREADS_UNIX98_EXT */
# else /* NT, POSIX, and VxWorks don't support this natively. */

  int result = 0;

  if (PDL_OS::mutex_lock (&rw->lock_) == -1)
    result = -1; // -1 means didn't get the mutex.
  else
    {
      while (rw->ref_count_ != 0)
        {
          rw->num_waiting_writers_++;

          if (PDL_OS::cond_wait (&rw->waiting_writers_, &rw->lock_) == -1)
            {
              result = -2; // -2 means we need to release the mutex.
              break;
            }

          rw->num_waiting_writers_--;
        }
    }
  if (result == 0)
    rw->ref_count_ = -1;
  if (result != -1)
    PDL_OS::mutex_unlock (&rw->lock_);

  return 0;
# endif /* ! PDL_LACKS_RWLOCK_T */
#else
  PDL_UNUSED_ARG (rw);
  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_HAS_THREADS */
}

PDL_INLINE int
PDL_OS::rw_unlock (PDL_rwlock_t *rw)
{
  PDL_TRACE ("PDL_OS::rw_unlock");
#if defined (PDL_HAS_THREADS)
# if !defined (PDL_LACKS_RWLOCK_T) || defined (PDL_HAS_PTHREADS_UNIX98_EXT)
#  if defined (PDL_HAS_PTHREADS_UNIX98_EXT)
  PDL_OSCALL_RETURN (PDL_ADAPT_RETVAL (pthread_rwlock_unlock (rw),
                                       pdl_result_),
                     int, -1);
#  else /* Solaris */
  PDL_OSCALL_RETURN (PDL_ADAPT_RETVAL (::rw_unlock (rw), pdl_result_), int, -1);
#  endif /* PDL_HAS_PTHREADS_UNIX98_EXT */
# else /* NT, POSIX, and VxWorks don't support this natively. */
  if (PDL_OS::mutex_lock (&rw->lock_) == -1)
    return -1;

  if (rw->ref_count_ > 0) // Releasing a reader.
    rw->ref_count_--;
  else if (rw->ref_count_ == -1) // Releasing a writer.
    rw->ref_count_ = 0;
  else
    return -1; // @@ PDL_ASSERT (!"count should not be 0!\n");


  int result = 0;
  PDL_Errno_Guard error ((PDL_ERRNO_TYPE&)errno, 0);

  if (rw->important_writer_ && rw->ref_count_ == 1)
    // only the reader requesting to upgrade its lock is left over.
    {
      result = PDL_OS::cond_signal (&rw->waiting_important_writer_);
      error = errno;
    }
  else if (rw->num_waiting_writers_ > 0 && rw->ref_count_ == 0)
    // give preference to writers over readers...
    {
      result = PDL_OS::cond_signal (&rw->waiting_writers_);
      error =  errno;
    }
  else if (rw->num_waiting_readers_ > 0 && rw->num_waiting_writers_ == 0)
    {
      result = PDL_OS::cond_broadcast (&rw->waiting_readers_);
      error = errno;
    }

  PDL_OS::mutex_unlock (&rw->lock_);
  return result;
# endif /* ! pdl_lacks_rwlock_t */
#else
  PDL_UNUSED_ARG (rw);
  PDL_NOTSUP_RETURN (-1);
#endif /* pdl_has_threads */
}

// Note that the caller of this method *must* already possess this
// lock as a read lock.
// return {-1 and no errno set means: error,
//         -1 and errno==EBUSY set means: could not upgrade,
//         0 means: upgraded successfully}


PDL_INLINE int
PDL_OS::rw_trywrlock_upgrade (PDL_rwlock_t *rw)
{
  PDL_TRACE ("PDL_OS::rw_wrlock");
#if defined (PDL_HAS_THREADS)
# if !defined (PDL_LACKS_RWLOCK_T)
  // Some native rwlocks, such as those on Solaris and HP-UX 11, don't
  // support the upgrade feature . . .
  PDL_UNUSED_ARG (rw);
  PDL_NOTSUP_RETURN (-1);
# else /* NT, POSIX, and VxWorks don't support this natively. */
  // The PDL rwlock emulation does support upgrade . . .
  int result = 0;

  if (PDL_OS::mutex_lock (&rw->lock_) == -1)
    return -1;
    // -1 means didn't get the mutex, error
  else if (rw->important_writer_)
    // an other reader upgrades already
    {
      result = -1;
      errno = EBUSY;
    }
  else
    {
      while (rw->ref_count_ > 1) // wait until only I am left
        {
          rw->num_waiting_writers_++; // prohibit any more readers
          rw->important_writer_ = 1;

          if (PDL_OS::cond_wait (&rw->waiting_important_writer_, &rw->lock_) == -1)
            {
              result = -1;
              // we know that we have the lock again, we have this guarantee,
              // but something went wrong
            }
          rw->important_writer_ = 0;
          rw->num_waiting_writers_--;
        }
      if (result == 0)
        {
          // nothing bad happend
          rw->ref_count_ = -1;
          // now I am a writer
          // everything is O.K.
        }
    }

  PDL_OS::mutex_unlock (&rw->lock_);

  return result;

# endif /* ! PDL_LACKS_RWLOCK_T */
#else
  PDL_UNUSED_ARG (rw);
  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_HAS_THREADS */
}

#if defined (PDL_HAS_THREADS) && (!defined (PDL_LACKS_RWLOCK_T) || \
                                   defined (PDL_HAS_PTHREADS_UNIX98_EXT))
PDL_INLINE int
PDL_OS::rwlock_init (PDL_rwlock_t *rw,
                     int type,
                     LPCTSTR name,
                     void *arg)
{
  // PDL_TRACE ("PDL_OS::rwlock_init");
#  if defined (PDL_HAS_PTHREADS_UNIX98_EXT)
  PDL_UNUSED_ARG (name);
  PDL_UNUSED_ARG (arg);

  int status;
  pthread_rwlockattr_t attr;
  pthread_rwlockattr_init (&attr);
  pthread_rwlockattr_setpshared (&attr, (type == USYNC_THREAD ?
                                         PTHREAD_PROCESS_PRIVATE :
                                         PTHREAD_PROCESS_SHARED));
  status = PDL_ADAPT_RETVAL (pthread_rwlock_init (rw, &attr), status);
  pthread_rwlockattr_destroy (&attr);

  return status;

#  else
  type = type;
  name = name;
  PDL_OSCALL_RETURN (PDL_ADAPT_RETVAL (::rwlock_init (rw, type, arg), pdl_result_), int, -1);
#  endif /* PDL_HAS_PTHREADS_UNIX98_EXT */
}
#endif /* PDL_HAS THREADS && !defined (PDL_LACKS_RWLOCK_T) */

PDL_INLINE int
PDL_OS::rwlock_destroy (PDL_rwlock_t *rw)
{
  PDL_TRACE ("PDL_OS::rwlock_destroy");
#if defined (PDL_HAS_THREADS)
# if !defined (PDL_LACKS_RWLOCK_T) || defined (PDL_HAS_PTHREADS_UNIX98_EXT)
#  if defined (PDL_HAS_PTHREADS_UNIX98_EXT)
  PDL_OSCALL_RETURN (PDL_ADAPT_RETVAL (pthread_rwlock_destroy (rw),
                                       pdl_result_),
                     int, -1);
#  else /* Solaris */
  PDL_OSCALL_RETURN (PDL_ADAPT_RETVAL (::rwlock_destroy (rw), pdl_result_), int, -1);
#  endif /* PDL_HAS_PTHREADS_UNIX98_EXT */
# else /* NT, POSIX, and VxWorks don't support this natively. */
  PDL_OS::mutex_destroy (&rw->lock_);
  PDL_OS::cond_destroy (&rw->waiting_readers_);
  PDL_OS::cond_destroy (&rw->waiting_important_writer_);
  return PDL_OS::cond_destroy (&rw->waiting_writers_);
# endif /* PDL_HAS_STHREADS && !defined (PDL_LACKS_RWLOCK_T) */
#else
  PDL_UNUSED_ARG (rw);
  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_HAS_THREADS */
}

PDL_INLINE int
PDL_OS::event_init (PDL_event_t *event,
                    int manual_reset,
                    int initial_state,
                    int type,
                    LPCTSTR name,
                    void *arg,
                    LPSECURITY_ATTRIBUTES sa)
{
#if defined (PDL_WIN32)
  PDL_UNUSED_ARG (type);
  PDL_UNUSED_ARG (arg);
  *event = ::CreateEvent (PDL_OS::default_win32_security_attributes(sa),
                          manual_reset,
                          initial_state,
                          name);
  if (*event == NULL)
    PDL_FAIL_RETURN (-1);
  else
    return 0;
#elif defined (PDL_HAS_THREADS)
  PDL_UNUSED_ARG (sa);
  event->manual_reset_ = manual_reset;
  event->is_signaled_ = initial_state;
  event->waiting_threads_ = 0;

  int result = PDL_OS::cond_init (&event->condition_,
                                  type,
                                  name,
                                  arg);
  if (result == 0)
    result = PDL_OS::mutex_init (&event->lock_,
                                 type,
                                 name,
                                 arg);
  return result;
#else
  PDL_UNUSED_ARG (event);
  PDL_UNUSED_ARG (manual_reset);
  PDL_UNUSED_ARG (initial_state);
  PDL_UNUSED_ARG (type);
  PDL_UNUSED_ARG (name);
  PDL_UNUSED_ARG (arg);
  PDL_UNUSED_ARG (sa);
  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_WIN32 */
}

PDL_INLINE int
PDL_OS::event_destroy (PDL_event_t *event)
{
#if defined (PDL_WIN32)
#if !defined (PDL_HAS_WINCE)
  PDL_WIN32CALL_RETURN (PDL_ADAPT_RETVAL (::CloseHandle (*event), pdl_result_), int, -1);
#else
  BOOL sResult = ::CloseHandle(*event);
  if (sResult != 0)
    return 0;
  else
    {
      errno = GetLastError();
      return -1;
    }
#endif /*PDL_HAS_WINCE */
#elif defined (PDL_HAS_THREADS)
  int r1 = PDL_OS::mutex_destroy (&event->lock_);
  int r2 = PDL_OS::cond_destroy (&event->condition_);
  return r1 != 0 || r2 != 0 ? -1 : 0;
#else
  PDL_UNUSED_ARG (event);
  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_WIN32 */
}

PDL_INLINE int
PDL_OS::event_wait (PDL_event_t *event)
{
#if defined (PDL_WIN32)
  switch (::WaitForSingleObject (*event, INFINITE))
    {
    case WAIT_OBJECT_0:
      return 0;
    default:
      PDL_OS::set_errno_to_last_error ();
      return -1;
    }
#elif defined (PDL_HAS_THREADS)
  int result = 0;
  int error = 0;

  // grab the lock first
  if (PDL_OS::mutex_lock (&event->lock_) == 0)
    {
      if (event->is_signaled_ == 1)
        // Event is currently signaled.
        {
          if (event->manual_reset_ == 0)
            // AUTO: reset state
            event->is_signaled_ = 0;
        }
      else
        // event is currently not signaled
        {
          event->waiting_threads_++;

          if (PDL_OS::cond_wait (&event->condition_,
                                 &event->lock_) != 0)
            {
              result = -1;
              error = errno;
              // Something went wrong...
            }

          event->waiting_threads_--;
        }

      // Now we can let go of the lock.
      PDL_OS::mutex_unlock (&event->lock_);

      if (result == -1)
        // Reset errno in case mutex_unlock() also fails...
        errno = error;
    }
  else
    result = -1;
  return result;
#else
  PDL_UNUSED_ARG (event);
  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_WIN32 */
}

PDL_INLINE int
PDL_OS::event_timedwait (PDL_event_t *event,
                         PDL_Time_Value *timeout)
{
#if defined (PDL_WIN32)
  DWORD result;

  if (timeout == 0)
    // Wait forever
    result = ::WaitForSingleObject (*event, INFINITE);
  else if (timeout->sec () == 0 && timeout->usec () == 0)
    // Do a "poll".
    result = ::WaitForSingleObject (*event, 0);
  else
    {
      // Wait for upto <relative_time> number of milliseconds.  Note
      // that we must convert between absolute time (which is passed
      // as a parameter) and relative time (which is what
      // WaitForSingleObjects() expects).
      PDL_Time_Value relative_time;
      relative_time = *timeout - PDL_OS::gettimeofday ();

      // Watchout for situations where a context switch has caused the
      // current time to be > the timeout.  Thanks to Norbert Rapp
      // <NRapp@nexus-informatics.de> for pointing this.
      int msec_timeout;
      PDL_Time_Value PDL_Time_Value_zero;
      PDL_Time_Value_zero.initialize();
      if (relative_time < PDL_Time_Value_zero)
        msec_timeout = 0;
      else
        msec_timeout = relative_time.msec ();
      result = ::WaitForSingleObject (*event, msec_timeout);
    }

  switch (result)
    {
    case WAIT_OBJECT_0:
      return 0;
    case WAIT_TIMEOUT:
      errno = ETIME;
      return -1;
    default:
      // This is a hack, we need to find an appropriate mapping...
      PDL_OS::set_errno_to_last_error ();
      return -1;
    }
#elif defined (PDL_HAS_THREADS)
  int result = 0;
  int error = 0;

  // grab the lock first
  if (PDL_OS::mutex_lock (&event->lock_) == 0)
    {
      if (event->is_signaled_ == 1)
        // event is currently signaled
        {
          if (event->manual_reset_ == 0)
            // AUTO: reset state
            event->is_signaled_ = 0;
        }
      else
        // event is currently not signaled
        {
          event->waiting_threads_++;

          if (PDL_OS::cond_timedwait (&event->condition_,
                                      &event->lock_,
                                      timeout) != 0)
            {
              result = -1;
              error = errno;
            }

          event->waiting_threads_--;
        }

      // Now we can let go of the lock.
      PDL_OS::mutex_unlock (&event->lock_);

      if (result == -1)
        // Reset errno in case mutex_unlock() also fails...
        errno = error;
    }
  else
    result = -1;
  return result;
#else
  PDL_UNUSED_ARG (event);
  PDL_UNUSED_ARG (timeout);
  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_WIN32 */
}

PDL_INLINE int
PDL_OS::event_signal (PDL_event_t *event)
{
#if defined (PDL_WIN32)
  PDL_WIN32CALL_RETURN (PDL_ADAPT_RETVAL (::SetEvent (*event), pdl_result_), int, -1);
#elif defined (PDL_HAS_THREADS)
  int result = 0;
  int error = 0;

  // grab the lock first
  if (PDL_OS::mutex_lock (&event->lock_) == 0)
    {
      // Manual-reset event.
      if (event->manual_reset_ == 1)
        {
          // signal event
          event->is_signaled_ = 1;
          // wakeup all
          if (PDL_OS::cond_broadcast (&event->condition_) != 0)
            {
              result = -1;
              error = errno;
            }
        }
      // Auto-reset event
      else
        {
          if (event->waiting_threads_ == 0)
            // No waiters: signal event.
            event->is_signaled_ = 1;

          // Waiters: wakeup one waiter.
          else if (PDL_OS::cond_signal (&event->condition_) != 0)
            {
              result = -1;
              error = errno;
            }
        }

      // Now we can let go of the lock.
      PDL_OS::mutex_unlock (&event->lock_);

      if (result == -1)
        // Reset errno in case mutex_unlock() also fails...
        errno = error;
    }
  else
    result = -1;
  return result;
#else
  PDL_UNUSED_ARG (event);
  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_WIN32 */
}

PDL_INLINE int
PDL_OS::event_pulse (PDL_event_t *event)
{
#if defined (PDL_WIN32)
  PDL_WIN32CALL_RETURN (PDL_ADAPT_RETVAL (::PulseEvent (*event), pdl_result_), int, -1);
#elif defined (PDL_HAS_THREADS)
  int result = 0;
  int error = 0;

  // grab the lock first
  if (PDL_OS::mutex_lock (&event->lock_) == 0)
    {
      // Manual-reset event.
      if (event->manual_reset_ == 1)
        {
          // Wakeup all waiters.
          if (PDL_OS::cond_broadcast (&event->condition_) != 0)
            {
              result = -1;
              error = errno;
            }
        }
      // Auto-reset event: wakeup one waiter.
      else if (PDL_OS::cond_signal (&event->condition_) != 0)
        {
          result = -1;
          error = errno;
        }

      // Reset event.
      event->is_signaled_ = 0;

      // Now we can let go of the lock.
      PDL_OS::mutex_unlock (&event->lock_);

      if (result == -1)
        // Reset errno in case mutex_unlock() also fails...
        errno = error;
    }
  else
    result = -1;
  return result;
#else
  PDL_UNUSED_ARG (event);
  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_WIN32 */
}

PDL_INLINE int
PDL_OS::event_reset (PDL_event_t *event)
{
#if defined (PDL_WIN32)
  PDL_WIN32CALL_RETURN (PDL_ADAPT_RETVAL (::ResetEvent (*event), pdl_result_), int, -1);
#elif defined (PDL_HAS_THREADS)
  int result = 0;

  // Grab the lock first.
  if (PDL_OS::mutex_lock (&event->lock_) == 0)
    {
      // Reset event.
      event->is_signaled_ = 0;

      // Now we can let go of the lock.
      PDL_OS::mutex_unlock (&event->lock_);
    }
  else
    result = -1;
  return result;
#else
  PDL_UNUSED_ARG (event);
  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_WIN32 */
}

#if defined (PDL_WIN32)

/*BUGBUG_NT*/
#if defined(_DEBUG)
# define PDL_SOCKCALL_RETURN(OP,TYPE,FAILVALUE) \
  do { TYPE pdl_result_ = (TYPE) OP; \
      if (pdl_result_ == FAILVALUE) { \
		int ___ = (int)::WSAGetLastError ();\
		errno = ___; \
		pdlPrintWin32Error(errno);\
	    return (TYPE) FAILVALUE; \
	  } else return pdl_result_; \
  } while (0)
//fix BUG-17609
# define PDL_SOCKCALL(OP,TYPE,FAILVALUE,RESULT) \
  do {  RESULT = (TYPE) OP; \
      if ( RESULT == FAILVALUE) { \
        PDL_OS::set_errno_to_wsa_last_error(); \
        pdlPrintWin32Error(errno);  \
       } \
  } while (0)
#else
# define PDL_SOCKCALL_RETURN(OP,TYPE,FAILVALUE) \
  do { TYPE pdl_result_ = (TYPE) OP; \
      if (pdl_result_ == FAILVALUE) { \
		int ___ = (int)::WSAGetLastError ();\
		errno = ___; \
	    return (TYPE) FAILVALUE; \
	  } else return pdl_result_; \
  } while (0)
//fix BUG-17609
# define PDL_SOCKCALL(OP,TYPE,FAILVALUE,RESULT) \
  do {  RESULT = (TYPE) OP; \
      if ( RESULT == FAILVALUE) { \
           PDL_OS::set_errno_to_wsa_last_error();  \
	} \
  } while (0)
#endif
/*BUGBUG_NT*/

#else
# define PDL_SOCKCALL_RETURN(OP,TYPE,FAILVALUE) PDL_OSCALL_RETURN(OP,TYPE,FAILVALUE)
#endif /* PDL_WIN32 */

#if defined (PDL_LACKS_NETDB_REENTRANT_FUNCTIONS)
# if defined (PDL_MT_SAFE) && (PDL_MT_SAFE != 0)
#   define PDL_NETDBCALL_RETURN(OP,TYPE,FAILVALUE,TARGET,SIZE) \
  do \
  { \
    if (PDL_OS::netdb_acquire ())  \
      return FAILVALUE; \
    else \
      { \
        TYPE pdl_result_; \
        PDL_OSCALL (OP, TYPE, FAILVALUE, pdl_result_); \
        if (pdl_result_ != FAILVALUE) \
          ::memcpy (TARGET, \
                    pdl_result_, \
                    SIZE < sizeof (TYPE) ? SIZE : sizeof (TYPE)); \
        PDL_OS::netdb_release (); \
        return pdl_result_; \
      } \
  } while(0)
# else /* ! (PDL_MT_SAFE && PDL_MT_SAFE != 0) */
#   define PDL_NETDBCALL_RETURN(OP,TYPE,FAILVALUE,TARGET,SIZE) \
  do \
  { \
        TYPE pdl_result_; \
        PDL_OSCALL(OP,TYPE,FAILVALUE,pdl_result_); \
        if (pdl_result_ != FAILVALUE) \
          ::memcpy (TARGET, \
                    pdl_result_, \
                    SIZE < sizeof (TYPE) ? SIZE : sizeof (TYPE)); \
        return pdl_result_; \
  } while(0)
# endif /* PDL_MT_SAFE && PDL_MT_SAFE != 0 */
#endif /* PDL_LACKS_NETDB_REENTRANT_FUNCTIONS */

PDL_INLINE PDL_SOCKET
PDL_OS::accept (PDL_SOCKET handle,
                struct sockaddr *addr,
                int *addrlen)
{
  PDL_TRACE ("PDL_OS::accept");

#if defined (PDL_PSOS)
#  if !defined (PDL_PSOS_DIAB_PPC)
  PDL_SOCKCALL_RETURN (::accept ((PDL_SOCKET) handle,
                                 (struct sockaddr_in *) addr,
                                 (PDL_SOCKET_LEN *) addrlen),
                       PDL_SOCKET,
                       PDL_INVALID_SOCKET);
#  else
PDL_SOCKCALL_RETURN (::accept ((PDL_SOCKET) handle,
                               (struct sockaddr *) addr,
                               (PDL_SOCKET_LEN *) addrlen),
                     PDL_SOCKET,
                     PDL_INVALID_SOCKET);
#  endif /* defined PDL_PSOS_DIAB_PPC */
#else
  // On a non-blocking socket with no connections to accept, this
  // system call will return EWOULDBLOCK or EAGAIN, depending on the
  // platform.  UNIX 98 allows either errno, and they may be the same
  // numeric value.  So to make life easier for upper PDL layers as
  // well as application programmers, always change EAGAIN to
  // EWOULDBLOCK.  Rather than hack the PDL_OSCALL_RETURN macro, it's
  // handled explicitly here.  If the PDL_OSCALL macro ever changes,
  // this function needs to be reviewed.  On Win32, the regular macros
  // can be used, as this is not an issue.

#  if defined (PDL_WIN32)
  PDL_SOCKCALL_RETURN (::accept ((PDL_SOCKET) handle,
                                 addr,
                                 (PDL_SOCKET_LEN *) addrlen),
                       PDL_SOCKET,
                       PDL_INVALID_SOCKET);
#elif defined(ITRON)
  /* empty */
  PDL_NOTSUP_RETURN (-1);
#  else
#    if defined (PDL_HAS_BROKEN_ACCEPT_ADDR)
  // Apparently some platforms like VxWorks can't correctly deal with
  // a NULL addr.

   sockaddr_in fake_addr;
   int fake_addrlen;

   if (addrlen == 0)
     addrlen = &fake_addrlen;

   if (addr == 0)
     {
       addr = (sockaddr *) &fake_addr;
       *addrlen = sizeof fake_addr;
     }
#    endif /* VXWORKS */
  PDL_SOCKET pdl_result = ::accept ((PDL_SOCKET) handle,
                                    addr,
                                    (PDL_SOCKET_LEN *) addrlen) ;
  if (pdl_result == PDL_INVALID_SOCKET && errno == EAGAIN)
    errno = EWOULDBLOCK;
  return pdl_result;

#  endif /* defined (PDL_WIN32) */
#endif /* defined (PDL_PSOS) */
}


PDL_INLINE int
PDL_OS::enum_protocols (int *protocols,
                        PDL_Protocol_Info *protocol_buffer,
                        u_long *buffer_length)
{
#if defined (PDL_HAS_WINSOCK2) && (PDL_HAS_WINSOCK2 != 0)

  PDL_SOCKCALL_RETURN (::WSAEnumProtocols (protocols,
                                           protocol_buffer,
                                           buffer_length),
                       int,
                       SOCKET_ERROR);

#else
  PDL_UNUSED_ARG (protocols);
  PDL_UNUSED_ARG (protocol_buffer);
  PDL_UNUSED_ARG (buffer_length);
  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_HAS_WINSOCK2 */
}


PDL_INLINE int
PDL_OS::ioctl (PDL_SOCKET socket,
               u_long io_control_code,
               void *in_buffer_p,
               u_long in_buffer,
               void *out_buffer_p,
               u_long out_buffer,
               u_long *bytes_returned,
               PDL_OVERLAPPED *overlapped,
               PDL_OVERLAPPED_COMPLETION_FUNC func)
{
#if defined (PDL_HAS_WINSOCK2) && (PDL_HAS_WINSOCK2 != 0)
  PDL_SOCKCALL_RETURN (::WSAIoctl ((PDL_SOCKET) socket,
                                   io_control_code,
                                   in_buffer_p,
                                   in_buffer,
                                   out_buffer_p,
                                   out_buffer,
                                   bytes_returned,
                                   (WSAOVERLAPPED *) overlapped,
                                   func),
                       int,
                       SOCKET_ERROR);
#else
  PDL_UNUSED_ARG (socket);
  PDL_UNUSED_ARG (io_control_code);
  PDL_UNUSED_ARG (in_buffer_p);
  PDL_UNUSED_ARG (in_buffer);
  PDL_UNUSED_ARG (out_buffer_p);
  PDL_UNUSED_ARG (out_buffer);
  PDL_UNUSED_ARG (bytes_returned);
  PDL_UNUSED_ARG (overlapped);
  PDL_UNUSED_ARG (func);
  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_HAS_WINSOCK2 */
}


PDL_INLINE int
PDL_OS::bind (PDL_SOCKET handle, struct sockaddr *addr, int addrlen)
{
  PDL_TRACE ("PDL_OS::bind");
#if defined (PDL_PSOS) && !defined (PDL_PSOS_DIAB_PPC)
  PDL_SOCKCALL_RETURN (::bind ((PDL_SOCKET) handle,
                               (struct sockaddr_in *) addr,
                               (PDL_SOCKET_LEN) addrlen),
                       int, -1);
#elif defined(ITRON)
  /* empty */
  PDL_NOTSUP_RETURN (-1);
#else /* !defined (PDL_PSOS) || defined (PDL_PSOS_DIAB_PPC) */
  PDL_SOCKCALL_RETURN (::bind ((PDL_SOCKET) handle,
                               addr,
                               (PDL_SOCKET_LEN) addrlen), int, -1);
#endif /* defined (PDL_PSOS) && !defined (PDL_PSOS_DIAB_PPC) */
}

PDL_INLINE int
PDL_OS::connect (PDL_SOCKET handle,
                 struct sockaddr *addr,
                 int addrlen)
{
  PDL_TRACE ("PDL_OS::connect");
#if defined (PDL_PSOS) && !defined (PDL_PSOS_DIAB_PPC)
  PDL_SOCKCALL_RETURN (::connect ((PDL_SOCKET) handle,
                                  (struct sockaddr_in *) addr,
                                  (PDL_SOCKET_LEN) addrlen),
                       int, -1);
#elif defined(ITRON)
  /* empty */
  PDL_NOTSUP_RETURN (-1);
#else  /* !defined (PDL_PSOS) || defined (PDL_PSOS_DIAB_PPC) */
  PDL_SOCKCALL_RETURN (::connect ((PDL_SOCKET) handle,
                                  addr,
                                  (PDL_SOCKET_LEN) addrlen), int, -1);
#endif /* defined (PDL_PSOS)  && !defined (PDL_PSOS_DIAB_PPC) */
}


#if !defined (VXWORKS)
PDL_INLINE struct hostent *
PDL_OS::gethostbyname (const char *name)
{
  PDL_TRACE ("PDL_OS::gethostbyname");
# if defined (PDL_PSOS)
  PDL_UNUSED_ARG (name);
  PDL_NOTSUP_RETURN (0);
# elif defined (PDL_HAS_NONCONST_GETBY)
  PDL_SOCKCALL_RETURN (::gethostbyname ((char *) name), struct hostent *, 0);
#elif defined(ITRON)
  /* empty */
  PDL_NOTSUP_RETURN (NULL);
# else
#  if !defined(CYGWIN32)
     PDL_SOCKCALL_RETURN (::gethostbyname (name), struct hostent *, 0);
#  else
     PDL_SOCKCALL_RETURN (gethostbyname (name), struct hostent *, 0);
#  endif
# endif /* PDL_HAS_NONCONST_GETBY */
}

PDL_INLINE struct hostent *
PDL_OS::gethostbyname2 (const char *name, int family)
{
  PDL_TRACE ("PDL_OS::gethostbyname2");
  PDL_UNUSED_ARG (name);
  PDL_UNUSED_ARG (family);

  // gethostbyname2 is not reentrant
  // and obsolete(defined in RFC 2133
  // but removed in RFC 2553 and RFC 3493)

  PDL_NOTSUP_RETURN (0);
}

PDL_INLINE struct hostent *
PDL_OS::gethostbyaddr (const char *addr, int length, int type)
{
  PDL_TRACE ("PDL_OS::gethostbyaddr");
# if defined (PDL_PSOS)
  PDL_UNUSED_ARG (addr);
  PDL_UNUSED_ARG (length);
  PDL_UNUSED_ARG (type);
  PDL_NOTSUP_RETURN (0);
# elif defined (PDL_HAS_NONCONST_GETBY)
  PDL_SOCKCALL_RETURN (::gethostbyaddr ((char *) addr, (PDL_SOCKET_LEN) length, type),
                       struct hostent *, 0);
#elif defined(ITRON)
  /* empty */
  PDL_NOTSUP_RETURN (NULL);
# else
#  if !defined(CYGWIN32)
     PDL_SOCKCALL_RETURN (::gethostbyaddr (addr, (PDL_SOCKET_LEN) length, type),
                          struct hostent *, 0);
#  else
     PDL_SOCKCALL_RETURN (gethostbyaddr (addr, (PDL_SOCKET_LEN) length, type),
                          struct hostent *, 0);
#  endif
# endif /* PDL_HAS_NONCONST_GETBY */
}
#endif /* ! VXWORKS */

// Set of wrappers for IPv6 socket API

PDL_INLINE const char * 
PDL_OS::gai_strerror(int errcode)
{
  PDL_TRACE ("PDL_OS::gai_strerror()");

# if defined(PDL_HAS_IP6)  
 
  return ::gai_strerror(errcode);

# else

  PDL_UNUSED_ARG (errcode);
  PDL_NOTSUP_RETURN(NULL);

# endif
}

PDL_INLINE int
PDL_OS::getaddrinfo(const char *node,
                    const char *service,
                    const struct addrinfo *hints,
                    struct addrinfo **res)
{

  PDL_TRACE ("PDL_OS::getaddrinfo()");

# if defined(PDL_HAS_IP6)

  return ::getaddrinfo(node,
                       service,
                       hints,
                       res);

# else

  PDL_UNUSED_ARG (node);
  PDL_UNUSED_ARG (service);
  PDL_UNUSED_ARG (hints);
  PDL_UNUSED_ARG (res);
  PDL_NOTSUP_RETURN(-1);

# endif
}

PDL_INLINE int 
PDL_OS::getnameinfo(const struct sockaddr *sa,
                    socklen_t  salen,
                    char      *host,
                    size_t     hostlen,
                    char      *serv,
                    size_t     servlen,
                    int        flags)
{

  PDL_TRACE ("PDL_OS::getnameinfo()");
  
# if defined(PDL_HAS_IP6)

  return ::getnameinfo(sa,
                       salen,
                       host,
                       hostlen,
                       serv,
                       servlen,
                       flags);
# else

  PDL_UNUSED_ARG (sa);
  PDL_UNUSED_ARG (salen);
  PDL_UNUSED_ARG (host);
  PDL_UNUSED_ARG (hostlen);
  PDL_UNUSED_ARG (serv);
  PDL_UNUSED_ARG (servlen);
  PDL_UNUSED_ARG (flags);
  PDL_NOTSUP_RETURN(-1);

# endif
}


PDL_INLINE  void 
PDL_OS::freeaddrinfo(struct addrinfo *res)
{
  PDL_TRACE ("PDL_OS::freeaddrinfo()");

# if defined(PDL_HAS_IP6)

  ::freeaddrinfo(res);

# else

  PDL_UNUSED_ARG (res);
  PDL_NOTSUP;

# endif
}

PDL_INLINE bool
PDL_OS::in6_is_addr_unspecified (void *addr)
{
  PDL_TRACE ("PDL_OS::in6_is_addr_unspecified()");

# if defined(PDL_HAS_IP6)

  return IN6_IS_ADDR_UNSPECIFIED((struct in6_addr *)addr);

# else

  PDL_UNUSED_ARG (addr);
  PDL_NOTSUP_RETURN(false);

# endif
}

PDL_INLINE bool
PDL_OS::in6_is_addr_loopback (void *addr)
{
  PDL_TRACE ("PDL_OS::in6_is_addr_loopback()");

# if defined(PDL_HAS_IP6)

  return IN6_IS_ADDR_LOOPBACK((struct in6_addr *)addr);

# else

  PDL_UNUSED_ARG (addr);
  PDL_NOTSUP_RETURN(false);

# endif
}

PDL_INLINE bool
PDL_OS::in6_is_addr_multicast (void *addr)
{
  PDL_TRACE ("PDL_OS::in6_is_addr_multicast()");

# if defined(PDL_HAS_IP6)

  return IN6_IS_ADDR_MULTICAST((struct in6_addr *)addr);

# else

  PDL_UNUSED_ARG (addr);
  PDL_NOTSUP_RETURN(false);

# endif
}

PDL_INLINE bool
PDL_OS::in6_is_addr_linklocal (void *addr)
{
  PDL_TRACE ("PDL_OS::in6_is_addr_linklocal()");

# if defined(PDL_HAS_IP6)

  return IN6_IS_ADDR_LINKLOCAL((struct in6_addr *)addr);

# else

  PDL_UNUSED_ARG (addr);
  PDL_NOTSUP_RETURN(false);

# endif
}

PDL_INLINE bool
PDL_OS::in6_is_addr_sitelocal (void *addr)
{
  PDL_TRACE ("PDL_OS::in6_is_addr_sitelocal()");

# if defined(PDL_HAS_IP6)

  return IN6_IS_ADDR_SITELOCAL((struct in6_addr *)addr);

# else

  PDL_UNUSED_ARG (addr);
  PDL_NOTSUP_RETURN(false);

# endif
}

PDL_INLINE bool
PDL_OS::in6_is_addr_v4mapped (void *addr)
{
  PDL_TRACE ("PDL_OS::in6_is_addr_v4mapped()");

# if defined(PDL_HAS_IP6)

  return IN6_IS_ADDR_V4MAPPED((struct in6_addr *)addr);

# else

  PDL_UNUSED_ARG (addr); 
  PDL_NOTSUP_RETURN(false);

# endif
}

PDL_INLINE bool
PDL_OS::in6_is_addr_v4compat(void *addr)
{
  PDL_TRACE ("PDL_OS::in6_is_addr_v4compat()");

# if defined(PDL_HAS_IP6)

  return IN6_IS_ADDR_V4COMPAT((struct in6_addr *)addr);

# else

  PDL_UNUSED_ARG (addr);
  PDL_NOTSUP_RETURN(false);

# endif

}

PDL_INLINE bool
PDL_OS::in6_is_addr_mc_nodelocal(void *addr)
{
  PDL_TRACE ("PDL_OS::in6_is_addr_mc_nodelocal()");

# if defined(PDL_HAS_IP6)

  return IN6_IS_ADDR_MC_NODELOCAL((struct in6_addr *)addr);

# else

  PDL_UNUSED_ARG (addr);
  PDL_NOTSUP_RETURN(false);

# endif
}

PDL_INLINE bool  
PDL_OS::in6_is_addr_mc_linklocal(void *addr)
{
  PDL_TRACE ("PDL_OS::in6_is_addr_mc_linklocal()");

# if defined(PDL_HAS_IP6)

  return IN6_IS_ADDR_MC_LINKLOCAL((struct in6_addr *)addr);

# else

  PDL_UNUSED_ARG (addr);
  PDL_NOTSUP_RETURN(false);

# endif
}

PDL_INLINE bool  
PDL_OS::in6_is_addr_mc_sitelocal(void *addr)
{
  PDL_TRACE ("PDL_OS::in6_is_addr_mc_sitelocal()");

# if defined(PDL_HAS_IP6)

  return IN6_IS_ADDR_MC_SITELOCAL((struct in6_addr *)addr);

# else

  PDL_UNUSED_ARG (addr);
  PDL_NOTSUP_RETURN(false);

# endif
}

PDL_INLINE bool  
PDL_OS::in6_is_addr_mc_orglocal(void *addr)
{
  PDL_TRACE ("PDL_OS::in6_is_addr_mc_orglocal()");

# if defined(PDL_HAS_IP6)

  return IN6_IS_ADDR_MC_ORGLOCAL((struct in6_addr *)addr);

# else

  PDL_UNUSED_ARG (addr);  
  PDL_NOTSUP_RETURN(false);

# endif
}

PDL_INLINE bool  
PDL_OS::in6_is_addr_mc_global(void *addr)
{
  PDL_TRACE ("PDL_OS::in6_is_addr_mc_global()");

# if defined(PDL_HAS_IP6)

  return IN6_IS_ADDR_MC_GLOBAL((struct in6_addr *)addr);

# else

  PDL_UNUSED_ARG (addr);
  PDL_NOTSUP_RETURN(false);

# endif
}

// It would be really cool to add another version of select that would
// function like the one we're defending against below!
PDL_INLINE int
PDL_OS::select (int width,
                fd_set *rfds, fd_set *wfds, fd_set *efds,
                const PDL_Time_Value *timeout)
{
  PDL_TRACE ("PDL_OS::select");
  ssize_t result;
#if defined(ITRON)
  /* empty */
  PDL_NOTSUP_RETURN (-1);
#else
#if defined (PDL_HAS_NONCONST_SELECT_TIMEVAL)
  // We must defend against non-conformity!
  timeval copy;
  timeval *timep;

  if (timeout != 0)
    {
      copy = *timeout;
      timep = &copy;
    }
  else
    timep = 0;
#else
  const timeval *timep = (timeout == 0 ? (const timeval *)0 : *timeout);
#endif /* PDL_HAS_NONCONST_SELECT_TIMEVAL */
#if defined (PDL_WIN32)
  PDL_SOCKCALL_RETURN (::select (width,
                                 (PDL_FD_SET_TYPE *) rfds,
                                 (PDL_FD_SET_TYPE *) wfds,
                                 (PDL_FD_SET_TYPE *) efds,
                                 timep),
                       int, -1);
#else
  again:
# if defined(HPUX_VERS) && (HPUX_VERS >= 1131)
  struct timespec  ts;
  struct timespec *tsp;

  if( timep != 0 )
  {
    ts.tv_sec = copy.tv_sec;
    ts.tv_nsec = copy.tv_usec * 1000;
    tsp = &ts;
  }
  else
  {
    tsp = (struct timespec *)0;
  }

  result = ::pselect (width,
            (PDL_FD_SET_TYPE *) rfds,
            (PDL_FD_SET_TYPE *) wfds,
            (PDL_FD_SET_TYPE *) efds,
            tsp, NULL);
# else
  result = ::select (width,
            (PDL_FD_SET_TYPE *) rfds,
            (PDL_FD_SET_TYPE *) wfds,
            (PDL_FD_SET_TYPE *) efds,
            timep);
# endif
  if (result == -1 && errno == EINTR)
  {
      goto again;
  }
  return result;
#endif // defined (PDL_WIN32)
#endif
}

PDL_INLINE int
PDL_OS::select (int width,
                fd_set *rfds, fd_set *wfds, fd_set *efds,
                const PDL_Time_Value &timeout)
{
  PDL_TRACE ("PDL_OS::select");
  ssize_t result;
#if defined(ITRON)
  /* empty */
  PDL_NOTSUP_RETURN (-1);
#else
#if defined (PDL_HAS_NONCONST_SELECT_TIMEVAL)
# define ___PDL_TIMEOUT &copy
  timeval copy = timeout;
#else
# define ___PDL_TIMEOUT timep
  const timeval *timep = timeout;
#endif /* PDL_HAS_NONCONST_SELECT_TIMEVAL */
#if defined(PDL_WIN32)
  PDL_SOCKCALL_RETURN (::select (width,
                                 (PDL_FD_SET_TYPE *) rfds,
                                 (PDL_FD_SET_TYPE *) wfds,
                                 (PDL_FD_SET_TYPE *) efds,
                                 ___PDL_TIMEOUT),
                       int, -1);
#else
  again:
# if defined(HPUX_VERS) && (HPUX_VERS >= 1131)
  struct timespec ts;
  ts.tv_sec = copy.tv_sec;
  ts.tv_nsec = copy.tv_usec * 1000;

  result = ::pselect (width,
      (PDL_FD_SET_TYPE *) rfds,
      (PDL_FD_SET_TYPE *) wfds,
      (PDL_FD_SET_TYPE *) efds,
      &ts, NULL);
# else
  result = ::select (width,
      (PDL_FD_SET_TYPE *) rfds,
      (PDL_FD_SET_TYPE *) wfds,
      (PDL_FD_SET_TYPE *) efds,
      ___PDL_TIMEOUT);
# endif
  if (result == -1 && errno == EINTR)
  {
      goto again;
  }
  return result;
#endif // defined PDL_WIN32
#endif
#undef ___PDL_TIMEOUT
}

PDL_INLINE int
PDL_OS::recv (PDL_SOCKET handle, char *buf, int len, int flags)
{
  PDL_TRACE ("PDL_OS::recv");

  // On UNIX, a non-blocking socket with no data to receive, this
  // system call will return EWOULDBLOCK or EAGAIN, depending on the
  // platform.  UNIX 98 allows either errno, and they may be the same
  // numeric value.  So to make life easier for upper PDL layers as
  // well as application programmers, always change EAGAIN to
  // EWOULDBLOCK.  Rather than hack the PDL_OSCALL_RETURN macro, it's
  // handled explicitly here.  If the PDL_OSCALL macro ever changes,
  // this function needs to be reviewed.  On Win32, the regular macros
  // can be used, as this is not an issue.
#if defined (PDL_WIN32)
  PDL_SOCKCALL_RETURN (::recv ((PDL_SOCKET) handle, buf, len, flags), int, -1);
#elif defined(ITRON)
  /* empty */
  PDL_NOTSUP_RETURN (-1);
#else

  int pdl_result_;
  again:
  pdl_result_ = ::recv ((PDL_SOCKET) handle, buf, len, flags);
  if (pdl_result_ == -1 && errno == EINTR)
  {
      goto again;
  }
  if (pdl_result_ == -1 && errno == EAGAIN)
    errno = EWOULDBLOCK;
  return pdl_result_;

#endif /* defined (PDL_WIN32) */
}

PDL_INLINE int
PDL_OS::recvfrom (PDL_SOCKET handle,
                  char *buf,
                  int len,
                  int flags,
                  struct sockaddr *addr,
                  int *addrlen)
{
  PDL_TRACE ("PDL_OS::recvfrom");
  int result;
  again:
#if defined (PDL_WIN32)
  result = (int)::recvfrom ((PDL_SOCKET) handle,
                           buf,
                           (PDL_SOCKET_LEN) len,
                           flags,
                           addr,
                           (PDL_SOCKET_LEN *) addrlen);
  if (result == SOCKET_ERROR)
    {
      PDL_OS::set_errno_to_wsa_last_error ();
      if (errno == WSAEMSGSIZE &&
          PDL_BIT_ENABLED (flags, MSG_PEEK))
        return len;
      else
        return -1;
    }
  else
    return result;
#elif defined (PDL_PSOS)
#  if !defined PDL_PSOS_DIAB_PPC
  result = ::recvfrom((PDL_SOCKET) handle, buf, (PDL_SOCKET_LEN) len, flags,
                      (struct sockaddr_in *) addr, (PDL_SOCKET_LEN *) addrlen);

#  else
  result = ::recvfrom ((PDL_SOCKET) handle, buf, (PDL_SOCKET_LEN) len, flags,
                                   (struct sockaddr *) addr, (PDL_SOCKET_LEN *) addrlen);
#  endif /* defined PDL_PSOS_DIAB_PPC */
#elif defined(ITRON)
  /* empty */
  PDL_NOTSUP_RETURN (-1);
#else /* non Win32 and non PSOS */
  result = ::recvfrom ((PDL_SOCKET) handle, buf, (PDL_SOCKET_LEN) len, flags,
                       addr, (PDL_SOCKET_LEN *) addrlen);
#endif /* defined (PDL_PSOS) */
  if (result == -1 && errno == EINTR)
  {
      goto again;
  }
  return result;
}

PDL_INLINE int
PDL_OS::send (PDL_SOCKET handle, const char *buf, int len, int flags)
{
  PDL_TRACE ("PDL_OS::send");
  int result = 0;
  again:
#if defined (PDL_WIN32)
  //fix BUG-17609
  PDL_SOCKCALL(::send ((PDL_SOCKET) handle, buf, len, flags),
               int ,
               SOCKET_ERROR,
               result);               
#elif defined (VXWORKS) || defined (HPUX) || defined (PDL_PSOS)
  result = ::send ((PDL_SOCKET) handle, (char *) buf, len, flags);
#elif defined(ITRON)
  /* empty */
  PDL_NOTSUP_RETURN (-1);
#else
  //fix BUG-18029.
  result = ::send ((PDL_SOCKET) handle, buf, len, flags);
#endif /* PDL_WIN32 */
  
#if defined(PDL_WIN32)
  //fix BUG-17609
  if ((result == SOCKET_ERROR) && (errno == WSAEINTR))
  {
      goto again;
  }
#else    
  if (result == -1 && errno == EINTR)
  {
      goto again;
  }
#endif  
  return result;

}

PDL_INLINE int
PDL_OS::recvfrom (PDL_SOCKET handle,
                  iovec *buffers,
                  int buffer_count,
                  size_t &number_of_bytes_recvd,
                  int &flags,
                  struct sockaddr *addr,
                  int *addrlen,
                  PDL_OVERLAPPED *overlapped,
                  PDL_OVERLAPPED_COMPLETION_FUNC func)
{
  PDL_TRACE ("PDL_OS::recvfrom");

#if defined (PDL_HAS_WINSOCK2) && (PDL_HAS_WINSOCK2 != 0)
  DWORD bytes_recvd;
  DWORD the_flags = flags;
  int result = ::WSARecvFrom ((SOCKET) handle,
                              (WSABUF*)buffers,
                              buffer_count,
                              &bytes_recvd,
                              &the_flags,
                              addr,
                              addrlen,
                              overlapped,
                              func);
  flags = the_flags;
  number_of_bytes_recvd = PDL_static_cast (size_t, bytes_recvd);
  return result;
#else
  PDL_UNUSED_ARG (handle);
  PDL_UNUSED_ARG (buffers);
  PDL_UNUSED_ARG (buffer_count);
  PDL_UNUSED_ARG (number_of_bytes_recvd);
  PDL_UNUSED_ARG (flags);
  PDL_UNUSED_ARG (addr);
  PDL_UNUSED_ARG (addrlen);
  PDL_UNUSED_ARG (overlapped);
  PDL_UNUSED_ARG (func);
  PDL_NOTSUP_RETURN (-1);
#endif /* defined (PDL_HAS_WINSOCK2) && (PDL_HAS_WINSOCK2 != 0) */
}

PDL_INLINE int
PDL_OS::sendto (PDL_SOCKET handle,
                const char *buf,
                int len,
                int flags,
                const struct sockaddr *addr,
                int addrlen)
{
    PDL_TRACE ("PDL_OS::sendto");
    int result;
  again:
#if defined (VXWORKS)
    result = ::sendto ((PDL_SOCKET) handle, (char *) buf, len, flags,
                       PDL_const_cast (struct sockaddr *, addr), addrlen);
#elif defined (PDL_PSOS)
#  if !defined (PDL_PSOS_DIAB_PPC)
    result = ::sendto ((PDL_SOCKET) handle, (char *) buf, len, flags,
                       (struct sockaddr_in *) addr, addrlen);
#  if defined (PDL_PSOS_DIAB_PPC)
    result = ::sendto ((PDL_SOCKET) handle, (char *) buf, len, flags,
                       (struct sockaddr *) addr, addrlen);
#  endif /*defined PDL_PSOS_DIAB_PPC */
#endif
#elif defined(ITRON)
/* empty */
    PDL_NOTSUP_RETURN (-1);
#elif defined (PDL_WIN32)
    PDL_SOCKCALL_RETURN (::sendto ((PDL_SOCKET) handle, (char *) buf, len, flags,
                                 (struct sockaddr *) addr, addrlen),
                       int, -1);
#else
    result = ::sendto ((PDL_SOCKET) handle, buf, len, flags,
                       PDL_const_cast (struct sockaddr *, addr), addrlen);
#endif /* VXWORKS */
    if (result == -1 && errno == EINTR)
    {
        goto again;
    }
    return result;
}

PDL_INLINE int
PDL_OS::sendto (PDL_SOCKET handle,
                const iovec *buffers,
                int buffer_count,
                size_t &number_of_bytes_sent,
                int flags,
                const struct sockaddr *addr,
                int addrlen,
                PDL_OVERLAPPED *overlapped,
                PDL_OVERLAPPED_COMPLETION_FUNC func)
{
  PDL_TRACE ("PDL_OS::sendto");

#if defined (PDL_HAS_WINSOCK2) && (PDL_HAS_WINSOCK2 != 0)
  DWORD bytes_sent;
  int result = ::WSASendTo ((SOCKET) handle,
                            (WSABUF*)buffers,
                            buffer_count,
                            &bytes_sent,
                            flags,
                            addr,
                            addrlen,
                            overlapped,
                            func);
  number_of_bytes_sent = PDL_static_cast (size_t, bytes_sent);
  return result;
#else
  PDL_UNUSED_ARG (overlapped);
  PDL_UNUSED_ARG (func);

  number_of_bytes_sent = 0;

  int result = 0;

  for (int i = 0; i < buffer_count; i++)
    {
       result = PDL_OS::sendto (handle,
                                PDL_reinterpret_cast (char *PDL_CAST_CONST,
                                                      buffers[i].iov_base),
                                buffers[i].iov_len,
                                flags,
                                addr,
                                addrlen);
       if (result == -1)
         break;
       number_of_bytes_sent += result;
    }

  return result;
#endif /* defined (PDL_HAS_WINSOCK2) && (PDL_HAS_WINSOCK2 != 0) */
}

PDL_INLINE int
PDL_OS::getpeername (PDL_SOCKET handle, struct sockaddr *addr,
                     int *addrlen)
{
  PDL_TRACE ("PDL_OS::getpeername");
#if defined (PDL_PSOS) && !defined PDL_PSOS_DIAB_PPC
  PDL_SOCKCALL_RETURN (::getpeername ((PDL_SOCKET) handle,
                                      (struct sockaddr_in *) addr,
                                      (PDL_SOCKET_LEN *) addrlen),
                       int, -1);
#elif defined(ITRON)
  /* empty */
  PDL_NOTSUP_RETURN (-1);
#else
  PDL_SOCKCALL_RETURN (::getpeername ((PDL_SOCKET) handle,
                                      addr,
                                      (PDL_SOCKET_LEN *) addrlen),
                       int, -1);
#endif /* defined (PDL_PSOS) */
}

PDL_INLINE struct protoent *
PDL_OS::getprotobyname (const char *name)
{
#if defined (VXWORKS) || defined (PDL_HAS_WINCE) || (defined (ghs) && defined (__Chorus)) || defined (PDL_PSOS)
  PDL_UNUSED_ARG (name);
  PDL_NOTSUP_RETURN (0);
#elif defined (PDL_HAS_NONCONST_GETBY)
  PDL_SOCKCALL_RETURN (::getprotobyname ((char *) name),
                       struct protoent *, 0);
#elif defined(ITRON)
  /* empty */
  PDL_NOTSUP_RETURN (NULL);
#else
#  if !defined(CYGWIN32)
     PDL_SOCKCALL_RETURN (::getprotobyname (name), struct protoent *, 0);
#  else
     PDL_SOCKCALL_RETURN (getprotobyname (name), struct protoent *, 0);
#  endif
#endif /* VXWORKS */
}

PDL_INLINE struct protoent *
PDL_OS::getprotobyname_r (const char *name,
                          struct protoent *result,
                          PDL_PROTOENT_DATA buffer)
{
#if defined (VXWORKS) || defined (PDL_HAS_WINCE) || (defined (ghs) && defined (__Chorus)) || defined (PDL_PSOS)
  PDL_UNUSED_ARG (name);
  PDL_UNUSED_ARG (result);
  PDL_UNUSED_ARG (buffer);
  PDL_NOTSUP_RETURN (0);
#elif defined (PDL_HAS_REENTRANT_FUNCTIONS) && !defined (UNIXWARE)
# if defined (AIX) || defined (DIGITAL_UNIX) || defined (HPUX_10)
  if (::getprotobyname_r (name, result, (struct protoent_data *) buffer) == 0)
    return result;
  else
    return 0;
# else
#   if defined(PDL_LACKS_NETDB_REENTRANT_FUNCTIONS)
  PDL_UNUSED_ARG (result);
  PDL_NETDBCALL_RETURN (::getprotobyname (name),
                        struct protoent *, 0,
                        buffer, sizeof (PDL_PROTOENT_DATA));
#   else
    PDL_SOCKCALL_RETURN (::getprotobyname_r (name, result, buffer, sizeof (PDL_PROTOENT_DATA)),
                       struct protoent *, 0);
#   endif /* PDL_LACKS_NETDB_REENTRANT_FUNCTIONS */
# endif /* defined (AIX) || defined (DIGITAL_UNIX) */
#elif defined (PDL_HAS_NONCONST_GETBY)
  PDL_UNUSED_ARG (result);
  PDL_UNUSED_ARG (buffer);
  PDL_SOCKCALL_RETURN (::getprotobyname ((char *) name),
                       struct protoent *, 0);
#elif defined(ITRON)
  /* empty */
  PDL_NOTSUP_RETURN (NULL);
#else
  PDL_UNUSED_ARG (buffer);
  PDL_UNUSED_ARG (result);

#  if !defined(CYGWIN32)
     PDL_SOCKCALL_RETURN (::getprotobyname (name), struct protoent *, 0);
#  else
     PDL_SOCKCALL_RETURN (getprotobyname (name), struct protoent *, 0);
#  endif
#endif /* defined (PDL_HAS_REENTRANT_FUNCTIONS) !defined (UNIXWARE) */
}

PDL_INLINE struct protoent *
PDL_OS::getprotobynumber (int proto)
{
#if defined (VXWORKS) || defined (PDL_HAS_WINCE) || (defined (ghs) && defined (__Chorus)) || defined (PDL_PSOS)
  PDL_UNUSED_ARG (proto);
  PDL_NOTSUP_RETURN (0);
#elif defined(ITRON)
  /* empty */
  PDL_NOTSUP_RETURN (NULL);
#else
#  if !defined(CYGWIN32)
     PDL_SOCKCALL_RETURN (::getprotobynumber (proto), struct protoent *, 0);
#  else
     PDL_SOCKCALL_RETURN (getprotobynumber (proto), struct protoent *, 0);
#  endif
#endif /* VXWORKS */
}

PDL_INLINE struct protoent *
PDL_OS::getprotobynumber_r (int proto,
                            struct protoent *result,
                            PDL_PROTOENT_DATA buffer)
{
#if defined (VXWORKS) || defined (PDL_HAS_WINCE) || (defined (ghs) && defined (__Chorus)) || defined (PDL_PSOS)
  PDL_UNUSED_ARG (proto);
  PDL_UNUSED_ARG (result);
  PDL_UNUSED_ARG (buffer);
  PDL_NOTSUP_RETURN (0);
#elif defined (PDL_HAS_REENTRANT_FUNCTIONS) && !defined (UNIXWARE)
# if defined (AIX) || defined (DIGITAL_UNIX) || defined (HPUX_10)
  if (::getprotobynumber_r (proto, result, (struct protoent_data *) buffer) == 0)
    return result;
  else
    return 0;
# else
#   if defined(PDL_LACKS_NETDB_REENTRANT_FUNCTIONS)
  PDL_UNUSED_ARG (result);
  PDL_NETDBCALL_RETURN (::getprotobynumber (proto),
                        struct protoent *, 0,
                        buffer, sizeof (PDL_PROTOENT_DATA));
#   else
  PDL_SOCKCALL_RETURN (::getprotobynumber_r (proto, result, buffer, sizeof (PDL_PROTOENT_DATA)),
                       struct protoent *, 0);
#   endif /* PDL_LACKS_NETDB_REENTRANT_FUNCTIONS */
# endif /* defined (AIX) || defined (DIGITAL_UNIX) */
#elif defined(ITRON)
  /* empty */
  PDL_NOTSUP_RETURN (NULL);
#else
  PDL_UNUSED_ARG (buffer);
  PDL_UNUSED_ARG (result);

#  if !defined(CYGWIN32)
     PDL_SOCKCALL_RETURN (::getprotobynumber (proto), struct protoent *, 0);
#  else
     PDL_SOCKCALL_RETURN (getprotobynumber (proto), struct protoent *, 0);
#  endif
#endif /* defined (PDL_HAS_REENTRANT_FUNCTIONS) && !defined (UNIXWARE) */
}

PDL_INLINE struct servent *
PDL_OS::getservbyname (const char *svc, const char *proto)
{
  PDL_TRACE ("PDL_OS::getservbyname");
#if defined (PDL_LACKS_GETSERVBYNAME)
  PDL_UNUSED_ARG (svc);
  PDL_UNUSED_ARG (proto);
  PDL_NOTSUP_RETURN (0);
#elif defined (PDL_HAS_NONCONST_GETBY)
  PDL_SOCKCALL_RETURN (::getservbyname ((char *) svc, (char *) proto),
                       struct servent *, 0);
#else
  PDL_SOCKCALL_RETURN (::getservbyname (svc, proto),
                       struct servent *, 0);
#endif /* PDL_HAS_NONCONST_GETBY */
}

PDL_INLINE int
PDL_OS::getsockname (PDL_SOCKET handle,
                     struct sockaddr *addr,
                     int *addrlen)
{
  PDL_TRACE ("PDL_OS::getsockname");
#if defined (PDL_PSOS) && !defined (PDL_PSOS_DIAB_PPC)
  PDL_SOCKCALL_RETURN (::getsockname ((PDL_SOCKET) handle, (struct sockaddr_in *) addr,
                                      (PDL_SOCKET_LEN *) addrlen),
                       int, -1);
#elif defined(ITRON)
  /* empty */
  PDL_NOTSUP_RETURN (-1);
#else
  PDL_SOCKCALL_RETURN (::getsockname ((PDL_SOCKET) handle, addr, (PDL_SOCKET_LEN *) addrlen),
                       int, -1);
#endif /* defined (PDL_PSOS) */
}

PDL_INLINE int
PDL_OS::getsockopt (PDL_SOCKET handle,
                    int level,
                    int optname,
                    char *optval,
                    int *optlen)
{
  PDL_TRACE ("PDL_OS::getsockopt");
#if defined(ITRON)
  /* empty */
  PDL_NOTSUP_RETURN (-1);
#else
  PDL_SOCKCALL_RETURN (::getsockopt ((PDL_SOCKET) handle, level, optname, optval, (PDL_SOCKET_LEN *) optlen),
                       int, -1);
#endif
}

PDL_INLINE int
PDL_OS::listen (PDL_SOCKET handle, int backlog)
{
  PDL_TRACE ("PDL_OS::listen");
#if defined(ITRON)
  /* empty */
  PDL_NOTSUP_RETURN (-1);
#else
  PDL_SOCKCALL_RETURN (::listen ((PDL_SOCKET) handle, backlog), int, -1);
#endif
}

PDL_INLINE int
PDL_OS::setsockopt (PDL_SOCKET handle, int level, int optname,
                    const char *optval, int optlen)
{
  PDL_TRACE ("PDL_OS::setsockopt");
#if defined(ITRON)
  /* empty */
  PDL_NOTSUP_RETURN (-1);
#else
  PDL_SOCKCALL_RETURN (::setsockopt ((PDL_SOCKET) handle, level, optname,
                                     (PDL_SOCKOPT_TYPE1) optval, optlen),
                       int, -1);
#endif
}

PDL_INLINE int
PDL_OS::shutdown (PDL_SOCKET handle, int how)
{
  PDL_TRACE ("PDL_OS::shutdown");
#if defined(ITRON)
  /* empty */
  PDL_NOTSUP_RETURN (-1);
#else
  PDL_SOCKCALL_RETURN (::shutdown ((PDL_SOCKET) handle, how), int, -1);
#endif
}

PDL_INLINE PDL_SOCKET
PDL_OS::socket (int domain,
                int type,
                int proto)
{
  PDL_TRACE ("PDL_OS::socket");

#if defined(ITRON)
  /* empty */
  PDL_NOTSUP_RETURN (-1);
#else
  PDL_SOCKCALL_RETURN (::socket (domain,
                                 type,
                                 proto),
                       PDL_SOCKET,
                       PDL_INVALID_SOCKET);
#endif
}

PDL_INLINE PDL_SOCKET
PDL_OS::socket (int domain,
                int type,
                int proto,
                PDL_Protocol_Info *protocolinfo,
                PDL_SOCK_GROUP g,
                u_long flags)
{
  PDL_TRACE ("PDL_OS::socket");

#if defined (PDL_HAS_WINSOCK2) && (PDL_HAS_WINSOCK2 != 0)
  PDL_SOCKCALL_RETURN (::WSASocket (domain,
                                    type,
                                    proto,
                                    protocolinfo,
                                    g,
                                    flags),
                       PDL_SOCKET,
                       PDL_INVALID_SOCKET);
#else
  PDL_UNUSED_ARG (protocolinfo);
  PDL_UNUSED_ARG (g);
  PDL_UNUSED_ARG (flags);

  return PDL_OS::socket (domain,
                         type,
                         proto);
#endif /* PDL_HAS_WINSOCK2 */
}

PDL_INLINE int
PDL_OS::atoi (const char *s)
{
  PDL_TRACE ("PDL_OS::atoi");
  PDL_OSCALL_RETURN (::atoi (s), int, -1);
}

PDL_INLINE int
PDL_OS::recvmsg (PDL_SOCKET handle, struct msghdr *msg, int flags)
{
  PDL_TRACE ("PDL_OS::recvmsg");
#if !defined (PDL_LACKS_RECVMSG)
# if (defined (PDL_HAS_WINSOCK2) && (PDL_HAS_WINSOCK2 != 0))
  DWORD bytes_received = 0;

  int result = ::WSARecvFrom ((SOCKET) handle,
                              (WSABUF *) msg->msg_iov,
                              msg->msg_iovlen,
                              &bytes_received,
                              (DWORD *) &flags,
                              msg->msg_name,
                              &msg->msg_namelen,
                              0,
                              0);

  if (result != 0)
    {
      PDL_OS::set_errno_to_last_error ();
      return -1;
    }
  else
    return (ssize_t) bytes_received;
# elif defined(ITRON)
  /* empty */
  PDL_NOTSUP_RETURN (-1);
# else /* PDL_HAS_WINSOCK2 */
  PDL_SOCKCALL_RETURN (::recvmsg (handle, msg, flags), int, -1);
# endif /* PDL_HAS_WINSOCK2 */
#else
  PDL_UNUSED_ARG (flags);
  PDL_UNUSED_ARG (msg);
  PDL_UNUSED_ARG (handle);

  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_LACKS_RECVMSG */
}

PDL_INLINE int
PDL_OS::sendmsg (PDL_SOCKET handle,
                 const struct msghdr *msg,
                 int flags)
{
  PDL_TRACE ("PDL_OS::sendmsg");
  int result;
  again:

#if !defined (PDL_LACKS_SENDMSG)
# if (defined (PDL_HAS_WINSOCK2) && (PDL_HAS_WINSOCK2 != 0))
  DWORD bytes_sent = 0;
  result = ::WSASendTo ((SOCKET) handle,
                            (WSABUF *) msg->msg_iov,
                            msg->msg_iovlen,
                            &bytes_sent,
                            flags,
                            msg->msg_name,
                            msg->msg_namelen,
                            0,
                            0);

  if (result != 0)
    {
      PDL_OS::set_errno_to_last_error ();
      return -1;
    }
  else
    return (ssize_t) bytes_sent;
# elif defined (PDL_LACKS_POSIX_PROTOTYPES) ||  defined (PDL_PSOS)
  result =  ::sendmsg (handle, (struct msghdr *) msg, flags);

# elif defined(ITRON)
  /* empty */
  PDL_NOTSUP_RETURN (-1);
# else
  result = ::sendmsg (handle, (PDL_SENDMSG_TYPE *) msg, flags);

# endif /* PDL_LACKS_POSIX_PROTOTYPES */
#else
  PDL_UNUSED_ARG (flags);
  PDL_UNUSED_ARG (msg);
  PDL_UNUSED_ARG (handle);

  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_LACKS_SENDMSG */
  if (result == -1 && errno == EINTR)
  {
      goto again;
  }
  return result;

}

PDL_INLINE int
PDL_OS::fclose (FILE *fp)
{
#if !defined (PDL_HAS_WINCE)
  PDL_TRACE ("PDL_OS::fclose");
  PDL_OSCALL_RETURN (::fclose (fp), int, -1);
#else
  BOOL sResult = ::fclose (fp);
  if (sResult == EOF)
  {
      errno = GetLastError();
      return -1;
  }

    return 0;
#endif /* !PDL_HAS_WINCE */
}

#if !defined (PDL_HAS_WINCE)
// Here are functions that CE doesn't support at all.
// Notice that some of them might have UNICODE version.
PDL_INLINE char *
PDL_OS::fgets (char *buf, int size, FILE *fp)
{
  PDL_TRACE ("PDL_OS::fgets");
  PDL_OSCALL_RETURN (::fgets (buf, size, fp), char *, 0);
}

#if !defined (PDL_WIN32)
// Win32 implementation of fopen(const char*, const char*)
// is in OS.cpp.
PDL_INLINE FILE *
PDL_OS::fopen (const char *filename, const char *mode)
{
  PDL_TRACE ("PDL_OS::fopen");
  PDL_OSCALL_RETURN (::fopen (filename, mode), FILE *, 0);
}
#endif /* PDL_WIN32 */
#endif /* PDL_HAS_WINCE */

PDL_INLINE int
PDL_OS::fflush (FILE *fp)
{
  PDL_TRACE ("PDL_OS::fflush");
  PDL_OSCALL_RETURN (::fflush (fp), int, -1);
}

PDL_INLINE size_t
PDL_OS::fread (void *ptr, size_t size, size_t nelems, FILE *fp)
{
  PDL_TRACE ("PDL_OS::fread");
#if defined (PDL_HAS_WINCE)

  size_t sReadByte = ::fread (ptr, size, nelems, fp);
  if((size * nelems) != sReadByte)
  {
    if(ferror(fp) != 0)
	{
	  errno = GetLastError();
	}
  }

  return sReadByte;

#elif defined (PDL_LACKS_POSIX_PROTOTYPES)
  PDL_OSCALL_RETURN (::fread ((char *) ptr, size, nelems, fp), int, 0);
#else
  PDL_OSCALL_RETURN (::fread (ptr, size, nelems, fp), int, 0);
#endif /* PDL_LACKS_POSIX_PROTOTYPES */
}

PDL_INLINE size_t
PDL_OS::fwrite (const void *ptr, size_t size, size_t nitems, FILE *fp)
{
  PDL_TRACE ("PDL_OS::fwrite");
#if defined (PDL_HAS_WINCE)
  size_t sWriteByte = ::fwrite (ptr, size, nitems, fp);
  if((size * nitems) != sWriteByte)
  {
    if(ferror(fp) != 0)
	{
	  errno = GetLastError();
	}
  }

  return sWriteByte;

#elif defined (PDL_LACKS_POSIX_PROTOTYPES)
  PDL_OSCALL_RETURN (::fwrite ((const char *) ptr, size, nitems, fp), int, 0);
#else
  PDL_OSCALL_RETURN (::fwrite (ptr, size, nitems, fp), int, 0);
#endif /* PDL_LACKS_POSIX_PROTOTYPES */
}

#if !defined (PDL_HAS_WINCE)
PDL_INLINE int
PDL_OS::truncate (const char *filename,
                  PDL_OFF_T offset)
{
  PDL_TRACE ("PDL_OS::truncate");
#if defined (PDL_WIN32)
  PDL_HANDLE handle = PDL_OS::open (filename,
                                    O_WRONLY,
                                    PDL_DEFAULT_FILE_PERMS);
  if (handle == PDL_INVALID_HANDLE)
  {
      PDL_FAIL_RETURN (-1);
  }
  else
  {
      LARGE_INTEGER li;
      li.QuadPart = offset;
      if (0xFFFFFFFF == ::SetFilePointer(handle, li.LowPart, &li.HighPart, FILE_BEGIN) &&
          GetLastError() != NO_ERROR)
      {
          PDL_FAIL_RETURN (-1);
      }
      else
      {
          PDL_WIN32CALL_RETURN (PDL_ADAPT_RETVAL (::SetEndOfFile (handle),
                                                  pdl_result_), int, -1);
      }
  }
  /* NOTREACHED */
#elif !defined (PDL_LACKS_TRUNCATE)
  PDL_OSCALL_RETURN (::truncate (filename, offset), int, -1);
#else
  PDL_UNUSED_ARG (filename);
  PDL_UNUSED_ARG (offset);
  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_WIN32 */
}
#endif /* PDL_HAS_WINCE */

// Accessors to PWD file.

PDL_INLINE struct passwd *
PDL_OS::getpwnam (const char *name)
{
#if !defined (PDL_LACKS_PWD_FUNCTIONS)
# if !defined (PDL_WIN32)
  return ::getpwnam (name);
# else
  PDL_UNUSED_ARG (name);
  PDL_NOTSUP_RETURN (0);
# endif /* PDL_WIN32 */
#else
  PDL_UNUSED_ARG (name);
  PDL_NOTSUP_RETURN (0);
#endif /* ! PDL_LACKS_PWD_FUNCTIONS */
}

PDL_INLINE void
PDL_OS::setpwent (void)
{
#if !defined (PDL_LACKS_PWD_FUNCTIONS)
# if !defined (PDL_WIN32)
  ::setpwent ();
# else
# endif /* PDL_WIN32 */
#else
#endif /* ! PDL_LACKS_PWD_FUNCTIONS */
}

PDL_INLINE void
PDL_OS::endpwent (void)
{
#if !defined (PDL_LACKS_PWD_FUNCTIONS)
# if !defined (PDL_WIN32)
  ::endpwent ();
# else
# endif /* PDL_WIN32 */
#else
#endif /* ! PDL_LACKS_PWD_FUNCTIONS */
}

PDL_INLINE struct passwd *
PDL_OS::getpwent (void)
{
#if !defined (PDL_LACKS_PWD_FUNCTIONS)
# if !defined (PDL_WIN32)
  return ::getpwent ();
# else
  PDL_NOTSUP_RETURN (0);
# endif /* PDL_WIN32 */
#else
  PDL_NOTSUP_RETURN (0);
#endif /* ! PDL_LACKS_PWD_FUNCTIONS */
}

PDL_INLINE struct passwd *
PDL_OS::getpwnam_r (const char *name, struct passwd *pwent,
                    char *buffer, int buflen)
{
#if !defined (PDL_LACKS_PWD_FUNCTIONS)
# if defined (PDL_HAS_REENTRANT_FUNCTIONS)
#   if !defined (PDL_LACKS_PWD_REENTRANT_FUNCTIONS)
#     if defined (PDL_HAS_PTHREADS_STD) && \
      !defined (PDL_HAS_STHREADS) || \
      defined (__USLC__) // Added by Roland Gigler for SCO UnixWare 7.
  struct passwd *result;
  int status;
#       if defined (DIGITAL_UNIX)
  ::_Pgetpwnam_r (name, pwent, buffer, buflen, &result);
#       else
  status = ::getpwnam_r (name, pwent, buffer, buflen, &result);
  if (status != 0)
    {
      errno = status;
      result = 0;
    }
#       endif /* (DIGITAL_UNIX) */
  return result;
#     elif defined (AIX) || defined (HPUX_10)
  if (::getpwnam_r (name, pwent, buffer, buflen) == -1)
    return 0;
  else
    return pwent;
#     else
  return ::getpwnam_r (name, pwent, buffer, buflen);
#     endif /* PDL_HAS_PTHREADS_STD */
#   else
  PDL_UNUSED_ARG (name);
  PDL_UNUSED_ARG (pwent);
  PDL_UNUSED_ARG (buffer);
  PDL_UNUSED_ARG (buflen);
  PDL_NOTSUP_RETURN (0);
#   endif /* ! PDL_LACKS_PWD_REENTRANT_FUNCTIONS */
# else
  PDL_UNUSED_ARG (name);
  PDL_UNUSED_ARG (pwent);
  PDL_UNUSED_ARG (buffer);
  PDL_UNUSED_ARG (buflen);
  PDL_NOTSUP_RETURN (0);
# endif /* PDL_HAS_REENTRANT_FUNCTIONS */
#else
  PDL_UNUSED_ARG (name);
  PDL_UNUSED_ARG (pwent);
  PDL_UNUSED_ARG (buffer);
  PDL_UNUSED_ARG (buflen);
  PDL_NOTSUP_RETURN (0);
#endif /* ! PDL_LACKS_PWD_FUNCTIONS */
}

// DNS accessors.

#if !defined (VXWORKS)
PDL_INLINE struct hostent *
PDL_OS::gethostbyaddr_r (const char *addr, int length, int type,
                         hostent *result, PDL_HOSTENT_DATA buffer,
                         int *h_errnop)
{
  PDL_TRACE ("PDL_OS::gethostbyaddr_r");
# if defined (PDL_PSOS)
  PDL_UNUSED_ARG (addr);
  PDL_UNUSED_ARG (length);
  PDL_UNUSED_ARG (type);
  PDL_UNUSED_ARG (result);
  PDL_UNUSED_ARG (buffer);
  PDL_UNUSED_ARG (h_errnop);
  PDL_NOTSUP_RETURN (0);
# elif defined (PDL_HAS_REENTRANT_FUNCTIONS) && !defined (UNIXWARE)
#   if defined (AIX) || defined (DIGITAL_UNIX) || defined (HPUX_10)
  ::memset (buffer, 0, sizeof (PDL_HOSTENT_DATA));

  if (::gethostbyaddr_r ((char *) addr, length, type, result,
                         (struct hostent_data *) buffer)== 0)
    return result;
  else
    {
      *h_errnop = h_errno;
      return (struct hostent *) 0;
    }
#   else
#     if defined(PDL_LACKS_NETDB_REENTRANT_FUNCTIONS)
  PDL_UNUSED_ARG (result);
  PDL_UNUSED_ARG (h_errnop);
  PDL_NETDBCALL_RETURN (::gethostbyaddr (addr, (PDL_SOCKET_LEN) length, type),
                        struct hostent *, 0,
                        buffer, sizeof (PDL_HOSTENT_DATA));
#     else
  PDL_SOCKCALL_RETURN (::gethostbyaddr_r (addr, length, type, result,
                                          buffer, sizeof (PDL_HOSTENT_DATA),
                                          h_errnop),
                       struct hostent *, 0);
#     endif /* PDL_LACKS_NETDB_REENTRANT_FUNCTIONS */
#   endif /* defined (AIX) || defined (DIGITAL_UNIX) */
# elif defined (PDL_HAS_NONCONST_GETBY)
  PDL_UNUSED_ARG (result);
  PDL_UNUSED_ARG (buffer);
  PDL_UNUSED_ARG (h_errnop);
  PDL_SOCKCALL_RETURN (::gethostbyaddr ((char *) addr, (PDL_SOCKET_LEN) length, type),
                       struct hostent *, 0);
# elif defined(ITRON)
  /* empty */
  PDL_NOTSUP_RETURN (NULL);
# else
  PDL_UNUSED_ARG (h_errnop);
  PDL_UNUSED_ARG (buffer);
  PDL_UNUSED_ARG (result);

#  if !defined(CYGWIN32)
     PDL_SOCKCALL_RETURN (::gethostbyaddr (addr, (PDL_SOCKET_LEN) length, type),
                          struct hostent *, 0);
#  else
     PDL_SOCKCALL_RETURN (gethostbyaddr (addr, (PDL_SOCKET_LEN) length, type),
                          struct hostent *, 0);
#  endif
# endif /* defined (PDL_HAS_REENTRANT_FUNCTIONS) && !defined (UNIXWARE) */
}

PDL_INLINE struct hostent *
PDL_OS::gethostbyname_r (const char *name, hostent *result,
                         PDL_HOSTENT_DATA buffer,
                         int *h_errnop)
{
  PDL_TRACE ("PDL_OS::gethostbyname_r");
#if defined (PDL_PSOS)
  PDL_UNUSED_ARG (name);
  PDL_UNUSED_ARG (result);
  PDL_UNUSED_ARG (buffer);
  PDL_UNUSED_ARG (h_errnop);
  PDL_NOTSUP_RETURN (0);
# elif defined (PDL_HAS_REENTRANT_FUNCTIONS) && !defined (UNIXWARE)
#   if defined (DIGITAL_UNIX) || \
       (defined (PDL_AIX_MINOR_VERS) && (PDL_AIX_MINOR_VERS > 2))
  PDL_UNUSED_ARG (result);
  PDL_UNUSED_ARG (buffer);
  PDL_UNUSED_ARG (h_errnop);

  // gethostbyname returns thread-specific storage on Digital Unix and
  // AIX 4.3
  PDL_SOCKCALL_RETURN (::gethostbyname (name), struct hostent *, 0);
#   elif defined (AIX) || defined (HPUX_10)
  ::memset (buffer, 0, sizeof (PDL_HOSTENT_DATA));

  if (::gethostbyname_r (name, result, (struct hostent_data *) buffer) == 0)
    return result;
  else
    {
      *h_errnop = h_errno;
      return (struct hostent *) 0;
    }
#   else
#     if defined(PDL_LACKS_NETDB_REENTRANT_FUNCTIONS)
  PDL_UNUSED_ARG (result);
  PDL_UNUSED_ARG (h_errnop);
  PDL_NETDBCALL_RETURN (::gethostbyname (name),
                        struct hostent *, 0,
                        buffer, sizeof (PDL_HOSTENT_DATA));
#     else
  PDL_SOCKCALL_RETURN (::gethostbyname_r (name, result, buffer,
                                          sizeof (PDL_HOSTENT_DATA), h_errnop),
                       struct hostent *, 0);
#     endif /* PDL_LACKS_NETDB_REENTRANT_FUNCTIONS */
#   endif /* defined (AIX) || defined (DIGITAL_UNIX) */
# elif defined (PDL_HAS_NONCONST_GETBY)
  PDL_UNUSED_ARG (result);
  PDL_UNUSED_ARG (buffer);
  PDL_UNUSED_ARG (h_errnop);
  PDL_SOCKCALL_RETURN (::gethostbyname ((char *) name), struct hostent *, 0);
#elif defined(ITRON)
  /* empty */
  PDL_NOTSUP_RETURN (NULL);
# else
  PDL_UNUSED_ARG (result);
  PDL_UNUSED_ARG (buffer);
  PDL_UNUSED_ARG (h_errnop);

#  if !defined(CYGWIN32)
     PDL_SOCKCALL_RETURN (::gethostbyname (name), struct hostent *, 0);
#  else
     PDL_SOCKCALL_RETURN (gethostbyname (name), struct hostent *, 0);
#  endif
# endif /* defined (PDL_HAS_REENTRANT_FUNCTIONS) && !defined (UNIXWARE) */
}
#endif /* ! VXWORKS */

#if 0
// @@ gets is evil anyway.
//    and it is *** DEPRECATED *** now.  If you
//    really needs gets, use PDL_OS::gets (char*, int)
//    instead.
PDL_INLINE char *
PDL_OS::gets (char *str)
{
  PDL_TRACE ("PDL_OS::gets");
  PDL_OSCALL_RETURN (::gets (str), char *, 0);
}
#endif /* 0 */

PDL_INLINE struct servent *
PDL_OS::getservbyname_r (const char *svc, const char *proto,
                         struct servent *result, PDL_SERVENT_DATA buf)
{
  PDL_TRACE ("PDL_OS::getservbyname_r");
#if defined (PDL_LACKS_GETSERVBYNAME)
  PDL_UNUSED_ARG (svc);
  PDL_UNUSED_ARG (proto);
  PDL_UNUSED_ARG (result);
  PDL_UNUSED_ARG (buf);
  PDL_NOTSUP_RETURN (0);
#elif defined (PDL_HAS_REENTRANT_FUNCTIONS) && !defined (UNIXWARE)
# if defined (AIX) || defined (DIGITAL_UNIX) || defined (HPUX_10)
  ::memset (buf, 0, sizeof (PDL_SERVENT_DATA));

  if (::getservbyname_r (svc, proto, result, (struct servent_data *) buf) == 0)
    return result;
  else
    return (struct servent *) 0;
# else
#   if defined(PDL_LACKS_NETDB_REENTRANT_FUNCTIONS)
  PDL_UNUSED_ARG (result);
  PDL_NETDBCALL_RETURN (::getservbyname (svc, proto),
                        struct servent *, 0,
                        buf, sizeof (PDL_SERVENT_DATA));
#   else
  PDL_SOCKCALL_RETURN (::getservbyname_r (svc, proto, result, buf,
                                          sizeof (PDL_SERVENT_DATA)),
                       struct servent *, 0);
#   endif /* PDL_LACKS_NETDB_REENTRANT_FUNCTIONS */
# endif /* defined (AIX) || defined (DIGITAL_UNIX) */
#elif defined (PDL_HAS_NONCONST_GETBY)
  PDL_UNUSED_ARG (buf);
  PDL_UNUSED_ARG (result);
  PDL_SOCKCALL_RETURN (::getservbyname ((char *) svc, (char *) proto),
                       struct servent *, 0);
#else
  PDL_UNUSED_ARG (buf);
  PDL_UNUSED_ARG (result);

  PDL_SOCKCALL_RETURN (::getservbyname (svc, proto),
                       struct servent *, 0);
#endif /* defined (PDL_HAS_REENTRANT_FUNCTIONS) && !defined (UNIXWARE) */
}

/*
  fix BUG-28834 IP Access List가 잘못되었다. 
 inet_addr함수가  in_addr_t를 return하는 데
 in_addr_t는 unsigned int이다.
 return value는 BigEndian이다.
 따라서 return long은 잘못된 것이다.
 */
PDL_INLINE unsigned int 
PDL_OS::inet_addr (const char *name)
{
  PDL_TRACE ("PDL_OS::inet_addr");
#if defined (VXWORKS) || defined (PDL_PSOS)
#ifndef INADDR_NONE
#define INADDR_NONE     (-1)
#endif
  u_long ret = 0;
  u_int segment;
  u_int valid = 1;

  for (u_int i = 0; i < 4; ++i)
    {
      ret <<= 8;
      if (*name != '\0')
        {
          segment = 0;

          while (*name >= '0'  &&  *name <= '9')
            {
              segment *= 10;
              segment += *name++ - '0';
            }
          if (*name != '.' && *name != '\0')
            {
              valid = 0;
              break;
            }

          ret |= segment;

          if (*name == '.')
            {
              ++name;
            }
        }
    }
  return valid ?  htonl (ret) : INADDR_NONE;
#elif defined (PDL_HAS_NONCONST_GETBY)
  return ::inet_addr ((char *) name);
#else
  return ::inet_addr (name);
#endif /* PDL_HAS_NONCONST_GETBY */
}

PDL_INLINE char *
PDL_OS::inet_ntoa (const struct in_addr addr)
{
  PDL_TRACE ("PDL_OS::inet_ntoa");
#if defined (PDL_PSOS)
  PDL_UNUSED_ARG (addr);
  PDL_NOTSUP_RETURN (0);
#else
  PDL_OSCALL_RETURN (::inet_ntoa (addr), char *, 0);
#endif /* defined (PDL_PSOS) */
}

PDL_INLINE int
PDL_OS::inet_pton (int family, const char *strptr, void *addrptr)
{
  PDL_TRACE ("PDL_OS::inet_pton");

#if defined (PDL_HAS_IP6)

#if defined (PDL_WIN32)

  DWORD dwRetval = 0;
  int   ret = 0;

  struct addrinfo *result = NULL;
  struct addrinfo hints;

  struct sockaddr_in *v4 = NULL;
  struct sockaddr_in6 *v6 = NULL;

  ::ZeroMemory(&hints, sizeof(hints));
  hints.ai_family = family;
    
  dwRetval = ::getaddrinfo(strptr, NULL, &hints, &result);
    
  if (dwRetval != 0) 
  {
    return 0; //Error
  }

  switch(family)
  {
    case AF_INET:

      v4 = (struct sockaddr_in *)result->ai_addr;
      ::memcpy(addrptr, (void *)&v4->sin_addr ,sizeof(struct in_addr));
      ret = 1;

    break;

    case AF_INET6:

        v6 = (struct sockaddr_in6 *)result->ai_addr;
        ::memcpy(addrptr, (void *)&v6->sin6_addr ,sizeof(struct in6_addr));
        ret = 1; 

    break;
          
    default:
       ret = -1; //Error
       ::WSASetLastError(WSAEAFNOSUPPORT);
  }

  ::freeaddrinfo(result);
 
  return ret;

#else
  PDL_OSCALL_RETURN (::inet_pton (family, strptr, addrptr), int, -1);
#endif

#else
  if (family == AF_INET)
    {
      struct in_addr in_val;

      if (PDL_OS::inet_aton (strptr, &in_val))
        {
          PDL_OS::memcpy (addrptr, &in_val, sizeof (struct in_addr));
          return 1; // Success
        }

      return 0; // Input is not a valid presentation format
    }

  PDL_NOTSUP_RETURN(-1);
#endif  /* PDL_HAS_IP6 */
}

PDL_INLINE const char *
PDL_OS::inet_ntop (int family, const void *addrptr, char *strptr, size_t len)
{
  PDL_TRACE ("PDL_OS::inet_ntop");

#if defined (PDL_HAS_IP6)

#if defined (PDL_WIN32)

  int ret = 0;
  struct sockaddr_in in;
  struct sockaddr_in6 in6;

  if (family == AF_INET)
  {
    ::memset(&in, 0, sizeof(in));

    in.sin_family = AF_INET;

    ::memcpy(&in.sin_addr, addrptr, sizeof(struct in_addr));
 
    ret = ::getnameinfo((struct sockaddr *)&in, 
                        sizeof(struct sockaddr_in), 
                        strptr, 
                        (DWORD)len, 
                        NULL, 0, NI_NUMERICHOST);

    if (ret != 0)
    {
      return NULL; //Error
    } 

  } 
  else if (family == AF_INET6)
  {
    ::memset(&in6, 0, sizeof(in6));

    in6.sin6_family = AF_INET6;

    ::memcpy(&in6.sin6_addr, addrptr, sizeof(struct in_addr6));

    ret = ::getnameinfo((struct sockaddr *)&in6, 
                        sizeof(struct sockaddr_in6), 
                        strptr, 
                        (DWORD)len, 
                        NULL, 0, NI_NUMERICHOST);
    if (ret != 0)
    {
      return NULL; //Error
    }
  }

  return strptr;

#else

  PDL_OSCALL_RETURN (::inet_ntop (family, addrptr, strptr, len), const char *, 0);

#endif

#else
  const u_char *p =
    PDL_reinterpret_cast (const u_char *, addrptr);

  if (family == AF_INET)
    {
      char temp[INET_ADDRSTRLEN];

      // Stevens uses snprintf() in his implementation but snprintf()
      // doesn't appear to be very portable.  For now, hope that using
      // sprintf() will not cause any string/memory overrun problems.
      PDL_OS::sprintf (temp,
                       "%d.%d.%d.%d",
                       p[0], p[1], p[2], p[3]);

      if (PDL_OS::strlen (temp) >= len)
        {
          errno = ENOSPC;
          return 0; // Failure
        }

      PDL_OS::strcpy (strptr, temp);
      return strptr;
    }

  PDL_NOTSUP_RETURN(0);
#endif /* PDL_HAS_IP6 */
}

PDL_INLINE int
PDL_OS::set_errno_to_last_error (void)
{
# if defined (PDL_WIN32)
// Borland C++ Builder 4 has a bug in the RTL that resets the
// <GetLastError> value to zero when errno is accessed.  Thus, we have
// to use this to set errno to GetLastError.  It's bad, but only for
// WIN32
#   if defined(__BORLANDC__) && (__BORLANDC__ == 0x540)
  int last_error = ::GetLastError ();
  return errno = last_error;
#   else /* defined(__BORLANDC__) && (__BORLANDC__ == 0x540) */
  return errno = ::GetLastError ();
#   endif /* defined(__BORLANDC__) && (__BORLANDC__ == 0x540) */
#else
  return errno;
# endif /* defined(PDL_WIN32) */
}

PDL_INLINE int
PDL_OS::set_errno_to_wsa_last_error (void)
{
# if defined (PDL_WIN32)
// Borland C++ Builder 4 has a bug in the RTL that resets the
// <GetLastError> value to zero when errno is accessed.  Thus, we have
// to use this to set errno to GetLastError.  It's bad, but only for
// WIN32
#   if defined(__BORLANDC__) && (__BORLANDC__ == 0x540)
  int last_error = ::WSAGetLastError ();
  return errno = last_error;
#   else /* defined(__BORLANDC__) && (__BORLANDC__ == 0x540) */
  return errno = (int)::WSAGetLastError ();
#   endif /* defined(__BORLANDC__) && (__BORLANDC__ == 0x540) */
#else
  return errno;
# endif /* defined(PDL_WIN32) */
}

PDL_INLINE int
PDL_OS::last_error (void)
{
  // PDL_TRACE ("PDL_OS::last_error");

#if defined (PDL_WIN32)
  int lerror = ::GetLastError ();
  int lerrno = errno;
  return lerrno == 0 ? lerror : lerrno;
#else
  return errno;
#endif /* PDL_WIN32 */
}

PDL_INLINE void
PDL_OS::last_error (int error)
{
  PDL_TRACE ("PDL_OS::last_error");
#if defined (PDL_WIN32)
  ::SetLastError (error);
#else
  errno = error;
#endif /* PDL_WIN32 */
}

#if !defined (PDL_HAS_WINCE)
PDL_INLINE void
PDL_OS::perror (const char *s)
{
  PDL_TRACE ("PDL_OS::perror");

#if defined(DEBUG)
  assert(!bDaemonPritable);
#endif

  ::perror (s);
}
#endif /* ! PDL_HAS_WINCE */

// @@ Do we need to implement puts on WinCE???
PDL_INLINE int
PDL_OS::puts (const char *s)
{
  PDL_TRACE ("PDL_OS::puts");

#if defined(DEBUG)
  assert( !bDaemonPritable );
#endif

  PDL_OSCALL_RETURN (::puts (s), int, -1);
}

PDL_INLINE int
PDL_OS::fputs (const char *s, FILE *stream)
{
  PDL_TRACE ("PDL_OS::puts");

#if defined(DEBUG)
  if (bDaemonPritable)
  {
      assert(stream != stdout && stream != stderr);
  }
#endif

  PDL_OSCALL_RETURN (::fputs (s, stream), int, -1);
}

PDL_INLINE PDL_SignalHandler
PDL_OS::signal (int signum, PDL_SignalHandler func)
{
  if (signum == 0)
    return 0;
  else
#if defined (PDL_PSOS) && !defined (PDL_PSOS_TM) && !defined (PDL_PSOS_DIAB_MIPS) && !defined (PDL_PSOS_DIAB_PPC)
    return (PDL_SignalHandler) ::signal (signum, (void (*)(void)) func);
#elif defined (PDL_PSOS_DIAB_MIPS) || defined (PDL_PSOS_DIAB_PPC)
    return 0;
#elif defined (PDL_PSOS_TM)
    // @@ It would be good to rework this so the PDL_PSOS_TM specific
    //    branch is not needed, but prying it out of PDL_LACKS_UNIX_SIGNALS
    //    will take some extra work - deferred for now.
    return (PDL_SignalHandler) ::signal (signum, (void (*)(int)) func);
#elif defined (PDL_WIN32) && !defined (PDL_HAS_WINCE) || !defined (PDL_LACKS_UNIX_SIGNALS)
#  if !defined (PDL_HAS_TANDEM_SIGNALS) && !defined (PDL_HAS_LYNXOS_SIGNALS)
    return ::signal (signum, func);
#  else
    return (PDL_SignalHandler) ::signal (signum, (void (*)(int)) func);
#  endif /* !PDL_HAS_TANDEM_SIGNALS */
#else
    // @@ Don't know how to implement signal on WinCE (yet.)
    PDL_UNUSED_ARG (signum);
    PDL_UNUSED_ARG (func);
    PDL_NOTSUP_RETURN (0);     // Should return SIG_ERR but it is not defined on WinCE.
#endif /* ! PDL_HAS_WINCE */
}

PDL_INLINE int
PDL_OS::system (const char *s)
{
  // PDL_TRACE ("PDL_OS::system");
#if !defined (CHORUS) && !defined (PDL_HAS_WINCE) && !defined(PDL_PSOS)
    // PDL_TRACE ("PDL_OS::system");
    PDL_OSCALL_RETURN (::system (s), int, -1);
#else
    PDL_UNUSED_ARG (s);
    PDL_NOTSUP_RETURN (-1);
#endif /* !CHORUS */
}

PDL_INLINE int
PDL_OS::thr_continue (PDL_hthread_t target_thread)
{
  PDL_TRACE ("PDL_OS::thr_continue");
#if defined (PDL_HAS_THREADS)
# if defined (PDL_HAS_STHREADS)
  PDL_OSCALL_RETURN (PDL_ADAPT_RETVAL (::thr_continue (target_thread), pdl_result_), int, -1);
# elif defined (PDL_HAS_PTHREADS)
#  if defined (PDL_HAS_PTHREADS_UNIX98_EXT)
  PDL_OSCALL_RETURN (PDL_ADAPT_RETVAL (pthread_continue (target_thread),
                                       pdl_result_),
                     int, -1);
#  else
  PDL_UNUSED_ARG (target_thread);
  PDL_NOTSUP_RETURN (-1);
#  endif /* PDL_HAS_PTHREADS_UNIX98_EXT */
# elif defined (PDL_HAS_WTHREADS)
  DWORD result = ::ResumeThread (target_thread);
  if (result == PDL_SYSCALL_FAILED)
    PDL_FAIL_RETURN (-1);
  else
    return 0;
# elif defined (PDL_PSOS)
  PDL_OSCALL_RETURN (PDL_ADAPT_RETVAL (::t_resume (target_thread), pdl_result_), int, -1);
# elif defined (VXWORKS)
  PDL_OSCALL_RETURN (::taskResume (target_thread.id), int, -1);
# endif /* PDL_HAS_STHREADS */
#else
  PDL_UNUSED_ARG (target_thread);
  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_HAS_THREADS */
}

PDL_INLINE int
PDL_OS::thr_cmp (PDL_hthread_t t1, PDL_hthread_t t2)
{
#if defined (PDL_HAS_PTHREADS)
# if defined (pthread_equal)
  // If it's a macro we can't say "::pthread_equal"...
  return pthread_equal (t1, t2);
# else
  return ::pthread_equal (t1, t2);
# endif /* pthread_equal */
#else /* For STHREADS, WTHREADS, and VXWORKS ... */
#if defined (VXWORKS)
  return t1.id == t2.id;
#else
  // Hum, Do we need to treat WTHREAD differently?
  // levine 13 oct 98 % Probably, PDL_hthread_t is a HANDLE.
  return t1 == t2;
#endif
#endif /* Threads variety case */
}

PDL_INLINE int
PDL_OS::thr_getconcurrency (void)
{
  PDL_TRACE ("PDL_OS::thr_getconcurrency");
#if defined (PDL_HAS_THREADS)
# if defined (PDL_HAS_STHREADS)
  return ::thr_getconcurrency ();
# elif defined (PDL_HAS_PTHREADS) || defined (VXWORKS) || defined (PDL_PSOS)
  PDL_NOTSUP_RETURN (-1);
# elif defined (PDL_HAS_WTHREADS)
  PDL_NOTSUP_RETURN (-1);
# endif /* PDL_HAS_STHREADS */
#else
  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_HAS_THREADS */
}

PDL_INLINE int
PDL_OS::thr_getprio (PDL_hthread_t thr_id, int &prio)
{
  PDL_TRACE ("PDL_OS::thr_getprio");
#if defined (PDL_HAS_THREADS)
# if defined (PDL_HAS_STHREADS)
  PDL_OSCALL_RETURN (PDL_ADAPT_RETVAL (::thr_getprio (thr_id, &prio), pdl_result_), int, -1);
# elif (defined (PDL_HAS_PTHREADS) && !defined (PDL_LACKS_SETSCHED))

#   if defined (PDL_HAS_PTHREADS_DRAFT4)
  int result;
  result = ::pthread_getprio (thr_id);
  if (result != -1)
    {
      prio = result;
      return 0;
    }
  else
    return -1;
#   elif defined (PDL_HAS_PTHREADS_DRAFT6)

  pthread_attr_t  attr;
  if (pthread_getschedattr (thr_id, &attr) == 0)
    {
      prio = pthread_attr_getprio(&attr);
      return 0;
    }
  return -1;
#   else

  struct sched_param param;
  int result;
  int policy = 0;

  PDL_OSCALL (PDL_ADAPT_RETVAL (::pthread_getschedparam (thr_id, &policy, &param),
                                result), int,
              -1, result);
  prio = param.sched_priority;
  return result;
#   endif /* PDL_HAS_PTHREADS_DRAFT4 */
# elif defined (PDL_HAS_WTHREADS)
  prio = ::GetThreadPriority (thr_id);
  if (prio == THREAD_PRIORITY_ERROR_RETURN)
    PDL_FAIL_RETURN (-1);
  else
    return 0;
# elif defined (PDL_PSOS)
  // passing a 0 in the second argument does not alter task priority, third arg gets existing one
  PDL_OSCALL_RETURN (PDL_ADAPT_RETVAL (::t_setpri (thr_id, 0, (u_long *) &prio), pdl_result_), int, -1);
# elif defined (VXWORKS)
  PDL_OSCALL_RETURN (::taskPriorityGet (thr_id.id, &prio), int, -1);
# else
  PDL_UNUSED_ARG (thr_id);
  PDL_UNUSED_ARG (prio);
  PDL_NOTSUP_RETURN (-1);
# endif /* PDL_HAS_STHREADS */
#else
  PDL_UNUSED_ARG (thr_id);
  PDL_UNUSED_ARG (prio);
  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_HAS_THREADS */
}


#if defined (PDL_HAS_TSS_EMULATION)

#if defined (PDL_HAS_THREAD_SPECIFIC_STORAGE)
PDL_INLINE int
PDL_OS::thr_getspecific (PDL_OS_thread_key_t key, void **data)
{
  PDL_TRACE ("PDL_OS::thr_getspecific");
#if defined (PDL_HAS_THREADS)
# if defined (PDL_HAS_STHREADS)
    PDL_OSCALL_RETURN (PDL_ADAPT_RETVAL (::thr_getspecific (key, data), pdl_result_), int, -1);
# elif defined (PDL_HAS_PTHREADS)
#   if defined (PDL_HAS_PTHREADS_DRAFT4) || defined (PDL_HAS_PTHREADS_DRAFT6)
    return pthread_getspecific (key, data);
#   else /* this is PDL_HAS_PTHREADS_DRAFT7 or STD */
#if (pthread_getspecific)
    // This is a macro on some platforms, e.g., CHORUS!
    *data = pthread_getspecific (key);
#else
    *data = pthread_getspecific (key);
#endif /* pthread_getspecific */
#   endif       /*  PDL_HAS_PTHREADS_DRAFT4, 6 */
    return 0;
#   elif defined (PDL_PSOS) && defined (PDL_PSOS_HAS_TSS)
    PDL_hthread_t tid;
    PDL_OS::thr_self (tid);
    return (::tsd_getval (key, tid, data) == 0) ? 0 : -1;
# elif defined (PDL_HAS_WTHREADS)

  // The following handling of errno is designed like this due to
  // PDL_Log_Msg::instance calling PDL_OS::thr_getspecific.
  // Basically, it is ok for a system call to reset the error to zero.
  // (It really shouldn't, though).  However, we have to remember to
  // store errno *immediately* after an error is detected.  Calling
  // PDL_ERROR_RETURN((..., errno)) did not work because errno was
  // cleared before being passed to the thread-specific instance of
  // PDL_Log_Msg.  The workaround for was to make it so
  // thr_getspecific did not have the side effect of clearing errno.
  // The correct fix is for PDL_ERROR_RETURN to store errno
  //(actually PDL_OS::last_error) before getting the PDL_Log_Msg tss
  // pointer, which is how it is implemented now.  However, other uses
  // of PDL_Log_Msg may not work correctly, so we're keeping this as
  // it is for now.

  PDL_Errno_Guard error ((PDL_ERRNO_TYPE&)errno);
  *data = ::TlsGetValue (key);
#   if !defined (PDL_HAS_WINCE)
  if (*data == 0 && (error = ::GetLastError ()) != NO_ERROR)
    return -1;
  else
#   endif /* PDL_HAS_WINCE */
    return 0;
# endif /* PDL_HAS_STHREADS */
#else
  PDL_UNUSED_ARG (key);
  PDL_UNUSED_ARG (data);
  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_HAS_THREADS */
}
#endif /* PDL_HAS_THREAD_SPECIFIC_STORAGE */

#if !defined (PDL_HAS_THREAD_SPECIFIC_STORAGE)
PDL_INLINE
void **&
PDL_TSS_Emulation::tss_base ()
{
# if defined (VXWORKS)
  return (void **&) taskIdCurrent->PDL_VXWORKS_SPARE;
# elif defined (PDL_PSOS)
  // not supported
  long x=0;   //JINLU
  return (void **&) x;
#elif defined(ITRON)
  long y=0;   //JINLU
  PDL_NOTSUP_RETURN ((void **&) y);
# else
  // Uh oh.
  PDL_NOTSUP_RETURN (0);
# endif /* VXWORKS */
}
#endif /* ! PDL_HAS_THREAD_SPECIFIC_STORAGE */

PDL_INLINE
PDL_TSS_Emulation::PDL_TSS_DESTRUCTOR
PDL_TSS_Emulation::tss_destructor (const PDL_thread_key_t key)
{
  PDL_KEY_INDEX (key_index, key);
  return tss_destructor_ [key_index];
}

PDL_INLINE
void
PDL_TSS_Emulation::tss_destructor (const PDL_thread_key_t key,
                                   PDL_TSS_DESTRUCTOR destructor)
{
  PDL_KEY_INDEX (key_index, key);
  tss_destructor_ [key_index] = destructor;
}

PDL_INLINE
void *&
PDL_TSS_Emulation::ts_object (const PDL_thread_key_t key)
{
  PDL_KEY_INDEX (key_index, key);

#if defined (PDL_PSOS)
  u_long tss_base;
  t_getreg (0, PSOS_TASK_REG_TSS, &tss_base);
  return ((void **) tss_base)[key_index];
#else
# if defined (VXWORKS)
    /* If someone wants tss_base make sure they get one.  This
       gets used if someone spawns a VxWorks task directly, not
       through PDL.  The allocated array will never be deleted! */
    if (0 == taskIdCurrent->PDL_VXWORKS_SPARE)
      {
        taskIdCurrent->PDL_VXWORKS_SPARE =
          PDL_reinterpret_cast (int, new void *[PDL_TSS_THREAD_KEYS_MAX]);

        // Zero the entire TSS array.  Do it manually instead of using
        // memset, for optimum speed.  Though, memset may be faster :-)
        void **tss_base_p =
          PDL_reinterpret_cast (void **, taskIdCurrent->PDL_VXWORKS_SPARE);
        for (u_int i = 0; i < PDL_TSS_THREAD_KEYS_MAX; ++i, ++tss_base_p)
          {
            *tss_base_p = 0;
          }
      }
#     endif /* VXWORKS */

  return tss_base ()[key_index];
#endif /* defined (PDL_PSOS) */
}

#endif /* PDL_HAS_TSS_EMULATION */


PDL_INLINE int
PDL_OS::thr_getspecific (PDL_thread_key_t key, void **data)
{
  // PDL_TRACE ("PDL_OS::thr_getspecific");
#if defined (PDL_HAS_THREADS)
# if defined (PDL_HAS_TSS_EMULATION)
    PDL_KEY_INDEX (key_index, key);
    if (key_index >= PDL_TSS_Emulation::total_keys ())
      {
        errno = EINVAL;
        data = 0;
        return -1;
      }
    else
      {
        *data = PDL_TSS_Emulation::ts_object (key);
        return 0;
      }
# elif defined (PDL_HAS_STHREADS)
    PDL_OSCALL_RETURN (PDL_ADAPT_RETVAL (::thr_getspecific (key, data), pdl_result_), int, -1);
# elif defined (PDL_HAS_PTHREADS)
#   if defined (PDL_HAS_PTHREADS_DRAFT4) || defined (PDL_HAS_PTHREADS_DRAFT6)
      return ::pthread_getspecific (key, data);
#   else /* this is Draft 7 or STD */
      *data = pthread_getspecific (key);
      return 0;
#   endif       /*  PDL_HAS_PTHREADS_DRAFT4, 6 */
# elif defined (PDL_HAS_WTHREADS)

  // The following handling of errno is designed like this due to
  // PDL_Log_Msg::instance calling PDL_OS::thr_getspecific.
  // Basically, it is ok for a system call to reset the error to zero.
  // (It really shouldn't, though).  However, we have to remember to
  // store errno *immediately* after an error is detected.  Calling
  // PDL_ERROR_RETURN((..., errno)) did not work because errno was
  // cleared before being passed to the thread-specific instance of
  // PDL_Log_Msg.  The workaround for was to make it so
  // thr_getspecific did not have the side effect of clearing errno.
  // The correct fix is for PDL_ERROR_RETURN to store errno
  //(actually PDL_OS::last_error) before getting the PDL_Log_Msg tss
  // pointer, which is how it is implemented now.  However, other uses
  // of PDL_Log_Msg may not work correctly, so we're keeping this as
  // it is for now.

  PDL_Errno_Guard error ((PDL_ERRNO_TYPE&)errno);
  *data = ::TlsGetValue (key);
#   if !defined (PDL_HAS_WINCE)
  if (*data == 0 && (error = ::GetLastError ()) != NO_ERROR)

    return -1;
  else
#   endif /* PDL_HAS_WINCE */
    return 0;
# else
  PDL_UNUSED_ARG (key);
  PDL_UNUSED_ARG (data);
  PDL_NOTSUP_RETURN (-1);
# endif /* PDL_HAS_STHREADS */
#else
  PDL_UNUSED_ARG (key);
  PDL_UNUSED_ARG (data);
  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_HAS_THREADS */
}

PDL_INLINE int
PDL_OS::thr_join (PDL_hthread_t thr_handle,
                  void **status)
{
  PDL_TRACE ("PDL_OS::thr_join");
#if defined(ITRON)
  /* empty */
  PDL_NOTSUP_RETURN (0);
#else
#if defined (PDL_HAS_THREADS)
# if defined (PDL_HAS_STHREADS)
  PDL_OSCALL_RETURN (PDL_ADAPT_RETVAL (::thr_join (thr_handle, 0, status), pdl_result_),
                     int, -1);
# elif defined (PDL_HAS_PTHREADS)
#   if defined (PDL_HAS_PTHREADS_DRAFT4) || defined (PDL_HAS_PTHREADS_DRAFT6)
#     if defined (PDL_LACKS_NULL_PTHREAD_STATUS)
  void *temp;
  PDL_OSCALL_RETURN (::pthread_join (thr_handle,
    status == 0  ?  &temp  :  status), int, -1);
#     else
  PDL_OSCALL_RETURN (::pthread_join (thr_handle, status), int, -1);
#     endif
#   else
  PDL_OSCALL_RETURN (PDL_ADAPT_RETVAL (::pthread_join (thr_handle, status), pdl_result_),
                     int, -1);
#   endif /* PDL_HAS_PTHREADS_DRAFT4, 6 */
# elif defined (PDL_HAS_WTHREADS)
  void *local_status = 0;

  // Make sure that status is non-NULL.
  if (status == 0)
    status = &local_status;

  if (::WaitForSingleObject (thr_handle, INFINITE) == WAIT_OBJECT_0
      && ::GetExitCodeThread (thr_handle, (LPDWORD) status) != FALSE)
    {
      ::CloseHandle (thr_handle);
      return 0;
    }
  PDL_FAIL_RETURN (-1);
  /* NOTREACHED */
# elif defined (VXWORKS) || defined (PDL_PSOS)
  PDL_Time_Value a;
  a.initialize(0,100000);
  while( (::taskIdVerify(thr_handle.id) == OK) &&
         (strcmp(::taskTcb(thr_handle.id)->name, thr_handle.name) == 0) )
  {
    PDL_OS::sleep(a);
  }
  return 0;
# else
  PDL_UNUSED_ARG (thr_handle);
  PDL_UNUSED_ARG (status);
  PDL_NOTSUP_RETURN (-1);
# endif /* PDL_HAS_STHREADS */
#else
  PDL_UNUSED_ARG (thr_handle);
  PDL_UNUSED_ARG (status);
  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_HAS_THREADS */
#endif
}


PDL_INLINE int
PDL_OS::thr_setcancelstate (int new_state, int *old_state)
{
  PDL_TRACE ("PDL_OS::thr_setcancelstate");
#if defined (PDL_HAS_THREADS)
# if defined (PDL_HAS_PTHREADS) && !defined (PDL_LACKS_PTHREAD_CANCEL)
#   if defined (PDL_HAS_PTHREADS_DRAFT4)
  int old;
  old = pthread_setcancel (new_state);
  if (old == -1)
    return -1;
  *old_state = old;
  return 0;
#   elif defined (PDL_HAS_PTHREADS_DRAFT6)
  PDL_UNUSED_ARG(old_state);
  PDL_OSCALL_RETURN (pthread_setintr (new_state), int, -1);
#   else /* this is draft 7 or std */
  PDL_OSCALL_RETURN (PDL_ADAPT_RETVAL (::pthread_setcancelstate (new_state,
                                                                 old_state),
                                       pdl_result_),
                     int, -1);
#   endif /* PDL_HAS_PTHREADS_DRAFT4 */
# elif defined (PDL_HAS_STHREADS)
  PDL_UNUSED_ARG (new_state);
  PDL_UNUSED_ARG (old_state);
  PDL_NOTSUP_RETURN (-1);
# elif defined (PDL_HAS_WTHREADS)
/*BUGBUG_NT*/
/*
  PDL_UNUSED_ARG (new_state);
  PDL_UNUSED_ARG (old_state);
  PDL_NOTSUP_RETURN (-1);
*/
  return 0;
/*BUGBUG_NT*/
# else /* Could be PDL_HAS_PTHREADS && PDL_LACKS_PTHREAD_CANCEL */
  PDL_UNUSED_ARG (new_state);
  PDL_UNUSED_ARG (old_state);
  PDL_NOTSUP_RETURN (-1);
# endif /* PDL_HAS_PTHREADS */
#else
  PDL_UNUSED_ARG (new_state);
  PDL_UNUSED_ARG (old_state);
  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_HAS_THREADS */
}

PDL_INLINE int
PDL_OS::thr_setcanceltype (int new_type, int *old_type)
{
  PDL_TRACE ("PDL_OS::thr_setcanceltype");
#if defined (PDL_HAS_THREADS)
# if defined (PDL_HAS_PTHREADS) && !defined (PDL_LACKS_PTHREAD_CANCEL)
#   if defined (PDL_HAS_PTHREADS_DRAFT4)
  int old;
  old = pthread_setasynccancel(new_type);
  if (old == -1)
    return -1;
  *old_type = old;
  return 0;
#   elif defined (PDL_HAS_PTHREADS_DRAFT6)
  PDL_UNUSED_ARG(old_type);
  PDL_OSCALL_RETURN (pthread_setintrtype (new_type), int, -1);
#   else /* this is draft 7 or std */
  PDL_OSCALL_RETURN (PDL_ADAPT_RETVAL (::pthread_setcanceltype (new_type,
                                                                old_type),
                                       pdl_result_),
                     int, -1);
#   endif /* PDL_HAS_PTHREADS_DRAFT4 */
# else /* Could be PDL_HAS_PTHREADS && PDL_LACKS_PTHREAD_CANCEL */
/*BUGBUG_NT*/
  #if defined(PDL_WIN32)
	return 0;
  #else
    PDL_UNUSED_ARG (new_type);
    PDL_UNUSED_ARG (old_type);
    PDL_NOTSUP_RETURN (-1);
  #endif
/*BUGBUG_NT*/
# endif /* PDL_HAS_PTHREADS */
#else
  PDL_UNUSED_ARG (new_type);
  PDL_UNUSED_ARG (old_type);
  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_HAS_THREADS */
}

PDL_INLINE int
PDL_OS::thr_cancel (PDL_thread_t thr_id)
{
  PDL_TRACE ("PDL_OS::thr_cancel");
#if defined (PDL_HAS_THREADS)
# if defined (PDL_HAS_PTHREADS) && !defined (PDL_LACKS_PTHREAD_CANCEL)
#   if defined (PDL_HAS_PTHREADS_DRAFT4) || defined (PDL_HAS_PTHREADS_DRAFT6)
  PDL_OSCALL_RETURN (::pthread_cancel (thr_id), int, -1);
#   else
  PDL_OSCALL_RETURN (PDL_ADAPT_RETVAL (::pthread_cancel (thr_id),
                                       pdl_result_),
                     int, -1);
#   endif /* PDL_HAS_PTHREADS_DRAFT4 || PDL_HAS_PTHREADS_DRAFT6 */
# else /* Could be PDL_HAS_PTHREADS && PDL_LACKS_PTHREAD_CANCEL */
  PDL_UNUSED_ARG (thr_id);
  PDL_NOTSUP_RETURN (-1);
# endif /* PDL_HAS_PTHREADS */
#else
  PDL_UNUSED_ARG (thr_id);
  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_HAS_THREADS */
}

PDL_INLINE int
PDL_OS::sigwait (sigset_t *set, int *sig)
{
  PDL_TRACE ("PDL_OS::sigwait");
  int local_sig;
  if (sig == 0)
    sig = &local_sig;
#if defined (PDL_HAS_THREADS)
# if (defined (__FreeBSD__) && (__FreeBSD__ < 3)) || defined (CHORUS) || defined (PDL_PSOS)
    PDL_UNUSED_ARG (set);
    PDL_NOTSUP_RETURN (-1);
# elif (defined (PDL_HAS_STHREADS) && !defined (_POSIX_PTHREAD_SEMANTICS))
    *sig = ::sigwait (set);
    return *sig;
# elif defined (PDL_HAS_PTHREADS)
  // LynxOS and Digital UNIX have their own hoops to jump through.
#   if defined (__Lynx__)
    // Second arg is a void **, which we don't need (the selected
    // signal number is returned).
    *sig = ::sigwait (set, 0);
    return *sig;
#   elif defined (DIGITAL_UNIX)  &&  defined (__DECCXX_VER)
      // DEC cxx (but not g++) needs this direct call to its internal
      // sigwait ().  This allows us to #undef sigwait, so that we can
      // have PDL_OS::sigwait.  cxx gets confused by PDL_OS::sigwait
      // if sigwait is _not_ #undef'ed.
      errno = ::_Psigwait (set, sig);
      return errno == 0  ?  *sig  :  -1;
#   else /* ! __Lynx __ && ! (DIGITAL_UNIX && __DECCXX_VER) */
#     if (defined (PDL_HAS_PTHREADS_DRAFT4) || (defined (PDL_HAS_PTHREADS_DRAFT6)) && !defined(PDL_HAS_FSU_PTHREADS)) || (defined (_UNICOS) && _UNICOS == 9)
#       if defined (HPUX_10)
        *sig = cma_sigwait (set);
#       else
        *sig = ::sigwait (set);
#       endif  /* HPUX_10 */
        return *sig;
#     elif defined(PDL_HAS_FSU_PTHREADS)
        return ::sigwait (set, sig);
#     else   /* this is draft 7 or std */
#      if !defined(CYGWIN32)
        errno = ::sigwait (set, sig);
        return errno == 0  ?  *sig  :  -1;
#      else
        // cygwin2.340.25 version
        // maybe after this, it'll be fixed
        return 0;
#      endif
#     endif /* PDL_HAS_PTHREADS_DRAFT4, 6 */
#   endif /* ! __Lynx__ && ! (DIGITAL_UNIX && __DECCXX_VER) */
# elif defined (PDL_HAS_WTHREADS)
    PDL_UNUSED_ARG (set);
    PDL_NOTSUP_RETURN (-1);
# elif defined (VXWORKS)
    // Second arg is a struct siginfo *, which we don't need (the
    // selected signal number is returned).  Third arg is timeout:  0
    // means forever.
    *sig = ::sigtimedwait (set, 0, 0);
    return *sig;
# endif /* __FreeBSD__ */
#else
    PDL_UNUSED_ARG (set);
    PDL_UNUSED_ARG (sig);
    PDL_NOTSUP_RETURN (-1);
#endif /* PDL_HAS_THREADS */
}

PDL_INLINE int
PDL_OS::sigtimedwait (const sigset_t *set,
                      siginfo_t *info,
                      const PDL_Time_Value *timeout)
{
  PDL_TRACE ("PDL_OS::sigtimedwait");
#if defined(ITRON)
  /* empty */
  PDL_NOTSUP_RETURN (-1);
#else
#if defined (PDL_HAS_SIGTIMEDWAIT)
  timespec_t ts;
  timespec_t *tsp;

  if (timeout != 0)
    {
      ts = *timeout; // Calls PDL_Time_Value::operator timespec_t().
      tsp = &ts;
    }
  else
    tsp = 0;

  PDL_OSCALL_RETURN (::sigtimedwait (set, info, tsp),
                     int, -1);
#else
    PDL_UNUSED_ARG (set);
    PDL_UNUSED_ARG (info);
    PDL_UNUSED_ARG (timeout);
    PDL_NOTSUP_RETURN (-1);
#endif /* PDL_HAS_THREADS */
#endif
}

PDL_INLINE void
PDL_OS::thr_testcancel (void)
{
  PDL_TRACE ("PDL_OS::thr_testcancel");
#if defined (PDL_HAS_THREADS)
# if defined (PDL_HAS_PTHREADS) && !defined (PDL_LACKS_PTHREAD_CANCEL)
#if defined(PDL_HAS_PTHREADS_DRAFT6)
  ::pthread_testintr ();
#else /* PDL_HAS_PTHREADS_DRAFT6 */
  ::pthread_testcancel ();
#endif /* !PDL_HAS_PTHREADS_DRAFT6 */
# elif defined (PDL_HAS_STHREADS)
# elif defined (PDL_HAS_WTHREADS)
# elif defined (VXWORKS) || defined (PDL_PSOS)
# else
  // no-op:  can't use PDL_NOTSUP_RETURN because there is no return value
# endif /* PDL_HAS_PTHREADS */
#else
#endif /* PDL_HAS_THREADS */
}

PDL_INLINE int
PDL_OS::thr_sigsetmask (int how,
                        const sigset_t *nsm,
                        sigset_t *osm)
{
  PDL_TRACE ("PDL_OS::thr_sigsetmask");
#if defined (PDL_HAS_THREADS)
# if defined (PDL_LACKS_PTHREAD_THR_SIGSETMASK)
  // DCE threads and Solaris 2.4 have no such function.
  PDL_UNUSED_ARG (osm);
  PDL_UNUSED_ARG (nsm);
  PDL_UNUSED_ARG (how);

  PDL_NOTSUP_RETURN (-1);
# elif defined (PDL_HAS_SIGTHREADMASK)
  PDL_OSCALL_RETURN (PDL_ADAPT_RETVAL (::sigthreadmask (how, nsm, osm),
                                       pdl_result_), int, -1);
# elif defined (PDL_HAS_STHREADS)
  PDL_OSCALL_RETURN (PDL_ADAPT_RETVAL (::thr_sigsetmask (how, nsm, osm),
                                       pdl_result_),
                     int, -1);
# elif defined (PDL_HAS_PTHREADS)
#   if defined (AIX)
  PDL_OSCALL_RETURN (sigthreadmask (how, nsm, osm), int, -1);
  // Draft 4 and 6 implementations will sometimes have a sigprocmask () that
  // modifies the calling thread's mask only.  If this is not so for your
  // platform, define PDL_LACKS_PTHREAD_THR_SIGSETMASK.
#   elif defined(PDL_HAS_PTHREADS_DRAFT4) || \
    defined (PDL_HAS_PTHREADS_DRAFT6) || (defined (_UNICOS) && _UNICOS == 9)
  PDL_OSCALL_RETURN (::sigprocmask (how, nsm, osm), int, -1);
#   else
  PDL_OSCALL_RETURN (PDL_ADAPT_RETVAL (::pthread_sigmask (how, nsm, osm),
                                       pdl_result_), int, -1);
#   endif /* AIX */

#if 0
  /* Don't know if anyt platform actually needs this... */
  // as far as I can tell, this is now pthread_sigaction() -- jwr
  PDL_OSCALL_RETURN (PDL_ADAPT_RETVAL (::pthread_sigaction (how, nsm, osm),
                                       pdl_result_), int, -1);
#endif /* 0 */

# elif defined (PDL_HAS_WTHREADS) || defined (PDL_PSOS)
  PDL_UNUSED_ARG (osm);
  PDL_UNUSED_ARG (nsm);
  PDL_UNUSED_ARG (how);

  PDL_NOTSUP_RETURN (-1);
# elif defined (VXWORKS)
  switch (how)
    {
    case SIG_BLOCK:
    case SIG_UNBLOCK:
      {
        // get the old mask
        *osm = ::sigsetmask (*nsm);
        // create a new mask:  the following assumes that sigset_t is 4 bytes,
        // which it is on VxWorks 5.2, so bit operations are done simply . . .
        ::sigsetmask (how == SIG_BLOCK ? (*osm |= *nsm) : (*osm &= ~*nsm));
        break;
      }
    case SIG_SETMASK:
      *osm = ::sigsetmask (*nsm);
      break;
    default:
      return -1;
    }

  return 0;
# else /* Should not happen. */
  PDL_UNUSED_ARG (how);
  PDL_UNUSED_ARG (nsm);
  PDL_UNUSED_ARG (osm);
  PDL_NOTSUP_RETURN (-1);
# endif /* PDL_LACKS_PTHREAD_THR_SIGSETMASK */
#else
  PDL_UNUSED_ARG (how);
  PDL_UNUSED_ARG (nsm);
  PDL_UNUSED_ARG (osm);
  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_HAS_THREADS */
}

PDL_INLINE int
PDL_OS::thr_kill (PDL_thread_t thr_id, int signum)
{
  PDL_TRACE ("PDL_OS::thr_kill");
#if defined (PDL_HAS_THREADS)
# if defined (PDL_HAS_PTHREADS)
#   if defined (PDL_HAS_PTHREADS_DRAFT4) || defined(PDL_LACKS_PTHREAD_KILL)
  PDL_UNUSED_ARG (signum);
  PDL_UNUSED_ARG (thr_id);
  PDL_NOTSUP_RETURN (-1);
#   else
  PDL_OSCALL_RETURN (PDL_ADAPT_RETVAL (::pthread_kill (thr_id, signum),
                                       pdl_result_),
                     int, -1);
#   endif /* PDL_HAS_PTHREADS_DRAFT4 */
# elif defined (PDL_HAS_STHREADS)
  PDL_OSCALL_RETURN (PDL_ADAPT_RETVAL (::thr_kill (thr_id, signum),
                                       pdl_result_),
                     int, -1);
# elif defined (PDL_HAS_WTHREADS) || defined (PDL_PSOS)
  PDL_UNUSED_ARG (signum);
  PDL_UNUSED_ARG (thr_id);

  PDL_NOTSUP_RETURN (-1);
# elif defined (VXWORKS)
  PDL_hthread_t tid;
  PDL_OSCALL (::taskNameToId (thr_id), int, ERROR, tid.id);

  if (tid.id == ERROR)
    return -1;
  else
    PDL_OSCALL_RETURN (::kill (tid.id, signum), int, -1);

# else /* This should not happen! */
  PDL_UNUSED_ARG (thr_id);
  PDL_UNUSED_ARG (signum);
  PDL_NOTSUP_RETURN (-1);
# endif /* PDL_HAS_STHREADS */
#else
  PDL_UNUSED_ARG (thr_id);
  PDL_UNUSED_ARG (signum);
  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_HAS_THREADS */
}

PDL_INLINE size_t
PDL_OS::thr_min_stack (void)
{
  PDL_TRACE ("PDL_OS::thr_min_stack");
#if defined (PDL_HAS_THREADS)
# if defined (PDL_HAS_STHREADS)
#   if defined (PDL_HAS_THR_MINSTACK)
  // Tandem did some weirdo mangling of STHREAD names...
  PDL_OSCALL_RETURN (PDL_ADAPT_RETVAL (::thr_minstack (),
                                       pdl_result_),
                     int, -1);
#   else
  PDL_OSCALL_RETURN (PDL_ADAPT_RETVAL (::thr_min_stack (),
                                       pdl_result_),
                     int, -1);
#   endif /* !PDL_HAS_THR_MINSTACK */
# elif defined (PDL_HAS_PTHREADS)
#   if defined (_SC_THREAD_STACK_MIN)
  return (size_t) PDL_OS::sysconf (_SC_THREAD_STACK_MIN);
#   elif defined (PTHREAD_STACK_MIN)
  return PTHREAD_STACK_MIN;
#   else
  PDL_NOTSUP_RETURN (0);
#   endif /* _SC_THREAD_STACK_MIN */
# elif defined (PDL_HAS_WTHREADS)
  PDL_NOTSUP_RETURN (0);
# elif defined (PDL_PSOS)
  // there does not appear to be a way to get the
  // task stack size except at task creation
  PDL_NOTSUP_RETURN (0);
# elif defined (VXWORKS)
  TASK_DESC taskDesc;
  STATUS status;

  PDL_hthread_t tid;
  PDL_OS::thr_self (tid);

  PDL_OSCALL (PDL_ADAPT_RETVAL (::taskInfoGet (tid.id, &taskDesc),
                                status),
              STATUS, -1, status);
  return status == OK ? taskDesc.td_stackSize : 0;
# else /* Should not happen... */
  PDL_NOTSUP_RETURN (0);
# endif /* PDL_HAS_STHREADS */
#else
  PDL_NOTSUP_RETURN (0);
#endif /* PDL_HAS_THREADS */
}

PDL_INLINE int
PDL_OS::thr_setconcurrency (int hint)
{
  PDL_TRACE ("PDL_OS::thr_setconcurrency");
#if defined (PDL_HAS_THREADS)
# if defined (PDL_HAS_STHREADS)
  PDL_OSCALL_RETURN (PDL_ADAPT_RETVAL (::thr_setconcurrency (hint),
                                       pdl_result_),
                     int, -1);
# elif defined (PDL_HAS_PTHREADS)
  PDL_UNUSED_ARG (hint);
  PDL_NOTSUP_RETURN (-1);
# elif defined (PDL_HAS_WTHREADS)
  PDL_UNUSED_ARG (hint);

  PDL_NOTSUP_RETURN (-1);
# elif defined (VXWORKS) || defined (PDL_PSOS)
  PDL_UNUSED_ARG (hint);
  PDL_NOTSUP_RETURN (-1);
# endif /* PDL_HAS_STHREADS */
#else
  PDL_UNUSED_ARG (hint);
  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_HAS_THREADS */
}

PDL_INLINE int
PDL_OS::thr_setprio (PDL_hthread_t thr_id, int prio)
{
  PDL_TRACE ("PDL_OS::thr_setprio");
#if defined (PDL_HAS_THREADS)
# if defined (PDL_HAS_STHREADS)
  PDL_OSCALL_RETURN (PDL_ADAPT_RETVAL (::thr_setprio (thr_id, prio),
                                       pdl_result_),
                     int, -1);
# elif (defined (PDL_HAS_PTHREADS) && !defined (PDL_LACKS_SETSCHED))

#   if defined (PDL_HAS_PTHREADS_DRAFT4)
  int result;
  result = ::pthread_setprio(thr_id, prio);
  return (result == -1 ? -1 : 0);
#   elif defined (PDL_HAS_PTHREADS_DRAFT6)
  pthread_attr_t  attr;
  if (pthread_getschedattr(thr_id, &attr) == -1)
    return -1;
  if (pthread_attr_setprio (attr, prio) == -1)
    return -1;
  return pthread_setschedattr(thr_id, attr);
#   else
  struct sched_param param;
  int policy = 0;
  int result;

  PDL_OSCALL (PDL_ADAPT_RETVAL (::pthread_getschedparam (thr_id, &policy, &param),
                                result), // not sure if use of result here is cool, cjc
              int, -1, result);
  if (result == -1)
    return result; // error in pthread_getschedparam
  param.sched_priority = prio;
  PDL_OSCALL_RETURN (PDL_ADAPT_RETVAL (::pthread_setschedparam (thr_id, policy, &param),
                                       result),
                     int, -1);
#   endif /* PDL_HAS_PTHREADS_DRAFT4 */
# elif defined (PDL_HAS_WTHREADS)
  PDL_WIN32CALL_RETURN (PDL_ADAPT_RETVAL (::SetThreadPriority (thr_id, prio),
                                          pdl_result_),
                        int, -1);
# elif defined (PDL_PSOS)
  u_long oldprio;
  PDL_OSCALL_RETURN (PDL_ADAPT_RETVAL (::t_setpri (thr_id, prio, &oldprio),
                                       pdl_result_),
                     int, -1);
# elif defined (VXWORKS)
  PDL_OSCALL_RETURN (::taskPrioritySet (thr_id.id, prio), int, -1);
# else
  // For example, platforms that support Pthreads but LACK_SETSCHED.
  PDL_UNUSED_ARG (thr_id);
  PDL_UNUSED_ARG (prio);
  PDL_NOTSUP_RETURN (-1);
# endif /* PDL_HAS_STHREADS */
#else
  PDL_UNUSED_ARG (thr_id);
  PDL_UNUSED_ARG (prio);
  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_HAS_THREADS */
}

PDL_INLINE int
PDL_OS::thr_suspend (PDL_hthread_t target_thread)
{
  PDL_TRACE ("PDL_OS::thr_suspend");
#if defined (PDL_HAS_THREADS)
# if defined (PDL_HAS_STHREADS)
  PDL_OSCALL_RETURN (PDL_ADAPT_RETVAL (::thr_suspend (target_thread), pdl_result_), int, -1);
# elif defined (PDL_HAS_PTHREADS)
#  if defined (PDL_HAS_PTHREADS_UNIX98_EXT)
  PDL_OSCALL_RETURN (PDL_ADAPT_RETVAL (pthread_suspend (target_thread),
                                       pdl_result_),
                     int, -1);
#  else
  PDL_UNUSED_ARG (target_thread);
  PDL_NOTSUP_RETURN (-1);
#  endif /* PDL_HAS_PTHREADS_UNIX98_EXT */
# elif defined (PDL_HAS_WTHREADS)
  if (::SuspendThread (target_thread) != PDL_SYSCALL_FAILED)
    return 0;
  else
    PDL_FAIL_RETURN (-1);
  /* NOTREACHED */
# elif defined (PDL_PSOS)
  PDL_OSCALL_RETURN (PDL_ADAPT_RETVAL (::t_suspend (target_thread), pdl_result_), int, -1);
# elif defined (VXWORKS)
  PDL_OSCALL_RETURN (::taskSuspend (target_thread.id), int, -1);
# endif /* PDL_HAS_STHREADS */
#else
  PDL_UNUSED_ARG (target_thread);
  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_HAS_THREADS */
}

PDL_INLINE void
PDL_OS::thr_yield (void)
{
  PDL_TRACE ("PDL_OS::thr_yield");
#if defined (PDL_HAS_THREADS)
# if defined (PDL_HAS_STHREADS)
  ::thr_yield ();
# elif defined (PDL_HAS_PTHREADS)
#   if defined (PDL_HAS_PTHREADS_STD)
  // Note - this is a POSIX.4 function - not a POSIX.1c function...
  ::sched_yield ();
#   elif defined (PDL_HAS_PTHREADS_DRAFT6)
  ::pthread_yield (NULL);
#   else    /* Draft 4 and 7 */
  ::pthread_yield ();
#   endif  /*  PDL_HAS_IRIX62_THREADS */
# elif defined (PDL_HAS_WTHREADS)
  ::Sleep (0);
# elif defined (VXWORKS)
  // An argument of 0 to ::taskDelay doesn't appear to yield the
  // current thread.
  ::taskDelay (1);
# endif /* PDL_HAS_STHREADS */
#else
  ;
#endif /* PDL_HAS_THREADS */
}

PDL_INLINE int
PDL_OS::priority_control (PDL_idtype_t idtype, PDL_id_t id, int cmd, void *arg)
{
  PDL_TRACE ("PDL_OS::priority_control");
#if defined (PDL_HAS_PRIOCNTL)
  PDL_OSCALL_RETURN (priocntl (idtype, id, cmd, PDL_static_cast (caddr_t, arg)),
                     int, -1);
#else  /* ! PDL_HAS_PRIOCNTL*/
  PDL_UNUSED_ARG (idtype);
  PDL_UNUSED_ARG (id);
  PDL_UNUSED_ARG (cmd);
  PDL_UNUSED_ARG (arg);
  PDL_NOTSUP_RETURN (-1);
#endif /* ! PDL_HAS_PRIOCNTL*/
}

PDL_INLINE void
PDL_OS::rewind (FILE *fp)
{
#if !defined (PDL_HAS_WINCE)
  PDL_TRACE ("PDL_OS::rewind");
  ::rewind (fp);
#else
  // In WinCE, "FILE *" is actually a HANDLE.
  ::SetFilePointer (fp, 0L, 0L, FILE_BEGIN);
#endif /* ! PDL_HAS_WINCE */
}

PDL_INLINE ssize_t
PDL_OS::readv (PDL_HANDLE handle,
               const iovec *iov,
               int iovlen)
{
  PDL_TRACE ("PDL_OS::readv");
  ssize_t result;
  again:
#if defined(ITRON)
  /* empty */
  PDL_NOTSUP_RETURN (-1);
#else
  result = ::readv (handle, iov, iovlen);
#endif
  if (result == -1 && errno == EINTR)
  {
      goto again;
  }
  return result;
}

PDL_INLINE ssize_t
PDL_OS::writev (PDL_HANDLE handle,
                const iovec *iov,
                int iovcnt)
{
  PDL_TRACE ("PDL_OS::writev");
  ssize_t result;
  again:
#if defined(ITRON)
  /* empty */
  PDL_NOTSUP_RETURN (-1);
#else
  result = ::writev (handle, (PDL_WRITEV_TYPE *) iov, iovcnt);
#endif
  if (result == -1 && errno == EINTR)
  {
      goto again;
  }
  return result;
}

#if !defined(PDL_LACKS_PREADV)
PDL_INLINE ssize_t
PDL_OS::preadv (PDL_HANDLE handle,
                const iovec *iov,
                int iovlen,
                PDL_OFF_T offset)
{
  PDL_TRACE ("PDL_OS::readv");
  ssize_t result;
  again:
#if defined(ITRON)
  /* empty */
  PDL_NOTSUP_RETURN (-1);
#else
  result = ::preadv (handle, iov, iovlen, offset);
#endif
  if (result == -1 && errno == EINTR)
  {
      goto again;
  }
  return result;
}

PDL_INLINE ssize_t
PDL_OS::pwritev (PDL_HANDLE handle,
                 const iovec *iov,
                 int iovcnt,
                 PDL_OFF_T offset)
{
  PDL_TRACE ("PDL_OS::writev");
  ssize_t result;
  again:
#if defined(ITRON)
  /* empty */
  PDL_NOTSUP_RETURN (-1);
#else
  result = ::pwritev (handle, (PDL_WRITEV_TYPE *) iov, iovcnt, offset);
#endif
  if (result == -1 && errno == EINTR)
  {
      goto again;
  }
  return result;
}
#endif

PDL_INLINE ssize_t
PDL_OS::recvv (PDL_SOCKET handle,
               iovec *buffers,
               int n)
{
#if defined (PDL_HAS_WINSOCK2)

  DWORD bytes_received = 0;
  int result = 1;

  // Winsock 2 has WSARecv and can do this directly, but Winsock 1 needs
  // to do the recvs piece-by-piece.

# if (PDL_HAS_WINSOCK2 != 0)
  DWORD flags = 0;
  result = ::WSARecv ((SOCKET) handle,
                      (WSABUF *) buffers,
                      n,
                      &bytes_received,
                      &flags,
                      0,
                      0);
# else
  int i, chunklen;
  char *chunkp = 0;

  // Step through the buffers requested by caller; for each one, cycle
  // through reads until it's filled or an error occurs.
  for (i = 0; i < n && result > 0; i++)
    {
      chunkp = buffers[i].iov_base;     // Point to part of chunk being read
      chunklen = buffers[i].iov_len;    // Track how much to read to chunk
      while (chunklen > 0 && result > 0)
        {
          result = ::recv ((SOCKET) handle, chunkp, chunklen, 0);
          if (result > 0)
            {
              chunkp += result;
              chunklen -= result;
              bytes_received += result;
            }
        }
    }
# endif /* PDL_HAS_WINSOCK2 != 0 */

  if (result == SOCKET_ERROR)
    {
      PDL_OS::set_errno_to_wsa_last_error ();
      return -1;
    }
  else
    return (ssize_t) bytes_received;
#else /* (PDL_HAS_WINSOCK2) */
  return PDL_OS::readv (handle, buffers, n);
#endif /* PDL_HAS_WINSOCK2 */
}

PDL_INLINE ssize_t
PDL_OS::sendv (PDL_SOCKET handle,
               const iovec *buffers,
               int n)
{
#if defined (PDL_HAS_WINSOCK2)
  DWORD bytes_sent = 0;
  int result = 0;

  // Winsock 2 has WSASend and can do this directly, but Winsock 1 needs
  // to do the sends one-by-one.
# if (PDL_HAS_WINSOCK2 != 0)
  result = ::WSASend ((SOCKET) handle,
                      (WSABUF *) buffers,
                      n,
                      &bytes_sent,
                      0,
                      0,
                      0);
# else
  int i;
  for (i = 0; i < n && result != SOCKET_ERROR; i++)
    {
      result = ::send ((SOCKET) handle,
                       buffers[i].iov_base,
                       buffers[i].iov_len,
                       0);
      bytes_sent += buffers[i].iov_len;     // Gets ignored on error anyway
    }
# endif /* PDL_HAS_WINSOCK2 != 0 */

  if (result == SOCKET_ERROR)
    {
      PDL_OS::set_errno_to_wsa_last_error ();
      return -1;
    }
  else
    return (ssize_t) bytes_sent;

#else
  return PDL_OS::writev (handle, buffers, n);
#endif /* PDL_HAS_WINSOCK2 */
}

PDL_INLINE int
PDL_OS::poll (struct pollfd *pollfds, u_long len, const PDL_Time_Value *timeout)
{
  PDL_TRACE ("PDL_OS::poll");
#if defined (PDL_HAS_POLL)
  int result;
  int to = timeout == 0 ? -1 : int (timeout->msec ());

again:
  result = ::poll( pollfds, len, to );

  if ( result == -1 && errno == EINTR )
  {
      goto again;
  }
  else
  {
      /* Nothing to do*/
  }
  return result;
#else
  PDL_UNUSED_ARG (timeout);
  PDL_UNUSED_ARG (len);
  PDL_UNUSED_ARG (pollfds);

  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_HAS_POLL */
}

PDL_INLINE int
PDL_OS::poll (struct pollfd *pollfds, u_long len, const PDL_Time_Value &timeout)
{
  PDL_TRACE ("PDL_OS::poll");
#if defined (PDL_HAS_POLL)
  int result;

again:
  result = ::poll( pollfds, len, int( timeout.msec() ) );

  if ( result == -1 && errno == EINTR )
  {
      goto again;
  }
  else
  {
      /* Nothing to do*/
  }
  return result;

#else
  PDL_UNUSED_ARG (timeout);
  PDL_UNUSED_ARG (len);
  PDL_UNUSED_ARG (pollfds);

  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_HAS_POLL */
}
PDL_INLINE int
PDL_OS::epoll_create (int aSize)
{
  PDL_TRACE ("PDL_OS::epoll_create");
#if defined (PDL_HAS_EPOLL)
  PDL_OSCALL_RETURN (::epoll_create (aSize), int, -1);
#else
  PDL_UNUSED_ARG (aSize);

  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_HAS_EPOLL */
}

PDL_INLINE int
PDL_OS::epoll_ctl (int epfd, int op, int fd, struct epoll_event *event)
{
  PDL_TRACE ("PDL_OS::epoll_ctl");
#if defined (PDL_HAS_EPOLL)
  PDL_OSCALL_RETURN (::epoll_ctl (epfd, op, fd, event), int, -1);
#else
  PDL_UNUSED_ARG (epfd);
  PDL_UNUSED_ARG (op);
  PDL_UNUSED_ARG (fd);
  PDL_UNUSED_ARG (event);

  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_HAS_EPOLL */
}

PDL_INLINE int
PDL_OS::epoll_wait (int epfd, struct epoll_event *events, int maxevents, const PDL_Time_Value *timeout)
{
  PDL_TRACE ("PDL_OS::epoll_wait");
#if defined (PDL_HAS_EPOLL)

  int result;
  int to = timeout == 0 ? -1 : int (timeout->msec ());

again:
  result = ::epoll_wait( epfd, events, maxevents, to );

  if ( result == -1 && errno == EINTR )
  {
      goto again;
  }
  else
  {
      /* Nothing to do */
  }
  return result;

#else
  PDL_UNUSED_ARG (epfd);
  PDL_UNUSED_ARG (events);
  PDL_UNUSED_ARG (maxevents);
  PDL_UNUSED_ARG (timeout);

  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_HAS_EPOLL */
}


PDL_INLINE char *
PDL_OS::compile (const char *instring, char *expbuf, char *endbuf)
{
  PDL_TRACE ("PDL_OS::compile");
#if defined (PDL_HAS_REGEX)
  PDL_OSCALL_RETURN (::compile (instring, expbuf, endbuf), char *, 0);
#else
  PDL_UNUSED_ARG (instring);
  PDL_UNUSED_ARG (expbuf);
  PDL_UNUSED_ARG (endbuf);

  PDL_NOTSUP_RETURN (0);
#endif /* PDL_HAS_REGEX */
}

PDL_INLINE PDL_OFF_T
PDL_OS::filesize (LPCTSTR filename)
{
  PDL_TRACE ("PDL_OS::filesize");

#if defined(ITRON)
  /* empty */
  PDL_NOTSUP_RETURN (-1);
#else
  PDL_HANDLE h = PDL_OS::open (filename, O_RDONLY);
  if (h != PDL_INVALID_HANDLE)
    {
      PDL_OFF_T size = PDL_OS::filesize (h);
      PDL_OS::close (h);
      return size;
    }
  else
    return -1;
#endif
}

PDL_INLINE int
PDL_OS::closesocket (PDL_SOCKET handle)
{
  PDL_TRACE ("PDL_OS::close");
#if defined (PDL_WIN32)
  PDL_SOCKCALL_RETURN (::closesocket ((SOCKET) handle), int, -1);
#elif defined (PDL_PSOS_DIAB_PPC)
  PDL_OSCALL_RETURN (::pna_close (handle), int, -1);
#elif defined(ITRON)
  /* empty */
  PDL_NOTSUP_RETURN (-1);
#else
  PDL_OSCALL_RETURN (::close (handle), int, -1);
#endif /* PDL_WIN32 */
}

PDL_INLINE int
PDL_OS::access (const char *path, int amode)
{
  PDL_TRACE ("PDL_OS::access");
#if defined (PDL_LACKS_ACCESS)
#if defined (VXWORKS)
  struct stat a;
  PDL_OSCALL_RETURN (::stat ((char *)path, &a), int, -1);
#else
  PDL_UNUSED_ARG (path);
  PDL_UNUSED_ARG (amode);
  PDL_NOTSUP_RETURN (-1);
#endif
#elif defined (PDL_WIN32)
#if !defined (PDL_HAS_WINCE)
  PDL_OSCALL_RETURN (::_access (path, amode), int, -1);
#else
  int       sSize = 0;
  wchar_t * sWPath = NULL;
  int       sRc = -1;
  int       sFileAttr = 0xFFFFFFFF;

  sSize = mbstowcs(NULL, path, 0);

  sWPath = (wchar_t *)PDL_OS::malloc(sizeof(wchar_t) * (sSize + 1));

  if(sWPath != NULL)
  {
    mbstowcs(sWPath, path, sSize + 1);

    sFileAttr = GetFileAttributes(sWPath);

    if(sFileAttr != 0xFFFFFFFF)
    {
      if((amode & W_OK) && (sFileAttr & FILE_ATTRIBUTE_READONLY) )
      {
        // file has read-only attributes
        sRc = -1;
      }
      else
      {
        sRc = 0;
      }
    }
    else
    {
      errno = GetLastError();
    }

    free(sWPath);
  }

  return sRc;
#endif /* PDL_HAS_WINCE */
#else
  PDL_OSCALL_RETURN (::access (path, amode), int, -1);
#endif /* PDL_LACKS_ACCESS */
}

#if !defined (PDL_HAS_WINCE)
PDL_INLINE
PDL_HANDLE PDL_OS::creat (LPCTSTR filename,
                          mode_t   mode)
#else
PDL_INLINE
PDL_HANDLE PDL_OS::creat (char*    filename,
                          mode_t   mode)
#endif /*PDL_HAS_WINCE*/
{
  PDL_TRACE ("PDL_OS::creat");
#if defined (PDL_WIN32)
/*BUGBUG_NT*/
  PDL_UNUSED_ARG(mode);
  return PDL_OS::open (filename, O_RDWR | _O_CREAT | _O_TRUNC);
/*BUGBUG_NT*/
#elif defined(PDL_PSOS)
   PDL_OSCALL_RETURN(::create_f((char *)filename, 1024,
                              S_IRUSR | S_IWUSR | S_IXUSR),
                     PDL_HANDLE, PDL_INVALID_HANDLE);
#elif defined(PDL_PSOS_TM)
  PDL_UNUSED_ARG (filename);
  PDL_UNUSED_ARG (mode);
  PDL_NOTSUP_RETURN (-1);
#elif defined(PDL_PSOS)
  PDL_UNUSED_ARG (filename);
  PDL_UNUSED_ARG (mode);
  PDL_NOTSUP_RETURN (-1);
#else
  PDL_OSCALL_RETURN (::creat (filename, mode),
                     PDL_HANDLE, PDL_INVALID_HANDLE);
#endif /* PDL_WIN32 */
}

#if !defined (PDL_WIN32) && !defined (VXWORKS) && !defined (CHORUS) && !defined (PDL_PSOS)
// Don't inline on those platforms because this function contains
// string literals, and some compilers, e.g., g++, don't handle those
// efficiently in unused inline functions.
PDL_INLINE int
PDL_OS::uname (struct utsname *name)
{
  PDL_TRACE ("PDL_OS::uname");
#if defined(ITRON)
  /* empty */
  PDL_NOTSUP_RETURN (-1);
#else
  PDL_OSCALL_RETURN (::uname (name), int, -1);
#endif
}
#endif /* ! PDL_WIN32 && ! VXWORKS && ! CHORUS */

PDL_INLINE int
PDL_OS::hostname (char name[], size_t maxnamelen)
{
  PDL_TRACE ("PDL_OS::hostname");
#if !defined (PDL_HAS_WINCE)
# if defined (PDL_HAS_PHARLAP)
  // PharLap only can do net stuff with the RT version.
#   if defined (PDL_HAS_PHARLAP_RT)
  // @@This is not at all reliable... requires ethernet and BOOTP to be used.
  // A more reliable way is to go thru the devices w/ EtsTCPGetDeviceCfg until
  // a legit IP address is found, then get its name w/ gethostbyaddr.
  PDL_SOCKCALL_RETURN (gethostname (name, maxnamelen), int, SOCKET_ERROR);
#   else
  PDL_UNUSED_ARG (name);
  PDL_UNUSED_ARG (maxnamelen);
  PDL_NOTSUP_RETURN (-1);
#   endif /* PDL_HAS_PHARLAP_RT */
# elif defined (PDL_WIN32)
  PDL_WIN32CALL_RETURN (PDL_ADAPT_RETVAL (::GetComputerNameA (name, LPDWORD (&maxnamelen)),
                                          pdl_result_), int, -1);
# elif defined (VXWORKS)
  PDL_OSCALL_RETURN (::gethostname (name, maxnamelen), int, -1);
# elif defined (CHORUS)
  if (::gethostname (name, maxnamelen) == -1)
    return -1;
  else
    {
      if (PDL_OS::strlen (name) == 0)
        {
          // Try the HOST environment variable.
          char *const hostenv = ::getenv ("HOST");
          if (hostenv)
            PDL_OS::strncpy (name, hostenv, maxnamelen);
        }
      return 0;
    }
# else /* !PDL_WIN32 */
  struct utsname host_info;

  if (PDL_OS::uname (&host_info) == -1)
    return -1;
  else
    {
      PDL_OS::strncpy (name, host_info.nodename, maxnamelen);
      return 0;
    }
# endif /* PDL_WIN32 */
#else /* PDL_HAS_WINCE */
  // @@ Don'T know how to implement this (yet.)  Can probably get around
  // this by peeking into the Register set.
  PDL_UNUSED_ARG (name);
  PDL_UNUSED_ARG (maxnamelen);
  PDL_NOTSUP_RETURN (-1);
#endif /* ! PDL_HAS_WINCE */
}

PDL_INLINE int
PDL_OS::msgctl (int msqid, int cmd, struct msqid_ds *val)
{
  PDL_TRACE ("PDL_OS::msgctl");
#if defined (PDL_HAS_SYSV_IPC)
  PDL_OSCALL_RETURN (::msgctl (msqid, cmd, val), int, -1);
#else
  PDL_UNUSED_ARG (msqid);
  PDL_UNUSED_ARG (cmd);
  PDL_UNUSED_ARG (val);

  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_HAS_SYSV_IPC */
}

PDL_INLINE int
PDL_OS::msgget (key_t key, int msgflg)
{
  PDL_TRACE ("PDL_OS::msgget");
#if defined (PDL_HAS_SYSV_IPC)
  PDL_OSCALL_RETURN (::msgget (key, msgflg), int, -1);
#else
  PDL_UNUSED_ARG (key);
  PDL_UNUSED_ARG (msgflg);

  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_HAS_SYSV_IPC */
}

PDL_INLINE int
PDL_OS::msgrcv (int int_id, void *buf, size_t len,
                long type, int flags)
{
  PDL_TRACE ("PDL_OS::msgrcv");
#if defined (PDL_HAS_SYSV_IPC)
# if defined (PDL_LACKS_POSIX_PROTOTYPES) || defined (PDL_LACKS_SOME_POSIX_PROTOTYPES)
  PDL_OSCALL_RETURN (::msgrcv (int_id, (msgbuf *) buf, len, type, flags),
                     int, -1);
# else
  PDL_OSCALL_RETURN (::msgrcv (int_id, buf, len, type, flags),
                     int, -1);
# endif /* PDL_LACKS_POSIX_PROTOTYPES */
#else
  PDL_UNUSED_ARG (int_id);
  PDL_UNUSED_ARG (buf);
  PDL_UNUSED_ARG (len);
  PDL_UNUSED_ARG (type);
  PDL_UNUSED_ARG (flags);

  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_HAS_SYSV_IPC */
}

PDL_INLINE int
PDL_OS::msgsnd (int int_id, const void *buf, size_t len, int flags)
{
  PDL_TRACE ("PDL_OS::msgsnd");
#if defined (PDL_HAS_SYSV_IPC)
# if defined (PDL_HAS_NONCONST_MSGSND)
  PDL_OSCALL_RETURN (::msgsnd (int_id, (void *) buf, len, flags), int, -1);
# elif defined (PDL_LACKS_POSIX_PROTOTYPES) || defined (PDL_LACKS_SOME_POSIX_PROTOTYPES)
  PDL_OSCALL_RETURN (::msgsnd (int_id, (msgbuf *) buf, len, flags), int, -1);
# else
  PDL_OSCALL_RETURN (::msgsnd (int_id, buf, len, flags), int, -1);
# endif /* PDL_LACKS_POSIX_PROTOTYPES || PDL_HAS_NONCONST_MSGSND */
#else
  PDL_UNUSED_ARG (int_id);
  PDL_UNUSED_ARG (buf);
  PDL_UNUSED_ARG (len);
  PDL_UNUSED_ARG (flags);

  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_HAS_SYSV_IPC */
}

PDL_INLINE u_int
PDL_OS::alarm (u_int secs)
{
  PDL_TRACE ("PDL_OS::alarm");

#if defined (PDL_WIN32) || defined (VXWORKS) || defined (CHORUS) || defined (PDL_PSOS)
  PDL_UNUSED_ARG (secs);

  PDL_NOTSUP_RETURN (0);
#elif defined(ITRON)
  /* empty */
  PDL_NOTSUP_RETURN (-1);
#else
  return ::alarm (secs);
#endif /* PDL_WIN32 || VXWORKS || CHORUS */
}

PDL_INLINE u_int
PDL_OS::ualarm (u_int usecs, u_int interval)
{
  PDL_TRACE ("PDL_OS::ualarm");

#if defined (PDL_HAS_UALARM)
  return ::ualarm (usecs, interval);
#elif !defined (PDL_LACKS_UNIX_SIGNALS)
  PDL_UNUSED_ARG (interval);
  return ::alarm (usecs * PDL_ONE_SECOND_IN_USECS);
#else
  PDL_UNUSED_ARG (usecs);
  PDL_UNUSED_ARG (interval);
  PDL_NOTSUP_RETURN (0);
#endif /* PDL_HAS_UALARM */
}

PDL_INLINE u_int
PDL_OS::ualarm (const PDL_Time_Value &tv,
                const PDL_Time_Value &tv_interval)
{
  PDL_TRACE ("PDL_OS::ualarm");

#if defined (PDL_HAS_UALARM)
  u_int usecs = (tv.sec () * PDL_ONE_SECOND_IN_USECS) + tv.usec ();
  u_int interval = (tv_interval.sec () * PDL_ONE_SECOND_IN_USECS) + tv_interval.usec ();
  return ::ualarm (usecs, interval);
#elif !defined (PDL_LACKS_UNIX_SIGNALS)
  PDL_UNUSED_ARG (tv_interval);
  return ::alarm (tv.sec ());
#else
  PDL_UNUSED_ARG (tv_interval);
  PDL_UNUSED_ARG (tv);
  PDL_NOTSUP_RETURN (0);
#endif /* PDL_HAS_UALARM */
}

PDL_INLINE int
PDL_OS::dlclose (PDL_SHLIB_HANDLE handle)
{
  PDL_TRACE ("PDL_OS::dlclose");
#if defined (PDL_HAS_SVR4_DYNAMIC_LINKING)

# if !defined (PDL_HAS_AUTOMATIC_INIT_FINI)
  // SunOS4 does not automatically call _fini()!
  void *ptr;

  PDL_OSCALL (::dlsym (handle, "_fini"), void *, 0, ptr);

  if (ptr != 0)
    (*((int (*)(void)) ptr)) (); // Call _fini hook explicitly.
# endif /* PDL_HAS_AUTOMATIC_INIT_FINI */
#if defined (_M_UNIX)
  PDL_OSCALL_RETURN (::_dlclose (handle), int, -1);
#else /* _MUNIX */
    PDL_OSCALL_RETURN (::dlclose (handle), int, -1);
#endif /* _M_UNIX */
#elif defined (PDL_WIN32)
  PDL_WIN32CALL_RETURN (PDL_ADAPT_RETVAL (::FreeLibrary (handle), pdl_result_), int, -1);
#elif defined (__hpux)
  // HP-UX 10.x and 32-bit 11.00 do not pay attention to the ref count
  // when unloading a dynamic lib.  So, if the ref count is more than
  // 1, do not unload the lib.  This will cause a library loaded more
  // than once to not be unloaded until the process runs down, but
  // that's life.  It's better than unloading a library that's in use.
  // So far as I know, there's no way to decrement the refcnt that the
  // kernel is looking at - the shl_descriptor is a copy of what the
  // kernel has, not the actual struct.  On 64-bit HP-UX using dlopen,
  // this problem has been fixed.
  struct shl_descriptor  desc;
  if (shl_gethandle_r (handle, &desc) == -1)
    return -1;
  if (desc.ref_count > 1)
    return 0;
# if defined(__GNUC__) || __cplusplus >= 199707L
  PDL_OSCALL_RETURN (::shl_unload (handle), int, -1);
# else
  PDL_OSCALL_RETURN (::cxxshl_unload (handle), int, -1);
# endif  /* aC++ vs. Hp C++ */
#else
  PDL_UNUSED_ARG (handle);
  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_HAS_SVR4_DYNAMIC_LINKING */
}

PDL_INLINE ASYS_TCHAR *
PDL_OS::dlerror (void)
{
  PDL_TRACE ("PDL_OS::dlerror");
# if defined (PDL_HAS_SVR4_DYNAMIC_LINKING)
#if defined(_M_UNIX)
  PDL_OSCALL_RETURN ((char *)::_dlerror (), char *, 0);
#else /* _M_UNIX */
  PDL_OSCALL_RETURN ((char *)::dlerror (), char *, 0);
#endif /* _M_UNIX */
# elif defined (__hpux)
  PDL_OSCALL_RETURN (::strerror(errno), char *, 0);
# elif defined (PDL_WIN32)
  static ASYS_TCHAR buf[128];
#   if defined (PDL_HAS_PHARLAP)
  PDL_OS::sprintf (buf, "error code %d", GetLastError());
#   else
#if !defined (PDL_HAS_MOSTLY_UNICODE_APIS)
  FormatMessageA (FORMAT_MESSAGE_FROM_SYSTEM,
                  NULL,
                  ::GetLastError (),
                  0,
                  buf,
                  sizeof buf,
                  NULL);
#else
  FormatMessage (FORMAT_MESSAGE_FROM_SYSTEM,
                 NULL,
                 ::GetLastError (),
                 0,
                 buf,
                 sizeof buf / sizeof ASYS_TCHAR,
                 NULL);
#endif /* PDL_HAS_MOSTLY_UNICODE_APIS */
#   endif /* PDL_HAS_PHARLAP */
  return buf;
# else
  PDL_NOTSUP_RETURN (0);
# endif /* PDL_HAS_SVR4_DYNAMIC_LINKING */
}

PDL_INLINE PDL_SHLIB_HANDLE
PDL_OS::dlopen (const ASYS_TCHAR *fname,
                int mode)
{
  PDL_TRACE ("PDL_OS::dlopen");

  // Get the correct OS type.
  PDL_DL_TYPE filename = PDL_const_cast (PDL_DL_TYPE, fname);

# if defined (PDL_HAS_SVR4_DYNAMIC_LINKING)
  void *handle;
#   if defined (PDL_HAS_SGIDLADD)
  PDL_OSCALL (::sgidladd (filename, mode), void *, 0, handle);
#   elif defined (_M_UNIX)
  PDL_OSCALL (::_dlopen (filename, mode), void *, 0, handle);
#   else
  PDL_OSCALL (::dlopen (filename, mode), void *, 0, handle);
#   endif /* PDL_HAS_SGIDLADD */
#   if !defined (PDL_HAS_AUTOMATIC_INIT_FINI)
  if (handle != 0)
    {
      void *ptr;
      // Some systems (e.g., SunOS4) do not automatically call _init(), so
      // we'll have to call it manually.

      PDL_OSCALL (::dlsym (handle, "_init"), void *, 0, ptr);

      if (ptr != 0 && (*((int (*)(void)) ptr)) () == -1) // Call _init hook explicitly.
        {
          // Close down the handle to prevent leaks.
          ::dlclose (handle);
          return 0;
        }
    }
#   endif /* PDL_HAS_AUTOMATIC_INIT_FINI */
  return handle;
# elif defined (PDL_WIN32)
  PDL_UNUSED_ARG (mode);

#   if defined (PDL_HAS_MOSTLY_UNICODE_APIS)
  PDL_WIN32CALL_RETURN (::LoadLibrary (filename), PDL_SHLIB_HANDLE, 0);
#   else
  PDL_WIN32CALL_RETURN (::LoadLibraryA (filename), PDL_SHLIB_HANDLE, 0);
#   endif /* PDL_HAS_MOSTLY_UNICODE_APIS */
# elif defined (__hpux)

#   if defined(__GNUC__) || __cplusplus >= 199707L
  PDL_OSCALL_RETURN (::shl_load(filename, mode, 0L), PDL_SHLIB_HANDLE, 0);
#   else
  PDL_OSCALL_RETURN (::cxxshl_load(filename, mode, 0L), PDL_SHLIB_HANDLE, 0);
#   endif  /* aC++ vs. Hp C++ */

# else
  PDL_UNUSED_ARG (filename);
  PDL_UNUSED_ARG (mode);
  PDL_NOTSUP_RETURN (0);
# endif /* PDL_HAS_SVR4_DYNAMIC_LINKING */
}

PDL_INLINE void *
PDL_OS::dlsym (PDL_SHLIB_HANDLE handle,
               const ASYS_TCHAR *sname)
{
  PDL_TRACE ("PDL_OS::dlsym");

#if defined (PDL_HAS_DLSYM_SEGFAULT_ON_INVALID_HANDLE)
  // Check if the handle is valid before making any calls using it.
  if (handle == PDL_SHLIB_INVALID_HANDLE)
    return 0;
#endif /* PDL_HAS_DLSYM_SEGFAULT_ON_INVALID_HANDLE */

  // Get the correct OS type.
#if defined (PDL_HAS_WINCE)
  const wchar_t *symbolname = sname;
#elif defined (PDL_HAS_CHARPTR_DL)
  char *symbolname = PDL_const_cast (char *, sname);
#elif !defined (PDL_WIN32) || !defined (PDL_HAS_UNICODE)
  const char *symbolname = sname;
#endif /* PDL_HAS_WINCE */

# if defined (PDL_HAS_SVR4_DYNAMIC_LINKING)

#   if defined (PDL_LACKS_POSIX_PROTOTYPES)
  PDL_OSCALL_RETURN (::dlsym (handle, symbolname), void *, 0);
#   elif defined (PDL_USES_ASM_SYMBOL_IN_DLSYM)
  int l = PDL_OS::strlen (symbolname) + 2;
  char *asm_symbolname = 0;
  PDL_NEW_RETURN (asm_symbolname, char, l, 0);
  PDL_OS::strcpy (asm_symbolname, "_") ;
  PDL_OS::strcpy (asm_symbolname + 1, symbolname) ;
  void *pdl_result;
  PDL_OSCALL (::dlsym (handle, asm_symbolname), void *, 0, pdl_result);
  PDL_OS::free(asm_symbolname);
  return pdl_result;
#   elif defined (_M_UNIX)
  PDL_OSCALL_RETURN (::_dlsym (handle, symbolname), void *, 0);
#   else
  PDL_OSCALL_RETURN (::dlsym (handle, symbolname), void *, 0);
#   endif /* PDL_LACKS_POSIX_PROTOTYPES */

# elif defined (PDL_WIN32) && defined (PDL_HAS_UNICODE) && !defined (PDL_HAS_WINCE)

  PDL_WIN32CALL_RETURN (::GetProcAddress (handle, ASYS_ONLY_MULTIBYTE_STRING(sname)), void *, 0);

# elif defined (PDL_WIN32)

  PDL_WIN32CALL_RETURN (::GetProcAddress (handle, symbolname), void *, 0);

# elif defined (__hpux)

  void *value;
  int status;
  shl_t _handle = handle;
  PDL_OSCALL (::shl_findsym(&_handle, symbolname, TYPE_UNDEFINED, &value), int, -1, status);
  return status == 0 ? value : 0;

# else

  PDL_UNUSED_ARG (handle);
  PDL_UNUSED_ARG (symbolname);
  PDL_NOTSUP_RETURN (0);

# endif /* PDL_HAS_SVR4_DYNAMIC_LINKING */
}

PDL_INLINE int
PDL_OS::step (const char *str, char *expbuf)
{
  PDL_TRACE ("PDL_OS::step");
#if defined (PDL_HAS_REGEX)
  PDL_OSCALL_RETURN (::step (str, expbuf), int, -1);
#else
  PDL_UNUSED_ARG (str);
  PDL_UNUSED_ARG (expbuf);

  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_HAS_REGEX */
}

PDL_INLINE long
PDL_OS::sysinfo (int cmd, char *buf, long count)
{
  PDL_TRACE ("PDL_OS::sysinfo");
#if defined (PDL_HAS_SYSINFO)
  PDL_OSCALL_RETURN (::sysinfo (cmd, buf, count), long, -1);
#else
  PDL_UNUSED_ARG (cmd);
  PDL_UNUSED_ARG (buf);
  PDL_UNUSED_ARG (count);

  PDL_NOTSUP_RETURN (0);
#endif /* PDL_HAS_SYSINFO */
}

PDL_INLINE ssize_t
PDL_OS::write (PDL_HANDLE handle, const void *buf, size_t nbyte)
{
    PDL_TRACE ("PDL_OS::write");
    ssize_t result;
  again:
#if defined (PDL_WIN32)
    DWORD bytes_written; // This is set to 0 byte WriteFile.

    // Strictly correctly, we should loop writing all the data if more
    // than a DWORD length can hold.
    DWORD short_nbyte = PDL_static_cast (DWORD, nbyte);
    if (::WriteFile (handle, buf, short_nbyte, &bytes_written, 0))
        return (ssize_t) bytes_written;
    else
/*BUGBUG_NT*/
    {
        pdlPrintWin32Error( GetLastError() );
        PDL_FAIL_RETURN (-1);
    }
/*BUGBUG_NT*/
#elif defined (PDL_PSOS)
# if defined (PDL_PSOS_LACKS_PHILE)
    PDL_UNUSED_ARG (handle);
    PDL_UNUSED_ARG (buf);
    PDL_UNUSED_ARG (nbyte);
    PDL_NOTSUP_RETURN (-1);
# else
    if(::write_f(handle, (void *) buf, nbyte) == 0)
        return (ssize_t) nbyte;
    else
        return -1;
# endif /* defined (PDL_PSOS_LACKS_PHILE) */
#elif defined(ITRON)
    /* empty */
    PDL_NOTSUP_RETURN (-1);
#else
# if defined (PDL_LACKS_POSIX_PROTOTYPES)
    result = ::write (handle, (const char *) buf, nbyte);

# elif defined (PDL_PSOS)
    result = ::write_f(handle, (void *) buf, nbyte);

# elif defined (PDL_HAS_CHARPTR_SOCKOPT)
    result = ::write (handle, (char *) buf, nbyte);
# else
    result = ::write (handle, buf, nbyte);

# endif /* PDL_LACKS_POSIX_PROTOTYPES */
#endif /* PDL_WIN32 */
    if (result == -1 && errno == EINTR)
    {
        goto again;
    }
    return result;
}

PDL_INLINE ssize_t
PDL_OS::write (PDL_HANDLE handle, const void *buf, size_t nbyte,
               PDL_OVERLAPPED *overlapped)
{
  PDL_TRACE ("PDL_OS::write");
  overlapped = overlapped;

#if defined (PDL_WIN32)
  DWORD bytes_written; // This is set to 0 byte WriteFile.

  DWORD short_nbyte = PDL_static_cast (DWORD, nbyte);
  if (::WriteFile (handle, buf, short_nbyte, &bytes_written, overlapped))
    return (ssize_t) bytes_written;
  else
    return -1;
#else
  return PDL_OS::write (handle, buf, nbyte);
#endif /* PDL_WIN32 */
}

PDL_INLINE ssize_t
PDL_OS::read (PDL_HANDLE handle, void *buf, size_t len)
{
    PDL_TRACE ("PDL_OS::read");
    ssize_t result;

  again:
#if defined (PDL_WIN32)
    DWORD ok_len;
    if (::ReadFile (handle, buf, PDL_static_cast (DWORD, len), &ok_len, 0))
        return (ssize_t) ok_len;
    else
/*BUGBUG_NT*/
    {
        pdlPrintWin32Error( GetLastError() );
        PDL_FAIL_RETURN (-1);
    }
/*BUGBUG_NT*/

#elif defined (PDL_PSOS)
# if defined (PDL_PSOS_LACKS_PHILE)
    PDL_UNUSED_ARG (handle);
    PDL_UNUSED_ARG (buf);
    PDL_UNUSED_ARG (len);
    PDL_NOTSUP_RETURN (-1);
# else
    unsigned long count;

    result = ::read_f (handle, buf, len, &count);
    if (result != 0)
    {
        return PDL_static_cast (ssize_t, -1);
    }
    else
    {
        return PDL_static_cast (ssize_t,
                                (count = len ? count : 0));
    }
# endif /* defined (PDL_PSOS_LACKS_PHILE */
#elif defined(ITRON)
    /* empty */
    PDL_NOTSUP_RETURN (-1);
#else

# if defined (PDL_LACKS_POSIX_PROTOTYPES) || defined (PDL_HAS_CHARPTR_SOCKOPT)
    result = ::read (handle, (char *) buf, len);
# else
    result = ::read (handle, buf, len);
# endif /* PDL_LACKS_POSIX_PROTOTYPES */
    if (result == -1 && errno == EAGAIN)
        errno = EWOULDBLOCK;
#endif /* PDL_WIN32 */
    if (result == -1 && errno == EINTR)
    {
        goto again; // we have to re-read on signal
    }
    return result;
}

PDL_INLINE ssize_t
PDL_OS::read (PDL_HANDLE handle, void *buf, size_t len,
              PDL_OVERLAPPED *overlapped)
{
    PDL_TRACE ("PDL_OS::read");
    overlapped = overlapped;
#if defined (PDL_WIN32)
    DWORD ok_len;
    DWORD short_len = PDL_static_cast (DWORD, len);
    if (::ReadFile (handle, buf, short_len, &ok_len, overlapped))
        return (ssize_t) ok_len;
    else
        PDL_FAIL_RETURN (-1);
#else
    return PDL_OS::read (handle, buf, len);
#endif /* PDL_WIN32 */
}

PDL_INLINE int
PDL_OS::readlink (const char *path, char *buf, size_t bufsiz)
{
  PDL_TRACE ("PDL_OS::readlink");
# if defined (PDL_LACKS_READLINK) || \
     defined (PDL_HAS_WINCE) || defined (PDL_WIN32)
  PDL_UNUSED_ARG (path);
  PDL_UNUSED_ARG (buf);
  PDL_UNUSED_ARG (bufsiz);
  PDL_NOTSUP_RETURN (-1);
# else
#   if !defined(PDL_HAS_NONCONST_READLINK)
      PDL_OSCALL_RETURN (::readlink (path, buf, bufsiz), int, -1);
#   else
      PDL_OSCALL_RETURN (::readlink ((char *)path, buf, bufsiz), int, -1);
#   endif
# endif /* PDL_LACKS_READLINK */
}

PDL_INLINE int
PDL_OS::getmsg (PDL_HANDLE handle,
                struct strbuf *ctl,
                struct strbuf *data,
                int *flags)
{
  PDL_TRACE ("PDL_OS::getmsg");
#if defined (PDL_HAS_STREAM_PIPES)
  PDL_OSCALL_RETURN (::getmsg (handle, ctl, data, flags), int, -1);
#else
  PDL_UNUSED_ARG (handle);
  PDL_UNUSED_ARG (ctl);
  PDL_UNUSED_ARG (data);
  PDL_UNUSED_ARG (flags);

  // I'm not sure how to implement this correctly.
  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_HAS_STREAM_PIPES */
}

PDL_INLINE int
PDL_OS::getpmsg (PDL_HANDLE handle,
                 struct strbuf *ctl,
                 struct strbuf *data,
                 int *band,
                 int *flags)
{
  PDL_TRACE ("PDL_OS::getpmsg");
#if defined (PDL_HAS_STREAM_PIPES)
  PDL_OSCALL_RETURN (::getpmsg (handle, ctl, data, band, flags), int, -1);
#else
  PDL_UNUSED_ARG (handle);
  PDL_UNUSED_ARG (ctl);
  PDL_UNUSED_ARG (data);
  PDL_UNUSED_ARG (band);
  PDL_UNUSED_ARG (flags);

  // I'm not sure how to implement this correctly.
  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_HAS_STREAM_PIPES */
}

PDL_INLINE int
PDL_OS::getrusage (int who, struct rusage *ru)
{
  PDL_TRACE ("PDL_OS::getrusage");

#if defined (PDL_HAS_SYSCALL_GETRUSAGE)
  // This nonsense is necessary for HP/UX...
  PDL_OSCALL_RETURN (::syscall (SYS_GETRUSAGE, who, ru), int, -1);
#elif defined (PDL_HAS_GETRUSAGE)
# if defined (PDL_WIN32)
  PDL_UNUSED_ARG (who);

  FILETIME dummy_1;
  FILETIME dummy_2;
  PDL_WIN32CALL_RETURN (PDL_ADAPT_RETVAL (::GetProcessTimes (::GetCurrentProcess(),
                                                             &dummy_1,   // start
                                                             &dummy_2,     // exited
                                                             &ru->ru_stime,
                                                             &ru->ru_utime),
                                          pdl_result_),
                        int, -1);
# else
#   if defined (PDL_HAS_RUSAGE_WHO_ENUM)
  PDL_OSCALL_RETURN (::getrusage ((PDL_HAS_RUSAGE_WHO_ENUM) who, ru), int, -1);
#   else
  PDL_OSCALL_RETURN (::getrusage (who, ru), int, -1);
#   endif /* PDL_HAS_RUSAGE_WHO_ENUM */
# endif /* PDL_WIN32 */
#else
  who = who;
  ru = ru;
  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_HAS_SYSCALL_GETRUSAGE */
}

PDL_INLINE int
PDL_OS::isastream (PDL_HANDLE handle)
{
  PDL_TRACE ("PDL_OS::isastream");
#if defined (PDL_HAS_STREAM_PIPES)
  PDL_OSCALL_RETURN (::isastream (handle), int, -1);
#else
  PDL_UNUSED_ARG (handle);

  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_HAS_STREAM_PIPES */
}

// Implements simple read/write control for pages.  Affects a page if
// part of the page is referenced.  Currently PROT_READ, PROT_WRITE,
// and PROT_RDWR has been mapped in OS.h.  This needn't have anything
// to do with a mmap region.

PDL_INLINE int
PDL_OS::mprotect (void *addr, size_t len, int prot)
{
  PDL_TRACE ("PDL_OS::mprotect");
#if defined (PDL_WIN32) && !defined (PDL_HAS_PHARLAP)
  DWORD dummy; // Sigh!
  return ::VirtualProtect(addr, len, prot, &dummy) ? 0 : -1;
#elif !defined (PDL_LACKS_MPROTECT)
  PDL_OSCALL_RETURN (::mprotect ((PDL_MMAP_TYPE) addr, len, prot), int, -1);
#else
  PDL_UNUSED_ARG (addr);
  PDL_UNUSED_ARG (len);
  PDL_UNUSED_ARG (prot);
  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_WIN32 */
}

PDL_INLINE int
PDL_OS::msync (void *addr, size_t len, int sync)
{
  PDL_TRACE ("PDL_OS::msync");

#if defined (PDL_WIN32) && !defined (PDL_HAS_PHARLAP)
  PDL_UNUSED_ARG (sync);
  PDL_WIN32CALL_RETURN (PDL_ADAPT_RETVAL (::FlushViewOfFile (addr, len), pdl_result_), int, -1);
#elif !defined (PDL_LACKS_MSYNC)
# if !defined (PDL_HAS_BROKEN_NETBSD_MSYNC)
  PDL_OSCALL_RETURN (::msync ((PDL_MMAP_TYPE) addr, len, sync), int, -1);
# else
  PDL_OSCALL_RETURN (::msync ((PDL_MMAP_TYPE) addr, len), int, -1);
  PDL_UNUSED_ARG (sync);
# endif /* PDL_HAS_BROKEN_NETBSD_MSYNC */
#else
  PDL_UNUSED_ARG (addr);
  PDL_UNUSED_ARG (len);
  PDL_UNUSED_ARG (sync);
  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_WIN32 */
}

PDL_INLINE int
PDL_OS::munmap (void *addr, size_t len)
{
  PDL_TRACE ("PDL_OS::munmap");
#if defined (PDL_WIN32)
  PDL_UNUSED_ARG (len);

  PDL_WIN32CALL_RETURN (PDL_ADAPT_RETVAL (::UnmapViewOfFile (addr), pdl_result_), int, -1);
#elif !defined (PDL_LACKS_MMAP)
  PDL_OSCALL_RETURN (::munmap ((PDL_MMAP_TYPE) addr, len), int, -1);
#else
  PDL_UNUSED_ARG (addr);
  PDL_UNUSED_ARG (len);
  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_WIN32 */
}

PDL_INLINE int
PDL_OS::madvise (caddr_t addr, size_t len, int advice)
{
  PDL_TRACE ("PDL_OS::madvise");
#if defined (PDL_WIN32)
  PDL_UNUSED_ARG (addr);
  PDL_UNUSED_ARG (len);
  PDL_UNUSED_ARG (advice);

  PDL_NOTSUP_RETURN (-1);
#elif !defined (PDL_LACKS_MADVISE)
  PDL_OSCALL_RETURN (::madvise (addr, len, advice), int, -1);
#else
  PDL_UNUSED_ARG (addr);
  PDL_UNUSED_ARG (len);
  PDL_UNUSED_ARG (advice);
  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_WIN32 */
}

PDL_INLINE int
PDL_OS::putmsg (PDL_HANDLE handle, const struct strbuf *ctl,
                const struct strbuf *data, int flags)
{
  PDL_TRACE ("PDL_OS::putmsg");
#if defined (PDL_HAS_STREAM_PIPES)
  PDL_OSCALL_RETURN (::putmsg (handle,
                               (PDL_STRBUF_TYPE) ctl,
                               (PDL_STRBUF_TYPE) data,
                               flags), int, -1);
#else
  PDL_UNUSED_ARG (flags);
  if (ctl == 0 && data == 0)
    {
      errno = EINVAL;
      return 0;
    }
  // Handle the two easy cases.
  else if (ctl != 0)
    return PDL_OS::write (handle, ctl->buf, ctl->len);
  else if (data != 0)
    return PDL_OS::write (handle, data->buf, data->len);
  else
    {
      // This is the hard case.
      char *buf;
      PDL_NEW_RETURN (buf, char, ctl->len + data->len, -1);
      PDL_OS::memcpy (buf, ctl->buf, ctl->len);
      PDL_OS::memcpy (buf + ctl->len, data->buf, data->len);
      int result = PDL_OS::write (handle, buf, ctl->len + data->len);
      PDL_OS::free(buf);
      return result;
    }
#endif /* PDL_HAS_STREAM_PIPES */
}

PDL_INLINE int
PDL_OS::putpmsg (PDL_HANDLE handle,
                 const struct strbuf *ctl,
                 const struct strbuf *data,
                 int band,
                 int flags)
{
  PDL_TRACE ("PDL_OS::putpmsg");
#if defined (PDL_HAS_STREAM_PIPES)
  PDL_OSCALL_RETURN (::putpmsg (handle,
                                (PDL_STRBUF_TYPE) ctl,
                                (PDL_STRBUF_TYPE) data,
                                band, flags), int, -1);
#else
  PDL_UNUSED_ARG (flags);
  PDL_UNUSED_ARG (band);
  return PDL_OS::putmsg (handle, ctl, data, flags);
#endif /* PDL_HAS_STREAM_PIPES */
}

PDL_INLINE int
PDL_OS::semctl (int int_id, int semnum, int cmd, semun value)
{
#if defined (WIN32_ODBC_CLI2) && defined(PDL_HAS_WINCE)
    return 0;
#else
  PDL_TRACE ("PDL_OS::semctl");
#if defined (PDL_HAS_SYSV_IPC)
  PDL_OSCALL_RETURN (::semctl (int_id, semnum, cmd, value), int, -1);
#elif defined (PDL_WIN32) && !defined (PDL_HAS_WINCE) && !defined (WIN32_ODBC_CLI2)
  PDL_NOTSUP_RETURN (-1);
#elif defined (__QNX__)
    return qnxSem::semctl(int_id, semnum, cmd, value);
#else
  PDL_UNUSED_ARG (int_id);
  PDL_UNUSED_ARG (semnum);
  PDL_UNUSED_ARG (cmd);
  PDL_UNUSED_ARG (value);

  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_HAS_SYSV_IPC */
#endif /* WIN32_ODBC_CLI2 && PDL_HAS_WINCE */
}

PDL_INLINE int
PDL_OS::semget (key_t key, int nsems, int flags)
{
#if defined (WIN32_ODBC_CLI2) && defined(PDL_HAS_WINCE)
    return 0;
#else
  PDL_TRACE ("PDL_OS::semget");
#if defined (PDL_HAS_SYSV_IPC)
  PDL_OSCALL_RETURN (::semget (key, nsems, flags), int, -1);
#elif defined (PDL_WIN32) && !defined (PDL_HAS_WINCE) && !defined (WIN32_ODBC_CLI2)
  PDL_NOTSUP_RETURN (-1);
#elif defined (__QNX__)
  return qnxSem::semget(key, nsems, flags);
#else
  PDL_UNUSED_ARG (key);
  PDL_UNUSED_ARG (nsems);
  PDL_UNUSED_ARG (flags);

  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_HAS_SYSV_IPC */
#endif /* WIN32_ODBC_CLI2 && PDL_HAS_WINCE */
}

PDL_INLINE int
PDL_OS::semop (int int_id, struct sembuf *sops, size_t nsops)
{
#if defined (WIN32_ODBC_CLI2) && defined(PDL_HAS_WINCE)
    return 0;
#else
  PDL_TRACE ("PDL_OS::semop");
#if defined (PDL_HAS_SYSV_IPC)
  PDL_OSCALL_RETURN (::semop (int_id, sops, nsops), int, -1);
#elif defined (PDL_WIN32) && !defined (PDL_HAS_WINCE) && !defined (WIN32_ODBC_CLI2)
  PDL_NOTSUP_RETURN (-1);
#elif defined (__QNX__)
	return qnxSem::semop(int_id, sops, nsops);
#else
  PDL_UNUSED_ARG (int_id);
  PDL_UNUSED_ARG (sops);
  PDL_UNUSED_ARG (nsops);

  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_HAS_SYSV_IPC */
#endif /* WIN32_ODBC_CLI2 && PDL_HAS_WINCE */
}


PDL_INLINE void *
PDL_OS::shmat (PDL_HANDLE int_id, void *shmaddr, int shmflg)
{
#if defined (WIN32_ODBC_CLI2) && defined(PDL_HAS_WINCE)
    return NULL;
#else
  PDL_TRACE ("PDL_OS::shmat");
#if defined (PDL_HAS_SYSV_IPC)
# if defined (PDL_LACKS_POSIX_PROTOTYPES) || defined (PDL_LACKS_SOME_POSIX_PROTOTYPES)
  PDL_OSCALL_RETURN (::shmat (int_id, (char *)shmaddr, shmflg), void *, (void *) -1);
# else
  PDL_OSCALL_RETURN (::shmat (int_id, shmaddr, shmflg), void *, (void *) -1);
# endif /* PDL_LACKS_POSIX_PROTOTYPES */
#elif defined (PDL_WIN32) && !defined (PDL_HAS_WINCE) && !defined (WIN32_ODBC_CLI2)
  PDL_NOTSUP_RETURN ((void *)-1);
#elif defined (__QNX__)
    return qnxShm::shmat(int_id, shmaddr, shmflg);
#else
    PDL_UNUSED_ARG (int_id);
    PDL_UNUSED_ARG (shmaddr);
    PDL_UNUSED_ARG (shmflg);

    PDL_NOTSUP_RETURN ((void *) -1);
#endif /* PDL_HAS_SYSV_IPC */
#endif /* WIN32_ODBC_CLI2 && PDL_HAS_WINCE */
}

PDL_INLINE int
PDL_OS::shmctl (PDL_HANDLE int_id, int cmd, struct shmid_ds *buf)
{
#if defined (WIN32_ODBC_CLI2) && defined(PDL_HAS_WINCE)
    return 0;
#else
  PDL_TRACE ("PDL_OS::shmctl");
#if defined (PDL_HAS_SYSV_IPC)
  PDL_OSCALL_RETURN (::shmctl (int_id, cmd, buf), int, -1);
#elif defined (PDL_WIN32) && !defined (PDL_HAS_WINCE) && !defined (WIN32_ODBC_CLI2)
  PDL_NOTSUP_RETURN (-1);
#elif defined (__QNX__)
    return qnxShm::shmctl(int_id, cmd, buf);
#else
    PDL_UNUSED_ARG (buf);
    PDL_UNUSED_ARG (cmd);
    PDL_UNUSED_ARG (int_id);

    PDL_NOTSUP_RETURN (-1);
#endif /* PDL_HAS_SYSV_IPC */
#endif /* WIN32_ODBC_CLI2 && PDL_HAS_WINCE */
}

PDL_INLINE int
PDL_OS::shmdt (void *shmaddr)
{
#if defined (WIN32_ODBC_CLI2) && defined(PDL_HAS_WINCE)
    return 0;
#else
  PDL_TRACE ("PDL_OS::shmdt");
#if defined (PDL_HAS_SYSV_IPC)
  PDL_OSCALL_RETURN (::shmdt ((char *) shmaddr), int, -1);
#elif defined (PDL_WIN32) && !defined (PDL_HAS_WINCE) && !defined (WIN32_ODBC_CLI2)
  PDL_NOTSUP_RETURN (-1);
#elif defined (__QNX__)
    return qnxShm::shmdt(shmaddr);
#else
    PDL_UNUSED_ARG (shmaddr);

    PDL_NOTSUP_RETURN (-1);
#endif /* PDL_HAS_SYSV_IPC */
#endif /* WIN32_ODBC_CLI2 && PDL_HAS_WINCE */
}

PDL_INLINE PDL_HANDLE
PDL_OS::shmget (key_t key, size_t size, int flags)
{
#if defined (WIN32_ODBC_CLI2) && defined(PDL_HAS_WINCE)
    return PDL_INVALID_HANDLE;
#else
  PDL_TRACE ("PDL_OS::shmget");

#if defined (PDL_HAS_SYSV_IPC)
  PDL_OSCALL_RETURN (::shmget (key, size, flags), PDL_HANDLE, PDL_INVALID_HANDLE);
#elif defined (PDL_WIN32) && !defined (PDL_HAS_WINCE) && !defined (WIN32_ODBC_CLI2)
  PDL_NOTSUP_RETURN (PDL_INVALID_HANDLE);
#elif defined (__QNX__)
    return qnxShm::shmget(key, (size_t) size, flags);
#else
    PDL_UNUSED_ARG (flags);
    PDL_UNUSED_ARG (size);
    PDL_UNUSED_ARG (key);

    PDL_NOTSUP_RETURN (PDL_INVALID_HANDLE);
#endif /* PDL_HAS_SYSV_IPC */
#endif /* WIN32_ODBC_CLI2 && PDL_HAS_WINCE */
}

PDL_INLINE void
PDL_OS::tzset (void)
{
# if !defined (PDL_HAS_WINCE) && !defined (VXWORKS) && !defined (PDL_PSOS)
#   if defined (PDL_WIN32)
  ::_tzset ();  // For Win32.
#   else
  ::tzset ();   // For UNIX platforms.
#   endif /* PDL_WIN32 */
# else
  errno = ENOTSUP;
# endif /* !PDL_HAS_WINCE && !VXWORKS && !PDL_PSOS */
}

PDL_INLINE long
PDL_OS::timezone (void)
{
# if !defined (PDL_HAS_WINCE) && !defined (VXWORKS) && !defined (PDL_PSOS) \
&& !defined (CHORUS)
#   if defined (PDL_WIN32)
  return _timezone;  // For Win32.
#   elif defined (__Lynx__) || defined (__FreeBSD__) || defined (PDL_HAS_SUNOS4_GETTIMEOFDAY)
  long result = 0;
  struct timeval time;
  struct timezone zone;
  PDL_UNUSED_ARG (result);
  PDL_OSCALL (::gettimeofday (&time, &zone), int, -1, result);
  return zone.tz_minuteswest * 60;
# elif defined(ITRON)
  /* empty */
  PDL_NOTSUP_RETURN (-1);
#   else
  return ::timezone;   // For UNIX platforms.
#   endif
# else
  PDL_NOTSUP_RETURN (0);
# endif /* !PDL_HAS_WINCE && !VXWORKS && !PDL_PSOS */
}

#if !defined (PDL_LACKS_DIFFTIME)
PDL_INLINE double
PDL_OS::difftime (time_t t1, time_t t2)
{
#if defined (PDL_PSOS) && ! defined (PDL_PSOS_HAS_TIME)
  // simulate difftime ; just subtracting ; PDL_PSOS case
  return ((double)t1) - ((double)t2);
#else
# if defined (PDL_DIFFTIME)
  return PDL_DIFFTIME (t1, t2);
# else
  return ::difftime (t1, t2);
# endif /* PDL_DIFFTIME  && ! PDL_PSOS_HAS_TIME */
#endif // PDL_PSOS
}
#endif /* ! PDL_LACKS_DIFFTIME */

#if !defined (PDL_HAS_MOSTLY_UNICODE_APIS)
PDL_INLINE char *
PDL_OS::ctime (const time_t *t)
{
  PDL_TRACE ("PDL_OS::ctime");
# if defined (PDL_HAS_BROKEN_CTIME)
  PDL_OSCALL_RETURN (::asctime (::localtime (t)), char *, 0);
#elif defined(PDL_PSOS) && ! defined (PDL_PSOS_HAS_TIME)
  return "ctime-return";
# else
  PDL_OSCALL_RETURN (::ctime (t), char *, 0);
# endif    /* PDL_HAS_BROKEN_CTIME) */
}
#endif /* PDL_HAS_MOSTLY_UNICODE_APIS */

#if !defined (PDL_HAS_WINCE)
PDL_INLINE char *
PDL_OS::ctime_r (const time_t *t, char *buf, int buflen)
{
  PDL_TRACE ("PDL_OS::ctime_r");
# if defined (PDL_HAS_REENTRANT_FUNCTIONS)
#   if defined (PDL_HAS_2_PARAM_ASCTIME_R_AND_CTIME_R)
  char *result;
#     if defined (DIGITAL_UNIX)
  PDL_OSCALL (::_Pctime_r (t, buf), char *, 0, result);
#     else
  PDL_OSCALL (::ctime_r (t, buf), char *, 0, result);
#     endif /* DIGITAL_UNIX */
  if (result != 0)
    ::strncpy (buf, result, buflen);
  return buf;
#   else

#     if defined (PDL_CTIME_R_RETURNS_INT)
  return (::ctime_r (t, buf, buflen) == -1 ? 0 : buf);
#     else
  PDL_OSCALL_RETURN (::ctime_r (t, buf, buflen), char *, 0);
#     endif /* PDL_CTIME_R_RETURNS_INT */

#   endif /* defined (PDL_HAS_2_PARAM_ASCTIME_R_AND_CTIME_R) */
# else
#    if defined(PDL_PSOS) && ! defined (PDL_PSOS_HAS_TIME)
   ::strncpy(buf, "ctime-return",buflen);
   return buf;
#    else
  char *result;
  PDL_OSCALL (::ctime (t), char *, 0, result);
  if (result != 0)
    ::strncpy (buf, result, buflen);
  return buf;
#    endif // PDL_PSOS
# endif /* defined (PDL_HAS_REENTRANT_FUNCTIONS) */
}
#else /* PDL_HAS_WINCE */
PDL_INLINE char *
PDL_OS::ctime_r (const time_t *t, char *buf, int buflen)
{
  return _fcvt(*t, sizeof(time_t), NULL, NULL);
}
#endif /* !PDL_HAS_WINCE */


PDL_INLINE struct tm *
PDL_OS::localtime (const time_t *t)
{
#if !defined (PDL_HAS_WINCE) && !defined(PDL_PSOS) || defined (PDL_PSOS_HAS_TIME)
  PDL_TRACE ("PDL_OS::localtime");
  PDL_OSCALL_RETURN (::localtime (t), struct tm *, 0);
#elif defined(PDL_HAS_WINCE)
  /*
  SYSTEMTIME tlocal;
  struct tm  tm_local;
  ::GetLocalTime(&tlocal);
  tm_local.tm_sec = tlocal.wSecond;
  tm_local.tm_min = tlocal.wMinute;
  tm_local.tm_hour = tlocal.wHour;
  tm_local.tm_mday = tlocal.wDay;
  tm_local.tm_mon = tlocal.wMonth;
  tm_local.tm_year = tlocal.wYear;

  return &tm_local;
  */
  PDL_UNUSED_ARG (t);
  PDL_NOTSUP_RETURN (0);
#else
  // @@ Don't you start wondering what kind of functions
  //    does WinCE really support?
  PDL_UNUSED_ARG (t);
  PDL_NOTSUP_RETURN (0);
#endif /* ! PDL_HAS_WINCE */
}

PDL_INLINE struct tm *
PDL_OS::gmtime (const time_t *t)
{
#if !defined (PDL_HAS_WINCE) && !defined (PDL_PSOS) || defined (PDL_PSOS_HAS_TIME)
  PDL_TRACE ("PDL_OS::localtime");
  PDL_OSCALL_RETURN (::gmtime (t), struct tm *, 0);
#else
  // @@ WinCE doesn't have gmtime also.
  PDL_UNUSED_ARG (t);
  PDL_NOTSUP_RETURN (0);
#endif /* ! PDL_HAS_WINCE */
}

PDL_INLINE struct tm *
PDL_OS::gmtime_r (const time_t *t, struct tm *res)
{
  PDL_TRACE ("PDL_OS::localtime_r");
#if defined (PDL_HAS_REENTRANT_FUNCTIONS)
# if defined (DIGITAL_UNIX)
  PDL_OSCALL_RETURN (::_Pgmtime_r (t, res), struct tm *, 0);
# elif defined (HPUX_10)
  return (::gmtime_r (t, res) == 0 ? res : (struct tm *) 0);
# else
  PDL_OSCALL_RETURN (::gmtime_r (t, res), struct tm *, 0);
# endif /* DIGITAL_UNIX */
#elif !defined (PDL_HAS_WINCE) && !defined(PDL_PSOS) || defined (PDL_PSOS_HAS_TIME)
  struct tm *result;
  PDL_OSCALL (::gmtime (t), struct tm *, 0, result) ;
  if (result != 0)
    *res = *result;
  return res;
#else
  // @@ Same as PDL_OS::gmtime (), you need to implement it
  //    yourself.
  PDL_UNUSED_ARG (t);
  PDL_UNUSED_ARG (res);
  PDL_NOTSUP_RETURN (0);
#endif /* PDL_HAS_REENTRANT_FUNCTIONS */
}

PDL_INLINE char *
PDL_OS::asctime (const struct tm *t)
{
#if !defined (PDL_HAS_WINCE) && !defined(PDL_PSOS) || defined (PDL_PSOS_HAS_TIME)
  PDL_TRACE ("PDL_OS::asctime");
  PDL_OSCALL_RETURN (::asctime (t), char *, 0);
#else
  // @@ WinCE doesn't have gmtime also.
  PDL_UNUSED_ARG (t);
  PDL_NOTSUP_RETURN (0);
#endif /* ! PDL_HAS_WINCE */
}

PDL_INLINE char *
PDL_OS::asctime_r (const struct tm *t, char *buf, int buflen)
{
  PDL_TRACE ("PDL_OS::asctime_r");
#if defined (PDL_HAS_REENTRANT_FUNCTIONS)
# if defined (PDL_HAS_2_PARAM_ASCTIME_R_AND_CTIME_R)
  char *result;
#   if defined (DIGITAL_UNIX)
  PDL_OSCALL (::_Pasctime_r (t, buf), char *, 0, result);
#   else
  PDL_OSCALL (::asctime_r (t, buf), char *, 0, result);
#   endif /* DIGITAL_UNIX */
  ::strncpy (buf, result, buflen);
  return buf;
# else
#   if defined (HPUX_10)
  return (::asctime_r(t, buf, buflen) == 0 ? buf : (char *)0);
#   else
  PDL_OSCALL_RETURN (::asctime_r (t, buf, buflen), char *, 0);
#   endif /* HPUX_10 */
# endif /* PDL_HAS_2_PARAM_ASCTIME_R_AND_CTIME_R */
#elif ! defined (PDL_HAS_WINCE) && !defined(PDL_PSOS) || defined (PDL_PSOS_HAS_TIME)
  char *result;
  PDL_OSCALL (::asctime (t), char *, 0, result);
  ::strncpy (buf, result, buflen);
  return buf;
#else
  // @@ Same as PDL_OS::asctime (), you need to implement it
  //    yourself.
  PDL_UNUSED_ARG (t);
  PDL_UNUSED_ARG (buf);
  PDL_UNUSED_ARG (buflen);
  PDL_NOTSUP_RETURN (0);
#endif /* defined (PDL_HAS_REENTRANT_FUNCTIONS) */
}

PDL_INLINE size_t
PDL_OS::strftime (char *s, size_t maxsize, const char *format,
                  const struct tm *timeptr)
{
#if !defined (PDL_HAS_WINCE) && !defined(PDL_PSOS) || defined (PDL_PSOS_HAS_TIME)
  return ::strftime (s, maxsize, format, timeptr);
#else
  PDL_UNUSED_ARG (s);
  PDL_UNUSED_ARG (maxsize);
  PDL_UNUSED_ARG (format);
  PDL_UNUSED_ARG (timeptr);
  PDL_NOTSUP_RETURN (0);
#endif /* PDL_HAS_WINCE */
}
#if !defined(PDL_HAS_WINCE)
PDL_INLINE int
PDL_OS::flock_init (PDL_OS::pdl_flock_t *lock,
                    int flags,
                    LPCTSTR name,
                    mode_t perms)
{
  PDL_TRACE ("PDL_OS::flock_init");
#if defined (CHORUS)
  lock->lockname_ = 0;
  // Let's see if it already exists.
  lock->handle_ = PDL_OS::shm_open (name,
                                    flags | O_CREAT | O_EXCL,
                                    perms);
  if (lock->handle_ == PDL_INVALID_HANDLE)
    {
      if (errno == EEXIST)
        // It's already there, so we'll just open it.
        lock->handle_ = PDL_OS::shm_open (name,
                                          flags | O_CREAT,
                                          PDL_DEFAULT_FILE_PERMS);
      else
        return -1;
    }
  else
    {
      // We own this shared memory object!  Let's set its size.
      if (PDL_OS::ftruncate (lock->handle_,
                             sizeof (PDL_mutex_t)) == -1)
        return -1;
      // Note that only the owner can destroy a file lock...
      PDL_ALLOCATOR_RETURN (lock->lockname_,
                            PDL_OS::strdup (name),
                            -1);
    }
  if (lock->handle_ == PDL_INVALID_HANDLE)
    return -1;

  lock->process_lock_ =
    (PDL_mutex_t *) PDL_OS::mmap (0,
                                  sizeof (PDL_mutex_t),
                                  PROT_RDWR,
                                  MAP_SHARED,
                                  lock->handle_,
                                  0);
  if (lock->process_lock_ == MAP_FAILED)
    return -1;

  if (lock->lockname_
      // Only initialize it if we're the one who created it.
      && PDL_OS::mutex_init (lock->process_lock_,
                             USYNC_PROCESS,
                             name,
                             0) != 0)
        return -1;
  return 0;
#else
#if defined (PDL_WIN32)
  // Once initialized, these values are never changed.
  lock->overlapped_.Internal = 0;
  lock->overlapped_.InternalHigh = 0;
  lock->overlapped_.OffsetHigh = 0;
  lock->overlapped_.hEvent = INVALID_HANDLE_VALUE;
#endif /* PDL_WIN32 */
  lock->handle_ = PDL_INVALID_HANDLE;
  lock->lockname_ = 0;

  if (name != 0)
    {
      PDL_OSCALL (PDL_OS::open (name, flags, perms),
                  PDL_HANDLE,
                  PDL_INVALID_HANDLE,
                  lock->handle_);
      lock->lockname_ = PDL_OS::strdup (name);
      return lock->handle_ == PDL_INVALID_HANDLE ? -1 : 0;
    }
  else
    return 0;
#endif /* CHORUS */
}
#else
PDL_INLINE int
PDL_OS::flock_init (PDL_OS::pdl_flock_t *lock,
                    int flags,
                    const char* name,
                    mode_t perms)
{
  PDL_TRACE ("PDL_OS::flock_init");
#if defined (CHORUS)
  lock->lockname_ = 0;
  // Let's see if it already exists.
  lock->handle_ = PDL_OS::shm_open (name,
                                    flags | O_CREAT | O_EXCL,
                                    perms);
  if (lock->handle_ == PDL_INVALID_HANDLE)
    {
      if (errno == EEXIST)
        // It's already there, so we'll just open it.
        lock->handle_ = PDL_OS::shm_open (name,
                                          flags | O_CREAT,
                                          PDL_DEFAULT_FILE_PERMS);
      else
        return -1;
    }
  else
    {
      // We own this shared memory object!  Let's set its size.
      if (PDL_OS::ftruncate (lock->handle_,
                             sizeof (PDL_mutex_t)) == -1)
        return -1;
      // Note that only the owner can destroy a file lock...
      PDL_ALLOCATOR_RETURN (lock->lockname_,
                            PDL_OS::strdup (name),
                            -1);
    }
  if (lock->handle_ == PDL_INVALID_HANDLE)
    return -1;

  lock->process_lock_ =
    (PDL_mutex_t *) PDL_OS::mmap (0,
                                  sizeof (PDL_mutex_t),
                                  PROT_RDWR,
                                  MAP_SHARED,
                                  lock->handle_,
                                  0);
  if (lock->process_lock_ == MAP_FAILED)
    return -1;

  if (lock->lockname_
      // Only initialize it if we're the one who created it.
      && PDL_OS::mutex_init (lock->process_lock_,
                             USYNC_PROCESS,
                             name,
                             0) != 0)
        return -1;
  return 0;
#else
#if defined (PDL_WIN32)
  // Once initialized, these values are never changed.
  lock->overlapped_.Internal = 0;
  lock->overlapped_.InternalHigh = 0;
  lock->overlapped_.OffsetHigh = 0;
  lock->overlapped_.hEvent = INVALID_HANDLE_VALUE;
#endif /* PDL_WIN32 */
  lock->handle_ = PDL_INVALID_HANDLE;
  lock->lockname_ = 0;

  if (name != 0)
    {
      PDL_OSCALL (PDL_OS::open (name, flags, perms),
                  PDL_HANDLE,
                  PDL_INVALID_HANDLE,
                  lock->handle_);
      lock->lockname_ = PDL_OS::strdup (name);
      return lock->handle_ == PDL_INVALID_HANDLE ? -1 : 0;
    }
  else
    return 0;
#endif /* CHORUS */
}
#endif /*PDL_HAS_WINCE*/
#if defined (PDL_WIN32)
PDL_INLINE void
PDL_OS::adjust_flock_params (PDL_OS::pdl_flock_t *lock,
                             short whence,
                             PDL_OFF_T &start,
                             PDL_OFF_T &len)
{
    switch (whence)
    {
        case SEEK_SET:
            break;
        case SEEK_CUR:
            start += SetFilePointer (lock->handle_, 0, 0, FILE_CURRENT);
            break;
        case SEEK_END:
            start = PDL_OS::filesize(lock->handle_);
            break;
    }
    LARGE_INTEGER li;
    li.QuadPart = start;
    lock->overlapped_.Offset = li.LowPart;
    lock->overlapped_.OffsetHigh = li.HighPart;
    if (len == 0)
    {
        len = PDL_OS::filesize(lock->handle_) - start;
    }
}
#endif /* PDL_WIN32 */

PDL_INLINE int
PDL_OS::flock_wrlock (PDL_OS::pdl_flock_t *lock,
                      short whence,
                      PDL_OFF_T start,
                      PDL_OFF_T len)
{
  PDL_TRACE ("PDL_OS::flock_wrlock");
#if defined (PDL_WIN32) && !defined (PDL_HAS_WINCE)
  PDL_OS::adjust_flock_params (lock, whence, start, len);
#  if defined (PDL_HAS_WINNT4) && (PDL_HAS_WINNT4 != 0)
  LARGE_INTEGER li;
  li.QuadPart = len;
  PDL_WIN32CALL_RETURN (PDL_ADAPT_RETVAL (::LockFileEx (lock->handle_,
                                                        LOCKFILE_EXCLUSIVE_LOCK,
                                                        0,
                                                        li.LowPart,
                                                        li.HighPart,
                                                        &lock->overlapped_),
                                          pdl_result_), int, -1);
#  else /* PDL_HAS_WINNT4 && (PDL_HAS_WINNT4 != 0) */
  LARGE_INTEGER li;
  li.QuadPart = len;
  PDL_WIN32CALL_RETURN (PDL_ADAPT_RETVAL (::LockFile (lock->handle_,
                                                      lock->overlapped_.Offset,
                                                      lock->overlapped_.OffsetHigh,
                                                      li.LowPart,
                                                      li.HighPart),
                                          pdl_result_), int, -1);
#  endif /* PDL_HAS_WINNT4 && (PDL_HAS_WINNT4 != 0) */
#elif defined (CHORUS)
  PDL_UNUSED_ARG (whence);
  PDL_UNUSED_ARG (start);
  PDL_UNUSED_ARG (len);
  return PDL_OS::mutex_lock (lock->process_lock_);
#elif defined (PDL_LACKS_FILELOCKS)
  PDL_UNUSED_ARG (lock);
  PDL_UNUSED_ARG (whence);
  PDL_UNUSED_ARG (start);
  PDL_UNUSED_ARG (len);
  PDL_NOTSUP_RETURN (-1);
#else
  lock->lock_.l_whence = whence;
  lock->lock_.l_start = start;
  lock->lock_.l_len = len;
  lock->lock_.l_type = F_WRLCK;         // set write lock
  // block, if no access
  PDL_OSCALL_RETURN (PDL_OS::fcntl (lock->handle_, F_SETLKW,
                                    PDL_reinterpret_cast (long, &lock->lock_)),
                     int, -1);
#endif /* PDL_WIN32 */
}

PDL_INLINE int
PDL_OS::flock_rdlock (PDL_OS::pdl_flock_t *lock,
                      short whence,
                      PDL_OFF_T start,
                      PDL_OFF_T len)
{
  PDL_TRACE ("PDL_OS::flock_rdlock");
#if defined (PDL_WIN32) && !defined (PDL_HAS_WINCE)
  PDL_OS::adjust_flock_params (lock, whence, start, len);
#  if defined (PDL_HAS_WINNT4) && (PDL_HAS_WINNT4 != 0)
  LARGE_INTEGER li;
  li.QuadPart = len;
  PDL_WIN32CALL_RETURN (PDL_ADAPT_RETVAL (::LockFileEx (lock->handle_,
                                                        0,
                                                        0,
                                                        li.LowPart,
                                                        li.HighPart,
                                                        &lock->overlapped_),
                                          pdl_result_), int, -1);
#  else /* PDL_HAS_WINNT4 && (PDL_HAS_WINNT4 != 0) */
  LARGE_INTEGER li;
  li.QuadPart = len;
  PDL_WIN32CALL_RETURN (PDL_ADAPT_RETVAL (::LockFile (lock->handle_,
                                                      lock->overlapped_.Offset,
                                                      lock->overlapped_.OffsetHigh,
                                                      li.LowPart,
                                                      li.HighPart),
                                          pdl_result_), int, -1);
#  endif /* PDL_HAS_WINNT4 && (PDL_HAS_WINNT4 != 0) */
#elif defined (CHORUS)
  PDL_UNUSED_ARG (whence);
  PDL_UNUSED_ARG (start);
  PDL_UNUSED_ARG (len);
  return PDL_OS::mutex_lock (lock->process_lock_);
#elif defined (PDL_LACKS_FILELOCKS)
  PDL_UNUSED_ARG (lock);
  PDL_UNUSED_ARG (whence);
  PDL_UNUSED_ARG (start);
  PDL_UNUSED_ARG (len);
  PDL_NOTSUP_RETURN (-1);
#else
  lock->lock_.l_whence = whence;
  lock->lock_.l_start = start;
  lock->lock_.l_len = len;
  lock->lock_.l_type = F_RDLCK;         // set read lock
  // block, if no access
  PDL_OSCALL_RETURN (PDL_OS::fcntl (lock->handle_, F_SETLKW,
                                    PDL_reinterpret_cast (long, &lock->lock_)),
                     int, -1);
#endif /* PDL_WIN32 */
}

PDL_INLINE int
PDL_OS::flock_trywrlock (PDL_OS::pdl_flock_t *lock,
                         short whence,
                         PDL_OFF_T start,
                         PDL_OFF_T len)
{
  PDL_TRACE ("PDL_OS::pdl_flock_trywrlock");
#if defined (PDL_WIN32) && !defined (PDL_HAS_WINCE)
#  if defined (PDL_HAS_WINNT4) && (PDL_HAS_WINNT4 != 0)
  PDL_OS::adjust_flock_params (lock, whence, start, len);

  LARGE_INTEGER li;
  li.QuadPart = len;
  PDL_WIN32CALL_RETURN (PDL_ADAPT_RETVAL (::LockFileEx (lock->handle_,
                                                        LOCKFILE_FAIL_IMMEDIATELY | LOCKFILE_EXCLUSIVE_LOCK,
                                                        0,
                                                        li.LowPart,
                                                        li.HighPart,
                                                        &lock->overlapped_),
                                          pdl_result_), int, -1);
#  else /* PDL_HAS_WINNT4 && (PDL_HAS_WINNT4 != 0) */
  PDL_UNUSED_ARG (lock);
  PDL_UNUSED_ARG (whence);
  PDL_UNUSED_ARG (start);
  PDL_UNUSED_ARG (len);
  PDL_NOTSUP_RETURN (-1);
#  endif /* PDL_HAS_WINNT4 && (PDL_HAS_WINNT4 != 0) */
#elif defined (CHORUS)
  PDL_UNUSED_ARG (whence);
  PDL_UNUSED_ARG (start);
  PDL_UNUSED_ARG (len);
  return PDL_OS::mutex_trylock (lock->process_lock_);
#elif defined (PDL_LACKS_FILELOCKS)
  PDL_UNUSED_ARG (lock);
  PDL_UNUSED_ARG (whence);
  PDL_UNUSED_ARG (start);
  PDL_UNUSED_ARG (len);
  PDL_NOTSUP_RETURN (-1);
#else
  lock->lock_.l_whence = whence;
  lock->lock_.l_start = start;
  lock->lock_.l_len = len;
  lock->lock_.l_type = F_WRLCK;         // set write lock

  int result = 0;
  // Does not block, if no access, returns -1 and set errno = EBUSY;
  PDL_OSCALL (PDL_OS::fcntl (lock->handle_,
                             F_SETLK,
                             PDL_reinterpret_cast (long, &lock->lock_)),
              int, -1, result);

# if ! defined (PDL_PSOS)
  if (result == -1 && (errno == EACCES || errno == EAGAIN))
    errno = EBUSY;
# endif /* ! defined (PDL_PSOS) */

  return result;
#endif /* PDL_WIN32 */
}

PDL_INLINE int
PDL_OS::flock_tryrdlock (PDL_OS::pdl_flock_t *lock,
                         short whence,
                         PDL_OFF_T start,
                         PDL_OFF_T len)
{
  PDL_TRACE ("PDL_OS::pdl_flock_tryrdlock");
#if defined (PDL_WIN32) && !defined (PDL_HAS_WINCE)
#  if defined (PDL_HAS_WINNT4) && (PDL_HAS_WINNT4 != 0)
  PDL_OS::adjust_flock_params (lock, whence, start, len);

  LARGE_INTEGER li;
  li.QuadPart = len;
  PDL_WIN32CALL_RETURN (PDL_ADAPT_RETVAL (::LockFileEx (lock->handle_,
                                                        LOCKFILE_FAIL_IMMEDIATELY,
                                                        0,
                                                        li.LowPart,
                                                        li.HighPart,
                                                        &lock->overlapped_),
                                          pdl_result_), int, -1);
#  else /* PDL_HAS_WINNT4 && (PDL_HAS_WINNT4 != 0) */
  PDL_UNUSED_ARG (lock);
  PDL_UNUSED_ARG (whence);
  PDL_UNUSED_ARG (start);
  PDL_UNUSED_ARG (len);
  PDL_NOTSUP_RETURN (-1);
#  endif /* PDL_HAS_WINNT4 && (PDL_HAS_WINNT4 != 0) */
#elif defined (CHORUS)
  PDL_UNUSED_ARG (whence);
  PDL_UNUSED_ARG (start);
  PDL_UNUSED_ARG (len);
  return PDL_OS::mutex_trylock (lock->process_lock_);
#elif defined (PDL_LACKS_FILELOCKS)
  PDL_UNUSED_ARG (lock);
  PDL_UNUSED_ARG (whence);
  PDL_UNUSED_ARG (start);
  PDL_UNUSED_ARG (len);
  PDL_NOTSUP_RETURN (-1);
#else
  lock->lock_.l_whence = whence;
  lock->lock_.l_start = start;
  lock->lock_.l_len = len;
  lock->lock_.l_type = F_RDLCK;         // set read lock

  int result = 0;
  // Does not block, if no access, returns -1 and set errno = EBUSY;
  PDL_OSCALL (PDL_OS::fcntl (lock->handle_, F_SETLK,
                             PDL_reinterpret_cast (long, &lock->lock_)),
              int, -1, result);

# if ! defined (PDL_PSOS)
  if (result == -1 && (errno == EACCES || errno == EAGAIN))
    errno = EBUSY;
# endif /* ! defined (PDL_PSOS) */

  return result;
#endif /* PDL_WIN32 */
}

PDL_INLINE int
PDL_OS::flock_unlock (PDL_OS::pdl_flock_t *lock,
                      short whence,
                      PDL_OFF_T start,
                      PDL_OFF_T len)
{
  PDL_TRACE ("PDL_OS::flock_unlock");
#if defined (PDL_WIN32) && !defined (PDL_HAS_WINCE)
  PDL_OS::adjust_flock_params (lock, whence, start, len);

  LARGE_INTEGER li;
  li.QuadPart = len;
  PDL_WIN32CALL_RETURN (PDL_ADAPT_RETVAL (::UnlockFile (lock->handle_,
                                                        lock->overlapped_.Offset,
                                                        lock->overlapped_.OffsetHigh,
                                                        li.LowPart,
                                                        li.HighPart),
                                          pdl_result_), int, -1);
#elif defined (CHORUS)
  PDL_UNUSED_ARG (whence);
  PDL_UNUSED_ARG (start);
  PDL_UNUSED_ARG (len);
  return PDL_OS::mutex_unlock (lock->process_lock_);
#elif defined (PDL_LACKS_FILELOCKS)
  PDL_UNUSED_ARG (lock);
  PDL_UNUSED_ARG (whence);
  PDL_UNUSED_ARG (start);
  PDL_UNUSED_ARG (len);
  PDL_NOTSUP_RETURN (-1);
#else
  lock->lock_.l_whence = whence;
  lock->lock_.l_start = start;
  lock->lock_.l_len = len;
  lock->lock_.l_type = F_UNLCK;   // Unlock file.

  // release lock
  PDL_OSCALL_RETURN (PDL_OS::fcntl (lock->handle_, F_SETLK,
                                    PDL_reinterpret_cast (long, &lock->lock_)),
                     int, -1);
#endif /* PDL_WIN32 */
}

PDL_INLINE int
PDL_OS::flock_destroy (PDL_OS::pdl_flock_t *lock)
{
  PDL_TRACE ("PDL_OS::flock_destroy");
  if (lock->handle_ != PDL_INVALID_HANDLE)
    {
      PDL_OS::flock_unlock (lock);
      // Close the handle.
      PDL_OS::close (lock->handle_);
      lock->handle_ = PDL_INVALID_HANDLE;
#if defined (CHORUS)
      // Are we the owner?
      if (lock->process_lock_ && lock->lockname_ != 0)
        {
          // Only destroy the lock if we're the owner
          PDL_OS::mutex_destroy (lock->process_lock_);
          PDL_OS::munmap (lock->process_lock_,
                          sizeof (PDL_mutex_t));
          PDL_OS::shm_unlink (lock->lockname_);
          PDL_OS::free (PDL_static_cast (void *,
                                         PDL_const_cast (LPTSTR,
                                                         lock->lockname_)));
        }
      else if (lock->process_lock_)
        // Just unmap the memory.
        PDL_OS::munmap (lock->process_lock_,
                     sizeof (PDL_mutex_t));
#else
      if (lock->lockname_ != 0)
        {
          PDL_OS::unlink (lock->lockname_);
          PDL_OS::free (PDL_static_cast (void *,
                                         PDL_const_cast (LPTSTR,
                                                         lock->lockname_)));
        }
#endif /* CHORUS */
      lock->lockname_ = 0;
    }
  return 0;
}

PDL_INLINE int
PDL_OS::execv (const char *path,
               char *const argv[])
{
  PDL_TRACE ("PDL_OS::execv");
#if defined (PDL_LACKS_EXEC)
  PDL_UNUSED_ARG (path);
  PDL_UNUSED_ARG (argv);

  PDL_NOTSUP_RETURN (-1);
#elif defined (CHORUS)
  KnCap cactorcap;
  int result = ::afexecv (path, &cactorcap, 0, argv);
  if (result != -1)
    PDL_OS::actorcaps_[result] = cactorcap;
  return result;
#elif defined (PDL_WIN32)
# if defined (__BORLANDC__) /* VSB */
  return ::execv (path, argv);
# elif defined (__MINGW32__)
  return ::_execvp (path, (char *const *) argv);
# else
  return ::_execv (path, (const char *const *) argv);
# endif /* __BORLANDC__ */
#elif defined (PDL_LACKS_POSIX_PROTOTYPES)
  PDL_OSCALL_RETURN (::execv (path, (const char **) argv), int, -1);
#else
  PDL_OSCALL_RETURN (::execv (path, argv), int, -1);
#endif /* PDL_WIN32 */
}

PDL_INLINE int
PDL_OS::execve (const char *path,
                char *const argv[],
                char *const envp[])
{
  PDL_TRACE ("PDL_OS::execve");
#if defined (PDL_LACKS_EXEC)
  PDL_UNUSED_ARG (path);
  PDL_UNUSED_ARG (argv);
  PDL_UNUSED_ARG (envp);

  PDL_NOTSUP_RETURN (-1);
#elif defined(CHORUS)
  KnCap cactorcap;
  int result = ::afexecve (path, &cactorcap, 0, argv, envp);
  if (result != -1)
    PDL_OS::actorcaps_[result] = cactorcap;
  return result;
#elif defined (PDL_WIN32)
# if defined (__BORLANDC__) /* VSB */
  return ::execve (path, argv, envp);
# elif defined (__MINGW32__)
  return ::_execve (path, (char *const *) argv, (char *const *) envp);
# else
  return ::_execve (path, (const char *const *) argv, (const char *const *) envp);
# endif /* __BORLANDC__ */
#elif defined (PDL_LACKS_POSIX_PROTOTYPES)
  PDL_OSCALL_RETURN (::execve (path, (const char **) argv, (char **) envp), int, -1);
#else
  PDL_OSCALL_RETURN (::execve (path, argv, envp), int, -1);
#endif /* PDL_WIN32 */
}

PDL_INLINE int
PDL_OS::execvp (const char *file,
                char *const argv[])
{
  PDL_TRACE ("PDL_OS::execvp");
#if defined (PDL_LACKS_EXEC)
  PDL_UNUSED_ARG (file);
  PDL_UNUSED_ARG (argv);

  PDL_NOTSUP_RETURN (-1);
#elif defined(CHORUS)
  KnCap cactorcap;
  int result = ::afexecvp (file, &cactorcap, 0, argv);
  if (result != -1)
    PDL_OS::actorcaps_[result] = cactorcap;
  return result;
#elif defined (PDL_WIN32)
# if defined (__BORLANDC__) /* VSB */
  return ::execvp (file, argv);
# elif defined (__MINGW32__)
  return ::_execvp (file, (char *const *) argv);
# else
  return ::_execvp (file, (const char *const *) argv);
# endif /* __BORLANDC__ */
#elif defined (PDL_LACKS_POSIX_PROTOTYPES)
  PDL_OSCALL_RETURN (::execvp (file, (const char **) argv), int, -1);
#else
  PDL_OSCALL_RETURN (::execvp (file, argv), int, -1);
#endif /* PDL_WIN32 */
}

#if !defined (PDL_HAS_WINCE)
PDL_INLINE FILE *
PDL_OS::fdopen (PDL_HANDLE handle, const char *mode)
{
  PDL_TRACE ("PDL_OS::fdopen");
# if defined (PDL_WIN32)
  // kernel file handle -> FILE* conversion...
  // Options: _O_APPEND, _O_RDONLY and _O_TEXT are lost

  FILE *file = 0;

#   if defined (PDL_WIN64)
  int crt_handle = ::_open_osfhandle ((intptr_t) handle, 0);
#   else
  int crt_handle = ::_open_osfhandle ((long) handle, 0);
#   endif /* PDL_WIN64 */

  if (crt_handle != -1)
    {
#   if defined(__BORLANDC__) /* VSB */
      file = ::_fdopen (crt_handle, (char *) mode);
#   else
      file = ::_fdopen (crt_handle, mode);
#   endif /* __BORLANDC__ */

      if (!file)
        {
#   if (defined(__BORLANDC__) && __BORLANDC__ >= 0x0530)
          ::_rtl_close (crt_handle);
#   else
          ::_close (crt_handle);
#   endif /* (defined(__BORLANDC__) && __BORLANDC__ >= 0x0530) */
        }
    }

  return file;
# elif defined (PDL_PSOS)
  // @@ it may be possible to implement this for pSOS,
  // but it isn't obvious how to do this (perhaps via
  // f_stat to glean the default volume, and then open_fn ?)
  PDL_UNUSED_ARG (handle);
  PDL_UNUSED_ARG (mode);
  PDL_NOTSUP_RETURN (0);
# else
  PDL_OSCALL_RETURN (::fdopen (handle, mode), FILE *, 0);
# endif /* PDL_WIN32 */
}
#endif /* ! PDL_HAS_WINCE */

PDL_INLINE int
PDL_OS::getrlimit (int resource, struct rlimit *rl)
{
  PDL_TRACE ("PDL_OS::getrlimit");

#if defined (PDL_LACKS_RLIMIT)

/*BUGBUG_NT*/
#if defined (PDL_HAS_WINCE)
  *rl = win32_rlimit;
  return 0;
#elif defined (VXWORKS)
  *rl = vxworks_rlimit;
  return 0;
#elif defined(SYMBIAN)
  PDL_UNUSED_ARG(resource);
  *rl = symbian_rlimit;
  return 0;
#elif defined (PDL_WIN32)
  PDL_OSCALL_RETURN (::getrlimit (resource, rl), int, -1);
#else
  PDL_UNUSED_ARG (resource);
  PDL_UNUSED_ARG (rl);

  PDL_NOTSUP_RETURN (-1);
#endif
/*BUGBUG_NT*/

#else
  /* BUG-18271 linux kernel 2.4 이하 버전에서는
     자체 구현된 rlimit 함수를 써야 한다.*/
# if defined (PDL_HAS_RLIMIT_RESOURCE_ENUM)
  PDL_OSCALL_RETURN (::getrlimit ((PDL_HAS_RLIMIT_RESOURCE_ENUM) resource, rl), int, -1);
# else
  PDL_OSCALL_RETURN (::getrlimit (resource, rl), int, -1);
# endif /* PDL_HAS_RLIMIT_RESOURCE_ENUM */
#endif /* PDL_LACKS_RLIMIT */
}

PDL_INLINE int
PDL_OS::setrlimit (int resource, PDL_SETRLIMIT_TYPE *rl)
{
  PDL_TRACE ("PDL_OS::setrlimit");

#if defined (PDL_LACKS_RLIMIT)
# if defined (PDL_HAS_WINCE)
  win32_rlimit = *rl;
  return 0;
# elif defined (VXWORKS)
  vxworks_rlimit = *rl;
  return 0;
# elif defined(SYMBIAN)
  PDL_UNUSED_ARG(resource);
  symbian_rlimit = *rl;
  return 0;
# elif defined (PDL_WIN32)
  PDL_OSCALL_RETURN (::setrlimit (resource, rl), int, -1);
# else
  PDL_UNUSED_ARG (resource);
  PDL_UNUSED_ARG (rl);
  PDL_NOTSUP_RETURN (-1);
# endif
#else

# if defined (PDL_HAS_RLIMIT_RESOURCE_ENUM)
  PDL_OSCALL_RETURN (::setrlimit ((PDL_HAS_RLIMIT_RESOURCE_ENUM) resource, rl), int, -1);
# else
  PDL_OSCALL_RETURN (::setrlimit (resource, rl), int, -1);
# endif /* PDL_HAS_RLIMIT_RESOURCE_ENUM */
#endif /* PDL_LACKS_RLIMIT */
}

PDL_INLINE int
PDL_OS::socketpair (int domain, int type,
                    int protocol, PDL_SOCKET sv[2])
{
  PDL_TRACE ("PDL_OS::socketpair");

#if defined (PDL_WIN32) || defined (PDL_LACKS_SOCKETPAIR)
  PDL_UNUSED_ARG (domain);
  PDL_UNUSED_ARG (type);
  PDL_UNUSED_ARG (protocol);
  PDL_UNUSED_ARG (sv);

  PDL_NOTSUP_RETURN (-1);
#else
  PDL_OSCALL_RETURN (::socketpair (domain, type, protocol, sv),
                     int, -1);
#endif /* PDL_WIN32 */
}

PDL_INLINE PDL_HANDLE
PDL_OS::dup (PDL_HANDLE handle)
{
  PDL_TRACE ("PDL_OS::dup");

#if defined (PDL_WIN32) && !defined (PDL_HAS_WINCE)
  PDL_HANDLE new_fd;
  if (::DuplicateHandle(::GetCurrentProcess (),
                        handle,
                        ::GetCurrentProcess(),
                        &new_fd,
                        0,
                        TRUE,
                        DUPLICATE_SAME_ACCESS))
    return new_fd;
  else
    PDL_FAIL_RETURN (PDL_INVALID_HANDLE);
  /* NOTREACHED */
#elif defined (VXWORKS) || defined (PDL_PSOS)
  PDL_UNUSED_ARG (handle);
  PDL_NOTSUP_RETURN (-1);
#elif defined (PDL_HAS_WINCE)
  PDL_UNUSED_ARG (handle);
  PDL_NOTSUP_RETURN (0);
#elif defined(ITRON)
  /* empty */
  PDL_NOTSUP_RETURN (-1);
#else
  PDL_OSCALL_RETURN (::dup (handle), PDL_HANDLE, PDL_INVALID_HANDLE);
#endif /* PDL_WIN32 */
}

PDL_INLINE int
PDL_OS::dup2 (PDL_HANDLE oldhandle, PDL_HANDLE newhandle)
{
  PDL_TRACE ("PDL_OS::dup2");
#if defined (PDL_WIN32) || defined (VXWORKS) || defined (PDL_PSOS)
  // msvcrt has _dup2 ?!
  PDL_UNUSED_ARG (oldhandle);
  PDL_UNUSED_ARG (newhandle);

  PDL_NOTSUP_RETURN (-1);
#elif defined(ITRON)
  /* empty */
  PDL_NOTSUP_RETURN (-1);
#else
  PDL_OSCALL_RETURN (::dup2 (oldhandle, newhandle), int, -1);
#endif /* PDL_WIN32 */
}

#if defined (ghs) && defined (PDL_HAS_PENTIUM)
  extern "C" PDL_hrtime_t PDL_gethrtime ();
#endif /* ghs && PDL_HAS_PENTIUM */

PDL_INLINE PDL_hrtime_t
PDL_OS::gethrtime (const PDL_HRTimer_Op op)
{
  PDL_TRACE ("PDL_OS::gethrtime");
#if defined (PDL_HAS_HI_RES_TIMER)
  PDL_UNUSED_ARG (op);
  return ::gethrtime ();
#elif defined (PDL_HAS_AIX_HI_RES_TIMER)
  PDL_UNUSED_ARG (op);
  timebasestruct_t tb;

  ::read_real_time(&tb, TIMEBASE_SZ);
  ::time_base_to_time(&tb, TIMEBASE_SZ);

  return PDL_hrtime_t(tb.tb_high) * PDL_ONE_SECOND_IN_NSECS + tb.tb_low;
#elif defined (ghs) && defined (PDL_HAS_PENTIUM)
  PDL_UNUSED_ARG (op);
  // Use .obj/gethrtime.o, which was compiled with g++.
  return PDL_gethrtime ();
#elif (defined(__KCC) || defined (__GNUG__)) && !defined (__MINGW32__) && defined (PDL_HAS_PENTIUM)
  PDL_UNUSED_ARG (op);

# if defined (PDL_LACKS_LONGLONG_T)
  double now;
# else  /* ! PDL_LACKS_LONGLONG_T */
  PDL_hrtime_t now;
# endif /* ! PDL_LACKS_LONGLONG_T */

  // See comments about the RDTSC Pentium instruction for the PDL_WIN32
  // version of PDL_OS::gethrtime (), below.
  //
  // Read the high-res tick counter directly into memory variable "now".
  // The A constraint signifies a 64-bit int.
  asm volatile ("rdtsc" : "=A" (now) : : "memory");

# if defined (PDL_LACKS_LONGLONG_T)
  PDL_UINT32 least, most;
  PDL_OS::memcpy (&least, &now, sizeof (PDL_UINT32));
  PDL_OS::memcpy (&most, (u_char *) &now + sizeof (PDL_UINT32),
                  sizeof (PDL_UINT32));

  PDL_hrtime_t ret (least, most);
  return ret;
# else  /* ! PDL_LACKS_LONGLONG_T */
  return now;
# endif /* ! PDL_LACKS_LONGLONG_T */
#elif defined (linux) && defined (PDL_HAS_ALPHA_TIMER)
  // NOTE:  alphas only have a 32 bit tick (cycle) counter.  The rpcc
  // instruction actually reads 64 bits, but the high 32 bits are
  // implementation-specific.  Linux and Digital Unix, for example,
  // use them for virtual tick counts, i.e., taking into account only
  // the time that the process was running.  This information is from
  // David Mosberger's article, see comment below.
  PDL_UINT32 now;

  // The following statement is based on code published by:
  // Mosberger, David, "How to Make Your Applications Fly, Part 1",
  // Linux Journal Issue 42, October 1997, page 50.  It reads the
  // high-res tick counter directly into the memory variable.
  asm volatile ("rpcc %0" : "=r" (now) : : "memory");

  return now;
#elif defined (PDL_WIN32) && defined (PDL_HAS_PENTIUM)
  LARGE_INTEGER freq;

  ::QueryPerformanceCounter (&freq);

  return freq.QuadPart;

#elif defined (CHORUS)
  if (op == PDL_OS::PDL_HRTIMER_GETTIME)
    {
      struct timespec ts;

      PDL_OS::clock_gettime (CLOCK_REALTIME, &ts);

      // Carefully create the return value to avoid arithmetic overflow
      // if PDL_hrtime_t is PDL_U_LongLong.
      PDL_hrtime_t now = ts.tv_sec;
      now *= PDL_U_ONE_SECOND_IN_NSECS;
      now += ts.tv_nsec;

      return now;
    }
  else
    {
      // Use the sysBench timer on Chorus.  On MVME177, at least, it only
      // has 32 bits.  Be careful, because using it disables interrupts!
      PDL_UINT32 now;
      if (::sysBench (op, (int *) &now) == K_OK)
        {
          now *= 1000u /* nanoseconds/microsecond */;
          return (PDL_hrtime_t) now;
        }
      else
        {
          // Something went wrong.  Just return 0.
          return (PDL_hrtime_t) 0;
        }
    }

#elif defined (PDL_HAS_POWERPC_TIMER) && (defined (ghs) || defined (__GNUG__))
  // PowerPC w/ GreenHills or g++.

  PDL_UNUSED_ARG (op);
  u_long most;
  u_long least;
  PDL_OS::readPPCTimeBase (most, least);
#if defined (PDL_LACKS_LONGLONG_T)
  return PDL_U_LongLong (least, most);
#else  /* ! PDL_LACKS_LONGLONG_T */
  return 0x100000000llu * most  +  least;
#endif /* ! PDL_LACKS_LONGLONG_T */

#elif defined (PDL_HAS_CLOCK_GETTIME) || defined (PDL_PSOS)
  // e.g., VxWorks (besides POWERPC && GreenHills) . . .
  PDL_UNUSED_ARG (op);
  struct timespec ts;

  PDL_OS::clock_gettime (CLOCK_REALTIME, &ts);

  // Carefully create the return value to avoid arithmetic overflow
  // if PDL_hrtime_t is PDL_U_LongLong.
  return PDL_static_cast (PDL_hrtime_t, ts.tv_sec) *
    PDL_U_ONE_SECOND_IN_NSECS  +  PDL_static_cast (PDL_hrtime_t, ts.tv_nsec);
#else
  PDL_UNUSED_ARG (op);
  const PDL_Time_Value now = PDL_OS::gettimeofday ();

  // Carefully create the return value to avoid arithmetic overflow
  // if PDL_hrtime_t is PDL_U_LongLong.
  return (PDL_static_cast (PDL_hrtime_t, now.sec ()) * (PDL_UINT32) 1000000  +
          PDL_static_cast (PDL_hrtime_t, now.usec ())) * (PDL_UINT32) 1000;
#endif /* PDL_HAS_HI_RES_TIMER */
}

PDL_INLINE int
PDL_OS::fdetach (const char *file)
{
  PDL_TRACE ("PDL_OS::fdetach");
#if defined (PDL_HAS_STREAM_PIPES)
  PDL_OSCALL_RETURN (::fdetach (file), int, -1);
#else
  PDL_UNUSED_ARG (file);

  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_HAS_STREAM_PIPES */
}

PDL_INLINE int
PDL_OS::fattach (int handle, const char *path)
{
  PDL_TRACE ("PDL_OS::fattach");
#if defined (PDL_HAS_STREAM_PIPES)
  PDL_OSCALL_RETURN (::fattach (handle, path), int, -1);
#else
  PDL_UNUSED_ARG (handle);
  PDL_UNUSED_ARG (path);

  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_HAS_STREAM_PIPES */
}

PDL_INLINE pid_t
PDL_OS::fork (void)
{
  PDL_TRACE ("PDL_OS::fork");
#if defined (PDL_LACKS_FORK)
  PDL_NOTSUP_RETURN (pid_t (-1));
#else
  PDL_OSCALL_RETURN (::fork (), pid_t, -1);
#endif /* PDL_WIN32 */
}

PDL_INLINE int
PDL_OS::getpagesize (void)
{
  PDL_TRACE ("PDL_OS::getpagesize");
#if defined (PDL_WIN32) && !defined (PDL_HAS_PHARLAP)
  SYSTEM_INFO sys_info;
  ::GetSystemInfo (&sys_info);
  return (int) sys_info.dwPageSize;
#elif defined (_SC_PAGESIZE)
  return (int) ::sysconf (_SC_PAGESIZE);
#elif defined (PDL_HAS_GETPAGESIZE)
  return ::getpagesize ();
#else
  // Use the default set in config.h
  return PDL_PAGE_SIZE;
#endif /* PDL_WIN32 */
}

PDL_INLINE int
PDL_OS::allocation_granularity (void)
{
#if defined (PDL_WIN32)
  SYSTEM_INFO sys_info;
  ::GetSystemInfo (&sys_info);
  return (int) sys_info.dwAllocationGranularity;
#else
  return PDL_OS::getpagesize ();
#endif /* PDL_WIN32 */
}

PDL_INLINE pid_t
PDL_OS::getpid (void)
{
  // PDL_TRACE ("PDL_OS::getpid");
#if defined (PDL_WIN32)
  return ::GetCurrentProcessId ();
#elif defined (VXWORKS) || defined (PDL_PSOS)
  // getpid() is not supported:  just one process anyways
  return ::taskIdSelf();
#elif defined (CHORUS)
  return (pid_t) (::agetId ());
#elif defined(ITRON)
  /* empty */
  PDL_NOTSUP_RETURN (-1);
#else
  PDL_OSCALL_RETURN (::getpid (), int, -1);
#endif /* PDL_WIN32 */
}

PDL_INLINE pid_t
PDL_OS::getpgid (pid_t pid)
{
  PDL_TRACE ("PDL_OS::getpgid");
#if defined (PDL_LACKS_GETPGID)
  PDL_UNUSED_ARG (pid);
  PDL_NOTSUP_RETURN (-1);
#elif defined (VXWORKS) || defined (PDL_PSOS)
  // getpgid() is not supported, only one process anyway.
  PDL_UNUSED_ARG (pid);
  return 0;
#elif defined(ITRON)
  /* empty */
  PDL_NOTSUP_RETURN (-1);
#elif defined (linux) && __GLIBC__ > 1 && __GLIBC_MINOR__ >= 0
  // getpgid() is from SVR4, which appears to be the reason why GLIBC
  // doesn't enable its prototype by default.
  // Rather than create our own extern prototype, just use the one
  // that is visible (ugh).
  PDL_OSCALL_RETURN (::__getpgid (pid), pid_t, -1);
#else
  PDL_OSCALL_RETURN (::getpgid (pid), pid_t, -1);
#endif /* PDL_WIN32 */
}

PDL_INLINE pid_t
PDL_OS::getppid (void)
{
  PDL_TRACE ("PDL_OS::getppid");
#if defined (PDL_LACKS_GETPPID)
  PDL_NOTSUP_RETURN (-1);
#elif defined (VXWORKS) || defined (PDL_PSOS)
  // getppid() is not supported, only one process anyway.
  return 0;
#elif defined(ITRON)
  /* empty */
  PDL_NOTSUP_RETURN (-1);
#else
  PDL_OSCALL_RETURN (::getppid (), pid_t, -1);
#endif /* PDL_WIN32 */
}

PDL_INLINE int
PDL_OS::setpgid (pid_t pid, pid_t pgid)
{
  PDL_TRACE ("PDL_OS::setpgid");
#if defined (PDL_LACKS_SETPGID)
  PDL_UNUSED_ARG (pid);
  PDL_UNUSED_ARG (pgid);
  PDL_NOTSUP_RETURN (-1);
#elif defined (VXWORKS) || defined (PDL_PSOS)
  // <setpgid> is not supported, only one process anyway.
  PDL_UNUSED_ARG (pid);
  PDL_UNUSED_ARG (pgid);
  return 0;
#elif defined(ITRON)
  /* empty */
  PDL_NOTSUP_RETURN (-1);
#else
  PDL_OSCALL_RETURN (::setpgid (pid, pgid), int, -1);
#endif /* PDL_WIN32 */
}

PDL_INLINE int
PDL_OS::setreuid (uid_t ruid, uid_t euid)
{
  PDL_TRACE ("PDL_OS::setreuid");
#if defined (PDL_LACKS_SETREUID)
  PDL_UNUSED_ARG (ruid);
  PDL_UNUSED_ARG (euid);
  PDL_NOTSUP_RETURN (-1);
#elif defined (VXWORKS) || defined (PDL_PSOS)
  // <setpgid> is not supported, only one process anyway.
  PDL_UNUSED_ARG (ruid);
  PDL_UNUSED_ARG (euid);
  return 0;
#elif defined(ITRON)
  /* empty */
  PDL_NOTSUP_RETURN (-1);
#else
  PDL_OSCALL_RETURN (::setreuid (ruid, euid), int, -1);
#endif /* PDL_WIN32 */
}

PDL_INLINE int
PDL_OS::setregid (gid_t rgid, gid_t egid)
{
  PDL_TRACE ("PDL_OS::setregid");
#if defined (PDL_LACKS_SETREGID)
  PDL_UNUSED_ARG (rgid);
  PDL_UNUSED_ARG (egid);
  PDL_NOTSUP_RETURN (-1);
#elif defined (VXWORKS) || defined (PDL_PSOS)
  // <setregid> is not supported, only one process anyway.
  PDL_UNUSED_ARG (rgid);
  PDL_UNUSED_ARG (egid);
  return 0;
#elif defined(ITRON)
  /* empty */
  PDL_NOTSUP_RETURN (-1);
#else
  PDL_OSCALL_RETURN (::setregid (rgid, egid), int, -1);
#endif /* PDL_WIN32 */
}

PDL_INLINE PDL_OFF_T
PDL_OS::lseek (PDL_HANDLE handle, PDL_OFF_T offset, int whence)
{
  PDL_TRACE ("PDL_OS::lseek");

#if defined (PDL_WIN32)
# if SEEK_SET != FILE_BEGIN || SEEK_CUR != FILE_CURRENT || SEEK_END != FILE_END
  //#error Windows NT is evil AND rude!
  switch (whence)
    {
    case SEEK_SET:
      whence = FILE_BEGIN;
      break;
    case SEEK_CUR:
      whence = FILE_CURRENT;
      break;
    case SEEK_END:
      whence = FILE_END;
      break;
    default:
      errno = EINVAL;
      return PDL_static_cast (PDL_OFF_T, -1); // rather safe than sorry
    }
# endif  /* SEEK_SET != FILE_BEGIN || SEEK_CUR != FILE_CURRENT || SEEK_END != FILE_END */

  LARGE_INTEGER li;
  li.QuadPart = offset;

  li.LowPart = ::SetFilePointer(handle, li.LowPart, &li.HighPart, whence);
  
  if (li.LowPart == INVALID_SET_FILE_POINTER &&
      GetLastError() != NO_ERROR)
  {
      PDL_FAIL_RETURN (PDL_static_cast (PDL_OFF_T, -1));
  }
  else
  {
      return li.QuadPart;
  }
#elif defined (PDL_PSOS)
# if defined (PDL_PSOS_LACKS_PHILE)
  PDL_UNUSED_ARG (handle);
  PDL_UNUSED_ARG (offset);
  PDL_UNUSED_ARG (whence);
  PDL_NOTSUP_RETURN (PDL_static_cast (PDL_OFF_T, -1));
# else
  unsigned long oldptr, newptr, result;
  // seek to the requested position
  result = ::lseek_f (handle, whence, offset, &oldptr);
  if (result != 0)
    {
      errno = result;
      return PDL_static_cast (PDL_OFF_T, -1);
    }
  // now do a dummy seek to the current position to obtain the position
  result = ::lseek_f (handle, SEEK_CUR, 0, &newptr);
  if (result != 0)
    {
      errno = result;
      return PDL_static_cast (PDL_OFF_T, -1);
    }
  return PDL_static_cast (PDL_OFF_T, newptr);
# endif /* defined (PDL_PSOS_LACKS_PHILE */
#elif defined(ITRON)
  /* empty */
  PDL_NOTSUP_RETURN (-1);
#else
  PDL_OSCALL_RETURN (::lseek (handle, offset, whence), PDL_OFF_T, -1);
#endif /* PDL_WIN32 */
}

#if defined (PDL_HAS_LLSEEK) || defined (PDL_HAS_LSEEK64)
PDL_INLINE PDL_LOFF_T
PDL_OS::llseek (PDL_HANDLE handle, PDL_LOFF_T offset, int whence)
{
  PDL_TRACE ("PDL_OS::llseek");

#if PDL_SIZEOF_LONG == 8
  /* The native lseek is 64 bit.  Use it. */
  return PDL_OS::lseek (handle, offset, whence);
#elif defined (PDL_HAS_LLSEEK) && defined (PDL_HAS_LSEEK64)
# error Either PDL_HAS_LSEEK64 and PDL_HAS_LLSEEK should be defined, not both!
#elif defined (PDL_HAS_LSEEK64)
  PDL_OSCALL_RETURN (::lseek64 (handle, offset, whence), PDL_LOFF_T, -1);
#elif defined (PDL_HAS_LLSEEK)
  PDL_OSCALL_RETURN (::llseek (handle, offset, whence), PDL_LOFF_T, -1);
#endif
}
#endif /* PDL_HAS_LLSEEK || PDL_HAS_LSEEK64 */

PDL_INLINE int
PDL_OS::fseek (FILE *fp, long offset, int whence)
{
#if defined (PDL_HAS_WINCE)
  return PDL_OS::lseek (fp, offset, whence);
#else /* PDL_HAS_WINCE */
# if defined (PDL_WIN32)
#   if SEEK_SET != FILE_BEGIN || SEEK_CUR != FILE_CURRENT || SEEK_END != FILE_END
  //#error Windows NT is evil AND rude!
  switch (whence)
    {
    case SEEK_SET:
      whence = FILE_BEGIN;
      break;
    case SEEK_CUR:
      whence = FILE_CURRENT;
      break;
    case SEEK_END:
      whence = FILE_END;
      break;
    default:
      errno = EINVAL;
      return -1; // rather safe than sorry
    }
#   endif  /* SEEK_SET != FILE_BEGIN || SEEK_CUR != FILE_CURRENT || SEEK_END != FILE_END */
# endif   /* PDL_WIN32 */
  PDL_OSCALL_RETURN (::fseek (fp, offset, whence), int, -1);
#endif /* PDL_HAS_WINCE */
}

PDL_INLINE pid_t
PDL_OS::waitpid (pid_t pid,
                 PDL_exitcode *status,
                 int wait_options,
                 PDL_HANDLE handle)
{
  PDL_TRACE ("PDL_OS::waitpid");
#if defined (VXWORKS) || defined (PDL_PSOS)
  PDL_UNUSED_ARG (pid);
  PDL_UNUSED_ARG (status);
  PDL_UNUSED_ARG (wait_options);
  PDL_UNUSED_ARG (handle);

  PDL_NOTSUP_RETURN (0);
#elif defined (PDL_WIN32)
  int blocking_period = PDL_BIT_ENABLED (wait_options, WNOHANG)
    ? 0 /* don't hang */
    : INFINITE;

  PDL_HANDLE phandle = handle;

  if (phandle == 0)
    {
      phandle = ::OpenProcess (SYNCHRONIZE,
                               FALSE,
                               pid);

      if (phandle == 0)
        {
          PDL_OS::set_errno_to_last_error ();
          return -1;
        }
    }

  pid_t result = pid;

  // Don't try to get the process exit status if wait failed so we can
  // keep the original error code intact.
  switch (::WaitForSingleObject (phandle,
                                 blocking_period))
    {
    case WAIT_OBJECT_0:
      if (status != 0)
        // The error status of <GetExitCodeProcess> is nonetheless
        // not tested because we don't know how to return the value.
        ::GetExitCodeProcess (phandle,
                              status);
      break;
    case WAIT_TIMEOUT:
      errno = ETIME;
      result = 0;
      break;
    default:
      PDL_OS::set_errno_to_last_error ();
      result = -1;
    }
  if (handle == 0)
    ::CloseHandle (phandle);
  return result;
#elif defined (CHORUS)
  PDL_UNUSED_ARG (status);
  PDL_UNUSED_ARG (wait_options);
  PDL_UNUSED_ARG (handle);
  PDL_OSCALL_RETURN (::await (&PDL_OS::actorcaps_[pid]),
                     pid_t, -1);
#elif defined(ITRON)
  /* empty */
  PDL_NOTSUP_RETURN (-1);
#else
  PDL_UNUSED_ARG (handle);
  PDL_OSCALL_RETURN (::waitpid (pid, status, wait_options),
                     pid_t, -1);
#endif /* PDL_WIN32 */
}

PDL_INLINE pid_t
PDL_OS::wait (pid_t pid,
              PDL_exitcode *status,
              int wait_options,
              PDL_HANDLE handle)
{
  PDL_TRACE ("PDL_OS::wait");
  return PDL_OS::waitpid (pid,
                          status,
                          wait_options,
                          handle);
}

PDL_INLINE pid_t
PDL_OS::wait (int *status)
{
  PDL_TRACE ("PDL_OS::wait");
#if defined (PDL_WIN32) || defined (VXWORKS) || defined(CHORUS) || defined (PDL_PSOS)
  PDL_UNUSED_ARG (status);

  PDL_NOTSUP_RETURN (0);
#else
# if defined (PDL_HAS_UNION_WAIT)
  PDL_OSCALL_RETURN (::wait ((union wait *) status), pid_t, -1);
#elif defined(ITRON)
  /* empty */
  PDL_NOTSUP_RETURN (-1);
# else
  PDL_OSCALL_RETURN (::wait (status), pid_t, -1);
# endif /* PDL_HAS_UNION_WAIT */
#endif /* defined (PDL_WIN32) */
}

PDL_INLINE int
PDL_OS::ioctl (PDL_SOCKET handle,
               int cmd,
               void *val)
{
  PDL_TRACE ("PDL_OS::ioctl");

#if defined (PDL_WIN32)
  PDL_SOCKCALL_RETURN (::ioctlsocket (handle, cmd, (u_long *) val), int, -1);
#elif defined (VXWORKS)
  PDL_OSCALL_RETURN (::ioctl (handle, cmd, PDL_reinterpret_cast (int, val)),
                     int, -1);
#elif defined (PDL_PSOS)
  PDL_OSCALL_RETURN (::ioctl (handle, cmd, (char *) val), int, -1);
#elif defined (CYGWIN32)
  PDL_UNUSED_ARG (handle);
   PDL_UNUSED_ARG (cmd);
   PDL_UNUSED_ARG (val);
   PDL_NOTSUP_RETURN (-1);
#elif defined(ITRON)
  /* empty */
  PDL_NOTSUP_RETURN (-1);
#else
  PDL_OSCALL_RETURN (::ioctl (handle, cmd, val), int, -1);
#endif /* PDL_WIN32 */
}

PDL_INLINE int
PDL_OS::kill (pid_t pid, int signum)
{
  PDL_TRACE ("PDL_OS::kill");
#if defined (PDL_HAS_WINCE) || defined (CHORUS) || defined (PDL_PSOS)
  PDL_UNUSED_ARG (pid);
  PDL_UNUSED_ARG (signum);
  PDL_NOTSUP_RETURN (-1);
#elif defined (PDL_WIN32)
  HANDLE procHandle = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
  if ( procHandle == NULL ) {
    if (signum == 0 || signum == SIGKILL) {
      errno = ESRCH;
    }
    return -1;
  }

  switch(signum) {
  // just polling if the process is alive
  case 0 :
	return 0;
  case SIGKILL:
  	if ( !TerminateProcess(procHandle, 0) ) {
		goto invalid_op;
  	}
  	break;
  default:
	goto invalid_op;
  }
  return 0;
invalid_op:
  CloseHandle(procHandle);
  errno = ESRCH; // no such process
  return -1;
#else
  PDL_OSCALL_RETURN (::kill (pid, signum), int, -1);
#endif /* PDL_WIN32 */
}

PDL_INLINE int
PDL_OS::sigaction (int signum,
                   const struct sigaction *nsa,
                   struct sigaction *osa)
{
  PDL_TRACE ("PDL_OS::sigaction");
  if (signum == 0)
    return 0;
#if defined (PDL_WIN32) && !defined (PDL_HAS_WINCE)
  struct sigaction sa;

  if (osa == 0)
    osa = &sa;

  if (nsa == 0)
    {
      osa->sa_handler = ::signal (signum, SIG_IGN);
      ::signal (signum, osa->sa_handler);
    }
  else
    osa->sa_handler = ::signal (signum, nsa->sa_handler);
  return osa->sa_handler == SIG_ERR ? -1 : 0;
#elif defined (CHORUS) || defined (PDL_HAS_WINCE) || defined(PDL_PSOS)
  PDL_UNUSED_ARG (signum);
  PDL_UNUSED_ARG (nsa);
  PDL_UNUSED_ARG (osa);
  PDL_NOTSUP_RETURN (-1);
#elif defined (PDL_LACKS_POSIX_PROTOTYPES) || defined (PDL_LACKS_SOME_POSIX_PROTOTYPES)
  PDL_OSCALL_RETURN (::sigaction (signum, (struct sigaction*) nsa, osa), int, -1);
#elif defined(ITRON)
  /* empty */
  PDL_NOTSUP_RETURN (-1);
#else
  PDL_OSCALL_RETURN (::sigaction (signum, nsa, osa), int, -1);
#endif /* PDL_LACKS_POSIX_PROTOTYPES */
}

#if !defined (PDL_HAS_MOSTLY_UNICODE_APIS)
PDL_INLINE char *
PDL_OS::getcwd (char *buf, size_t size)
{
  PDL_TRACE ("PDL_OS::getcwd");
# if defined (PDL_WIN32)
  return ::_getcwd (buf, size);
# elif defined (PDL_PSOS_LACKS_PHILE)
  PDL_UNUSED_ARG (buf);
  PDL_UNUSED_ARG (size);
  PDL_NOTSUP_RETURN ( (char*)-1);
# elif defined (PDL_PSOS)

  static char pathbuf [BUFSIZ];

  // blank the path buffer
  PDL_OS::memset (pathbuf, '\0', BUFSIZ);

  // the following was suggested in the documentation for get_fn ()
  u_long result;
  char cur_dir_name [BUFSIZ] = ".";

  u_long cur_dir = 0, prev_dir = 0;
  while ((PDL_OS::strlen (pathbuf) < BUFSIZ) &&
         (PDL_OS::strlen (cur_dir_name) < BUFSIZ - PDL_OS::strlen ("/..")))
  {
    // get the current directory handle
    result = ::get_fn (cur_dir_name, &cur_dir);

    // check whether we're at the root: this test is
    // really lame, but the get_fn documentation says
    // *either* condition indicates you're trying to
    // move above the root.
    if ((result != 0) || ( cur_dir == prev_dir))
    {
      break;
    }

    // change name to the parent directory
    PDL_OS::strcat (cur_dir_name, "/..");

    // open the parent directory
    XDIR xdir;
    result = ::open_dir (cur_dir_name, &xdir);
    if (result != 0)
    {
      return 0;
    }

    // look for an entry that matches the current directory handle
    struct dirent dir_entry;
    while (1)
    {
      // get the next directory entry
      result = ::read_dir (&xdir, &dir_entry);
      if (result != 0)
      {
        return 0;
      }

      // check for a match
      if (dir_entry.d_filno == cur_dir)
      {
        // prefix the previous path with the entry's name and break
        if (PDL_OS::strlen (pathbuf) + PDL_OS::strlen (dir_entry.d_name) < BUFSIZ)
        {
          PDL_OS::strcpy (pathbuf + PDL_OS::strlen (dir_entry.d_name), pathbuf);
          PDL_OS::strcpy (pathbuf, dir_entry.d_name);
          break;
        }
        else
        {
          // we're out of room in the buffer
          return 0;
        }
      }
    }

    // close the parent directory
    result = ::close_dir (&xdir);
    if (result != 0)
    {
      return 0;
    }

    // save the current directory handle as the previous
    prev_dir =  cur_dir;
  }

  // return the path, if there is one
  return (PDL_OS::strlen (pathbuf) > 0) ? pathbuf : (char *) 0;
#elif defined(ITRON)
  /* empty */
  PDL_NOTSUP_RETURN (0);
# else
  PDL_OSCALL_RETURN (::getcwd (buf, size), char *, 0);
# endif /* PDL_WIN32 */
}
#endif /* !PDL_HAS_MOSTLY_UNICODE_APIS */

#if !defined (ACE_LACKS_REALPATH)
PDL_INLINE char *
PDL_OS::realpath (const char *file_name,
                  char *resolved_name)
{
#if defined (PDL_WIN32)
    return ::_fullpath (resolved_name, file_name, MAX_PATH);
#else /* PDL_WIN32 */
    return ::realpath (file_name, resolved_name);
#endif
}
#endif /* ACE_LACKS_REALPATH */

PDL_INLINE int
PDL_OS::sleep (u_int seconds)
{
  PDL_TRACE ("PDL_OS::sleep");
#if defined (PDL_WIN32)
  ::Sleep (seconds * PDL_ONE_SECOND_IN_MSECS);
  return 0;
#if 0
#elif defined (HPUX_10) && defined (PDL_HAS_PTHREADS_DRAFT4)
  // On HP-UX 10, _CMA_NOWRAPPERS_ disables the mapping from sleep to cma_sleep
  // which makes sleep() put the whole process to sleep, and keeps it from
  // noticing pending cancels.  So, in this case, use pthread_delay_np
  struct timespec rqtp;
  rqtp.tv_sec = seconds;
  rqtp.tv_nsec = 0L;
  return pthread_delay_np (&rqtp);
#endif /* 0 */
#elif defined(ITRON)
  /* empty */
  PDL_NOTSUP_RETURN (-1);
#elif defined (PDL_HAS_CLOCK_GETTIME)
  struct timespec rqtp;
  // Initializer doesn't work with Green Hills 1.8.7
  rqtp.tv_sec = seconds;
  rqtp.tv_nsec = 0L;
  PDL_OSCALL_RETURN (::nanosleep (&rqtp, 0), int, -1);
#elif defined (PDL_PSOS)
  timeval wait;
  wait.tv_sec = seconds;
  wait.tv_usec = 0;
  PDL_OSCALL_RETURN (::select (0, 0, 0, 0, &wait), int, -1);
#else
  PDL_OSCALL_RETURN (::sleep (seconds), int, -1);
#endif /* PDL_WIN32 */
}

PDL_INLINE int
PDL_OS::sleep (const PDL_Time_Value &tv)
{
  PDL_TRACE ("PDL_OS::sleep");
#if defined (PDL_WIN32)
  ::Sleep (tv.msec ());
  return 0;
#elif defined(ITRON)
  /* empty */
  PDL_NOTSUP_RETURN (-1);
#else
# if defined (PDL_HAS_NONCONST_SELECT_TIMEVAL)
  // Copy the timeval, because this platform doesn't declare the timeval
  // as a pointer to const.
  timeval tv_copy = tv;
  PDL_OSCALL_RETURN (::select (0, 0, 0, 0, &tv_copy), int, -1);
# else  /* ! PDL_HAS_NONCONST_SELECT_TIMEVAL */
  const timeval *tvp = tv;
  PDL_OSCALL_RETURN (::select (0, 0, 0, 0, tvp), int, -1);
# endif /* PDL_HAS_NONCONST_SELECT_TIMEVAL */
#endif /* PDL_WIN32 */
}

PDL_INLINE int
PDL_OS::nanosleep (const struct timespec *requested,
                   struct timespec *remaining)
{
  PDL_TRACE ("PDL_OS::nanosleep");
#if defined (PDL_HAS_CLOCK_GETTIME) && !defined(ITRON)
  // ::nanosleep () is POSIX 1003.1b.  So is ::clock_gettime ().  So,
  // if PDL_HAS_CLOCK_GETTIME is defined, then ::nanosleep () should
  // be available on the platform.  On Solaris 2.x, both functions
  // require linking with -lposix4.
  return ::nanosleep ((PDL_TIMESPEC_PTR) requested, remaining);
#elif defined (PDL_PSOS)
#  if ! defined (PDL_PSOS_DIAB_MIPS)
  double ticks = KC_TICKS2SEC * requested->tv_sec +
                 ( PDL_static_cast (double, requested->tv_nsec) *
                   PDL_static_cast (double, KC_TICKS2SEC) ) /
                 PDL_static_cast (double, PDL_ONE_SECOND_IN_NSECS);

  if (ticks > PDL_static_cast (double, PDL_PSOS_Time_t::max_ticks))
  {
    ticks -= PDL_static_cast (double, PDL_PSOS_Time_t::max_ticks);
    remaining->tv_sec = PDL_static_cast (time_t,
                                         (ticks /
                                          PDL_static_cast (double,
                                                           KC_TICKS2SEC)));
    ticks -= PDL_static_cast (double, remaining->tv_sec) *
             PDL_static_cast (double, KC_TICKS2SEC);

    remaining->tv_nsec =
      PDL_static_cast (long,
                       (ticks * PDL_static_cast (double,
                                                 PDL_ONE_SECOND_IN_NSECS)) /
                       PDL_static_cast (double, KC_TICKS2SEC));

    ::tm_wkafter (PDL_PSOS_Time_t::max_ticks);
  }
  else
  {
    remaining->tv_sec = 0;
    remaining->tv_nsec = 0;
    ::tm_wkafter (PDL_static_cast (u_long, ticks));
  }

  // tm_wkafter always returns 0
#  endif /* PDL_PSOS_DIAB_MIPS */
  return 0;
#elif defined(ITRON)
  PDL_NOTSUP_RETURN (-1);
  /* empty */
#else
  PDL_UNUSED_ARG (remaining);

  // Convert into seconds and microseconds.
# if ! defined(PDL_HAS_BROKEN_TIMESPEC_MEMBERS)
  PDL_Time_Value tv;
  tv.initialize(requested->tv_sec, requested->tv_nsec / 1000);
# else
  PDL_Time_Value tv;
  tv.initialize(requested->ts_sec, requested->ts_nsec / 1000);
# endif /* PDL_HAS_BROKEN_TIMESPEC_MEMBERS */
  return PDL_OS::sleep (tv);
#endif /* PDL_HAS_CLOCK_GETTIME */
}

#if !defined (PDL_HAS_MOSTLY_UNICODE_APIS)
PDL_INLINE int
PDL_OS::mkdir (const char *path, mode_t mode)
{
# if defined (PDL_WIN32)
  PDL_UNUSED_ARG (mode);

#if defined (__IBMCPP__) && (__IBMCPP__ >= 400)
  PDL_OSCALL_RETURN (::_mkdir ((char *) path), int, -1);
#else
  PDL_OSCALL_RETURN (::_mkdir (path), int, -1);
 #endif /* __IBMCPP__ */
# elif defined (PDL_PSOS_LACKS_PHILE)
  PDL_UNUSED_ARG (path);
  PDL_UNUSED_ARG (mode);
  PDL_NOTSUP_RETURN (-1);
# elif defined (PDL_PSOS)
  //The pSOS make_dir fails if the last character is a '/'
  int location;
  char *phile_path;

  phile_path = (char *)PDL_OS::malloc(strlen(path));
  if (phile_path == 0)
    {
      PDL_OS::printf ("malloc in make_dir failed: [%X]\n",
                      errno);
      return -1;
    }
  else
    PDL_OS::strcpy (phile_path, path);

  location = PDL_OS::strlen(phile_path);
  if(phile_path[location-1] == '/')
  {
     phile_path[location-1] = 0;
  }

  u_long result;
  result = ::make_dir ((char *) phile_path, mode);
  if (result == 0x2011)  // Directory already exists
    {
      result = 0;
    }
  else if (result != 0)
    {
      result = -1;
    }

  PDL_OS::free(phile_path);
  return result;

# elif defined (VXWORKS)
  PDL_UNUSED_ARG (mode);
  PDL_OSCALL_RETURN (::mkdir ((char *) path), int, -1);
# else
  PDL_OSCALL_RETURN (::mkdir (path, mode), int, -1);
# endif /* VXWORKS */
}
#endif /* PDL_HAS_MOSTLY_UNICODE_APIS */

#if defined(PDL_HAS_WINCE)
#define MAX_NUM_ENV_BUFFER    (10)
#define MAX_ENV_BUFFER_LENGTH (128)

static char gEnvBuffer[MAX_NUM_ENV_BUFFER][MAX_ENV_BUFFER_LENGTH];
/* gEnvBuffer's Index */
static int  gIndex = 0;
#endif /* PDL_HAS_WINCE */

PDL_INLINE char *
PDL_OS::getenv (const char *symbol)
{
  PDL_TRACE ("PDL_OS::getenv");
#if !defined (PDL_HAS_WINCE) && !defined(PDL_PSOS)
  PDL_OSCALL_RETURN (::getenv (symbol), char *, 0);
#elif defined(PDL_HAS_WINCE)
  HKEY      hKey;
  int       sRc = 0;
  int       sIndex;
  int       sSymbolSize;
  DWORD     dwDsize = MAX_ENV_BUFFER_LENGTH;
  DWORD     dwValType;
  char    * sValue = NULL;
  char    * sEnvBuffer = NULL;
  wchar_t * sWSymbol = NULL;

  if((sRc = RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"Software\\ALTIBASE",
                         0, KEY_QUERY_VALUE, &hKey)) != ERROR_SUCCESS)
  {
    errno = sRc;
  }
  else
  {
    sSymbolSize = mbstowcs(NULL, symbol, 0) + 1;
    if((sWSymbol = (wchar_t *) PDL_OS::malloc(sizeof(wchar_t) 
                                              * sSymbolSize)) != NULL)
    {
      if((sEnvBuffer = (char *) PDL_OS::malloc(dwDsize)) != NULL)
      {
        mbstowcs(sWSymbol, symbol, sSymbolSize);

        if((sRc = RegQueryValueEx(hKey, sWSymbol, NULL, &dwValType,
                                 (BYTE *) sEnvBuffer, &dwDsize)) != ERROR_SUCCESS)
        {
          errno = sRc;
        }
        else
        {
          /*
           * gEnvBuffer를 검색해서 기존에 저장한 값이 있는지 확인한다.
           * 기존에 저장한 값이 있으면 그 포인터를 반환한다.
           */
          for(sIndex = 0; sIndex < gIndex; sIndex++)
          {
            if(PDL_OS::strncmp(gEnvBuffer[sIndex], sEnvBuffer, dwDsize) == 0)
            {
              sValue = gEnvBuffer[sIndex];
              break;
            }
          }

          /* 
           * gEnvBuffer에서 읽어온 값을 찾을 수 없다면 gEnvBuffer의 빈 공간에
           * 값을 저장한 후 그 포인터를 반환한다.
           * gEnvBuffer에 빈 공간이 없는 경우 값을 저장할 수 없으므로 NULL을
           * 반환한다.
           */
          if(sValue == NULL)
          {
            if(gIndex < MAX_NUM_ENV_BUFFER)
            {
              PDL_OS::strncpy(gEnvBuffer[gIndex], sEnvBuffer, dwDsize);
              sValue = gEnvBuffer[gIndex];
              gIndex++;
            }
          }
        }
        PDL_OS::free(sEnvBuffer);
      } // sEnvBuffer != NULL
      PDL_OS::free(sWSymbol);
    } // sWSymbol != NULL
    if((sRc = RegCloseKey(hKey)) != ERROR_SUCCESS)
    {
      sValue = NULL;
      errno = sRc;
    }
  }

  return sValue;
#else
  /* @@ pSOS doesn't have the concept of environment variables. */
  PDL_UNUSED_ARG (symbol);
  PDL_NOTSUP_RETURN (0);
#endif /* ! (PDL_HAS_WINCE && PDL_PSOS) */
}

PDL_INLINE int
PDL_OS::putenv (const char *string)
{
  PDL_TRACE ("PDL_OS::putenv");
  // VxWorks declares ::putenv with a non-const arg.
#if !defined (PDL_HAS_WINCE) && !defined (PDL_PSOS)
  PDL_OSCALL_RETURN (::putenv ((char *) string), int, -1);
#elif defined(PDL_HAS_WINCE)
  int       sRc = -1;
  int       rc = 0;
  int       sIndex = 0;
  int       sKeySize;
  HKEY      hKey;
  DWORD     dwDisp;
  wchar_t * sWKey = NULL;
  char    * sKey = NULL;
  char    * sValue = NULL;

  if(string != NULL)
  {
    if((sKey = (char *) PDL_OS::malloc(PDL_OS::strlen(string) + 1)) != NULL) 
    {
      strncpy(sKey, string, PDL_OS::strlen(string) + 1);

      if((sValue = PDL_OS::strchr(sKey, '=')) != NULL) // arg1 string : [KEY=VALUE]
      {
        sValue[0] = '\0'; // 1. replace '=' to '\0', sKey indicates input KEY or ""
        sValue++;         // 2. sValue indicates input VALUE or "" 

        for(sIndex = 0; sIndex < gIndex; sIndex++)
        {
          if(PDL_OS::strncmp(gEnvBuffer[sIndex], sValue, MAX_ENV_BUFFER_LENGTH) == 0)
          {
              break;
          }
        }

        if(sIndex < MAX_NUM_ENV_BUFFER) // sValue == gEnvBuffer[sIndex] || gEnvBuffer is not full
        {
          if((rc = RegCreateKeyEx(HKEY_LOCAL_MACHINE, L"Software\\ALTIBASE",
                                  0, NULL, NULL,  0, NULL, &hKey, &dwDisp))
             != ERROR_SUCCESS)
          {
            errno = rc;
          }
          else
          {
            sKeySize = mbstowcs(NULL, sKey, 0) + 1;
            if((sWKey = (wchar_t *)PDL_OS::malloc(sizeof(wchar_t) * sKeySize)) != NULL)
            {
              mbstowcs(sWKey, sKey, sKeySize);

              /*
               * REG_SZ        : A null-terminated string
               * REG_EXPAND_SZ : A null-terminated string that contains unexpanded
               *                 references to environment variables (ex, %PATH%).
               */
              if(( rc = RegSetValueEx(hKey, (LPCWSTR)sWKey, 0, REG_EXPAND_SZ,
                                      (const BYTE*) sValue, PDL_OS::strlen(sValue) + 1)) != ERROR_SUCCESS)
              {
                errno = rc;
              }
              else
              {
                if(sIndex == gIndex) // input VALUE is not existed in gEnvBuffer
                {
                  strncpy(gEnvBuffer[gIndex], sValue, MAX_ENV_BUFFER_LENGTH);
                  gIndex++;
                }
                sRc = 0;
              }
              if((rc = RegCloseKey(hKey)) != ERROR_SUCCESS)
              {
                sRc = -1;
                errno = rc;
              }
              PDL_OS::free(sWKey);
            } // sWKey != NULL
          }
        }
      } // arg1 string : [KEY=VALUE]
      PDL_OS::free(sKey);
    } // sKey != NULL
  } // string != NULL

  return sRc;
#else
  /* @@ pSOS doesn't have the concept of environment variables. */
  PDL_UNUSED_ARG (string);
  PDL_NOTSUP_RETURN (-1);
#endif /* ! PDL_HAS_WINCE && ! PDL_PSOS */
}


#if !defined (PDL_HAS_WCHAR_TYPEDEFS_CHAR)
PDL_INLINE size_t
PDL_OS::strlen (const wchar_t *s)
{
  // PDL_TRACE ("PDL_OS::strlen");
# if defined (PDL_HAS_UNICODE)
  return ::wcslen (s);
# else
#   if defined (PDL_HAS_XPG4_MULTIBYTE_CHAR)
  return wcslen (s);
#   else
  u_int len = 0;

  while (*s++ != 0)
    len++;

  return len;
#   endif /* PDL_HAS_XPG4_MULTIBYTE_CHAR */
# endif /* PDL_HAS_UNICODE */
}

PDL_INLINE wchar_t *
PDL_OS::strcpy (wchar_t *s, const wchar_t *t)
{
  // PDL_TRACE ("PDL_OS::strcpy");

# if defined (PDL_HAS_UNICODE)
  return ::wcscpy (s, t);
# else
#   if defined (PDL_HAS_XPG4_MULTIBYTE_CHAR)
  return wcscpy (s, t);
#   else
  wchar_t *result = s;

  while ((*s++ = *t++) != 0)
    continue;

  return result;
#   endif /* PDL_HAS_XPG4_MULTIBYTE_CHAR */
# endif /* PDL_HAS_UNICODE */
}

PDL_INLINE size_t
PDL_OS::strspn (const wchar_t *s, const wchar_t *t)
{
#if !defined (PDL_HAS_WINCE) && defined (PDL_HAS_UNICODE)
  PDL_TRACE ("PDL_OS::strspn");
  return ::wcsspn (s, t);
#else
  PDL_UNUSED_ARG (s);
  PDL_UNUSED_ARG (t);
  PDL_NOTSUP_RETURN (0);
#endif /* !PDL_HAS_WINCE && PDL_HAS_UNICODE */
}

PDL_INLINE int
PDL_OS::strcmp (const wchar_t *s, const wchar_t *t)
{
  PDL_TRACE ("PDL_OS::strcmp");
# if defined (PDL_HAS_UNICODE)
  return ::wcscmp (s, t);
# else
#   if defined (PDL_HAS_XPG4_MULTIBYTE_CHAR)
  return wcscmp (s, t);
#   else
  const wchar_t *scan1 = s;
  const wchar_t *scan2 = t;

  while (*scan1 != 0 && *scan1 == *scan2)
    {
      ++scan1;
      ++scan2;
    }

  // The following case analysis is necessary so that characters which
  // look negative collate low against normal characters but high
  // against the end-of-string NUL.

  if (*scan1 == '\0' && *scan2 == '\0')
    return 0;
  else if (*scan1 == '\0')
    return -1;
  else if (*scan2 == '\0')
    return 1;
  else
    return *scan1 - *scan2;
#   endif /* PDL_HAS_XPG4_MULTIBYTE_CHAR */
# endif /* PDL_HAS_UNICODE */
}
#endif /* ! PDL_HAS_WCHAR_TYPEDEFS_CHAR */

#if !defined (PDL_HAS_WCHAR_TYPEDEFS_USHORT)
PDL_INLINE size_t
PDL_OS::strlen (const PDL_USHORT16 *s)
{
  // PDL_TRACE ("PDL_OS::strlen");
  u_int len = 0;

  while (*s++ != 0)
    len++;

  return len;
}

PDL_INLINE PDL_USHORT16 *
PDL_OS::strcpy (PDL_USHORT16 *s, const PDL_USHORT16 *t)
{
  // PDL_TRACE ("PDL_OS::strcpy");

  PDL_USHORT16 *result = s;

  while ((*s++ = *t++) != 0)
    continue;

  return result;
}

PDL_INLINE int
PDL_OS::strcmp (const PDL_USHORT16 *s, const PDL_USHORT16 *t)
{
  PDL_TRACE ("PDL_OS::strcmp");

  while (*s != 0
         && *t != 0
         && *s == *t)
    {
      ++s;
      ++t;
    }

  return *s - *t;
}
#endif /* ! PDL_HAS_WCHAR_TYPEDEFS_USHORT */

PDL_INLINE u_int
PDL_OS::wslen (const WChar *s)
{
  u_int len = 0;

  while (*s++ != 0)
    len++;

  return len;
}

PDL_INLINE PDL_OS::WChar *
PDL_OS::wscpy (WChar *dest, const WChar *src)
{
  WChar *original_dest = dest;

  while ((*dest++ = *src++) != 0)
    continue;

  return original_dest;
}

PDL_INLINE int
PDL_OS::wscmp (const WChar *s, const WChar *t)
{
  const WChar *scan1 = s;
  const WChar *scan2 = t;

  while (*scan1 != 0 && *scan1 == *scan2)
    {
      ++scan1;
      ++scan2;
    }

  return *scan1 - *scan2;
}

PDL_INLINE int
PDL_OS::wsncmp (const WChar *s, const WChar *t, size_t len)
{
  const WChar *scan1 = s;
  const WChar *scan2 = t;

  while (len != 0 && *scan1 != 0 && *scan1 == *scan2)
    {
      ++scan1;
      ++scan2;
      --len;
    }

  return len == 0 ? 0 : *scan1 - *scan2;
}

#if defined (PDL_HAS_UNICODE)

PDL_INLINE int
PDL_OS::atoi (const wchar_t *s)
{
  return ::_wtoi (s);
}

PDL_INLINE wchar_t *
PDL_OS::strecpy (wchar_t *s, const wchar_t *t)
{
  PDL_TRACE ("PDL_OS::strecpy");
  register wchar_t *dscan = s;
  register const wchar_t *sscan = t;

  while ((*dscan++ = *sscan++) != '\0')
    continue;

  return dscan;
}

PDL_INLINE wchar_t *
PDL_OS::strpbrk (wchar_t *s, const wchar_t *t)
{
  PDL_TRACE ("PDL_OS::wcspbrk");
  return ::wcspbrk (s, t);
}

PDL_INLINE const wchar_t *
PDL_OS::strpbrk (const wchar_t *s, const wchar_t *t)
{
  PDL_TRACE ("PDL_OS::wcspbrk");
  return ::wcspbrk (s, t);
}

PDL_INLINE wchar_t *
PDL_OS::strcat (wchar_t *s, const wchar_t *t)
{
  PDL_TRACE ("PDL_OS::strcat");
  return ::wcscat (s, t);
}

PDL_INLINE const wchar_t *
PDL_OS::strchr (const wchar_t *s, wint_t c)
{
  PDL_TRACE ("PDL_OS::strchr");
  return (const wchar_t *) ::wcschr (s, c);
}

PDL_INLINE const wchar_t *
PDL_OS::strnchr (const wchar_t *s, wint_t c, size_t len)
{
  for (size_t i = 0; i < len; i++)
    if (s[i] == c)
      return s + i;

  return 0;
}

PDL_INLINE const wchar_t *
PDL_OS::strrchr (const wchar_t *s, wint_t c)
{
  PDL_TRACE ("PDL_OS::strrchr");
# if !defined (PDL_HAS_WINCE)
  return (const wchar_t *) ::wcsrchr (s, c);
# else
  const wchar_t *p = s + ::wcslen (s);

  while (*p != c)
    if (p == s)
      return 0;
    else
      p--;

  return p;
# endif /* PDL_HAS_WINCE */
}

PDL_INLINE wchar_t *
PDL_OS::strchr (wchar_t *s, wint_t c)
{
  PDL_TRACE ("PDL_OS::strchr");
  return ::wcschr (s, c);
}

PDL_INLINE wchar_t *
PDL_OS::strnchr (wchar_t *s, wint_t c, size_t len)
{
#if defined PDL_PSOS_DIAB_PPC  //Complier problem Diab 4.2b
  const wchar_t *const_wchar_s=s;
  return (wchar_t *) PDL_OS::strnchr (const_wchar_s, c, len);
#else
  return (wchar_t *) PDL_OS::strnchr ((const wchar_t *) s, c, len);
#endif
}

PDL_INLINE wchar_t *
PDL_OS::strrchr (wchar_t *s, wint_t c)
{
  PDL_TRACE ("PDL_OS::strrchr");
# if !defined (PDL_HAS_WINCE)
  return (wchar_t *) ::wcsrchr (s, c);
# else
  wchar_t *p = s + ::wcslen (s);

  while (*p != c)
    if (p == s)
      return 0;
    else
      p--;

  return p;
# endif /* PDL_HAS_WINCE */
}

PDL_INLINE wint_t
PDL_OS::to_lower (wint_t c)
{
  PDL_TRACE ("PDL_OS::to_lower");
  return ::towlower (c);
}

PDL_INLINE int
PDL_OS::strcasecmp (const wchar_t *s, const wchar_t *t)
{
  PDL_TRACE ("PDL_OS::strcasecmp");

# if !defined (PDL_WIN32)
  const wchar_t *scan1 = s;
  const wchar_t *scan2 = t;

  while (*scan1 != 0
         && PDL_OS::to_lower (*scan1) == PDL_OS::to_lower (*scan2))
    {
      ++scan1;
      ++scan2;
    }

  // The following case analysis is necessary so that characters which
  // look negative collate low against normal characters but high
  // against the end-of-string NUL.

  if (*scan1 == '\0' && *scan2 == '\0')
    return 0;
  else if (*scan1 == '\0')
    return -1;
  else if (*scan2 == '\0')
    return 1;
  else
    return PDL_OS::to_lower (*scan1) - PDL_OS::to_lower (*scan2);
# else /* PDL_WIN32 */
  return ::_wcsicmp (s, t);
# endif /* PDL_WIN32 */
}

PDL_INLINE int
PDL_OS::strncasecmp (const wchar_t *s,
                     const wchar_t *t,
                     size_t len)
{
  PDL_TRACE ("PDL_OS::strcasecmp");

# if !defined (PDL_WIN32)
  const wchar_t *scan1 = s;
  const wchar_t *scan2 = t;
  ssize_t count = ssize_t (n);

  while (--count >= 0
         && *scan1 != 0
         && PDL_OS::to_lower (*scan1) == PDL_OS::to_lower (*scan2))
    {
      ++scan1;
      ++scan2;
    }

  if (count < 0)
    return 0;

  // The following case analysis is necessary so that characters which
  // look negative collate low against normal characters but high
  // against the end-of-string NUL.

  if (*scan1 == '\0' && *scan2 == '\0')
    return 0;
  else if (*scan1 == '\0')
    return -1;
  else if (*scan2 == '\0')
    return 1;
  else
    return PDL_OS::to_lower (*scan1) - PDL_OS::to_lower (*scan2);
# else /* PDL_WIN32 */
  return ::_wcsnicmp (s, t, len);
# endif /* PDL_WIN32 */
}

PDL_INLINE int
PDL_OS::strncmp (const wchar_t *s, const wchar_t *t, size_t len)
{
  PDL_TRACE ("PDL_OS::strncmp");
  return ::wcsncmp (s, t, len);
}

PDL_INLINE wchar_t *
PDL_OS::strncpy (wchar_t *s, const wchar_t *t, size_t len)
{
  PDL_TRACE ("PDL_OS::strncpy");
  return ::wcsncpy (s, t, len);
}

PDL_INLINE wchar_t *
PDL_OS::strncat (wchar_t *s, const wchar_t *t, size_t len)
{
  PDL_TRACE ("PDL_OS::strncat");
  return ::wcsncat (s, t, len);
}

PDL_INLINE wchar_t *
PDL_OS::strtok (wchar_t *s, const wchar_t *tokens)
{
  PDL_TRACE ("PDL_OS::strtok");
  return ::wcstok (s, tokens);
}

PDL_INLINE long
PDL_OS::strtol (const wchar_t *s, wchar_t **ptr, int base)
{
  PDL_TRACE ("PDL_OS::strtol");
  return ::wcstol (s, ptr, base);
}

PDL_INLINE unsigned long
PDL_OS::strtoul (const wchar_t *s, wchar_t **ptr, int base)
{
  PDL_TRACE ("PDL_OS::strtoul");
  return ::wcstoul (s, ptr, base);
}

PDL_INLINE double
PDL_OS::strtod (const wchar_t *s, wchar_t **endptr)
{
  PDL_TRACE ("PDL_OS::strtod");
  return ::wcstod (s, endptr);
}

PDL_INLINE int
PDL_OS::pdl_isspace (wchar_t c)
{
  return iswspace (c);
}

# if defined (PDL_WIN32)

PDL_INLINE const wchar_t *
PDL_OS::strstr (const wchar_t *s, const wchar_t *t)
{
  PDL_TRACE ("PDL_OS::strstr");
  return (const wchar_t *) ::wcsstr (s, t);
}

PDL_INLINE wchar_t *
PDL_OS::strstr (wchar_t *s, const wchar_t *t)
{
  PDL_TRACE ("PDL_OS::strstr");
  return ::wcsstr (s, t);
}

PDL_INLINE const wchar_t *
PDL_OS::strnstr (const wchar_t *s1, const wchar_t *s2, size_t len2)
{
  // Substring length
  size_t len1 = PDL_OS::strlen (s1);

  // Check if the substring is longer than the string being searched.
  if (len2 > len1)
    return 0;

  // Go upto <len>
  size_t len = len1 - len2;

  for (size_t i = 0; i <= len; i++)
    {
      if (PDL_OS::memcmp (s1 + i, s2, len2 * sizeof (wchar_t)) == 0)
        // Found a match!  Return the index.
        return s1 + i;
    }

  return 0;
}

PDL_INLINE wchar_t *
PDL_OS::strnstr (wchar_t *s, const wchar_t *t, size_t len)
{
#if defined PDL_PSOS_DIAB_PPC  //Complier problem Diab 4.2b
  const wchar_t *const_wchar_s=s;
  return (wchar_t *) PDL_OS::strnstr (const_wchar_s, t, len);
#else
  return (wchar_t *) PDL_OS::strnstr ((const wchar_t *) s, t, len);
#endif
}

PDL_INLINE wchar_t *
PDL_OS::strdup (const wchar_t *s)
{
  // PDL_TRACE ("PDL_OS::strdup");

#   if defined (__BORLANDC__)
  wchar_t *buffer = (wchar_t *) malloc ((PDL_OS::strlen (s) + 1) * sizeof (wchar_t));
  return ::wcscpy (buffer, s);
#   else
  return ::wcsdup (s);
#   endif /* __BORLANDC__ */
}

PDL_INLINE int
PDL_OS::vsprintf (wchar_t *buffer, const wchar_t *format, va_list argptr)
{
  return ::vswprintf (buffer, format, argptr);
}

PDL_INLINE int
PDL_OS::hostname (wchar_t *name, size_t maxnamelen)
{
#   if !defined (PDL_HAS_WINCE) && !defined (PDL_HAS_PHARLAP)
  PDL_TRACE ("PDL_OS::hostname");
  PDL_WIN32CALL_RETURN (PDL_ADAPT_RETVAL (::GetComputerNameW (name, LPDWORD (&maxnamelen)),
                                          pdl_result_), int, -1);
#   else
  PDL_UNUSED_ARG (name);
  PDL_UNUSED_ARG (maxnamelen);
  PDL_NOTSUP_RETURN (-1);
#   endif /* ! PDL_HAS_WINCE */
}

PDL_INLINE int
PDL_OS::unlink (const wchar_t *path)
{
  PDL_TRACE ("PDL_OS::unlink");
#   if !defined (PDL_HAS_WINCE)
  PDL_OSCALL_RETURN (::_wunlink (path), int, -1);
#   else
  // @@ The problem is, DeleteFile is not actually equals to unlink. ;(
  PDL_WIN32CALL_RETURN (PDL_ADAPT_RETVAL (::DeleteFile (path), pdl_result_),
                        int, -1);
#   endif /* PDL_HAS_WINCE */
}

PDL_INLINE int
PDL_OS::remove (const wchar_t *path)
{
  PDL_TRACE ("PDL_OS::remove");
#   if !defined (PDL_HAS_WINCE)
  PDL_OSCALL_RETURN (::_wremove (path), int, -1);
#   else
  // @@ The problem is, DeleteFile is not actually equals to unlink. ;(
  PDL_WIN32CALL_RETURN (PDL_ADAPT_RETVAL (::DeleteFile (path), pdl_result_),
                        int, -1);
#   endif /* PDL_HAS_WINCE */
}

#  if defined (PDL_HAS_WINCE)
PDL_INLINE int
PDL_OS::unlink (const char *path)
{
  PDL_TRACE ("PDL_OS::unlink");

  wchar_t *wPath;
  wPath = PDL_Ascii_To_Wide::convert(path);

#   if !defined (PDL_HAS_WINCE)
  PDL_OSCALL_RETURN (::_wunlink (wPath), int, -1);
#   else
  // @@ The problem is, DeleteFile is not actually equals to unlink. ;(
  //PDL_WIN32CALL_RETURN (PDL_ADAPT_RETVAL (::DeleteFile (wPath), pdl_result_),
  //                      int, -1);
  if(::DeleteFile(wPath) != 0)
  {
    delete []wPath;
    return 0; // delete success
  }
  else
  {
    PDL_OS::set_errno_to_last_error();
    delete []wPath;
    return -1; // delete fail
  }
#   endif /* !PDL_HAS_WINCE */
}
#  endif /* PDL_HAS_WINCE */

#  if defined (PDL_HAS_WINCE)
PDL_INLINE int
PDL_OS::remove (const char *path)
{
  PDL_TRACE ("PDL_OS::remove");

  wchar_t *wPath;
  wPath = PDL_Ascii_To_Wide::convert(path);

#   if !defined (PDL_HAS_WINCE)
  PDL_OSCALL_RETURN (::_wremove (wPath), int, -1);
#   else
  // @@ The problem is, DeleteFile is not actually equals to unlink. ;(
  //PDL_WIN32CALL_RETURN (PDL_ADAPT_RETVAL (::DeleteFile (wPath), pdl_result_),
  //                      int, -1);
  if(::DeleteFile(wPath) != 0)
  {
    delete []wPath;
    return 0; // delete success
  }
  else
  {
    PDL_OS::set_errno_to_last_error();
    delete []wPath;
    return -1; // delete fail
  }
#   endif /* !PDL_HAS_WINCE */
}
#  endif /* PDL_HAS_WINCE */

PDL_INLINE wchar_t *
PDL_OS::getenv (const wchar_t *symbol)
{
#   if !defined (PDL_HAS_WINCE)
  PDL_TRACE ("PDL_OS::getenv");
  PDL_OSCALL_RETURN (::_wgetenv (symbol), wchar_t *, 0);
#   else
  PDL_UNUSED_ARG (symbol);
  PDL_NOTSUP_RETURN (0);
#   endif /* PDL_HAS_WINCE */
}

PDL_INLINE int
PDL_OS::putenv (const wchar_t *string)
{
  PDL_TRACE ("PDL_OS::putenv");
  // VxWorks declares ::putenv with a non-const arg.
#if !defined (PDL_HAS_WINCE) && !defined (PDL_PSOS)
  PDL_OSCALL_RETURN (::_wputenv ((wchar_t *) string), int, -1);
#else
  // @@ WinCE and pSOS don't have the concept of environment variables.
  PDL_UNUSED_ARG (string);
  PDL_NOTSUP_RETURN (-1);
#endif /* ! PDL_HAS_WINCE && ! PDL_PSOS */
}

PDL_INLINE int
PDL_OS::rename (const wchar_t *old_name, const wchar_t *new_name)
{
#   if !defined (PDL_HAS_WINCE)
  PDL_TRACE ("PDL_OS::rename");
  PDL_OSCALL_RETURN (::_wrename (old_name, new_name), int, -1);
#   else
  // @@ There should be a Win32 API that can do this.
  PDL_UNUSED_ARG (old_name);
  PDL_UNUSED_ARG (new_name);
  PDL_NOTSUP_RETURN (-1);
#   endif /* PDL_HAS_WINCE */
}

#if defined (PDL_HAS_WINCE)
PDL_INLINE int
PDL_OS::rename (const char *old_name, const char *new_name)
{
  wchar_t *wOldName;
  wchar_t *wNewName;
  wOldName = PDL_Ascii_To_Wide::convert(old_name);
  wNewName = PDL_Ascii_To_Wide::convert(new_name);

  // @@ There should be a Win32 API that can do this.
  //  PDL_WIN32CALL_RETURN (PDL_ADAPT_RETVAL (
  //	::MoveFile (wOldName, wNewName), pdl_result_),
  //                       int, -1);
  if(::MoveFile(wOldName, wNewName) != 0)
  {
    delete []wOldName;
    delete []wNewName;
    return 0;
  }
  else
  {
    PDL_OS::set_errno_to_last_error();
    delete []wOldName;
    delete []wNewName;
    return -1;
  }
}
#endif /*PDL_HAS_WINCE*/


PDL_INLINE int
PDL_OS::access (const wchar_t *path, int amode)
{
#   if !defined (PDL_HAS_WINCE)
  PDL_TRACE ("PDL_OS::access");
  PDL_OSCALL_RETURN (::_waccess (path, amode), int, -1);
#   else
  int sRc = -1;
  int sFileAttr = 0xFFFFFFFF;

  if(path != NULL)
  {
    sFileAttr = GetFileAttributes(path);

    if(sFileAttr != 0xFFFFFFFF)
    {
      if((amode & W_OK) && (sFileAttr & FILE_ATTRIBUTE_READONLY) )
      {
        // file has read-only attributes
        sRc = -1;
      }
      else
      {
        sRc = 0;
      }
    }
    else
    {
      errno = GetLastError();
    }
  }

  return sRc;
#   endif /* PDL_HAS_WINCE */
}

#   if !defined (PDL_WIN32)
// Win32 implementation of fopen(const wchar_t*, const wchar_t*)
// is in OS.cpp.
PDL_INLINE FILE *
PDL_OS::fopen (const wchar_t *filename, const wchar_t *mode)
{
  PDL_OSCALL_RETURN (::_wfopen (filename, mode), FILE *, 0);
}
#   endif /* PDL_WIN32 */

PDL_INLINE FILE *
PDL_OS::fdopen (PDL_HANDLE handle, const wchar_t *mode)
{
  PDL_TRACE ("PDL_OS::fdopen");
#   if !defined (PDL_HAS_WINCE)
  // kernel file handle -> FILE* conversion...
  // Options: _O_APPEND, _O_RDONLY and _O_TEXT are lost

  FILE *file = 0;

#   if defined (PDL_WIN64)
  int crt_handle = ::_open_osfhandle ((intptr_t) handle, 0);
#   else
  int crt_handle = ::_open_osfhandle ((long) handle, 0);
#   endif /* PDL_WIN64 */

  if (crt_handle != -1)
    {
#     if defined(__BORLANDC__)
      file = ::_wfdopen (crt_handle, PDL_const_cast (wchar_t *, mode));
#     else
      file = ::_wfdopen (crt_handle, mode);
#     endif /* defined(__BORLANDC__) */

      if (!file)
        {
#     if (defined(__BORLANDC__) && __BORLANDC__ >= 0x0530)
          ::_rtl_close (crt_handle);
#     else
          ::_close (crt_handle);
#     endif /* (defined(__BORLANDC__) && __BORLANDC__ >= 0x0530) */
        }
    }

  return file;
#   else  /* ! PDL_HAS_WINCE */
  // @@ this function has to be implemented???
  // Okey, don't know how to implement it on CE.
  PDL_UNUSED_ARG (handle);
  PDL_UNUSED_ARG (mode);
  PDL_NOTSUP_RETURN (0);
#   endif /* ! PDL_HAS_WINCE */
}

PDL_INLINE int
PDL_OS::stat(const wchar_t *file, PDL_stat *stp)
{
  PDL_TRACE ("PDL_OS::stat");
#   if defined (PDL_HAS_WINCE)
  WIN32_FIND_DATA fdata;
  HANDLE fhandle;

  fhandle = ::FindFirstFile (file, &fdata);
  if (fhandle == INVALID_HANDLE_VALUE)
  {
      PDL_OS::set_errno_to_last_error ();
      return -1;
  }
  else
  {
      /* BUG-18734: [Win32] Large File(> MAXDWORD)에 대한 fstat함수에 대한
       * 포팅이 필요합니다.
       * nFileSizeHigh :The high-order DWORD value of the file size,
       * in bytes. This value is zero (0) unless the file size is
       * greater than MAXDWORD. 
       *
       * nFileSizeLow: The low-order DWORD value of the file size,
       * in bytes.
       *
       * The size of the file is equal to
       * (nFileSizeHigh * (MAXDWORD+1)) + nFileSizeLow.
       *  = nFileSizeHigh << 32 | nFileSizeLow
       * */
      stp->st_size = (__int64)(fdata.nFileSizeHigh) << 32 |
          fdata.nFileSizeLow;
      stp->st_atime.initialize(fdata.ftLastAccessTime);
      stp->st_mtime.initialize(fdata.ftLastWriteTime);

#if !defined (PDL_HAS_WINCE)
      PDL_Time_Value sCreationTime;
      sCreationTime.initialize(fdata.ftCreationTime);

      stp->st_ctime = sCreationTime.sec ();
      stp->st_nlink = PDL_static_cast (short, fdata.nNumberOfLinks);
      stp->st_dev = stp->st_rdev = 0; // No equivalent conversion.
      stp->st_mode = S_IXOTH | S_IROTH |
          (fdata.dwFileAttributes & FILE_ATTRIBUTE_READONLY ? 0 : S_IWOTH);
#endif /* !PDL_HAS_WINCE */

  }
  
  return 0;
#   elif defined (__BORLANDC__)  && (__BORLANDC__ <= 0x540)
  PDL_OSCALL_RETURN (::_wstat (file, stp), int, -1);
#   else
  PDL_OSCALL_RETURN (::_wstat (file, (struct _stat *) stp), int, -1);
#   endif /* PDL_HAS_WINCE */
}

PDL_INLINE void
PDL_OS::perror (const wchar_t *s)
{
  PDL_TRACE ("PDL_OS::perror");
#   if !defined (PDL_HAS_WINCE)
  ::_wperror (s);
#   else
  // @@ Let's leave this to some later point.
  PDL_UNUSED_ARG (s);
  //@@??@@  PDL_NOTSUP_RETURN ();
#   endif /* ! PDL_HAS_WINCE */
}


// Here are functions that CE doesn't support at all.
// Notice that some of them might have UNICODE version.
PDL_INLINE wchar_t *
PDL_OS::fgets (wchar_t *buf, int size, FILE *fp)
{
#if !defined (PDL_HAS_WINCE)
  PDL_TRACE ("PDL_OS::fgets");
  PDL_OSCALL_RETURN (::fgetws (buf, size, fp), wchar_t *, 0);
#else
  PDL_UNUSED_ARG (buf);
  PDL_UNUSED_ARG (size);
  PDL_UNUSED_ARG (fp);
  PDL_NOTSUP_RETURN (0);
#endif /* PDL_HAS_WINCE */
}
#if defined (PDL_HAS_WINCE)
PDL_INLINE char *
PDL_OS::fgets (char *buf, int size, FILE *fp)
{
  PDL_TRACE ("PDL_OS::fgets");

  PDL_OSCALL_RETURN (::fgets (buf, size, fp), char *, 0);

}
#endif /* PDL_HAS_WINCE */

PDL_INLINE int
PDL_OS::system (const wchar_t *command)
{
#   if !defined (PDL_HAS_WINCE)
  PDL_OSCALL_RETURN (::_wsystem (command), int, -1);
#   else
  // @@ Should be able to emulate this using Win32 APIS.
  PDL_UNUSED_ARG (command);
  PDL_NOTSUP_RETURN (-1);
#   endif /* PDL_HAS_WINCE */
}

PDL_INLINE int
PDL_OS::mkdir (const wchar_t *path, mode_t mode)
{
  PDL_TRACE ("PDL_OS::mkdir");
#   if !defined (PDL_HAS_WINCE)
  PDL_UNUSED_ARG (mode);

  PDL_OSCALL_RETURN (::_wmkdir (path), int, -1);
#   else
  PDL_WIN32CALL_RETURN (PDL_ADAPT_RETVAL (::CreateDirectory (path, NULL),
                                          pdl_result_),
                        int, -1);
#   endif /* PDL_HAS_WINCE */
}

PDL_INLINE int
PDL_OS::chdir (const wchar_t *path)
{
  PDL_TRACE ("PDL_OS::chdir");
#   if defined (PDL_HAS_WINCE)
  PDL_UNUSED_ARG (path);
  PDL_NOTSUP_RETURN (-1);
#   else
  PDL_OSCALL_RETURN (::_wchdir (path), int, -1);
#   endif /* PDL_HAS_WINCE */
}

PDL_INLINE wchar_t *
PDL_OS::getcwd (wchar_t *buf, size_t size)
{
  PDL_TRACE ("PDL_OS::getcwd");
#   if defined (PDL_HAS_WINCE)
  PDL_UNUSED_ARG (buf);
  PDL_UNUSED_ARG (size);
  PDL_NOTSUP_RETURN (0);
#   else
  return ::_wgetcwd (buf, size);
#   endif /* PDL_HAS_WINCE */
}
#if defined(PDL_HAS_WINCE)
PDL_INLINE char *
PDL_OS::getcwd (char *buf, size_t size)
{
  PDL_TRACE ("PDL_OS::getcwd");
  PDL_UNUSED_ARG (buf);
  PDL_UNUSED_ARG (size);
  PDL_NOTSUP_RETURN (0);
}
#endif /* PDL_HAS_WINCE */


PDL_INLINE int
PDL_OS::mkfifo (const wchar_t *file, mode_t mode)
{
  // PDL_TRACE ("PDL_OS::mkfifo");
  PDL_UNUSED_ARG (file);
  PDL_UNUSED_ARG (mode);
  PDL_NOTSUP_RETURN (-1);
}
# endif /* PDL_WIN32 */
#endif /* PDL_HAS_UNICODE */

#if defined (PDL_LACKS_COND_T) && defined (PDL_HAS_THREADS)
PDL_INLINE long
PDL_cond_t::waiters (void) const
{
  return this->waiters_;
}
#endif /* PDL_LACKS_COND_T && PDL_HAS_THREADS */

#if !defined(ITRON)
PDL_INLINE int
PDL_OS::sigaddset (sigset_t *s, int signum)
{
  PDL_TRACE ("PDL_OS::sigaddset");
#if defined (PDL_LACKS_SIGSET) || defined (PDL_LACKS_SIGSET_DEFINITIONS)
  if (s == NULL)
    {
      errno = EFAULT;
      return -1;
    }
  else if (signum < 1 || signum >= PDL_NSIG)
    {
      errno = EINVAL;
      return -1;                 // Invalid signum, return error
    }
#   if defined (PDL_PSOS) && defined (__DIAB) && ! defined(PDL_PSOS_DIAB_MIPS) && !defined (PDL_PSOS_DIAB_PPC)
  // treat 0th u_long of sigset_t as high bits,
  // and 1st u_long of sigset_t as low bits.
  if (signum <= PDL_BITS_PER_ULONG)
    s->s[1] |= (1 << (signum - 1));
  else
    s->s[0] |= (1 << (signum - PDL_BITS_PER_ULONG - 1));
#   else
  *s |= (1 << (signum - 1)) ;
#   endif /* defined (PDL_PSOS) && defined (__DIAB) */
  return 0 ;
#else
  PDL_OSCALL_RETURN (::sigaddset (s, signum), int, -1);
#endif /* PDL_LACKS_SIGSET || PDL_LACKS_SIGSET_DEFINITIONS */
}
#endif

PDL_INLINE int
PDL_OS::sigdelset (sigset_t *s, int signum)
{
#if defined (PDL_LACKS_SIGSET) || defined (PDL_LACKS_SIGSET_DEFINITIONS)
  if (s == NULL)
    {
      errno = EFAULT;
      return -1;
    }
  else if (signum < 1 || signum >= PDL_NSIG)
    {
      errno = EINVAL;
      return -1;                 // Invalid signum, return error
    }
#   if defined (PDL_PSOS) && defined (__DIAB) && ! defined (PDL_PSOS_DIAB_MIPS) && !defined (PDL_PSOS_DIAB_PPC)
  // treat 0th u_long of sigset_t as high bits,
  // and 1st u_long of sigset_t as low bits.
  if (signum <= PDL_BITS_PER_ULONG)
    s->s[1] &= ~(1 << (signum - 1));
  else
    s->s[0] &= ~(1 << (signum - PDL_BITS_PER_ULONG - 1));
#   else
  *s &= ~(1 << (signum - 1)) ;
#   endif /* defined (PDL_PSOS) && defined (__DIAB) */
  return 0;
#elif defined(ITRON)
  /* empty */
  PDL_NOTSUP_RETURN (-1);
#else
  PDL_OSCALL_RETURN (::sigdelset (s, signum), int, -1);
#endif /* PDL_LACKS_SIGSET || PDL_LACKS_SIGSET_DEFINITIONS */
}

#if !defined(ITRON)
PDL_INLINE int
PDL_OS::sigemptyset (sigset_t *s)
{
#if defined (PDL_LACKS_SIGSET) || defined (PDL_LACKS_SIGSET_DEFINITIONS)
  if (s == NULL)
    {
      errno = EFAULT;
      return -1;
    }
#   if defined (PDL_PSOS) && defined (__DIAB) && ! defined (PDL_PSOS_DIAB_MIPS) && !defined (PDL_PSOS_DIAB_PPC)
  s->s[0] = 0;
  s->s[1] = 0;
#   else
  *s = 0 ;
#   endif /* defined (PDL_PSOS) && defined (__DIAB) */
  return 0 ;
#else
  PDL_OSCALL_RETURN (::sigemptyset (s), int, -1);
#endif /* PDL_LACKS_SIGSET || PDL_LACKS_SIGSET_DEFINITIONS */
}
#endif

PDL_INLINE int
PDL_OS::sigfillset (sigset_t *s)
{
#if defined (PDL_LACKS_SIGSET) || defined (PDL_LACKS_SIGSET_DEFINITIONS)
  if (s == NULL)
    {
      errno = EFAULT;
      return -1;
    }
#   if defined (PDL_PSOS) && defined (__DIAB) && ! defined (PDL_PSOS_DIAB_MIPS) && !defined (PDL_PSOS_DIAB_PPC)
  s->s[0] = ~(u_long) 0;
  s->s[1] = ~(u_long) 0;
#   else
  *s = ~(sigset_t) 0;
#   endif /* defined (PDL_PSOS) && defined (__DIAB) */
  return 0 ;
#elif defined(ITRON)
  /* empty */
  PDL_NOTSUP_RETURN (-1);
#else
  PDL_OSCALL_RETURN (::sigfillset (s), int, -1);
#endif /* PDL_LACKS_SIGSET || PDL_LACKS_SIGSET_DEFINITIONS */
}

PDL_INLINE int
PDL_OS::sigismember (sigset_t *s, int signum)
{
#if defined (PDL_LACKS_SIGSET) || defined (PDL_LACKS_SIGSET_DEFINITIONS)
  if (s == NULL)
    {
      errno = EFAULT;
      return -1;
    }
  else if (signum < 1 || signum >= PDL_NSIG)
    {
      errno = EINVAL;
      return -1;                 // Invalid signum, return error
    }
#   if defined (PDL_PSOS) && defined (__DIAB) && ! defined (PDL_PSOS_DIAB_MIPS) && !defined (PDL_PSOS_DIAB_PPC)
  // treat 0th u_long of sigset_t as high bits,
  // and 1st u_long of sigset_t as low bits.
  if (signum <= PDL_BITS_PER_ULONG)
    return ((s->s[1] & (1 << (signum - 1))) != 0);
  else
    return ((s->s[0] & (1 << (signum - PDL_BITS_PER_ULONG - 1))) != 0);
#   else
  return ((*s & (1 << (signum - 1))) != 0) ;
#   endif /* defined (PDL_PSOS) && defined (__DIAB) */
#elif defined(ITRON)
  /* empty */
  PDL_NOTSUP_RETURN (-1);
#else
#  if defined (PDL_HAS_SIGISMEMBER_BUG)
  if (signum < 1 || signum >= PDL_NSIG)
    {
      errno = EINVAL;
      return -1;                 // Invalid signum, return error
    }
#  endif /* PDL_HAS_SIGISMEMBER_BUG */
  PDL_OSCALL_RETURN (::sigismember (s, signum), int, -1);
#endif /* PDL_LACKS_SIGSET || PDL_LACKS_SIGSET_DEFINITIONS */
}

PDL_INLINE int
PDL_OS::sigsuspend (const sigset_t *sigset)
{
#if defined(ITRON)
  /* empty */
  PDL_NOTSUP_RETURN (-1);
#else
#if defined (PDL_HAS_SIGSUSPEND)
  sigset_t s;

  if (sigset == 0)
    {
      sigset = &s;
      PDL_OS::sigemptyset (&s);
    }
  PDL_OSCALL_RETURN (::sigsuspend (sigset), int, -1);
#else
  PDL_UNUSED_ARG (sigset);
  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_HAS_SIGSUSPEND */
#endif
}

PDL_INLINE int
PDL_OS::sigprocmask (int how, const sigset_t *nsp, sigset_t *osp)
{
#if defined (PDL_LACKS_SIGSET) || defined (PDL_LACKS_SIGSET_DEFINITIONS)
  PDL_UNUSED_ARG (how);
  PDL_UNUSED_ARG (nsp);
  PDL_UNUSED_ARG (osp);
  PDL_NOTSUP_RETURN (-1);
#else
# if defined (PDL_LACKS_POSIX_PROTOTYPES)
  PDL_OSCALL_RETURN (::sigprocmask (how, (int*) nsp, osp), int, -1);
# else
  PDL_OSCALL_RETURN (::sigprocmask (how, nsp, osp), int, -1);
# endif /* PDL_LACKS_POSIX_PROTOTYPES */
#endif /* PDL_LACKS_SIGSET || PDL_LACKS_SIGSET_DEFINITIONS */
}

PDL_INLINE int
PDL_OS::sigaltstack (const stack_t *ss, stack_t *oss)
{
#if defined (PDL_WIN32)
  PDL_UNUSED_ARG (ss);
  PDL_UNUSED_ARG (oss);
  PDL_NOTSUP_RETURN (0);
#else
  PDL_OSCALL_RETURN (::sigaltstack (ss, oss), int, -1);
#endif /* PDL_WIN32 */
}



PDL_INLINE int
PDL_OS::pthread_sigmask (int how, const sigset_t *nsp, sigset_t *osp)
{
#if defined (PDL_HAS_PTHREADS_STD)  &&  !defined (PDL_LACKS_PTHREAD_SIGMASK)
  PDL_OSCALL_RETURN (PDL_ADAPT_RETVAL (::pthread_sigmask (how, nsp, osp),
                                       pdl_result_),
                     int,
                     -1);

#else /* !PDL_HAS_PTHREADS_STD */
# if defined(PDL_WIN32)
  // for compatible
  return 0;
# elif defined(VXWORKS)
  return 0;
# elif defined(ANDROID)
  return 0;
# else
  PDL_UNUSED_ARG (how);
  PDL_UNUSED_ARG (nsp);
  PDL_UNUSED_ARG (osp);
  PDL_NOTSUP_RETURN (-1);
# endif
#endif /* PDL_HAS_PTHREADS_STD */
}

PDL_INLINE void *
PDL_OS::sbrk (int brk)
{
  PDL_TRACE ("PDL_OS::sbrk");

#if defined (PDL_LACKS_SBRK)
  PDL_UNUSED_ARG (brk);
  PDL_NOTSUP_RETURN (0);
#else
  PDL_OSCALL_RETURN (::sbrk (brk), void *, 0);
#endif /* VXWORKS */
}


PDL_INLINE
PDL_Thread_Adapter::~PDL_Thread_Adapter (void)
{
}

PDL_INLINE PDL_THR_C_FUNC
PDL_Thread_Adapter::entry_point (void)
{
  return this->entry_point_;
}

#if defined (PDL_HAS_WINCE)

//          **** Warning ****
// You should not use the following functions under CE at all.
// These functions are used to make Svc_Conf_l.cpp compile
// under WinCE.  Most of them doesn't do what they are expected
// to do under regular environments.
//          **** Warning ****

PDL_INLINE int
getc (FILE *fp)
{
  char retv;
  if (PDL_OS::fread (&retv, 1, 1, fp) != 1)
    return EOF;
  else
    return retv;
}

PDL_INLINE int
ferror (FILE *fp)
{
  // @@ Hey! What should I implement here?
  return 0;
}

//          **** Warning ****
// You should not use these functions under CE at all.
// These functions are used to make Svc_Conf_l.cpp compile
// under WinCE.  Most of them doesn't do what they are expected
// to do under regular environments.
//          **** Warning ****

PDL_INLINE int
isatty (int h)
{
  return PDL_OS::isatty (h);
}

//int PDL_HANDLE
//fileno (FILE *fp)
//{
// return fp;
//}

//          **** Warning ****
// You should not use these functions under CE at all.
// These functions are used to make Svc_Conf_l.cpp compile
// under WinCE.  Most of them doesn't do what they are expected
// to do under regular environments.
//          **** Warning ****

PDL_INLINE int
printf (const char *format, ...)
{
  PDL_UNUSED_ARG (format);
  return 0;
}

PDL_INLINE int
putchar (int c)
{
  PDL_UNUSED_ARG (c);
  return c;
}

//          **** End CE Warning ****

#endif /* PDL_HAS_WINCE */

#if defined (PDL_PSOS)
PDL_INLINE int
isatty (int h)
{
  return PDL_OS::isatty (h);
}
#if defined (fileno)
#undef fileno
#endif /* defined (fileno)*/
PDL_INLINE PDL_HANDLE
fileno (FILE *fp)
{
  return (PDL_HANDLE) fp;
}
#endif /* defined (PDL_PSOS) */

#if !defined(PDL_HAS_WINCE)

PDL_INLINE DIR *
PDL_OS::opendir (const ASYS_TCHAR *filename)
{
#if defined (PDL_HAS_DIRENT) || defined (PDL_WIN32)
#  if defined (PDL_PSOS)

  // The pointer to the DIR buffer must be passed to PDL_OS::closedir
  // in order to free it and avoid a memory leak.
  DIR *dir;
  u_long result;
  PDL_NEW_RETURN (dir, DIR, sizeof(DIR), 0);
#if defined (PDL_PSOS_DIAB_PPC)
  result = ::open_dir (PDL_const_cast (char *, filename), &(dir->xdir));
#else
  result = ::open_dir (PDL_const_cast (char *, filename), dir);
#endif /* defined PDL_PSOS_DIAB_PPC */
  if (0 == result)
    return dir;
  else
    {
      errno = result;
      return 0;
    }

#  elif defined (PDL_WIN32)
  return win32Dirent::opendir (filename);
#  elif defined(ITRON)
  PDL_UNUSED_ARG (filename);
  PDL_NOTSUP_RETURN (NULL);
#  else /* ! defined (PDL_PSOS) */
  // VxWorks' ::opendir () is declared with a non-const argument.
  return ::opendir (PDL_const_cast (char *, filename));
#  endif /* ! defined (PDL_PSOS) */
#else
  PDL_UNUSED_ARG (filename);
  PDL_NOTSUP_RETURN (0);
#endif /* PDL_HAS_DIRENT */
}

#else

PDL_INLINE DIR *
PDL_OS::opendir (const char *filename)
{
#if defined (PDL_HAS_DIRENT) || defined (PDL_WIN32)
#  if defined (PDL_PSOS)

  // The pointer to the DIR buffer must be passed to PDL_OS::closedir
  // in order to free it and avoid a memory leak.
  DIR *dir;
  u_long result;
  PDL_NEW_RETURN (dir, DIR, sizeof(DIR), 0);
#if defined (PDL_PSOS_DIAB_PPC)
  result = ::open_dir (PDL_const_cast (char *, filename), &(dir->xdir));
#else
  result = ::open_dir (PDL_const_cast (char *, filename), dir);
#endif /* defined PDL_PSOS_DIAB_PPC */
  if (0 == result)
    return dir;
  else
    {
      errno = result;
      return 0;
    }

#  elif defined (PDL_WIN32)
  return win32Dirent::opendir (filename);
#  elif defined(ITRON)
  PDL_UNUSED_ARG (filename);
  PDL_NOTSUP_RETURN (NULL);
#  else /* ! defined (PDL_PSOS) */
  // VxWorks' ::opendir () is declared with a non-const argument.
  return ::opendir (PDL_const_cast (char *, filename));
#  endif /* ! defined (PDL_PSOS) */
#else
  PDL_UNUSED_ARG (filename);
  PDL_NOTSUP_RETURN (0);
#endif /* PDL_HAS_DIRENT */
}
#endif /* PDL_HAS_WINCE */


PDL_INLINE void
PDL_OS::closedir (DIR *d)
{
#if defined (PDL_HAS_DIRENT) || defined(PDL_WIN32)
#  if defined (PDL_PSOS)
  u_long result;
#if defined (PDL_PSOS_DIAB_PPC)
  result = ::close_dir (&(d->xdir));
#else
  result = ::close_dir (d);
#endif /* defined PDL_PSOS_DIAB_PPC */
  PDL_OS::free(d);
  if (result != 0)
    errno = result;
#  elif defined(PDL_WIN32)
  win32Dirent::closedir (d);
#  elif defined(ITRON)
  PDL_UNUSED_ARG (d);
#  else /* ! defined (PDL_PSOS) */
  ::closedir (d);
#  endif /* ! defined (PDL_PSOS) */
#else
  PDL_UNUSED_ARG (d);
#endif /* PDL_HAS_DIRENT */
}


PDL_INLINE struct dirent *
PDL_OS::readdir (DIR *d)
{
#if defined (PDL_HAS_DIRENT) || defined(PDL_WIN32)
#  if defined (PDL_PSOS)
  // The returned pointer to the dirent struct must be deleted by the
  // caller to avoid a memory leak.
  struct dirent *dir_ent;
  u_long result;

  PDL_NEW_RETURN (dir_ent,
                  dirent,
                  sizeof(dirent),
                  0);
#if defined (PDL_PSOS_DIAB_PPC)
  result = ::read_dir (&(d->xdir), dir_ent);
#else
  result = ::read_dir (d, dir_ent);
#endif /* defined PDL_PSOS_DIAB_PPC) */

  if (0 == result)
    return dir_ent;
  else
    {
      errno = result;
      return 0;
    }

#  elif defined (PDL_WIN32)
  return win32Dirent::readdir (d);
#  elif defined(ITRON)
  PDL_UNUSED_ARG (d);
  PDL_NOTSUP_RETURN (0);
#  else /* ! defined (PDL_PSOS) */
  return ::readdir (d);
#  endif /* ! defined (PDL_PSOS) */
#elif defined (PDL_HAS_WINCE)
  return win32Dirent::readdir (d);
#else
  PDL_UNUSED_ARG (d);
  PDL_NOTSUP_RETURN (0);
#endif /* PDL_HAS_DIRENT */
}

PDL_INLINE int
PDL_OS::readdir_r (DIR *dirp,
                   struct dirent *entry,
                   struct dirent **result)
{
# if !defined (PDL_HAS_REENTRANT_FUNCTIONS)
  PDL_UNUSED_ARG (entry);
  // <result> has better not be 0!
  *result = PDL_OS::readdir (dirp);
  return 0;
# elif defined (PDL_HAS_DIRENT)  &&  !defined (PDL_LACKS_READDIR_R)
# if (defined (sun) && (defined (_POSIX_PTHREAD_SEMANTICS) || \
                        (_FILE_OFFSET_BITS == 64))) || \
      (!defined (sun) && (defined (PDL_HAS_PTHREADS_STD) || \
                         defined (PDL_HAS_PTHREADS_DRAFT7) || \
                         defined (__USE_POSIX)))
    return ::readdir_r (dirp, entry, result);
# else  /* ! POSIX.1c - this is draft 4 or draft 6 */
#   if defined (HPUX_10)   /* But HP 10.x doesn't follow the draft either */
    *result = entry;
    return ::readdir_r (dirp, entry);
#   else
    // <result> had better not be 0!
    *result = ::readdir_r (dirp, entry);
    return 0;
#   endif /* HPUX_10 */
# endif /* ! POSIX.1c */
#else  /* ! PDL_HAS_DIRENT  ||  PDL_LACKS_READDIR_R */
  PDL_UNUSED_ARG (dirp);
  PDL_UNUSED_ARG (entry);
  PDL_UNUSED_ARG (result);
  PDL_NOTSUP_RETURN (0);

#endif /* !PDL_HAS_REENTRANT_FUNCTIONS */
}

PDL_INLINE long
PDL_OS::telldir (DIR *d)
{
#if defined (PDL_HAS_DIRENT)  &&  !defined (PDL_LACKS_TELLDIR)
  return ::telldir (d);
#else  /* ! PDL_HAS_DIRENT  ||  PDL_LACKS_TELLDIR */
  PDL_UNUSED_ARG (d);
  PDL_NOTSUP_RETURN (-1);
#endif /* ! PDL_HAS_DIRENT  ||  PDL_LACKS_TELLDIR */
}

PDL_INLINE void
PDL_OS::seekdir (DIR *d, long loc)
{
#if defined (PDL_HAS_DIRENT)  &&  !defined (PDL_LACKS_SEEKDIR)
  ::seekdir (d, loc);
#else  /* ! PDL_HAS_DIRENT  ||  PDL_LACKS_SEEKDIR */
  PDL_UNUSED_ARG (d);
  PDL_UNUSED_ARG (loc);
#endif /* ! PDL_HAS_DIRENT  ||  PDL_LACKS_SEEKDIR */
}

PDL_INLINE void
PDL_OS::rewinddir (DIR *d)
{
#if defined (PDL_HAS_DIRENT) || defined(PDL_WIN32)
# if defined (PDL_LACKS_SEEKDIR)
#  if defined (PDL_LACKS_REWINDDIR)
  PDL_UNUSED_ARG (d);
#  elif defined(PDL_WIN32) && !defined(PDL_HAS_WINCE)
  win32Dirent::rewinddir (d);
#  elif defined(PDL_HAS_WINCE)
  PDL_OS::seekdir(d, long(0));
#  elif defined(ITRON)
  PDL_UNUSED_ARG (d);
#  else /* ! defined (PDL_LACKS_REWINDDIR) */
   ::rewinddir (d);
#  endif /* ! defined (PDL_LACKS_REWINDDIR) */
# else  /* ! PDL_LACKS_SEEKDIR */
    // We need to implement <rewinddir> using <seekdir> since it's often
    // defined as a macro...
   ::seekdir (d, long (0));
# endif /* ! PDL_LACKS_SEEKDIR */
#else
  PDL_UNUSED_ARG (d);
#endif /* PDL_HAS_DIRENT */
}

PDL_INLINE int PDL_OS::fchmod(int fd, mode_t mode)
{
#if defined (PDL_WIN32)
    return 0;
#else
    return ::fchmod(fd, mode);
#endif
}

PDL_INLINE void *
PDL_OS::bsearch (const void *key,
                 const void *base,
                 size_t nel,
                 size_t size,
                 PDL_COMPARE_FUNC compar)
{
#if !defined (PDL_LACKS_BSEARCH)
  return ::bsearch (key, base, nel, size, compar);
#else
#if defined(PDL_HAS_WINCE)
    int sFound = 0;
    int sMinimum = 0;
    int sMedium = 0;
    int sMaximum = nel -1;

    while( sMinimum <= sMaximum)
    {
        sMedium = (sMinimum + sMaximum) >> 1;
        if( compar( (char*)base + size*sMedium, key ) >= 0 )
        {
            sMaximum = sMedium - 1;
            sFound   = sMedium;
        }
        else
        {
            sMinimum = sMedium + 1;
            sFound   = sMinimum;
        }
    }
    return (void*)((char*)base + size*sFound);

#endif /*PDL_HAS_WINCE*/
  PDL_UNUSED_ARG (key);
  PDL_UNUSED_ARG (base);
  PDL_UNUSED_ARG (nel);
  PDL_UNUSED_ARG (size);
  PDL_UNUSED_ARG (compar);
  PDL_NOTSUP_RETURN (NULL);
#endif /* PDL_LACKS_BSEARCH */
}

PDL_INLINE void
PDL_OS::qsort (void *base,
               size_t nel,
               size_t width,
               PDL_COMPARE_FUNC compar)
{
#if !defined (PDL_LACKS_QSORT)
  ::qsort (base, nel, width, compar);
#else
  PDL_UNUSED_ARG (base);
  PDL_UNUSED_ARG (nel);
  PDL_UNUSED_ARG (width);
  PDL_UNUSED_ARG (compar);
#endif /* PDL_LACKS_QSORT */
}

PDL_INLINE int
PDL_OS::setuid (uid_t uid)
{
  PDL_TRACE ("PDL_OS::setuid");
# if defined (VXWORKS) || defined (PDL_PSOS)
  // setuid() is not supported:  just one user anyways
  PDL_UNUSED_ARG (uid);
  return 0;
# elif defined (PDL_WIN32) || defined(CHORUS)
  PDL_UNUSED_ARG (uid);
  PDL_NOTSUP_RETURN (-1);
#elif defined(ITRON)
  /* empty */
  PDL_NOTSUP_RETURN (-1);
#else
  PDL_OSCALL_RETURN (::setuid (uid), int,  -1);
# endif /* VXWORKS */
}

PDL_INLINE uid_t
PDL_OS::getuid (void)
{
  PDL_TRACE ("PDL_OS::getuid");
# if defined (VXWORKS) || defined (PDL_PSOS)
  // getuid() is not supported:  just one user anyways
  return 0;
# elif defined (PDL_WIN32) || defined(CHORUS)
  PDL_NOTSUP_RETURN (PDL_static_cast (uid_t, -1));
#elif defined(ITRON)
  /* empty */
  PDL_NOTSUP_RETURN (-1);
#else
  PDL_OSCALL_RETURN (::getuid (), uid_t, (uid_t) -1);
# endif /* VXWORKS */
}

PDL_INLINE int
PDL_OS::setgid (gid_t gid)
{
  PDL_TRACE ("PDL_OS::setgid");
# if defined (VXWORKS) || defined (PDL_PSOS)
  // setgid() is not supported:  just one user anyways
  PDL_UNUSED_ARG (gid);
  return 0;
# elif defined (PDL_WIN32) || defined(CHORUS)
  PDL_UNUSED_ARG (gid);
  PDL_NOTSUP_RETURN (-1);
#elif defined(ITRON)
  /* empty */
  PDL_NOTSUP_RETURN (-1);
#else
  PDL_OSCALL_RETURN (::setgid (gid), int,  -1);
# endif /* VXWORKS */
}

PDL_INLINE gid_t
PDL_OS::getgid (void)
{
  PDL_TRACE ("PDL_OS::getgid");
# if defined (VXWORKS) || defined (PDL_PSOS)
  // getgid() is not supported:  just one user anyways
  return 0;
# elif defined (PDL_WIN32) || defined(CHORUS)
  PDL_NOTSUP_RETURN (PDL_static_cast (gid_t, -1));
#elif defined(ITRON)
  /* empty */
  PDL_NOTSUP_RETURN (PDL_static_cast (gid_t, -1));
#else
  PDL_OSCALL_RETURN (::getgid (), gid_t, (gid_t) -1);
# endif /* VXWORKS */
}

PDL_INLINE PDL_EXIT_HOOK
PDL_OS::set_exit_hook (PDL_EXIT_HOOK exit_hook)
{
  PDL_EXIT_HOOK old_hook = exit_hook_;
  exit_hook_ = exit_hook;
  return old_hook;
}

PDL_INLINE int
PDL_OS::isatty (int handle)
{
# if defined (PDL_LACKS_ISATTY)
  PDL_UNUSED_ARG (handle);
  return 0;
# elif defined (PDL_WIN32)
  PDL_TRACE ("PDL_OS::isatty");
  return ::_isatty (handle);
#elif defined(ITRON)
  /* empty */
  PDL_NOTSUP_RETURN (-1);
# else
  PDL_TRACE ("PDL_OS::isatty");
  PDL_OSCALL_RETURN (::isatty (handle), int, -1);
# endif /* defined (PDL_LACKS_ISATTY) */
}

#if defined (PDL_WIN32)
# if !defined (PDL_HAS_WINCE)
PDL_INLINE int
PDL_OS::isatty (PDL_HANDLE handle)
{
#   if defined (PDL_WIN64)
  int fd = ::_open_osfhandle ((intptr_t) handle, 0);
#   else
  int fd = ::_open_osfhandle ((long) handle, 0);
#   endif /* PDL_WIN64 */
  return ::_isatty (fd);
}
# endif /* !PDL_HAS_WINCE */

PDL_INLINE void
PDL_OS::fopen_mode_to_open_mode_converter (char x, int &hmode)
{
    switch (x)
      {
      case 'r':
        if (PDL_BIT_DISABLED (hmode, _O_RDWR))
          {
            PDL_CLR_BITS (hmode, _O_WRONLY);
            PDL_SET_BITS (hmode, _O_RDONLY);
          }
        break;
      case 'w':
        if (PDL_BIT_DISABLED (hmode, _O_RDWR))
          {
            PDL_CLR_BITS (hmode, _O_RDONLY);
            PDL_SET_BITS (hmode, _O_WRONLY);
          }
        PDL_SET_BITS (hmode, _O_CREAT | _O_TRUNC);
        break;
      case 'a':
        if (PDL_BIT_DISABLED (hmode, _O_RDWR))
          {
            PDL_CLR_BITS (hmode, _O_RDONLY);
            PDL_SET_BITS (hmode, _O_WRONLY);
          }
        PDL_SET_BITS (hmode, _O_CREAT | _O_APPEND);
        break;
      case '+':
        PDL_CLR_BITS (hmode, _O_RDONLY | _O_WRONLY);
        PDL_SET_BITS (hmode, _O_RDWR);
        break;
      case 't':
        PDL_CLR_BITS (hmode, _O_BINARY);
        PDL_SET_BITS (hmode, _O_TEXT);
        break;
      case 'b':
        PDL_CLR_BITS (hmode, _O_TEXT);
        PDL_SET_BITS (hmode, _O_BINARY);
        break;
      }
}
#endif /* PDL_WIN32 */

// Return a dynamically allocated duplicate of <str>, substituting the
// environment variable if <str[0] == '$'>.  Note that the pointer is
// allocated with <PDL_OS::malloc> and must be freed by
// <PDL_OS::free>.

PDL_INLINE char *
PDL_OS::strenvdup (const char *str)
{
#if defined (PDL_HAS_WINCE)
     // WinCE doesn't have environment variables so we just skip it.
  return PDL_OS::strdup (str);
#else
  char *temp = 0;

  if (str[0] == '$'
      && (temp = PDL_OS::getenv (&str[1])) != 0)
    return PDL_OS::strdup (temp);
  else
    return PDL_OS::strdup (str);
#endif /* PDL_HAS_WINCE */
}

#if !defined (PDL_HAS_WCHAR_TYPEDEFS_CHAR) && defined (PDL_WIN32)
PDL_INLINE wchar_t *
PDL_OS::strenvdup (const wchar_t *str)
{
#if defined (PDL_HAS_WINCE)
     // WinCE doesn't have environment variables so we just skip it.
  return PDL_OS::strdup (str);
#else
  wchar_t *temp = 0;

  if (str[0] == '$'
      && (temp = PDL_OS::getenv (&str[1])) != 0)
    return PDL_OS::strdup (temp);
  else
    return PDL_OS::strdup (str);
#endif /* PDL_HAS_WINCE */
}
#endif /* PDL_HAS_WCHAR_TYPEDEFS_CHAR */


#if defined (PDL_WIN32)
PDL_INLINE const OSVERSIONINFO &
PDL_OS::get_win32_versioninfo ()
{
  return PDL_OS::win32_versioninfo_;
}
#endif /* PDL_WIN32 */

#if defined (PDL_WIN32)
PDL_INLINE char *
PDL_OS::getpass(const char *prompt)
{
  static char buf[128];
  int i;

  fputs(prompt, stdout);
  fflush(stdout);
  for (i = 0; i < sizeof(buf); i++) {
#if !defined(PDL_HAS_WINCE)
       buf[i] = _getch();
#else
       buf[i] = getc(stdin);
#endif /* !PDL_HAS_WINCE */
       if (buf[i] == '\r')
           break;
  }
  buf[i] = 0;
  fputs("\n", stdout);
  return buf;
}
#endif /* PDL_WIN32 */

#if defined (__QNX__)
# include "../qnx/qnxsem.cpp"
# include "../qnx/qnxshm.cpp"
#endif /* __QNX__ */

#if defined (PDL_WIN32)
# if !defined(PDL_HAS_WINCE)
# include <io.h>
# include <errno.h>
# endif /* !PDL_HAS_WINCE */
# include <stdlib.h>
# include <string.h>

struct _DIR
{
    HANDLE                      handle;   /* -1 for failed rewind */
    PDL_TEXT_WIN32_FIND_DATA    finddata;
    struct dirent               ent;      /* d_name null iff first time */
    char                       *dir;     /* NTBS */
    short                       finished; // additional flag
    long                        offset;   // additional flag
};

PDL_INLINE DIR *win32Dirent::opendir(const char *dir)
{
    DIR         *dp;
    char	    *findspec;
    HANDLE      handle;
    size_t      dirlen;

    dirlen = PDL_OS::strlen(dir);
    findspec = (char *)malloc(dirlen + 2 + 1);
    if (findspec == NULL)
    {
        return NULL;
    }

    if (dirlen == 0)
    {
        PDL_OS::strcpy(findspec, "*");
    }
    else if (isalpha(dir[0]) && dir[1] == ':' && dir[2] == '\0')
    {
        PDL_OS::sprintf(findspec, "%s*", dir);
    }
    else if (dir[dirlen - 1] == '/' || dir[dirlen - 1] == '\\')
    {
        PDL_OS::sprintf(findspec, "%s*", dir);
    }
    else
    {
        if (PDL_OS::strchr(dir, '/') != NULL)
        {
            PDL_OS::sprintf(findspec, "%s/*", dir);
        }
        else // if (strchr(dir, '\\') != NULL)
        {
            PDL_OS::sprintf(findspec, "%s\\*", dir);
        }
    }

    dp = (DIR *)malloc(sizeof(DIR));
    if (dp == NULL)
    {
        free(findspec);
        errno = ENOMEM;
        return NULL;
    }

    dp->offset = 0;
    dp->finished = 0;
    dp->dir = PDL_OS::strdup(dir);
    if (dp->dir == NULL)
    {
        free(dp);
        free(findspec);
        errno = ENOMEM;
        return NULL;
    }
#if defined(PDL_HAS_WINCE) && defined(PDL_HAS_UNICODE)
	wchar_t *wFindspec;
	wFindspec = PDL_Ascii_To_Wide::convert (findspec);
	handle = PDL_TEXT_FindFirstFile(wFindspec, &(dp->finddata));
	delete []wFindspec;
#else
    handle = PDL_TEXT_FindFirstFile(findspec, &(dp->finddata));
#endif /* PDL_HASH_WINCE && PDL_HAS_UNICODE */
    if (handle == INVALID_HANDLE_VALUE)
    {
        free(dp->dir);
        free(dp);
        free(findspec);
        errno = ENOENT;
        return NULL;
    }
    dp->handle = handle;

    free(findspec);
    return dp;
}


PDL_INLINE int win32Dirent::closedir(DIR *dp)
{
    FindClose(dp->handle);
    free(dp->dir);
    free(dp);

    return 0;
}

PDL_INLINE struct dirent *win32Dirent::readdir(DIR *dp)
{
    if (dp == NULL || dp->finished)
    {
        return NULL;
    }

    if (dp->offset != 0)
    {
        if (PDL_TEXT_FindNextFile(dp->handle, &(dp->finddata)) == 0)
        {
            dp->finished = 1;
            return NULL;
        }
    }
    dp->offset++;

#if defined(PDL_HAS_WINCE) && defined(PDL_HAS_UNICODE)
	char *sFileName;
	sFileName = PDL_Wide_To_Ascii::convert(dp->finddata.cFileName);
    ::strncpy(dp->ent.d_name, sFileName, 256);
#else
    ::strncpy(dp->ent.d_name, dp->finddata.cFileName, 256);
#endif /* PDL_HAS_WINCE && PDL_HAS_UNICODE */

    dp->ent.d_ino  = 1;

    return &(dp->ent);
}

#if !defined(PDL_HAS_WINCE)
PDL_INLINE void win32Dirent::rewinddir(DIR *dp)
{
    if(dp && dp->handle == INVALID_HANDLE_VALUE)
    {
        FindClose(dp->handle);
        dp->handle = PDL_TEXT_FindFirstFile(dp->dir, &(dp->finddata));
        ::memset(dp->ent.d_name, '\0', 256);
    }
    else
    {
        errno = EBADF;
    }
}
#endif /* !PDL_HAS_WINCE */



#endif
