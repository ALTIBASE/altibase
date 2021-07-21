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
 
// OS.cpp,v 4.497 2000/02/03 19:35:30 schmidt Exp

#define PDL_BUILD_DLL
#include "OS.h"
#include "PDL2.h"

#if defined (PDL_HAS_WINCE_BROKEN_ERRNO)

PDL_CE_Errno *PDL_CE_Errno::instance_ = 0;
DWORD         PDL_CE_Errno::errno_key_ = 0xffffffff;

void
PDL_CE_Errno::init ()
{
  PDL_NEW (PDL_CE_Errno::instance_,
           PDL_CE_Errno,
           sizeof(PDL_CE_Errno));
  PDL_CE_Errno::errno_key_ = TlsAlloc ();
}

void
PDL_CE_Errno::fini ()
{
  TlsFree (PDL_CE_Errno::errno_key_);
  delete PDL_CE_Errno::instance_;
  PDL_CE_Errno::instance_ = 0;
}

#endif /* PDL_HAS_WINCE_BROKEN_ERRNO */

#if defined(DEBUG)
int PDL_OS::bDaemonPritable = 0;
#endif

#if defined (PDL_THREADS_DONT_INHERIT_LOG_MSG)  || \
    defined (PDL_HAS_MINIMAL_PDL_OS)
# if defined (PDL_PSOS)
// Unique file identifier
int unique_file_id=0;
# endif /* PDL_PSOS */
#endif /* PDL_THREADS_DONT_INHERIT_LOG_MSG) || PDL_HAS_MINIMAL_PDL_OS */

// Perhaps we should *always* include OS.i in order to make sure
// we can always link against the OS symbols?
#if !defined (PDL_HAS_INLINED_OSCALLS)
# include "OS.i"
#endif /* PDL_HAS_INLINED_OS_CALLS */

PDL_RCSID(pdl, OS, "OS.cpp,v 4.497 2000/02/03 19:35:30 schmidt Exp")

#if defined (PDL_MT_SAFE) && (PDL_MT_SAFE != 0)
# if defined (PDL_HAS_WINCE)
const char *PDL_OS::day_of_week_name[] = {"Sun", "Mon",
                                          "Tue", "Wed",
                                          "Thu", "Fri",
                                          "Sat"};

const char *PDL_OS::month_name[] = {"Jan", "Feb",
                                    "Mar", "Apr",
                                    "May", "Jun",
                                    "Jul", "Aug",
                                    "Sep", "Oct",
                                    "Nov", "Dec"};

static const char *PDL_OS_CTIME_R_FMTSTR = "%3s %3s %02d %02d:%02d:%02d %04d\n";

int putenv(const char *string)
{
  return PDL_OS::putenv(string);
}

char * getenv(const char *string)
{
  return PDL_OS::getenv(string);
}
# endif /* PDL_HAS_WINCE */

# if defined (PDL_WIN32)
OSVERSIONINFO PDL_OS::win32_versioninfo_;
// Cached win32 version information.
# endif /* PDL_WIN32 */

class PDL_OS_Thread_Mutex_Guard
{
  // = TITLE
  //     This data structure is meant to be used within an PDL_OS
  //     function.  It performs automatic aquisition and release of
  //     an PDL_thread_mutex_t.
  //
  // = DESCRIPTION
  //     For internal use only by PDL_OS.
public:
  PDL_OS_Thread_Mutex_Guard (PDL_thread_mutex_t &m);
  // Implicitly and automatically acquire the lock.

  ~PDL_OS_Thread_Mutex_Guard (void);
  // Implicitly release the lock.

  int acquire (void);
  // Explicitly acquire the lock.

  int release (void);
  // Explicitly release the lock.

protected:
  PDL_thread_mutex_t &lock_;
  // Reference to the mutex.

  int owner_;
  // Keeps track of whether we acquired the lock or failed.

  // = Prevent assignment and initialization.
  PDL_OS_Thread_Mutex_Guard &operator= (const PDL_OS_Thread_Mutex_Guard &);
  PDL_OS_Thread_Mutex_Guard (const PDL_OS_Thread_Mutex_Guard &);
};

inline
int
PDL_OS_Thread_Mutex_Guard::acquire (void)
{
  return owner_ = PDL_OS::thread_mutex_lock (&lock_);
}

inline
int
PDL_OS_Thread_Mutex_Guard::release (void)
{
  if (owner_ == -1)
    return 0;
  else
    {
      owner_ = -1;
      return PDL_OS::thread_mutex_unlock (&lock_);
    }
}

inline
PDL_OS_Thread_Mutex_Guard::PDL_OS_Thread_Mutex_Guard (PDL_thread_mutex_t &m)
   : lock_ (m)
{
  acquire ();
}

PDL_OS_Thread_Mutex_Guard::~PDL_OS_Thread_Mutex_Guard ()
{
  release ();
}

class PDL_OS_Recursive_Thread_Mutex_Guard
{
  // = TITLE
  //     This data structure is meant to be used within an PDL_OS
  //     function.  It performs automatic aquisition and release of
  //     an PDL_recursive_thread_mutex_t.
  //
  // = DESCRIPTION
  //     For internal use only by PDL_OS.
public:
  PDL_OS_Recursive_Thread_Mutex_Guard (PDL_recursive_thread_mutex_t &m);
  // Implicitly and automatically acquire the lock.

  ~PDL_OS_Recursive_Thread_Mutex_Guard (void);
  // Implicitly release the lock.

  int acquire (void);
  // Explicitly acquire the lock.

  int release (void);
  // Explicitly release the lock.

protected:
  PDL_recursive_thread_mutex_t &lock_;
  // Reference to the mutex.

  int owner_;
  // Keeps track of whether we acquired the lock or failed.

  // = Prevent assignment and initialization.
  PDL_OS_Recursive_Thread_Mutex_Guard &operator= (
    const PDL_OS_Recursive_Thread_Mutex_Guard &);
  PDL_OS_Recursive_Thread_Mutex_Guard (
    const PDL_OS_Recursive_Thread_Mutex_Guard &);
};

inline
int
PDL_OS_Recursive_Thread_Mutex_Guard::acquire (void)
{
  return owner_ = PDL_OS::recursive_mutex_lock (&lock_);
}

inline
int
PDL_OS_Recursive_Thread_Mutex_Guard::release (void)
{
  if (owner_ == -1)
    return 0;
  else
    {
      owner_ = -1;
      return PDL_OS::recursive_mutex_unlock (&lock_);
    }
}

inline
PDL_OS_Recursive_Thread_Mutex_Guard::PDL_OS_Recursive_Thread_Mutex_Guard (
  PDL_recursive_thread_mutex_t &m)
   : lock_ (m),
     owner_ (-1)
{
  acquire ();
}

PDL_OS_Recursive_Thread_Mutex_Guard::~PDL_OS_Recursive_Thread_Mutex_Guard ()
{
  release ();
}

#define PDL_OS_GUARD \
  PDL_OS_Thread_Mutex_Guard (*(PDL_thread_mutex_t *) \
    PDL_OS_Object_Manager::preallocated_object[ \
      PDL_OS_Object_Manager::PDL_OS_MONITOR_LOCK]);

#define PDL_TSS_CLEANUP_GUARD \
  PDL_OS_Recursive_Thread_Mutex_Guard (*(PDL_recursive_thread_mutex_t *) \
    PDL_OS_Object_Manager::preallocated_object[ \
      PDL_OS_Object_Manager::PDL_TSS_CLEANUP_LOCK]);

#define PDL_TSS_BASE_GUARD \
  PDL_OS_Recursive_Thread_Mutex_Guard (*(PDL_recursive_thread_mutex_t *) \
    PDL_OS_Object_Manager::preallocated_object[ \
      PDL_OS_Object_Manager::PDL_TSS_BASE_LOCK]);


# if defined (PDL_LACKS_NETDB_REENTRANT_FUNCTIONS)
int
PDL_OS::netdb_acquire (void)
{
  return PDL_OS::thread_mutex_lock ((PDL_thread_mutex_t *)
    PDL_OS_Object_Manager::preallocated_object[
      PDL_OS_Object_Manager::PDL_OS_MONITOR_LOCK]);
}

int
PDL_OS::netdb_release (void)
{
  return PDL_OS::thread_mutex_unlock ((PDL_thread_mutex_t *)
    PDL_OS_Object_Manager::preallocated_object[
      PDL_OS_Object_Manager::PDL_OS_MONITOR_LOCK]);
}
# endif /* defined (PDL_LACKS_NETDB_REENTRANT_FUNCTIONS) */
#else  /* ! PDL_MT_SAFE */
# define PDL_OS_GUARD
# define PDL_TSS_CLEANUP_GUARD
# define PDL_TSS_BASE_GUARD
#endif /* ! PDL_MT_SAFE */

PDL_EXIT_HOOK PDL_OS::exit_hook_ = 0;

PDL_ALLOC_HOOK_DEFINE(PDL_Time_Value)

// Initializes the PDL_Time_Value object from a timeval.

#if defined (PDL_WIN32)
//  Initializes the PDL_Time_Value object from a Win32 FILETIME

// Static constant to remove time skew between FILETIME and POSIX
// time.
//
// In the beginning (Jan. 1, 1601), there was no time and no computer.
// And Bill said: "Let there be time," and there was time....
const DWORDLONG PDL_Time_Value::FILETIME_to_timval_skew =
PDL_INT64_LITERAL (0x19db1ded53e8000);

void PDL_Time_Value::initialize (const FILETIME &file_time)
{
  // PDL_TRACE ("PDL_Time_Value::initialize");
  this->set (file_time);
}

void PDL_Time_Value::set (const FILETIME &file_time)
{
  //  Initializes the PDL_Time_Value object from a Win32 FILETIME
  ULARGE_INTEGER _100ns =
  {
    file_time.dwLowDateTime,
    file_time.dwHighDateTime
  };
  _100ns.QuadPart -= PDL_Time_Value::FILETIME_to_timval_skew;

  // Convert 100ns units to seconds;
  this->tv_.tv_sec = (long) (_100ns.QuadPart / (10000 * 1000));
  // Convert remainder to microseconds;
  this->tv_.tv_usec = (long) ((_100ns.QuadPart % (10000 * 1000)) / 10);
}

// Returns the value of the object as a Win32 FILETIME.

PDL_Time_Value::operator FILETIME () const
{
  PDL_TRACE ("PDL_Time_Value::operator FILETIME");
  ULARGE_INTEGER _100ns;
  _100ns.QuadPart = (((DWORDLONG) this->tv_.tv_sec * (10000 * 1000) +
                      this->tv_.tv_usec * 10) +
                     PDL_Time_Value::FILETIME_to_timval_skew);
  FILETIME file_time;

  file_time.dwLowDateTime = _100ns.LowPart;
  file_time.dwHighDateTime = _100ns.HighPart;

  return file_time;
}

#endif /* PDL_WIN32 */


void
PDL_Time_Value::dump (void) const
{
  PDL_TRACE ("PDL_Time_Value::dump");
  PDL_DEBUG ((LM_DEBUG, PDL_BEGIN_DUMP, this));
  PDL_DEBUG ((LM_DEBUG, ASYS_TEXT ("\ntv_sec_ = %d"), this->tv_.tv_sec));
  PDL_DEBUG ((LM_DEBUG, ASYS_TEXT ("\ntv_usec_ = %d\n"), this->tv_.tv_usec));
  PDL_DEBUG ((LM_DEBUG, PDL_END_DUMP));
}

void
PDL_Time_Value::normalize (void)
{
  // PDL_TRACE ("PDL_Time_Value::normalize");
  // New code from Hans Rohnert...

  if (this->tv_.tv_usec >= PDL_ONE_SECOND_IN_USECS)
    {
      do
        {
          this->tv_.tv_sec++;
          this->tv_.tv_usec -= PDL_ONE_SECOND_IN_USECS;
        }
      while (this->tv_.tv_usec >= PDL_ONE_SECOND_IN_USECS);
    }
  else if (this->tv_.tv_usec <= -PDL_ONE_SECOND_IN_USECS)
    {
      do
        {
          this->tv_.tv_sec--;
          this->tv_.tv_usec += PDL_ONE_SECOND_IN_USECS;
        }
      while (this->tv_.tv_usec <= -PDL_ONE_SECOND_IN_USECS);
    }

  if (this->tv_.tv_sec >= 1 && this->tv_.tv_usec < 0)
    {
      this->tv_.tv_sec--;
      this->tv_.tv_usec += PDL_ONE_SECOND_IN_USECS;
    }
  else if (this->tv_.tv_sec < 0 && this->tv_.tv_usec > 0)
    {
      this->tv_.tv_sec++;
      this->tv_.tv_usec -= PDL_ONE_SECOND_IN_USECS;
    }
}

#if defined (PDL_HAS_POWERPC_TIMER) && defined (ghs)
void
PDL_OS::readPPCTimeBase (u_long &most, u_long &least)
{
  PDL_TRACE ("PDL_OS::readPPCTimeBase");

  // This function can't be inline because it depends on the arguments
  // being in particular registers (r3 and r4), in conformance with the
  // EABI standard.  It would be nice if we knew how to put the variable
  // names directly into the assembler instructions . . .
  asm("aclock:");
  asm("mftb  r5,TBU");
  asm("mftb  r6,TBL");
  asm("mftb  r7,TBU");
  asm("cmpw  r5,r7");
  asm("bne   aclock");

  asm("stw r5, 0(r3)");
  asm("stw r6, 0(r4)");
}
#elif defined (PDL_HAS_POWERPC_TIMER) && defined (__GNUG__)
void
PDL_OS::readPPCTimeBase (u_long &most, u_long &least)
{
  PDL_TRACE ("PDL_OS::readPPCTimeBase");

  // This function can't be inline because it defines a symbol,
  // aclock.  If there are multiple calls to the function in a
  // compilation unit, then that symbol would be multiply defined if
  // the function was inline.
  asm volatile ("aclock:\n"
                "mftbu 5\n"     /* upper time base register */
                "mftb 6\n"      /* lower time base register */
                "mftbu 7\n"     /* upper time base register */
                "cmpw 5,7\n" /* check for rollover of upper */
                "bne aclock\n"
                "stw 5,%0\n"                        /* most */
                "stw 6,%1"                         /* least */
                : "=m" (most), "=m" (least)      /* outputs */
                :                              /* no inputs */
                : "5", "6", "7", "memory"    /* constraints */);
}
#endif /* PDL_HAS_POWERPC_TIMER  &&  (ghs or __GNUG__) */

#if defined (PDL_WIN32) || defined (VXWORKS) || defined (CHORUS) || defined (PDL_PSOS)
// Don't inline on those platforms because this function contains
// string literals, and some compilers, e.g., g++, don't handle those
// efficiently in unused inline functions.
int
PDL_OS::uname (struct utsname *name)
{
  PDL_TRACE ("PDL_OS::uname");
# if defined (PDL_WIN32)
  size_t maxnamelen = sizeof name->nodename;
  PDL_OS::strcpy (name->sysname,
                  "Win32");

  OSVERSIONINFO vinfo;
  vinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
  ::GetVersionEx (&vinfo);

  SYSTEM_INFO sinfo;
#   if defined (PDL_HAS_PHARLAP)
  // PharLap doesn't do GetSystemInfo.  What's really wanted is the CPU
  // architecture, so we can get that with EtsGetSystemInfo. Fill in what's
  // wanted in the SYSTEM_INFO structure, and carry on. Note that the
  // CPU type values in EK_KERNELINFO have the same values are the ones
  // defined for SYSTEM_INFO.
  EK_KERNELINFO ets_kern;
  EK_SYSTEMINFO ets_sys;
  EtsGetSystemInfo (&ets_kern, &ets_sys);
  sinfo.wProcessorLevel = PDL_static_cast (WORD, ets_kern.CpuType);
  sinfo.wProcessorArchitecture = PROCESSOR_ARCHITECTURE_INTEL;
  sinfo.dwProcessorType = ets_kern.CpuType * 100 + 86;
#   else
  ::GetSystemInfo(&sinfo);

  PDL_OS::strcpy (name->sysname, "Win32");
#   endif /* PDL_HAS_PHARLAP */

  if (vinfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
  {
    // Get information from the two structures
    PDL_OS::sprintf (name->release,
#   if defined (PDL_HAS_WINCE)
                     "Windows CE %d.%d",
#   else
                     "Windows NT %d.%d",
#   endif /* PDL_HAS_WINCE */
                     vinfo.dwMajorVersion,
                     vinfo.dwMinorVersion);
    PDL_OS::sprintf (name->version,
                     "Build %d %s",
                     vinfo.dwBuildNumber,
                     vinfo.szCSDVersion);

    // We have to make sure that the size of (processor + subtype) is
    // not greater than the size of name->machine.  So we give half
    // the space to the processor and half the space to subtype.  The
    // -1 is necessary for because of the space between processor and
    // subtype in the machine name.
    const int bufsize = ((sizeof (name->machine) / sizeof (TCHAR)) / 2) - 1;
    char processor[bufsize] = "Unknown";
    char subtype[bufsize]   = "Unknown";

    WORD arch = sinfo.wProcessorArchitecture;

    switch (arch)
    {
    case PROCESSOR_ARCHITECTURE_INTEL:
      PDL_OS::strcpy (processor, "Intel");
      if (sinfo.wProcessorLevel == 3)
        PDL_OS::strcpy (subtype, "80386");
      else if (sinfo.wProcessorLevel == 4)
        PDL_OS::strcpy (subtype, "80486");
      else if (sinfo.wProcessorLevel == 5)
        PDL_OS::strcpy (subtype, "Pentium");
      else if (sinfo.wProcessorLevel == 6)
        PDL_OS::strcpy (subtype, "Pentium Pro");
      else if (sinfo.wProcessorLevel == 7)  // I'm guessing here
        PDL_OS::strcpy (subtype, "Pentium II");
      break;
    case PROCESSOR_ARCHITECTURE_MIPS:
      PDL_OS::strcpy (processor, "MIPS");
      PDL_OS::strcpy (subtype, "R4000");
      break;
    case PROCESSOR_ARCHITECTURE_ALPHA:
      PDL_OS::strcpy (processor, "Alpha");
      PDL_OS::sprintf (subtype, "%d", sinfo.wProcessorLevel);
      break;
    case PROCESSOR_ARCHITECTURE_PPC:
      PDL_OS::strcpy (processor, "PPC");
      if (sinfo.wProcessorLevel == 1)
        PDL_OS::strcpy (subtype, "601");
      else if (sinfo.wProcessorLevel == 3)
        PDL_OS::strcpy (subtype, "603");
      else if (sinfo.wProcessorLevel == 4)
        PDL_OS::strcpy (subtype, "604");
      else if (sinfo.wProcessorLevel == 6)
        PDL_OS::strcpy (subtype, "603+");
      else if (sinfo.wProcessorLevel == 9)
        PDL_OS::strcpy (subtype, "804+");
      else if (sinfo.wProcessorLevel == 20)
        PDL_OS::strcpy (subtype, "620");
      break;
    case PROCESSOR_ARCHITECTURE_UNKNOWN:
    default:
      // @@ We could provide WinCE specific info here.  But let's
      //    defer that to some later point.
      PDL_OS::strcpy (processor, "Unknown");
      break;
    }
    PDL_OS::sprintf(name->machine, "%s %s", processor, subtype);
  }
  else if (vinfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
  {
    // Get Windows 95 Information
    PDL_OS::strcpy (name->release, "Windows 95");
    PDL_OS::sprintf (name->version, "%d", LOWORD (vinfo.dwBuildNumber));
    if (sinfo.dwProcessorType == PROCESSOR_INTEL_386)
      PDL_OS::strcpy (name->machine, "Intel 80386");
    else if (sinfo.dwProcessorType == PROCESSOR_INTEL_486)
      PDL_OS::strcpy (name->machine, "Intel 80486");
    else if (sinfo.dwProcessorType == PROCESSOR_INTEL_PENTIUM)
      PDL_OS::strcpy (name->machine, "Intel Pentium");
  }
  else
  {
    // We don't know what this is!

    PDL_OS::strcpy (name->release, "???");
    PDL_OS::strcpy (name->version, "???");
    PDL_OS::strcpy (name->machine, "???");
  }

  return PDL_OS::hostname (name->nodename, maxnamelen);
# elif defined (VXWORKS)
  size_t maxnamelen = sizeof name->nodename;
  PDL_OS::strcpy (name->sysname, "VxWorks");
  PDL_OS::strcpy (name->release, "???");
  PDL_OS::strcpy (name->version, sysBspRev ());
  PDL_OS::strcpy (name->machine, sysModel ());

  return PDL_OS::hostname (name->nodename, maxnamelen);
# elif defined (CHORUS)
  size_t maxnamelen = sizeof name->nodename;
  PDL_OS::strcpy (name->sysname, "CHORUS/ClassiX");
  PDL_OS::strcpy (name->release, "???");
  PDL_OS::strcpy (name->version, "???");
  PDL_OS::strcpy (name->machine, "???");

  return PDL_OS::hostname (name->nodename, maxnamelen);
#elif defined (PDL_PSOS)
  PDL_UNUSED_ARG (name);
  PDL_NOTSUP_RETURN (-1);
#endif /* PDL_WIN32 */
}
#endif /* PDL_WIN32 || VXWORKS */


#if defined (VXWORKS)
struct hostent *
PDL_OS::gethostbyname (const char *name)
{
  PDL_TRACE ("PDL_OS::gethostbyname");

  // not thread safe!
  static hostent ret;
  static int first_addr;
  static char *hostaddr[2];
  static char *aliases[1];

  PDL_OSCALL (::hostGetByName ((char *) name), int, -1, first_addr);
  if (first_addr == -1)
    return 0;

  hostaddr[0] = (char *) &first_addr;
  hostaddr[1] = 0;
  aliases[0] = 0;

  // Might not be official: just echo input arg.
  ret.h_name = (char *) name;
  ret.h_addrtype = AF_INET;
  ret.h_length = 4;  // VxWorks 5.2/3 doesn't define IP_ADDR_LEN;
  ret.h_addr_list = hostaddr;
  ret.h_aliases = aliases;

  return &ret;
}

struct hostent *
PDL_OS::gethostbyaddr (const char *addr, int length, int type)
{
  PDL_TRACE ("PDL_OS::gethostbyaddr");

  if (length != 4 || type != AF_INET)
    {
      errno = EINVAL;
      return 0;
    }

  // not thread safe!
  static hostent ret;
  static char name [MAXNAMELEN + 1];
  static char *hostaddr[2];
  static char *aliases[1];

  if (::hostGetByAddr (*(int *) addr, name) != 0)
    {
      // errno will have been set to S_hostLib_UNKNOWN_HOST.
      return 0;
    }

  // Might not be official: just echo input arg.
  hostaddr[0] = (char *) addr;
  hostaddr[1] = 0;
  aliases[0] = 0;

  ret.h_name = name;
  ret.h_addrtype = AF_INET;
  ret.h_length = 4;  // VxWorks 5.2/3 doesn't define IP_ADDR_LEN;
  ret.h_addr_list = hostaddr;
  ret.h_aliases = aliases;

  return &ret;
}

struct hostent *
PDL_OS::gethostbyaddr_r (const char *addr, int length, int type,
                         hostent *result, PDL_HOSTENT_DATA buffer,
                         int *h_errnop)
{
  PDL_TRACE ("PDL_OS::gethostbyaddr_r");
  if (length != 4 || type != AF_INET)
    {
      errno = EINVAL;
      return 0;
    }

  if (PDL_OS::netdb_acquire ())
    return 0;
  else
    {
      // buffer layout:
      // buffer[0-3]: h_addr_list[0], the first (and only) addr.
      // buffer[4-7]: h_addr_list[1], the null terminator for the h_addr_list.
      // buffer[8]: the name of the host, null terminated.

      // Call ::hostGetByAddr (), which puts the (one) hostname into
      // buffer.
      if (::hostGetByAddr (*(int *) addr, &buffer[8]) == 0)
        {
          // Store the return values in result.
          result->h_name = &buffer[8];  // null-terminated host name
          result->h_addrtype = AF_INET;
          result->h_length = 4;  // VxWorks 5.2/3 doesn't define IP_ADDR_LEN.

          result->h_addr_list = (char **) buffer;
          // Might not be official: just echo input arg.
          result->h_addr_list[0] = (char *) addr;
          // Null-terminate the list of addresses.
          result->h_addr_list[1] = 0;
          // And no aliases, so null-terminate h_aliases.
          result->h_aliases = &result->h_addr_list[1];
        }
      else
        {
          // errno will have been set to S_hostLib_UNKNOWN_HOST.
          result = 0;
        }
    }

  PDL_OS::netdb_release ();
  *h_errnop = errno;
  return result;
}

struct hostent *
PDL_OS::gethostbyname_r (const char *name, hostent *result,
                         PDL_HOSTENT_DATA buffer,
                         int *h_errnop)
{
  PDL_TRACE ("PDL_OS::gethostbyname_r");

  if (PDL_OS::netdb_acquire ())
    return 0;
  else
    {
      int addr;
      PDL_OSCALL (::hostGetByName ((char *) name), int, -1, addr);

      if (addr == -1)
        {
          // errno will have been set to S_hostLib_UNKNOWN_HOST
          result = 0;
        }
      else
        {
          // Might not be official: just echo input arg.
          result->h_name = (char *) name;
          result->h_addrtype = AF_INET;
          result->h_length = 4;  // VxWorks 5.2/3 doesn't define IP_ADDR_LEN;

          // buffer layout:
          // buffer[0-3]: h_addr_list[0], pointer to the addr.
          // buffer[4-7]: h_addr_list[1], null terminator for the h_addr_list.
          // buffer[8-11]: the first (and only) addr.

          // Store the address list in buffer.
          result->h_addr_list = (char **) buffer;
          // Store the actual address _after_ the address list.
          result->h_addr_list[0] = (char *) &result->h_addr_list[2];
          result->h_addr_list[2] = (char *) addr;
          // Null-terminate the list of addresses.
          result->h_addr_list[1] = 0;
          // And no aliases, so null-terminate h_aliases.
          result->h_aliases = &result->h_addr_list[1];
        }
    }

  PDL_OS::netdb_release ();
  *h_errnop = errno;
  return result;
}
#endif /* VXWORKS */

void
PDL_OS::pdl_flock_t::dump (void) const
{
PDL_TRACE ("PDL_OS::pdl_flock_t::dump");

  PDL_DEBUG ((LM_DEBUG, PDL_BEGIN_DUMP, this));
  PDL_DEBUG ((LM_DEBUG, ASYS_TEXT ("handle_ = %u"), this->handle_));
#if defined (PDL_WIN32)
  PDL_DEBUG ((LM_DEBUG, ASYS_TEXT ("\nInternal = %d"), this->overlapped_.Internal));
  PDL_DEBUG ((LM_DEBUG, ASYS_TEXT ("\nInternalHigh = %d"), this->overlapped_.InternalHigh));
  PDL_DEBUG ((LM_DEBUG, ASYS_TEXT ("\nOffsetHigh = %d"), this->overlapped_.OffsetHigh));
  PDL_DEBUG ((LM_DEBUG, ASYS_TEXT ("\nhEvent = %d"), this->overlapped_.hEvent));
#elif !defined (CHORUS)
  PDL_DEBUG ((LM_DEBUG, ASYS_TEXT ("\nl_whence = %d"), this->lock_.l_whence));
  PDL_DEBUG ((LM_DEBUG, ASYS_TEXT ("\nl_start = %d"), this->lock_.l_start));
  PDL_DEBUG ((LM_DEBUG, ASYS_TEXT ("\nl_len = %d"), this->lock_.l_len));
  PDL_DEBUG ((LM_DEBUG, ASYS_TEXT ("\nl_type = %d"), this->lock_.l_type));
#endif /* PDL_WIN32 */
  PDL_DEBUG ((LM_DEBUG, PDL_END_DUMP));
}

#if defined (PDL_HAS_UNICODE)
void PDL_OS::checkUnicodeFormat (FILE* fp)
{
    if (fp != 0)
    {
        // Due to the ASYS_TCHAR definition, all default input files, such as
        // svc.conf, have to be in Unicode format (small endian) on WinCE
        // because PDL has all 'char' converted into ASYS_TCHAR.
        // However, for TAO, ASCII files, such as IOR file, can still be read and
        // be written without any error since given buffers are all in 'char' type
        // instead of ASYS_TCHAR.  Therefore, it is user's reponsibility to select
        // correct buffer type.

        // At this point, check if the file is Unicode or not.
        WORD first_two_bytes;
        int numRead = PDL_OS::fread(&first_two_bytes, sizeof(WORD), 1, fp);

        if (numRead == 1)
        {
            if ((first_two_bytes != 0xFFFE) &&  // not a small endian Unicode file
                (first_two_bytes != 0xFEFF))    // not a big endian Unicode file
            {
                PDL_OS::fseek(fp, 0, FILE_BEGIN);  // set file pointer back to the beginning
            }
        }
        // if it is a Unicode file, file pointer will be right next to the first two-bytes
    }
}
#endif  /* PDL_HAS_UNICODE */

#if defined (PDL_WIN32)
FILE *
PDL_OS::fopen (const char *filename,
               const char *mode)
{
  PDL_TRACE ("PDL_OS::fopen");
#if defined (PDL_HAS_WINCE)

  PDL_OSCALL_RETURN(::fopen(filename, mode), FILE*, NULL);

#else

  int hmode = _O_TEXT;

  for (const char *mode_ptr = mode; *mode_ptr != 0; mode_ptr++)
    PDL_OS::fopen_mode_to_open_mode_converter (*mode_ptr, hmode);

  PDL_HANDLE handle = PDL_OS::open (filename, hmode);
  if (handle != PDL_INVALID_HANDLE)
    {
      hmode &= _O_TEXT | _O_RDONLY | _O_APPEND;
#   if defined (PDL_WIN64)
      int fd = _open_osfhandle ((intptr_t) handle, hmode);
#   else
      int fd = _open_osfhandle ((long) handle, hmode);
#   endif /* PDL_WIN64 */
      if (fd != -1)
        {
#   if defined(__BORLANDC__)
          FILE *fp = _fdopen (fd, PDL_const_cast (char *, mode));
#   else /* defined(__BORLANDC__) */
          FILE *fp = _fdopen (fd, mode);
#   endif /* defined(__BORLANDC__) */
          if (fp != NULL)
            return fp;
          _close (fd);
        }
      PDL_OS::close (handle);
    }
  return NULL;
#endif  /* PDL_HAS_WINCE */
}
#endif /* PDL_WIN32 && !PDL_HAS_WINCE */

#if defined (PDL_WIN32) && defined (PDL_HAS_UNICODE)

FILE *
PDL_OS::fopen (const wchar_t *filename, const wchar_t *mode)
{
  PDL_TRACE ("PDL_OS::fopen");
  int hmode = _O_TEXT;

  for (const wchar_t *mode_ptr = mode; *mode_ptr != 0; mode_ptr++)
    PDL_OS::fopen_mode_to_open_mode_converter ((char) *mode_ptr, hmode);

  PDL_HANDLE handle = PDL_OS::open (filename, hmode);
  if (handle != PDL_INVALID_HANDLE)
    {
# if defined (PDL_HAS_WINCE)
      FILE *fp = ::_wfdopen (handle, mode);
      if (fp != 0)
      {
        checkUnicodeFormat(fp);
        return fp;
      }
# else
      hmode &= _O_TEXT | _O_RDONLY | _O_APPEND;
#   if defined (PDL_WIN64)
      int fd = _open_osfhandle (intptr_t (handle), hmode);
#   else
      int fd = _open_osfhandle (long (handle), hmode);
#   endif /* PDL_WIN64 */
      if (fd != -1)
        {
#   if defined(__BORLANDC__)
          FILE *fp = _wfdopen (fd, PDL_const_cast (wchar_t *, mode));
#   else /* defined(__BORLANDC__) */
          FILE *fp = _wfdopen (fd, mode);
#   endif /* defined(__BORLANDC__) */
          if (fp != NULL)
            return fp;
          _close (fd);
        }
#   endif /* PDL_HAS_WINCE */
      PDL_OS::close (handle);
    }
  return NULL;
}
# endif /* PDL_WIN32 && PDL_HAS_UNICODE */

// The following *printf functions aren't inline because
// they use varargs.

int
PDL_OS::fprintf (FILE *fp, const char *format, ...)
{
  PDL_TRACE ("PDL_OS::fprintf");

  int result = 0;

#if defined(DEBUG)
  if (bDaemonPritable)
  {
      assert(fp != stdout && fp != stderr);
  }
#endif

  va_list ap;
  va_start (ap, format);
  PDL_OSCALL (::vfprintf (fp, format, ap), int, -1, result);
  va_end (ap);
  return result;
}

int
PDL_OS::printf (const char *format, ...)
{
  PDL_TRACE ("PDL_OS::printf");

  int result;

#if defined(DEBUG)
  assert(!bDaemonPritable);
#endif

  va_list ap;
  va_start (ap, format);
  PDL_OSCALL (::vprintf (format, ap), int, -1, result);
  va_end (ap);
  return result;
}

int
PDL_OS::sprintf (char *buf, const char *format, ...)
{
  // PDL_TRACE ("PDL_OS::sprintf");

  int result;
  va_list ap;
  va_start (ap, format);
  PDL_OSCALL (PDL_SPRINTF_ADAPTER (::vsprintf (buf, format, ap)), int, -1, result);
  va_end (ap);
  return result;
}

char *
PDL_OS::gets (char *str, int n)
{
  PDL_TRACE ("PDL_OS::gets");
  int c;
  char *s = str;

  if (str == 0 || n < 0) n = 0;
  if (n == 0) str = 0;
  else n--;

  while ((c = getchar ()) != '\n')
    {

#   if defined (PDL_HAS_SIGNAL_SAFE_OS_CALLS)
      if (c == EOF && errno == EINTR && PDL_LOG_MSG->restart ())
        continue;
#   endif /* PDL_HAS_SIGNAL_SAFE_OS_CALLS */

      if (c == EOF)
        break;

      if (n > 0)
        n--, *s++ = c;
    }
  if (s) *s = '\0';

  return (c == EOF) ? 0 : str;
}

#if defined (PDL_HAS_UNICODE)
# if defined (PDL_WIN32)

int
PDL_OS::fprintf (FILE *fp, const wchar_t *format, ...)
{
  PDL_TRACE ("PDL_OS::fprintf");
#   if defined (PDL_HAS_WINCE)
  PDL_NOTSUP_RETURN (-1);
#   else
  int result = 0;
  va_list ap;
  va_start (ap, format);
  PDL_OSCALL (::vfwprintf (fp, format, ap), int, -1, result);
  va_end (ap);
  return result;
#   endif /* PDL_HAS_WINCE */
}

int
PDL_OS::sprintf (wchar_t *buf, const wchar_t *format, ...)
{
  PDL_TRACE ("PDL_OS::sprintf");
  int result;
  va_list ap;
  va_start (ap, format);
  PDL_OSCALL (::vswprintf (buf, format, ap), int, -1, result);
  va_end (ap);
  return result;
}


# endif /* PDL_WIN32 */

# if defined (PDL_LACKS_MKTEMP)
wchar_t *
PDL_OS::mktemp (wchar_t *s)
{
#ifdef PDL_HAS_WINCE
	assert(0);
    return NULL;
#else
  PDL_TRACE ("PDL_OS::mktemp");
  if (s == 0)
    // check for null template string failed!
    return 0;
  else
    {
      wchar_t *xxxxxx = PDL_OS::strstr ("XXXXXX");

      if (xxxxxx == 0)
        // the template string doesn't contain "XXXXXX"!
        return s;
      else
        {
          wchar_t unique_letter = L'a';
          struct stat sb;

          // Find an unused filename for this process.  It is assumed
          // that the user will open the file immediately after
          // getting this filename back (so, yes, there is a rpdl
          // condition if multiple threads in a process use the same
          // template).  This appears to match the behavior of the
          // SunOS 5.5 mktemp().
          PDL_OS::sprintf (xxxxxx, "%05d%c",
                           PDL_OS::getpid (), unique_letter);
          while (PDL_OS::stat (s, &sb) >= 0)
            {
              if (++unique_letter <= L'z')
                PDL_OS::sprintf (xxxxxx, "%05d%c",
                                 PDL_OS::getpid (), unique_letter);
              else
                {
                  // maximum of 26 unique files per template, per process
                  PDL_OS::sprintf (xxxxxx, "%s", L"");
                  return s;
                }
            }
        }
      return s;
    }
#endif /* PDL_HAS_WINCE */
}
# endif /* PDL_LACKS_MKTEMP */
#endif /* PDL_HAS_UNICODE */

int
PDL_OS::execl (const char * /* path */, const char * /* arg0 */, ...)
{
  PDL_TRACE ("PDL_OS::execl");
#if defined (PDL_WIN32) || defined (VXWORKS)
  PDL_NOTSUP_RETURN (-1);
#else
  PDL_NOTSUP_RETURN (-1);
  // Need to write this code.
  // PDL_OSCALL_RETURN (::execv (path, argv), int, -1);
#endif /* PDL_WIN32 */
}

int
PDL_OS::execle (const char * /* path */, const char * /* arg0 */, ...)
{
  PDL_TRACE ("PDL_OS::execle");
#if defined (PDL_WIN32) || defined (VXWORKS)
  PDL_NOTSUP_RETURN (-1);
#else
  PDL_NOTSUP_RETURN (-1);
  // Need to write this code.
  //  PDL_OSCALL_RETURN (::execve (path, argv, envp), int, -1);
#endif /* PDL_WIN32 */
}

int
PDL_OS::execlp (const char * /* file */, const char * /* arg0 */, ...)
{
  PDL_TRACE ("PDL_OS::execlp");
#if defined (PDL_WIN32) || defined (VXWORKS)
  PDL_NOTSUP_RETURN (-1);
#else
  PDL_NOTSUP_RETURN (-1);
  // Need to write this code.
  //  PDL_OSCALL_RETURN (::execvp (file, argv), int, -1);
#endif /* PDL_WIN32 */
}

int
PDL_OS::scheduling_class (const char *class_name, PDL_id_t &id)
{
#if defined (PDL_HAS_PRIOCNTL)
  // Get the priority class ID.
  pcinfo_t pcinfo;
  // The following is just to avoid Purify warnings about unitialized
  // memory reads.
  PDL_OS::memset (&pcinfo, 0, sizeof pcinfo);

  PDL_OS::strcpy (pcinfo.pc_clname, class_name);
  if (PDL_OS::priority_control (P_ALL /* ignored */,
                                P_MYID /* ignored */,
                                PC_GETCID,
                                (char *) &pcinfo) == -1)
    {
      return -1;
    }
  else
    {
      id = pcinfo.pc_cid;
      return 0;
    }
#else  /* ! PDL_HAS_PRIOCNTL */
  PDL_UNUSED_ARG (class_name);
  PDL_UNUSED_ARG (id);
  PDL_NOTSUP_RETURN (-1);
#endif /* ! PDL_HAS_PRIOCNTL */
}


int
PDL_OS::thr_setprio (const PDL_Sched_Priority prio)
{
  // Set the thread priority on the current thread.
  PDL_hthread_t my_thread_id;
  PDL_OS::thr_self (my_thread_id);

  int status = PDL_OS::thr_setprio (my_thread_id, prio);


  return status;
}

// = Static initialization.

// This is necessary to deal with POSIX pthreads insanity.  This
// guarantees that we've got a "zero'd" thread id even when
// PDL_thread_t, PDL_hthread_t, and PDL_thread_key_t are implemented
// as structures...  Under no circumstances should these be given
// initial values.
// Note: these three objects require static construction.
PDL_thread_t PDL_OS::NULL_thread;
PDL_hthread_t PDL_OS::NULL_hthread;
#if defined (PDL_HAS_TSS_EMULATION) || (defined (PDL_PSOS) && defined (PDL_PSOS_HAS_TSS))
  PDL_thread_key_t PDL_OS::NULL_key = PDL_static_cast (PDL_thread_key_t, -1);
#else  /* ! PDL_HAS_TSS_EMULATION */
  PDL_thread_key_t PDL_OS::NULL_key;
#endif /* ! PDL_HAS_TSS_EMULATION */

#if defined (CHORUS)
KnCap PDL_OS::actorcaps_[PDL_CHORUS_MAX_ACTORS];
// This is used to map an actor's id into a KnCap for killing and
// waiting actors.
#endif /* CHORUS */

#if defined (PDL_WIN32)

// = Static initialization.

// Keeps track of whether we've initialized the WinSock DLL.
int PDL_OS::socket_initialized_;

#endif /* WIN32 */

#if defined (PDL_WIN32) || defined (PDL_HAS_TSS_EMULATION) || (defined (PDL_PSOS) && defined (PDL_PSOS_HAS_TSS))

// Moved class PDL_TSS_Ref declaration to OS.h so it can be visible to
// the single file of template instantiations.

PDL_TSS_Ref::PDL_TSS_Ref (PDL_thread_t id)
  : tid_(id)
{
PDL_TRACE ("PDL_TSS_Ref::PDL_TSS_Ref");
}

PDL_TSS_Ref::PDL_TSS_Ref (void)
{
PDL_TRACE ("PDL_TSS_Ref::PDL_TSS_Ref");
}

// Check for equality.
int
PDL_TSS_Ref::operator== (const PDL_TSS_Ref &info) const
{
PDL_TRACE ("PDL_TSS_Ref::operator==");

  return this->tid_ == info.tid_;
}

// Check for inequality.
inline
int
PDL_TSS_Ref::operator!= (const PDL_TSS_Ref &tss_ref) const
{
PDL_TRACE ("PDL_TSS_Ref::operator==");

  return !(*this == tss_ref);
}

// moved class PDL_TSS_Info declaration
// to OS.h so it can be visible to the
// single file of template instantiations

PDL_TSS_Info::PDL_TSS_Info (PDL_thread_key_t key,
                            void (*dest)(void *),
                            void *tss_inst)
  : key_ (key),
    destructor_ (dest),
    tss_obj_ (tss_inst),
    thread_count_ (-1)
{
PDL_TRACE ("PDL_TSS_Info::PDL_TSS_Info");
}

PDL_TSS_Info::PDL_TSS_Info (void)
  : key_ (PDL_OS::NULL_key),
    destructor_ (0),
    tss_obj_ (0),
    thread_count_ (-1)
{
PDL_TRACE ("PDL_TSS_Info::PDL_TSS_Info");
}

# if defined (PDL_HAS_NONSCALAR_THREAD_KEY_T)
  static inline int operator== (const PDL_thread_key_t &lhs,
                                const PDL_thread_key_t &rhs)
  {
    return ! PDL_OS::memcmp (&lhs, &rhs, sizeof (PDL_thread_key_t));
  }

  static inline int operator!= (const PDL_thread_key_t &lhs,
                                const PDL_thread_key_t &rhs)
  {
    return ! (lhs == rhs);
  }
# endif /* PDL_HAS_NONSCALAR_THREAD_KEY_T */

// Check for equality.
int
PDL_TSS_Info::operator== (const PDL_TSS_Info &info) const
{
PDL_TRACE ("PDL_TSS_Info::operator==");

  return this->key_ == info.key_;
}

// Check for inequality.
int
PDL_TSS_Info::operator!= (const PDL_TSS_Info &info) const
{
PDL_TRACE ("PDL_TSS_Info::operator==");

  return !(*this == info);
}

void
PDL_TSS_Info::dump (void)
{
//  PDL_TRACE ("PDL_TSS_Info::dump");

  PDL_DEBUG ((LM_DEBUG, PDL_BEGIN_DUMP, this));
  PDL_DEBUG ((LM_DEBUG, ASYS_TEXT ("key_ = %u\n"), this->key_));
  PDL_DEBUG ((LM_DEBUG, ASYS_TEXT ("destructor_ = %u\n"), this->destructor_));
  PDL_DEBUG ((LM_DEBUG, ASYS_TEXT ("tss_obj_ = %u\n"), this->tss_obj_));
  PDL_DEBUG ((LM_DEBUG, PDL_END_DUMP));
}

// Moved class PDL_TSS_Keys declaration to OS.h so it can be visible
// to the single file of template instantiations.

PDL_TSS_Keys::PDL_TSS_Keys (void)
{
  for (u_int i = 0; i < PDL_WORDS; ++i)
    {
      key_bit_words_[i] = 0;
    }
  key_bit_words_initialized_ = 1;
}

inline
void
PDL_TSS_Keys::find (const u_int key, u_int &word, u_int &bit)
{
  word = key / PDL_BITS_PER_WORD;
  bit = key % PDL_BITS_PER_WORD;
}

int
PDL_TSS_Keys::test_and_set (const PDL_thread_key_t key)
{
  PDL_KEY_INDEX (key_index, key);
  u_int word, bit;
  find (key_index, word, bit);

  if (key_bit_words_initialized_ != 1)
    {
      for (u_int i = 0; i < PDL_WORDS; ++i)
        {
          key_bit_words_[i] = 0;
        }
    }

  if (PDL_BIT_ENABLED (key_bit_words_[word], 1 << bit))
    {
      return 1;
    }
  else
    {
      PDL_SET_BITS (key_bit_words_[word], 1 << bit);
      return 0;
    }
}

int
PDL_TSS_Keys::test_and_clear (const PDL_thread_key_t key)
{
  PDL_KEY_INDEX (key_index, key);

  u_int word, bit;
  find (key_index, word, bit);

  if (PDL_BIT_ENABLED (key_bit_words_[word], 1 << bit))
    {
      PDL_CLR_BITS (key_bit_words_[word], 1 << bit);
      return 0;
    }
  else
    {
      return 1;
    }
}

class PDL_TSS_Cleanup
  // = TITLE
  //     Singleton that knows how to clean up all the thread-specific
  //     resources for Win32.
  //
  // = DESCRIPTION
  //     All this nonsense is required since Win32 doesn't
  //     automatically cleanup thread-specific storage on thread exit,
  //     unlike real operating systems... ;-)
{
public:
  static PDL_TSS_Cleanup *instance (void);

  ~PDL_TSS_Cleanup (void);

  void exit (void *status);
  // Cleanup the thread-specific objects.  Does _NOT_ exit the thread.

  int insert (PDL_thread_key_t key, void (*destructor)(void *), void *inst);
  // Insert a <key, destructor> tuple into the table.

  int remove (PDL_thread_key_t key);
  // Remove a <key, destructor> tuple from the table.

  int detach (void *inst);
  // Detaches a tss_instance from its key.

  void key_used (PDL_thread_key_t key);
  // Mark a key as being used by this thread.

  int free_all_keys_left (void);
  // Free all keys left in the table before destruction.

  static int lockable () { return instance_ != 0; }
  // Indication of whether the PDL_TSS_CLEANUP_LOCK is usable, and
  // therefore whether we are in static constructor/destructor phase
  // or not.

protected:
  void dump (void);

  PDL_TSS_Cleanup (void);
  // Ensure singleton.

private:
  // Array of <PDL_TSS_Info> objects.
  typedef PDL_TSS_Info PDL_TSS_TABLE[PDL_DEFAULT_THREAD_KEYS];
  typedef PDL_TSS_Info *PDL_TSS_TABLE_ITERATOR;

  PDL_TSS_TABLE table_;
  // Table of <PDL_TSS_Info>'s.

  PDL_thread_key_t in_use_;
  // Key for the thread-specific array of whether each TSS key is in use.

  PDL_TSS_Keys *tss_keys ();
  // Accessor for this threads PDL_TSS_Keys instance.

#if defined (PDL_HAS_TSS_EMULATION)
  PDL_thread_key_t in_use_key_;
  // Key that is used by in_use_.  We save this key so that we know
  // not to call its destructor in free_all_keys_left ().
#endif /* PDL_HAS_TSS_EMULATION */

  // = Static data.
  static PDL_TSS_Cleanup *instance_;
  // Pointer to the singleton instance.
};

// = Static object initialization.

// Pointer to the singleton instance.
PDL_TSS_Cleanup *PDL_TSS_Cleanup::instance_ = 0;

PDL_TSS_Cleanup::~PDL_TSS_Cleanup (void)
{
  // Zero out the instance pointer to support lockable () accessor.
  PDL_TSS_Cleanup::instance_ = 0;
}

void
PDL_TSS_Cleanup::exit (void * /* status */)
{
  PDL_TRACE ("PDL_TSS_Cleanup::exit");

  PDL_TSS_TABLE_ITERATOR key_info = table_;
  PDL_TSS_Info info_arr[PDL_DEFAULT_THREAD_KEYS];
  int info_ix = 0;

  // While holding the lock, we only collect the PDL_TSS_Info objects
  // in an array without invoking the according destructors.
  {
    PDL_TSS_CLEANUP_GUARD

    // Iterate through all the thread-specific items and free them all
    // up.

    for (unsigned int i = 0;
         i < PDL_DEFAULT_THREAD_KEYS;
         ++key_info, ++i)
      {
        if (key_info->key_ == PDL_OS::NULL_key  ||
            ! key_info->key_in_use ()) continue;

        // If the key's PDL_TSS_Info in-use bit for this thread was set,
        // unset it and decrement the key's thread_count_.
        if (! tss_keys ()->test_and_clear (key_info->key_))
          {
            --key_info->thread_count_;
          }

        void *tss_info = 0;

        if (key_info->destructor_
            && PDL_OS::thr_getspecific (key_info->key_, &tss_info) == 0
            && tss_info)
          {
            info_arr[info_ix].key_ = key_info->key_;
            info_arr[info_ix].destructor_ = key_info->destructor_;
            info_arr[info_ix++].tss_obj_ = key_info->tss_obj_;
          }
      }
  }

  // Now we have given up the PDL_TSS_Cleanup::lock_ and we start
  // invoking destructors, in the reverse order of creation.
  for (int i = info_ix - 1; i >= 0; --i)
    {
      void *tss_info = 0;

      PDL_OS::thr_getspecific (info_arr[i].key_, &tss_info);

      if (tss_info != 0)
        {
          // Only call the destructor if the value is non-zero for this
          // thread.
          (*info_arr[i].destructor_)(tss_info);
        }
    }

  // Acquire the PDL_TSS_CLEANUP_LOCK, then free TLS keys and remove
  // entries from PDL_TSS_Info table.
  {
    PDL_TSS_CLEANUP_GUARD

  }
}

int
PDL_TSS_Cleanup::free_all_keys_left (void)
  // This is called from PDL_OS::cleanup_tss ().  When this gets
  // called, all threads should have exited except the main thread.
  // No key should be freed from this routine.  It there's any,
  // something might be wrong.
{
  PDL_thread_key_t key_arr[PDL_DEFAULT_THREAD_KEYS];
  PDL_TSS_TABLE_ITERATOR key_info = table_;
  unsigned int idx = 0;
  unsigned int i;

  for (i = 0;
       i < PDL_DEFAULT_THREAD_KEYS;
       ++key_info, ++i)
#if defined (PDL_HAS_TSS_EMULATION)
    if (key_info->key_ != in_use_key_)
#endif /* PDL_HAS_TSS_EMULATION */
      // Don't call PDL_OS::thr_keyfree () on PDL_TSS_Cleanup's own
      // key.  See the comments in PDL_OS::thr_key_detach ():  the key
      // doesn't get detached, so it will be in the table here.
      // However, there's no resource associated with it, so we don't
      // need to keyfree it.  The dynamic memory associated with it
      // was already deleted by PDL_TSS_Cleanup::exit (), so we don't
      // want to access it again.
      key_arr [idx++] = key_info->key_;

  for (i = 0; i < idx; i++)
    if (key_arr[i] != PDL_OS::NULL_key)
#if defined (PDL_HAS_TSS_EMULATION)
      PDL_OS::thr_keyfree (key_arr[i]);
#elif defined (PDL_PSOS) && defined (PDL_PSOS_HAS_TSS)
      // Don't call PDL_OS::thr_keyfree here.  It will try to use
      // <in_use_> which has already been cleaned up here.
      ::tsd_delete (key_arr[i]);
#else /* PDL_WIN32 */
      // Don't call PDL_OS::thr_keyfree here.  It will try to use
      // <in_use_> which has already been cleaned up here.
      TlsFree (key_arr[i]);
#endif /* PDL_HAS_TSS_EMULATION */

  return 0;
}

extern "C" void
PDL_TSS_Cleanup_keys_destroyer (void *tss_keys)
{
  PDL_OS::free(PDL_reinterpret_cast (PDL_TSS_Keys *, tss_keys));
}

PDL_TSS_Cleanup::PDL_TSS_Cleanup (void)
  : in_use_ (PDL_OS::NULL_key)
#if defined (PDL_HAS_TSS_EMULATION)
    // PDL_TSS_Emulation::total_keys () provides the value of the next
    // key to be created.
  , in_use_key_ (PDL_TSS_Emulation::total_keys ())
#endif /* PDL_HAS_TSS_EMULATION */
{
  PDL_TRACE ("PDL_TSS_Cleanup::PDL_TSS_Cleanup");
}

PDL_TSS_Cleanup *
PDL_TSS_Cleanup::instance (void)
{
  PDL_TRACE ("PDL_TSS_Cleanup::instance");

  // Create and initialize thread-specific key.
  if (PDL_TSS_Cleanup::instance_ == 0)
    {
        static PDL_TSS_Cleanup gTSS;
      // Insure that we are serialized!
      PDL_TSS_CLEANUP_GUARD

      // Now, use the Double-Checked Locking pattern to make sure we
      // only create the PDL_TSS_Cleanup instance once.
      if (PDL_TSS_Cleanup::instance_ == 0)
      {
          PDL_TSS_Cleanup::instance_ = &gTSS;
      }
    }

  return PDL_TSS_Cleanup::instance_;
}

int
PDL_TSS_Cleanup::insert (PDL_thread_key_t key,
                         void (*destructor)(void *),
                         void *inst)
{
PDL_TRACE ("PDL_TSS_Cleanup::insert");
  PDL_TSS_CLEANUP_GUARD

  PDL_KEY_INDEX (key_index, key);
  if (key_index < PDL_DEFAULT_THREAD_KEYS)
    {
      table_[key_index] = PDL_TSS_Info (key, destructor, inst);
      return 0;
    }
  else
    {
      return -1;
    }
}

int
PDL_TSS_Cleanup::remove (PDL_thread_key_t key)
{
  PDL_TRACE ("PDL_TSS_Cleanup::remove");
  PDL_TSS_CLEANUP_GUARD

  PDL_KEY_INDEX (key_index, key);
  if (key_index < PDL_DEFAULT_THREAD_KEYS)
    {
      // "Remove" the TSS_Info table entry by zeroing out its key_ and
      // destructor_ fields.  Also, keep track of the number threads
      // using the key.
      PDL_TSS_Info &info = this->table_ [key_index];

      // Don't bother to check <in_use_> if the program is shutting
      // down.  Doing so will cause a new PDL_TSS object getting
      // created again.
      if (!PDL_OS_Object_Manager::shutting_down ()
          && ! tss_keys ()->test_and_clear (info.key_))
        --info.thread_count_;

      info.key_ = PDL_OS::NULL_key;
      info.destructor_ = 0;
      return 0;
    }
  else
    return -1;
}

int
PDL_TSS_Cleanup::detach (void *inst)
{
  PDL_TSS_CLEANUP_GUARD

  PDL_TSS_TABLE_ITERATOR key_info = table_;
  int success = 0;
  int ref_cnt = 0;

  // Mark the key as detached in the TSS_Info table.
  // It only works for the first key that "inst" owns.
  // I don't know why.
  for (unsigned int i = 0;
       i < PDL_DEFAULT_THREAD_KEYS;
       ++key_info, ++i)
    {
      if (key_info->tss_obj_ == inst)
        {
          key_info->tss_obj_ = 0;
          ref_cnt = key_info->thread_count_;
          success = 1;
          break;
        }
    }

  if (success == 0)
    return -1;
  else if (ref_cnt == 0)
    {
      // Mark the key as no longer being used.
      key_info->key_in_use (0);
# if defined (PDL_WIN32) || (defined (PDL_PSOS) && defined (PDL_PSOS_HAS_TSS))
      PDL_thread_key_t temp_key = key_info->key_;
# endif /* PDL_WIN32 */
      int retv = this->remove (key_info->key_);

# if defined (PDL_WIN32)
      ::TlsFree (temp_key);
# elif defined (PDL_PSOS) && defined (PDL_PSOS_HAS_TSS)
      ::tsd_delete (temp_key);
# endif /* PDL_WIN32 */
      return retv;
    }

  return 0;
}

void
PDL_TSS_Cleanup::key_used (PDL_thread_key_t key)
{
  // If the key's PDL_TSS_Info in-use bit for this thread is not set,
  // set it and increment the key's thread_count_.
  if (! tss_keys ()->test_and_set (key))
    {
      PDL_TSS_CLEANUP_GUARD

      // Retrieve the key's PDL_TSS_Info and increment its thread_count_.
      PDL_KEY_INDEX (key_index, key);
      PDL_TSS_Info &key_info = this->table_ [key_index];
      if (key_info.thread_count_ == -1)
        key_info.key_in_use (1);
      else
        ++key_info.thread_count_;
    }
}

void
PDL_TSS_Cleanup::dump (void)
{
  // Iterate through all the thread-specific items and dump them all.

  PDL_TSS_TABLE_ITERATOR key_info = table_;
  for (unsigned int i = 0;
       i < PDL_DEFAULT_THREAD_KEYS;
       ++key_info, ++i)
    key_info->dump ();
}

PDL_TSS_Keys *
PDL_TSS_Cleanup::tss_keys ()
{
  if (in_use_ == PDL_OS::NULL_key)
    {
      PDL_TSS_CLEANUP_GUARD
      // Double-check;
      if (in_use_ == PDL_OS::NULL_key)
        {
          // Initialize in_use_ with a new key.
          if (PDL_OS::thr_keycreate (&in_use_,
                                     &PDL_TSS_Cleanup_keys_destroyer))
            return 0; // Major problems, this should *never* happen!
        }
    }

  PDL_TSS_Keys *ts_keys = 0;
  if (PDL_OS::thr_getspecific (in_use_,
        PDL_reinterpret_cast (void **, &ts_keys)) == -1)
    return 0; // This should not happen!

  if (ts_keys == 0)
    {
      PDL_NEW_RETURN (ts_keys,
                      PDL_TSS_Keys,
                      sizeof(PDL_TSS_Keys),
                      0);
      // Store the dynamically allocated pointer in thread-specific
      // storage.
      if (PDL_OS::thr_setspecific (in_use_,
            PDL_reinterpret_cast (void *,
                                  ts_keys)) == -1)
        {
          PDL_OS::free( ts_keys);
          return 0; // Major problems, this should *never* happen!
        }
    }

  return ts_keys;
}

# if defined (PDL_HAS_TSS_EMULATION)
u_int PDL_TSS_Emulation::total_keys_ = 0;

PDL_TSS_Emulation::PDL_TSS_DESTRUCTOR
PDL_TSS_Emulation::tss_destructor_[PDL_TSS_Emulation::PDL_TSS_THREAD_KEYS_MAX]
 = { 0 };

#   if defined (PDL_HAS_THREAD_SPECIFIC_STORAGE)

int PDL_TSS_Emulation::key_created_ = 0;

PDL_OS_thread_key_t PDL_TSS_Emulation::native_tss_key_;

/* static */
#     if defined (PDL_HAS_THR_C_FUNC)
extern "C"
#     endif /* PDL_HAS_THR_C_FUNC */
void
PDL_TSS_Emulation_cleanup (void *ptr)
{
   PDL_UNUSED_ARG (ptr);
   // Really this must be used for PDL_TSS_Emulation code to make the TSS
   // cleanup
}

void **
PDL_TSS_Emulation::tss_base (void* ts_storage[], u_int *ts_created)
{
  // TSS Singleton implementation.

  // Create the one native TSS key, if necessary.
  if (key_created_ == 0)
    {
      // Double-checked lock . . .
      PDL_TSS_BASE_GUARD

      if (key_created_ == 0)
        {
          PDL_NO_HEAP_CHECK;
          if (PDL_OS::thr_keycreate (&native_tss_key_,
                                     &PDL_TSS_Emulation_cleanup) != 0)
            {
              return 0; // Major problems, this should *never* happen!
            }
          key_created_ = 1;
        }
    }

  void **old_ts_storage = 0;

  // Get the tss_storage from thread-OS specific storage.
  if (PDL_OS::thr_getspecific (native_tss_key_,
                               (void **) &old_ts_storage) == -1)
    return 0; // This should not happen!

  // Check to see if this is the first time in for this thread.
  // This block can also be entered after a fork () in the child process,
  // at least on Pthreads Draft 4 platforms.
  if (old_ts_storage == 0)
    {
      if (ts_created)
        *ts_created = 1u;

      // Use the ts_storage passed as argument, if non-zero.  It is
      // possible that this has been implemented in the stack. At the
      // moment, this is unknown.  The cleanup must not do nothing.
      // If ts_storage is zero, allocate (and eventually leak) the
      // storage array.
      if (ts_storage == 0)
        {
          PDL_NO_HEAP_CHECK;

          PDL_NEW_RETURN (ts_storage,
                          void,
                          PDL_TSS_THREAD_KEYS_MAX * sizeof(void *),
                          0);

          // Zero the entire TSS array.  Do it manually instead of
          // using memset, for optimum speed.  Though, memset may be
          // faster :-)
          void **tss_base_p = ts_storage;

          for (u_int i = 0;
               i < PDL_TSS_THREAD_KEYS_MAX;
               ++i)
            *tss_base_p++ = 0;
        }

       // Store the pointer in thread-specific storage.  It gets
       // deleted via the PDL_TSS_Emulation_cleanup function when the
       // thread terminates.
       if (PDL_OS::thr_setspecific (native_tss_key_,
                                    (void *) ts_storage) != 0)
          return 0; // Major problems, this should *never* happen!
    }
  else
    if (ts_created)
      ts_created = 0;

  return ts_storage  ?  ts_storage  :  old_ts_storage;
}
#   endif /* PDL_HAS_THREAD_SPECIFIC_STORAGE */

u_int
PDL_TSS_Emulation::total_keys ()
{
  PDL_OS_Recursive_Thread_Mutex_Guard (
    *PDL_static_cast (PDL_recursive_thread_mutex_t *,
                      PDL_OS_Object_Manager::preallocated_object[
                        PDL_OS_Object_Manager::PDL_TSS_KEY_LOCK]));

  return total_keys_;
}

int
PDL_TSS_Emulation::next_key (PDL_thread_key_t &key)
{
  PDL_OS_Recursive_Thread_Mutex_Guard (
    *PDL_static_cast (PDL_recursive_thread_mutex_t *,
                      PDL_OS_Object_Manager::preallocated_object[
                        PDL_OS_Object_Manager::PDL_TSS_KEY_LOCK]));

  if (total_keys_ < PDL_TSS_THREAD_KEYS_MAX)
    {
#   if defined (PDL_HAS_NONSCALAR_THREAD_KEY_T)
      PDL_OS::memset (&key, 0, sizeof (PDL_thread_key_t));
      PDL_OS::memcpy (&key, &total_keys_, sizeof (u_int));
#   else
      key = total_keys_;
#   endif /* PDL_HAS_NONSCALAR_THREAD_KEY_T */

      ++total_keys_;
      return 0;
    }
  else
    {
      key = PDL_OS::NULL_key;
      return -1;
    }
}

void *
PDL_TSS_Emulation::tss_open (void *ts_storage[PDL_TSS_THREAD_KEYS_MAX])
{
#   if defined (PDL_PSOS)
  u_long tss_base;

  // Use the supplied array for this thread's TSS.
  tss_base = (u_long) ts_storage;
  t_setreg (0, PSOS_TASK_REG_TSS, tss_base);

  // Zero the entire TSS array.
  void **tss_base_p = ts_storage;
  for (u_int i = 0; i < PDL_TSS_THREAD_KEYS_MAX; ++i, ++tss_base_p)
    {
      *tss_base_p = 0;
    }

  return (void *) tss_base;
#   else  /* ! PDL_PSOS */
#     if defined (PDL_HAS_THREAD_SPECIFIC_STORAGE)
        // On VxWorks, in particular, don't check to see if the field
        // is 0.  It isn't always, specifically, when a program is run
        // directly by the shell (without spawning a new task) after
        // another program has been run.

  u_int ts_created = 0;
  tss_base (ts_storage, &ts_created);
  if (ts_created)
    {
#     else  /* ! PDL_HAS_THREAD_SPECIFIC_STORAGE */
      tss_base () = ts_storage;
#     endif

      // Zero the entire TSS array.  Do it manually instead of using
      // memset, for optimum speed.  Though, memset may be faster :-)
      void **tss_base_p = tss_base ();
      for (u_int i = 0; i < PDL_TSS_THREAD_KEYS_MAX; ++i, ++tss_base_p)
        {
          *tss_base_p = 0;
        }

      return tss_base ();
#     if defined (PDL_HAS_THREAD_SPECIFIC_STORAGE)
    }
  else
    {
      return 0;
    }
#     endif /* PDL_HAS_THREAD_SPECIFIC_STORAGE */
#   endif /* ! PDL_PSOS */
}

void
PDL_TSS_Emulation::tss_close ()
{
#if defined (PDL_HAS_THREAD_SPECIFIC_STORAGE)
  // Free native_tss_key_ here.
#endif /* PDL_HAS_THREAD_SPECIFIC_STORAGE */
}

# endif /* PDL_HAS_TSS_EMULATION */

#endif /* WIN32 || PDL_HAS_TSS_EMULATION */

void
PDL_OS::cleanup_tss (const u_int main_thread)
{
#if defined (PDL_HAS_TSS_EMULATION) || defined (PDL_WIN32) || (defined (PDL_PSOS) && defined (PDL_PSOS_HAS_TSS))
  // Call TSS destructors for current thread.
  PDL_TSS_Cleanup::instance ()->exit (0);
#endif /* PDL_HAS_TSS_EMULATION || PDL_WIN32 || PDL_PSOS_HAS_TSS */

  if (main_thread)
    {
#if !defined (PDL_HAS_TSS_EMULATION)  &&  !defined (PDL_HAS_MINIMAL_PDL_OS)
      // Just close the PDL_Log_Msg for the current (which should be
      // main) thread.  We don't have TSS emulation; if there's native
      // TSS, it should call its destructors when the main thread
      // exits.
      PDL_Log_Msg::close ();
#endif /* ! PDL_HAS_TSS_EMULATION  &&  ! PDL_HAS_MINIMAL_PDL_OS */

#if defined (PDL_WIN32) || defined (PDL_HAS_TSS_EMULATION) || (defined (PDL_PSOS) && defined (PDL_PSOS_HAS_TSS))
#if ! defined (PDL_HAS_TSS_EMULATION)
      // Don't do this with TSS_Emulation, because the the
      // PDL_TSS_Cleanup::instance () has already exited ().  We can't
      // safely access the TSS values that were created by the main
      // thread.

      // Remove all TSS_Info table entries.
      PDL_TSS_Cleanup::instance ()->free_all_keys_left ();
#endif /* ! PDL_HAS_TSS_EMULATION */

      // Finally, free up the PDL_TSS_Cleanup instance.  This method gets
      // called by the PDL_Object_Manager.
//      delete PDL_TSS_Cleanup::instance ();
#endif /* WIN32 || PDL_HAS_TSS_EMULATION || PDL_PSOS_HAS_TSS */

#if defined (PDL_HAS_TSS_EMULATION)
      PDL_TSS_Emulation::tss_close ();
#endif /* PDL_HAS_TSS_EMULATION */
    }
}

void
PDL_Thread_Adapter::inherit_log_msg (void)
{
}

#if defined (__IBMCPP__) && (__IBMCPP__ >= 400)
#define PDL_ENDTHREADEX(STATUS) ::_endthreadex ()
#define PDL_BEGINTHREADEX(STACK, STACKSIZE, ENTRY_POINT, ARGS, FLAGS, THR_ID) \
       (*THR_ID = ::_beginthreadex ((void(_Optlink*)(void*))ENTRY_POINT, STACK, STACKSIZE, ARGS), *THR_ID)
#elif defined (PDL_HAS_WINCE) && defined (UNDER_CE) && (UNDER_CE >= 211)
#define PDL_ENDTHREADEX(STATUS) ExitThread ((DWORD) STATUS)
#define PDL_BEGINTHREADEX(STACK, STACKSIZE, ENTRY_POINT, ARGS, FLAGS, THR_ID) \
      CreateThread (NULL, STACKSIZE, (unsigned long (__stdcall *) (void *)) ENTRY_POINT, ARGS, (FLAGS) & CREATE_SUSPENDED, (unsigned long *) THR_ID)
#else
#define PDL_ENDTHREADEX(STATUS) ::_endthreadex ((DWORD) STATUS)
#define PDL_BEGINTHREADEX(STACK, STACKSIZE, ENTRY_POINT, ARGS, FLAGS, THR_ID) \
      ::_beginthreadex (STACK, STACKSIZE, (unsigned (__stdcall *) (void *)) ENTRY_POINT, ARGS, FLAGS, (unsigned int *) THR_ID)
#endif /* defined (__IBMCPP__) && (__IBMCPP__ >= 400) */


PDL_THR_FUNC_RETURN
PDL_Thread_Adapter::invoke (void)
{
  PDL_THR_FUNC_INTERNAL func = PDL_reinterpret_cast (PDL_THR_FUNC_INTERNAL,
                                                     this->user_func_);
  void *arg = this->arg_;

  // Delete ourselves since we don't need <this> anymore.  Make sure
  // not to access <this> anywhere below this point.
  //delete this; //original source is uncommented -- hjohn

#if defined (PDL_NEEDS_LWP_PRIO_SET)
  // On SunOS, the LWP priority needs to be set in order to get
  // preemption when running in the RT class.  This is the PDL way to
  // do that . . .
  PDL_hthread_t thr_handle;
  PDL_OS::thr_self (thr_handle);
  int prio;

  // thr_getprio () on the current thread should never fail.
  PDL_OS::thr_getprio (thr_handle, prio);

  // PDL_OS::thr_setprio () has the special logic to set the LWP priority,
  // if running in the RT class.
  PDL_OS::thr_setprio (prio);

#endif /* PDL_NEEDS_LWP_PRIO_SET */

  PDL_THR_FUNC_RETURN status = 0;

  PDL_SEH_TRY
    {
      PDL_SEH_TRY
        {
#if defined (PDL_PSOS)
          (*func) (arg);
          // pSOS task functions do not return a value.
          status = 0;
#else /* ! PDL_PSOS */
          status = PDL_reinterpret_cast(PDL_THR_FUNC_RETURN, (*func) (arg));
#endif /* PDL_PSOS */
        }
#if defined (PDL_HAS_WIN32_STRUCTURAL_EXCEPTIONS)
      PDL_SEH_EXCEPT (PDL_OS_Object_Manager::seh_except_selector ()(
                          (void *) GetExceptionInformation ()))
        {
          PDL_OS_Object_Manager::seh_except_handler ()(0);
        }
#endif /* PDL_HAS_WIN32_STRUCTURAL_EXCEPTIONS */
	}

  PDL_SEH_FINALLY
    {
      // If we changed this to 1, change the respective if in
      // Task::svc_run to 0.

#if defined (PDL_WIN32) || defined (PDL_HAS_TSS_EMULATION)
          // Call TSS destructors.
      PDL_OS::cleanup_tss (0 /* not main thread */);

# if defined (PDL_WIN32)
      // Exit the thread.  Allow CWinThread-destructor to be invoked
      // from AfxEndThread.  _endthreadex will be called from
      // AfxEndThread so don't exit the thread now if we are running
      // an MFC thread.
      PDL_ENDTHREADEX (status);
# endif /* PDL_WIN32 */
#endif /* PDL_WIN32 || PDL_HAS_TSS_EMULATION */

#if defined (PDL_PSOS)
      // This sequence of calls is documented by ISI as the proper way to
      // clean up a pSOS task. They affect different components, so only
      // try the ones for components that are built with ACE.
#  if defined (SC_PREPC) && (SC_PREPC == YES)
      ::fclose (0);   // Return pREPC+ resources
#  endif /* SC_PREPC */
#  if defined (SC_PHILE) && (SC_PHILE == YES)
      ::close_f (0);  // Return pHILE+ resources
#  endif /* SC_PHILE */
#  if defined (SC_PNA) && (SC_PNA == YES)
      ::close (0);    // Return pNA+ resources
#  endif /* SC_PNA */
#  if defined (SC_SC_PREPC) && (SC_PREPC == YES)
      ::free (-1);    // Return pREPC+ memory
#  endif /* SC_PREPC */
      status = ::t_delete (0); // Suicide - only returns on error
#endif /* PDL_PSOS */
    }

	return status;
}

#if defined (PDL_HAS_WIN32_STRUCTURAL_EXCEPTIONS)
int PDL_SEH_Default_Exception_Selector (void *)
{
  PDL_DEBUG ((LM_DEBUG,
              ASYS_TEXT ("(%t) Win32 structured exception exiting thread\n")));
  return (DWORD) PDL_SEH_DEFAULT_EXCEPTION_HANDLING_ACTION;
}

int PDL_SEH_Default_Exception_Handler (void *)
{
  return 0;
}
#endif /* PDL_HAS_WIN32_STRUCTURAL_EXCEPTIONS */

// extern "C" void
// pdl_cleanup_destroyer (PDL_Cleanup *object, void *param)
// {
//   object->cleanup (param);
// }

// Run the thread entry point for the <PDL_Thread_Adapter>.  This must
// be an extern "C" to make certain compilers happy...

#if defined (PDL_PSOS)
extern "C" void
pdl_thread_adapter (unsigned long args)
{
  PDL_TRACE ("pdl_thread_adapter");

#if defined (PDL_HAS_TSS_EMULATION)
  // As early as we can in the execution of the new thread, allocate
  // its local TS storage.  Allocate it on the stack, to save dynamic
  // allocation/dealloction.
  void *ts_storage[PDL_TSS_Emulation::PDL_TSS_THREAD_KEYS_MAX];
  PDL_TSS_Emulation::tss_open (ts_storage);
#endif /* PDL_HAS_TSS_EMULATION */

  PDL_Thread_Adapter *thread_args = (PDL_Thread_Adapter *) args;

  // Invoke the user-supplied function with the args.
  thread_args->invoke ();
}
#else /* ! defined (PDL_PSOS) */
extern "C" PDL_THR_FUNC_RETURN
pdl_thread_adapter (void *args)
{
  PDL_TRACE ("pdl_thread_adapter");

#if defined (PDL_HAS_TSS_EMULATION)
  // As early as we can in the execution of the new thread, allocate
  // its local TS storage.  Allocate it on the stack, to save dynamic
  // allocation/dealloction.
  void *ts_storage[PDL_TSS_Emulation::PDL_TSS_THREAD_KEYS_MAX];
  PDL_TSS_Emulation::tss_open (ts_storage);
#endif /* PDL_HAS_TSS_EMULATION */

  PDL_Thread_Adapter *thread_args = (PDL_Thread_Adapter *) args;

  // Invoke the user-supplied function with the args.
  PDL_THR_FUNC_RETURN status = thread_args->invoke ();

  return status;
}
#endif /* PDL_PSOS */


PDL_Thread_Adapter::PDL_Thread_Adapter (PDL_THR_FUNC user_func,
                                        void *arg,
                                        PDL_THR_C_FUNC entry_point
#if defined (PDL_HAS_WIN32_STRUCTURAL_EXCEPTIONS)
                                        , PDL_SEH_EXCEPT_HANDLER selector,
                                        PDL_SEH_EXCEPT_HANDLER handler
#endif /* PDL_HAS_WIN32_STRUCTURAL_EXCEPTIONS */
                                        )
  : user_func_ (user_func),
    arg_ (arg),
    entry_point_ (entry_point)
{
PDL_TRACE ("PDL_Thread_Adapter::PDL_Thread_Adapter");
}

#if defined (PDL_HAS_WTHREADS)
# if !defined (PDL_HAS_WINCE)
/* ------------------------------------------------
 *  BUG-14680
 *  Memory Leak on windows Platform..
 *  thr_keycreate(key, cleanupfunc)
 *  above cleanupfunc doesn't call in windows
 *  unless calling thr_exit().
 *  so, we call thr_exit() function automatically
 *  just only on windows platforms by using
 *  indirection function structure(thrCreateStructArg).
 *
 *  by gamestar (2006/6/5)
 * ----------------------------------------------*/

typedef struct thrCreateStructArg
{
    PDL_THR_FUNC mFunc;
    void        *mArg;
} thrCreateStructArg;


static void *thrCreateIndirecFunc(thrCreateStructArg *thrCreateStructArg)
{
    PDL_THR_FUNC sFunc; // for calling free() before calling thr function
    void        *sArg;  // for calling free() before calling thr function
    void *sRc;

    // in order to free arg-structure
    sFunc = thrCreateStructArg->mFunc;
    sArg  = thrCreateStructArg->mArg;
    PDL_OS::free(thrCreateStructArg);

    // call thread function
    sRc = (*sFunc)(sArg);

    // call thr_exit() in order to free TSD memory area.
    PDL_OS::thr_exit(0);
    return NULL;
}
# endif
#endif

int
PDL_OS::thr_create (PDL_THR_FUNC func,
                    void *args,
                    long flags,
                    PDL_thread_t *thr_id,
                    PDL_hthread_t *thr_handle,
                    long priority,
                    void *stack,
                    size_t stacksize,
                    PDL_Thread_Adapter * /*_ thread_adapter _*/)
{
  PDL_TRACE ("PDL_OS::thr_create");
#if defined(_PDL_HAS_WINCE_)
    *thr_handle = (HANDLE)CreateThread( NULL,
        stacksize,
        (LPTHREAD_START_ROUTINE) func,
        args,
        0,
        thr_id);
    if(*thr_handle == NULL)
    {
        errno = GetLastError();
        return -1;
    }

    return 0;
#endif /* PDL_HAS_WINCE */

#if defined(ITRON)
  /* empty */
  PDL_NOTSUP_RETURN (0);
#else
  if (PDL_BIT_DISABLED (flags, THR_DETACHED) &&
      PDL_BIT_DISABLED (flags, THR_JOINABLE))
    PDL_SET_BITS (flags, THR_JOINABLE);

# if defined (PDL_NO_THREAD_ADAPTER)
#   define  PDL_THREAD_FUNCTION  func
#   define  PDL_THREAD_ARGUMENT  args
# else /* ! defined (PDL_NO_THREAD_ADAPTER) */
#   if defined (PDL_PSOS)
#     define  PDL_THREAD_FUNCTION (PSOS_TASK_ENTRY_POINT) thread_args->entry_point ()
#   else
#     define  PDL_THREAD_FUNCTION  thread_args->entry_point ()
#   endif /* defined (PDL_PSOS) */
#   define  PDL_THREAD_ARGUMENT  thread_args
# endif /* ! defined (PDL_NO_THREAD_ADAPTER) */

# if defined (PDL_HAS_THREADS)

#   if !defined (VXWORKS)
  // On VxWorks, the OS will provide a task name if the user doesn't.
  // So, we don't need to create a tmp_thr.  If the caller of this
  // member function is the Thread_Manager, than thr_id will be non-zero
  // anyways.
  PDL_thread_t tmp_thr;

  if (thr_id == 0)
    thr_id = &tmp_thr;
#   endif /* ! VXWORKS */

  PDL_hthread_t tmp_handle;
  if (thr_handle == 0)
    thr_handle = &tmp_handle;

#   if defined (PDL_HAS_PTHREADS)

  int result;
  pthread_attr_t attr;
#     if defined (PDL_HAS_PTHREADS_DRAFT4)
  if (::pthread_attr_create (&attr) != 0)
#     else /* PDL_HAS_PTHREADS_DRAFT4 */
  if (::pthread_attr_init (&attr) != 0)
#     endif /* PDL_HAS_PTHREADS_DRAFT4 */
      return -1;

  // *** Set Stack Size
#     if defined (PDL_NEEDS_HUGE_THREAD_STACKSIZE)
  if (stacksize < PDL_NEEDS_HUGE_THREAD_STACKSIZE)
    stacksize = PDL_NEEDS_HUGE_THREAD_STACKSIZE;
#     endif /* PDL_NEEDS_HUGE_THREAD_STACKSIZE */

#     if defined (CHORUS)
  // If it is a super actor, we can't set stacksize.  But for the time
  // being we are all non-super actors.  Should be fixed to take care
  // of super actors!!!
  if (stacksize == 0)
    stacksize = PDL_CHORUS_DEFAULT_MIN_STACK_SIZE;
  else if (stacksize < PDL_CHORUS_DEFAULT_MIN_STACK_SIZE)
    stacksize = PDL_CHORUS_DEFAULT_MIN_STACK_SIZE;
#     endif /*CHORUS */

  if (stacksize != 0)
    {
      size_t size = stacksize;

#     if defined (PTHREAD_STACK_MIN)
      if (size < PDL_static_cast (size_t, PTHREAD_STACK_MIN))
        size = PTHREAD_STACK_MIN;
#     endif /* PTHREAD_STACK_MIN */

#     if !defined (PDL_LACKS_THREAD_STACK_SIZE)      // JCEJ 12/17/96
#       if !defined (PDL_LACKS_THREAD_SETSTACK)
      if (stack != 0)
        result = PDL_ADAPT_RETVAL (pthread_attr_setstack (&attr, stack, size), result);
      else
        result = PDL_ADAPT_RETVAL (pthread_attr_setstacksize (&attr, size), result);
      if (result == -1)
#       elif defined (PDL_HAS_PTHREADS_DRAFT4) || defined (PDL_HAS_PTHREADS_DRAFT6)
      if (::pthread_attr_setstacksize (&attr, size) != 0)
#       else
      if (PDL_ADAPT_RETVAL(pthread_attr_setstacksize (&attr, size), result) == -1)
#       endif /* PDL_LACKS_THREAD_SETSTACK */
        {
#       if defined (PDL_HAS_PTHREADS_DRAFT4)
          ::pthread_attr_delete (&attr);
#       else /* PDL_HAS_PTHREADS_DRAFT4 */
          ::pthread_attr_destroy (&attr);
#       endif /* PDL_HAS_PTHREADS_DRAFT4 */
          return -1;
        }
#     else
      PDL_UNUSED_ARG (size);
#     endif /* !PDL_LACKS_THREAD_STACK_SIZE */
    }

  // *** Set Stack Address
#     if defined (PDL_LACKS_THREAD_SETSTACK)
#       if !defined (PDL_LACKS_THREAD_STACK_ADDR)
  if (stack != 0)
    {
      if (::pthread_attr_setstackaddr (&attr, stack) != 0)
        {
#         if defined (PDL_HAS_PTHREADS_DRAFT4)
          ::pthread_attr_delete (&attr);
#         else /* PDL_HAS_PTHREADS_DRAFT4 */
          ::pthread_attr_destroy (&attr);
#         endif /* PDL_HAS_PTHREADS_DRAFT4 */
          return -1;
        }
    }
#       else
  PDL_UNUSED_ARG (stack);
#       endif /* !PDL_LACKS_THREAD_STACK_ADDR */
#     endif /* !PDL_LACKS_THREAD_SETSTACK */

  // *** Deal with various attributes
  if (flags != 0)
    {
      // *** Set Detach state
#     if !defined (PDL_LACKS_SETDETACH)
      if (PDL_BIT_ENABLED (flags, THR_DETACHED)
          || PDL_BIT_ENABLED (flags, THR_JOINABLE))
        {
          int dstate = PTHREAD_CREATE_JOINABLE;

          if (PDL_BIT_ENABLED (flags, THR_DETACHED))
            dstate = PTHREAD_CREATE_DETACHED;

#       if defined (PDL_HAS_PTHREADS_DRAFT4)
          if (::pthread_attr_setdetach_np (&attr, dstate) != 0)
#       else /* PDL_HAS_PTHREADS_DRAFT4 */
#         if defined (PDL_HAS_PTHREADS_DRAFT6)
          if (::pthread_attr_setdetachstate (&attr, &dstate) != 0)
#         else
          if (PDL_ADAPT_RETVAL(::pthread_attr_setdetachstate (&attr, dstate),
                               result) != 0)
#         endif /* PDL_HAS_PTHREADS_DRAFT6 */
#       endif /* PDL_HAS_PTHREADS_DRAFT4 */
            {
#       if defined (PDL_HAS_PTHREADS_DRAFT4)
              ::pthread_attr_delete (&attr);
#       else /* PDL_HAS_PTHREADS_DRAFT4 */
              ::pthread_attr_destroy (&attr);
#       endif /* PDL_HAS_PTHREADS_DRAFT4 */
              return -1;
            }
        }

      // Note: if PDL_LACKS_SETDETACH and THR_DETACHED is enabled, we
      // call ::pthread_detach () below.  If THR_DETACHED is not
      // enabled, we call ::pthread_detach () in the Thread_Manager,
      // after joining with the thread.
#     endif /* PDL_LACKS_SETDETACH */

      // *** Set Policy
#     if !defined (PDL_LACKS_SETSCHED)
      // If we wish to set the priority explicitly, we have to enable
      // explicit scheduling, and a policy, too.
      if (priority != PDL_DEFAULT_THREAD_PRIORITY)
        {
          PDL_SET_BITS (flags, THR_EXPLICIT_SCHED);
          if (PDL_BIT_DISABLED (flags, THR_SCHED_FIFO)
              && PDL_BIT_DISABLED (flags, THR_SCHED_RR)
              && PDL_BIT_DISABLED (flags, THR_SCHED_DEFAULT))
            PDL_SET_BITS (flags, THR_SCHED_DEFAULT);
        }

      if (PDL_BIT_ENABLED (flags, THR_SCHED_FIFO)
          || PDL_BIT_ENABLED (flags, THR_SCHED_RR)
          || PDL_BIT_ENABLED (flags, THR_SCHED_DEFAULT))
        {
          int spolicy;

#       if defined (PDL_HAS_ONLY_SCHED_OTHER)
            // SunOS, thru version 5.6, only supports SCHED_OTHER.
            spolicy = SCHED_OTHER;
#       else
          // Make sure to enable explicit scheduling, in case we didn't
          // enable it above (for non-default priority).
          PDL_SET_BITS (flags, THR_EXPLICIT_SCHED);

          if (PDL_BIT_ENABLED (flags, THR_SCHED_DEFAULT))
            spolicy = SCHED_OTHER;
          else if (PDL_BIT_ENABLED (flags, THR_SCHED_FIFO))
            spolicy = SCHED_FIFO;
#         if defined (SCHED_IO)
          else if (PDL_BIT_ENABLED (flags, THR_SCHED_IO))
            spolicy = SCHED_IO;
#         else
          else if (PDL_BIT_ENABLED (flags, THR_SCHED_IO))
            {
              errno = ENOSYS;
              return -1;
            }
#         endif /* SCHED_IO */
          else
            spolicy = SCHED_RR;

#         if defined (PDL_HAS_FSU_PTHREADS)
          int ret;
          switch (spolicy)
            {
            case SCHED_FIFO:
            case SCHED_RR:
              ret = 0;
              break;
            default:
              ret = 22;
              break;
            }
          if (ret != 0)
            {
              ::pthread_attr_destroy (&attr);
              return -1;
            }
#         endif    /*  PDL_HAS_FSU_PTHREADS */

#       endif /* PDL_HAS_ONLY_SCHED_OTHER */

#       if defined (PDL_HAS_PTHREADS_DRAFT4)
          result = ::pthread_attr_setsched (&attr, spolicy);
#       elif defined (PDL_HAS_PTHREADS_DRAFT6)
          result = ::pthread_attr_setschedpolicy (&attr, spolicy);
#       else  /* draft 7 or std */
          PDL_ADAPT_RETVAL(::pthread_attr_setschedpolicy (&attr, spolicy),
                           result);
#       endif /* PDL_HAS_PTHREADS_DRAFT4 */
          if (result != 0)
              {
#       if defined (PDL_HAS_PTHREADS_DRAFT4)
                ::pthread_attr_delete (&attr);
#       else /* PDL_HAS_PTHREADS_DRAFT4 */
                ::pthread_attr_destroy (&attr);
#       endif /* PDL_HAS_PTHREADS_DRAFT4 */
                return -1;
              }
        }

      // *** Set Priority (use reasonable default priorities)
#       if defined(PDL_HAS_PTHREADS_STD)
      // If we wish to explicitly set a scheduling policy, we also
      // have to specify a priority.  We choose a "middle" priority as
      // default.  Maybe this is also necessary on other POSIX'ish
      // implementations?
      if ((PDL_BIT_ENABLED (flags, THR_SCHED_FIFO)
           || PDL_BIT_ENABLED (flags, THR_SCHED_RR)
           || PDL_BIT_ENABLED (flags, THR_SCHED_DEFAULT))
          && priority == PDL_DEFAULT_THREAD_PRIORITY)
        {
          if (PDL_BIT_ENABLED (flags, THR_SCHED_FIFO))
            priority = PDL_THR_PRI_FIFO_DEF;
          else if (PDL_BIT_ENABLED (flags, THR_SCHED_RR))
            priority = PDL_THR_PRI_RR_DEF;
          else // THR_SCHED_DEFAULT
            priority = PDL_THR_PRI_OTHER_DEF;
        }
#       endif /* PDL_HAS_PTHREADS_STD */
      if (priority != PDL_DEFAULT_THREAD_PRIORITY)
        {
          struct sched_param sparam;
          PDL_OS::memset ((void *) &sparam, 0, sizeof sparam);

#       if defined (PDL_HAS_IRIX62_THREADS)
          sparam.sched_priority = PDL_MIN (priority,
                                           (long) PTHREAD_MAX_PRIORITY);
#       elif defined (PTHREAD_MAX_PRIORITY) && !defined(PDL_HAS_PTHREADS_STD)
          /* For MIT pthreads... */
          sparam.prio = PDL_MIN (priority, PTHREAD_MAX_PRIORITY);
#       elif defined(PDL_HAS_PTHREADS_STD) && !defined (PDL_HAS_STHREADS)
          // The following code forces priority into range.
          if (PDL_BIT_ENABLED (flags, THR_SCHED_FIFO))
            sparam.sched_priority =
              PDL_MIN (PDL_THR_PRI_FIFO_MAX,
                       PDL_MAX (PDL_THR_PRI_FIFO_MIN, priority));
          else if (PDL_BIT_ENABLED(flags, THR_SCHED_RR))
            sparam.sched_priority =
              PDL_MIN (PDL_THR_PRI_RR_MAX,
                       PDL_MAX (PDL_THR_PRI_RR_MIN, priority));
          else // Default policy, whether set or not
            sparam.sched_priority =
              PDL_MIN (PDL_THR_PRI_OTHER_MAX,
                       PDL_MAX (PDL_THR_PRI_OTHER_MIN, priority));
#       elif defined (PRIORITY_MAX)
          sparam.sched_priority = PDL_MIN (priority,
                                           (long) PRIORITY_MAX);
#       else
          sparam.sched_priority = priority;
#       endif /* PDL_HAS_IRIX62_THREADS */

#       if defined (PDL_HAS_FSU_PTHREADS)
         if (sparam.sched_priority >= PTHREAD_MIN_PRIORITY
             && sparam.sched_priority <= PTHREAD_MAX_PRIORITY)
           attr.prio = sparam.sched_priority;
         else
           {
             pthread_attr_destroy (&attr);
             errno = EINVAL;
             return -1;
           }
#       else
         {
#         if defined (sun)  &&  defined (PDL_HAS_ONLY_SCHED_OTHER)
           // SunOS, through 5.6, POSIX only allows priorities > 0 to
           // ::pthread_attr_setschedparam.  If a priority of 0 was
           // requested, set the thread priority after creating it, below.
           if (priority > 0)
#         endif /* sun && PDL_HAS_ONLY_SCHED_OTHER */
             {
#         if defined (PDL_HAS_PTHREADS_DRAFT4) || defined (PDL_HAS_PTHREADS_DRAFT6)
               result = ::pthread_attr_setprio (&attr,
                                                sparam.sched_priority);
#         else /* this is draft 7 or std */
               PDL_ADAPT_RETVAL(::pthread_attr_setschedparam (&attr, &sparam),
                                result);
#         endif /* PDL_HAS_PTHREADS_DRAFT4, 6 */
               if (result != 0)
                 {
#         if defined (PDL_HAS_PTHREADS_DRAFT4)
                   ::pthread_attr_delete (&attr);
#         else /* PDL_HAS_PTHREADS_DRAFT4 */
                   ::pthread_attr_destroy (&attr);
#         endif /* PDL_HAS_PTHREADS_DRAFT4 */
                   return -1;
                 }
             }
         }
#       endif    /* PDL_HAS_FSU_PTHREADS */
        }

      // *** Set scheduling explicit or inherited
      if (PDL_BIT_ENABLED (flags, THR_INHERIT_SCHED)
          || PDL_BIT_ENABLED (flags, THR_EXPLICIT_SCHED))
        {
#       if defined (PDL_HAS_PTHREADS_DRAFT4)
          int sched = PTHREAD_DEFAULT_SCHED;
#       else /* PDL_HAS_PTHREADS_DRAFT4 */
          int sched = PTHREAD_EXPLICIT_SCHED;
#       endif /* PDL_HAS_PTHREADS_DRAFT4 */
          if (PDL_BIT_ENABLED (flags, THR_INHERIT_SCHED))
            sched = PTHREAD_INHERIT_SCHED;
          if (::pthread_attr_setinheritsched (&attr, sched) != 0)
            {
#       if defined (PDL_HAS_PTHREADS_DRAFT4)
              ::pthread_attr_delete (&attr);
#       else /* PDL_HAS_PTHREADS_DRAFT4 */
              ::pthread_attr_destroy (&attr);
#       endif /* PDL_HAS_PTHREADS_DRAFT4 */
              return -1;
            }
        }
#     else /* PDL_LACKS_SETSCHED */
      PDL_UNUSED_ARG (priority);
#     endif /* PDL_LACKS_SETSCHED */

      // *** Set Scope
#     if !defined (PDL_LACKS_THREAD_PROCESS_SCOPING)
      if (PDL_BIT_ENABLED (flags, THR_SCOPE_SYSTEM)
          || PDL_BIT_ENABLED (flags, THR_SCOPE_PROCESS))
        {
          int scope = PTHREAD_SCOPE_PROCESS;
          if (PDL_BIT_ENABLED (flags, THR_SCOPE_SYSTEM))
            scope = PTHREAD_SCOPE_SYSTEM;

          if (::pthread_attr_setscope (&attr, scope) != 0)
            {
#       if defined (PDL_HAS_PTHREADS_DRAFT4)
              ::pthread_attr_delete (&attr);
#       else /* PDL_HAS_PTHREADS_DRAFT4 */
              ::pthread_attr_destroy (&attr);
#       endif /* PDL_HAS_PTHREADS_DRAFT4 */
              return -1;
            }
        }
#     endif /* !PDL_LACKS_THREAD_PROCESS_SCOPING */

      if (PDL_BIT_ENABLED (flags, THR_NEW_LWP))
        {
          // Increment the number of LWPs by one to emulate the
          // SunOS semantics.
          int lwps = PDL_OS::thr_getconcurrency ();
          if (lwps == -1)
            {
              if (errno == ENOTSUP)
                {
                  // Suppress the ENOTSUP because it's harmless.
                  errno = 0;
                }
              else
                {
                  // This should never happen on SunOS:
                  // ::thr_getconcurrency () should always succeed.
                  return -1;
                }
            }
          else
            {
              if (PDL_OS::thr_setconcurrency (lwps + 1) == -1)
                {
                  if (errno == ENOTSUP)
                    {
                      // Unlikely:  ::thr_getconcurrency () is supported but
                      // ::thr_setconcurrency () is not?
                    }
                  else
                    {
                      return -1;
                    }
                }
            }
        }
    }

#     if defined (PDL_HAS_PTHREADS_DRAFT4)
  PDL_OSCALL (::pthread_create (thr_id, attr,
                                func,
                                args),
              int, -1, result);

#       if defined (PDL_LACKS_SETDETACH)
  if (PDL_BIT_ENABLED (flags, THR_DETACHED))
    {
#         if defined (HPUX_10)
    // HP-UX DCE threads' pthread_detach will idash thr_id if it's
    // just given as an argument.  This will cause PDL_Thread_Manager
    // (if it's doing this create) to lose track of the new thread
    // since the ID will be passed back equal to 0.  So give
    // pthread_detach a junker to scribble on.
      PDL_thread_t  junker;
      cma_handle_assign(thr_id, &junker);
      ::pthread_detach (&junker);
#         else
      ::pthread_detach (thr_id);
#         endif /* HPUX_10 */
    }
#       endif /* PDL_LACKS_SETDETACH */

  ::pthread_attr_delete (&attr);

#     elif defined (PDL_HAS_PTHREADS_DRAFT6)
  PDL_OSCALL (::pthread_create (thr_id, &attr,
                                func,
                                args),
              int, -1, result);
  ::pthread_attr_destroy (&attr);

#     else /* this is draft 7 or std */
  PDL_OSCALL (PDL_ADAPT_RETVAL (::pthread_create (thr_id,
                                                  &attr,
                                                  func,
                                                  args),
                                result),
              int, -1, result);
  ::pthread_attr_destroy (&attr);
#     endif /* PDL_HAS_PTHREADS_DRAFT4 */

  // This is a SunOS or POSIX implementation of pthreads,
  // where we assume that PDL_thread_t and PDL_hthread_t are the same.
  // If this *isn't* correct on some platform, please let us know.
  if (result != -1)
    *thr_handle = *thr_id;

#     if defined (sun)  &&  defined (PDL_HAS_ONLY_SCHED_OTHER)
        // SunOS prior to 5.7:

        // If the priority is 0, then we might have to set it now
        // because we couldn't set it with
        // ::pthread_attr_setschedparam, as noted above.  This doesn't
        // provide strictly correct behavior, because the thread was
        // created (above) with the priority of its parent.  (That
        // applies regardless of the inherit_sched attribute: if it
        // was PTHREAD_INHERIT_SCHED, then it certainly inherited its
        // parent's priority.  If it was PTHREAD_EXPLICIT_SCHED, then
        // "attr" was initialized by the SunOS ::pthread_attr_init
        // () to contain NULL for the priority, which indicated to
        // SunOS ::pthread_create () to inherit the parent
        // priority.)
        if (priority == 0)
          {
            // Check the priority of this thread, which is the parent
            // of the newly created thread.  If it is 0, then the
            // newly created thread will have inherited the priority
            // of 0, so there's no need to explicitly set it.
            struct sched_param sparam;
            int policy = 0;
            PDL_OSCALL (PDL_ADAPT_RETVAL (::pthread_getschedparam (thr_self (),
                                                                   &policy,
                                                                   &sparam),
                                          result), int,
                        -1, result);

            // The only policy supported by by SunOS, thru version 5.6,
            // is SCHED_OTHER, so that's hard-coded here.
            policy = PDL_SCHED_OTHER;

            if (sparam.sched_priority != 0)
              {
                PDL_OS::memset ((void *) &sparam, 0, sizeof sparam);
                // The memset to 0 sets the priority to 0, so we don't need
                // to explicitly set sparam.sched_priority.

                PDL_OSCALL_RETURN (PDL_ADAPT_RETVAL (::pthread_setschedparam (
                                                       *thr_id,
                                                       policy,
                                                       &sparam),
                                                     result),
                                   int, -1);
              }
          }

#     endif /* sun && PDL_HAS_ONLY_SCHED_OTHER */
  return result;
#   elif defined (PDL_HAS_STHREADS)
  int result;
  int start_suspended = PDL_BIT_ENABLED (flags, THR_SUSPENDED);

  if (priority != PDL_DEFAULT_THREAD_PRIORITY)
    // If we need to set the priority, then we need to start the
    // thread in a suspended mode.
    PDL_SET_BITS (flags, THR_SUSPENDED);

  PDL_OSCALL (PDL_ADAPT_RETVAL (::thr_create (stack, stacksize,
                                              func,
                                              args,
                                              flags, thr_id), result),
              int, -1, result);

  if (result != -1)
    {
      // With SunOS threads, PDL_thread_t and PDL_hthread_t are the same.
      *thr_handle = *thr_id;

      if (priority != PDL_DEFAULT_THREAD_PRIORITY)
        {
          // Set the priority of the new thread and then let it
          // continue, but only if the user didn't start it suspended
          // in the first place!
          if ((result = PDL_OS::thr_setprio (*thr_id, priority)) != 0)
            {
              errno = result;
              return -1;
            }

          if (start_suspended == 0)
            {
              if ((result = PDL_OS::thr_continue (*thr_id)) != 0)
                {
                  errno = result;
                  return -1;
                }
            }
        }
    }
  return result;
#   elif defined (PDL_HAS_WTHREADS)
  PDL_UNUSED_ARG (stack);
    {
      int start_suspended = PDL_BIT_ENABLED (flags, THR_SUSPENDED);

      if (priority != PDL_DEFAULT_THREAD_PRIORITY)
        // If we need to set the priority, then we need to start the
        // thread in a suspended mode.
        PDL_SET_BITS (flags, THR_SUSPENDED);

#if !defined (PDL_HAS_WINCE)
      {
        //  BUG-14680
        struct thrCreateStructArg *sThrArg;
        sThrArg = (struct thrCreateStructArg *)PDL_OS::malloc(sizeof(struct thrCreateStructArg));
        if (sThrArg == NULL)
        {
            PDL_FAIL_RETURN (-1);
        }
        sThrArg->mFunc = func;
        sThrArg->mArg  = args;
        *thr_handle = (void *) PDL_BEGINTHREADEX (0,
                                                stacksize,
                                                thrCreateIndirecFunc,
                                                sThrArg,
                                                flags,
                                                thr_id);
        if (*thr_handle == 0)
        {
            // free memory if creation failed.
            PDL_OS::free(sThrArg);
        }
      }
#else
	  *thr_handle = (void *) ::CreateThread (0,
                                             stacksize,
                                             (unsigned long (__stdcall *) (void *))func,
                                             args,
                                             flags,
                                             (unsigned long *)thr_id);

#endif /* PDL_HAS_WINCE */


      if (priority != PDL_DEFAULT_THREAD_PRIORITY && *thr_handle != 0)
        {
          // Set the priority of the new thread and then let it
          // continue, but only if the user didn't start it suspended
          // in the first place!
          PDL_OS::thr_setprio (*thr_handle, priority);

          if (start_suspended == 0)
            PDL_OS::thr_continue (*thr_handle);
        }
    }

  // Close down the handle if no one wants to use it.
  if (thr_handle == &tmp_handle)
    ::CloseHandle (tmp_handle);

  if (*thr_handle != 0)
    return 0;
  else
    PDL_FAIL_RETURN (-1);
  /* NOTREACHED */

#   elif defined (PDL_PSOS)

  // stack is created in the task's memory region 0
  PDL_UNUSED_ARG (stack);

  // task creation and start flags are fixed
  PDL_UNUSED_ARG (flags);

  // lowest priority is reserved for the IDLE pSOS+ system daemon,
  // highest are reserved for high priority pSOS+ system daemons
  if (priority < PSOS_TASK_MIN_PRIORITY)
  {
    priority = PSOS_TASK_MIN_PRIORITY;
  }
  else if (priority > PSOS_TASK_MAX_PRIORITY)
  {
    priority = PSOS_TASK_MAX_PRIORITY;
  }

  // set the stacksize to a default value if no size is specified
  if (stacksize == 0)
    stacksize = PDL_PSOS_DEFAULT_STACK_SIZE;

  PDL_hthread_t tid;
  *thr_handle = 0;

  // create the thread
  if (t_create ((char *) thr_id, // task name
                priority,        // (possibly adjusted) task priority
                stacksize,       // passed stack size is used for supervisor stack
                0,               // no user stack: tasks run strictly in supervisor mode
                T_LOCAL,         // local to the pSOS+ node (does not support pSOS+m)
                &tid)            // receives task id
      != 0)
    {
      return -1;
    }

  // pSOS tasks are passed an array of 4 u_longs
  u_long targs[4];
  targs[0] = (u_long) PDL_THREAD_ARGUMENT;
  targs[1] = 0;
  targs[2] = 0;
  targs[3] = 0;

  // start the thread
  if (t_start (tid,
               T_PREEMPT |            // Task can be preempted
//             T_NOTSLICE |           // Task is not timesliced with other tasks at same priority
               T_TSLICE |             // Task is timesliced with other tasks at same priority
               T_NOASR |              // Task level signals disabled
               T_SUPV |               // Task runs strictly in supervisor mode
               T_ISR,                 // Hardware interrupts are enabled
               PDL_THREAD_FUNCTION,   // Task entry point
               targs)                 // Task argument(s)
      != 0)
  {
    return -1;
  }

  // store the task id in the handle and return success
  *thr_handle = tid;
  return 0;

#   elif defined (VXWORKS)
  // The hard-coded values below are what ::sp () would use.  (::sp ()
  // hardcodes priority to 100, flags to VX_FP_TASK, and stacksize to
  // 20,000.)  stacksize should be an even integer.  If a stack is not
  // specified, ::taskSpawn () is used so that we can set the
  // priority, flags, and stacksize.  If a stack is specified,
  // ::taskInit ()/::taskActivate() are used.

  // If called with thr_create() defaults, use same default values as ::sp ():
  if (priority == PDL_DEFAULT_THREAD_PRIORITY) priority = 100;
  // Assumes that there is a floating point coprocessor.  As noted
  // above, ::sp () hardcodes this, so we should be safe with it.
  if (flags == 0) flags = VX_FP_TASK;
  if (stacksize == 0) stacksize = 20000;

  const u_int thr_id_provided =
    thr_id  &&  *thr_id  &&  (*thr_id)[0] != PDL_THR_ID_ALLOCATED;

  PDL_hthread_t tid;
  PDL_UNUSED_ARG (stack);
      // The call below to ::taskSpawn () causes VxWorks to assign a
      // unique task name of the form: "t" + an integer, because the
      // first argument is 0.
      tid.id = ::taskSpawn (thr_id_provided  ?  *thr_id  :  0,
                         priority,
                         (int) flags,
                         (int) stacksize,
                         (FUNCPTR)func,
                         (int) args,
                         0, 0, 0, 0, 0, 0, 0, 0, 0);

  if (tid.id == ERROR)
    return -1;
  else
    {
      if (! thr_id_provided  &&  thr_id)
        {
          if (*thr_id  &&  (*thr_id)[0] == PDL_THR_ID_ALLOCATED)
            // *thr_id was allocated by the Thread_Manager.  ::taskTcb
            // (int tid) returns the address of the WIND_TCB (task
            // control block).  According to the ::taskSpawn()
            // documentation, the name of the new task is stored at
            // pStackBase, but is that of the current task?  If so, it
            // might be a bit quicker than this extraction of the tcb
            // . . .
            PDL_OS::strncpy (*thr_id + 1, ::taskTcb (tid.id)->name, 10);
          else
            // *thr_id was not allocated by the Thread_Manager.
            // Pass back the task name in the location pointed to
            // by thr_id.
            *thr_id = ::taskTcb (tid.id)->name;
        }
      // else if the thr_id was provided, there's no need to overwrite
      // it with the same value (string).  If thr_id is 0, then we can't
      // pass the task name back.

      if (thr_handle)
        thr_handle->id = tid.id;
        //*thr_handle = tid;

      return 0;
    }

#   endif /* PDL_HAS_STHREADS */
# else
  PDL_UNUSED_ARG (func);
  PDL_UNUSED_ARG (args);
  PDL_UNUSED_ARG (flags);
  PDL_UNUSED_ARG (thr_id);
  PDL_UNUSED_ARG (thr_handle);
  PDL_UNUSED_ARG (priority);
  PDL_UNUSED_ARG (stack);
  PDL_UNUSED_ARG (stacksize);
  PDL_NOTSUP_RETURN (-1);
# endif /* PDL_HAS_THREADS */
#endif
}

void
PDL_OS::thr_exit (PDL_THR_FUNC_RETURN status)
{
PDL_TRACE ("PDL_OS::thr_exit");
# if defined (PDL_HAS_THREADS)
#   if defined (PDL_HAS_PTHREADS)
    ::pthread_exit (status);
#   elif defined (PDL_HAS_STHREADS)
    ::thr_exit (status);
#   elif defined (PDL_HAS_WTHREADS)
    // Can't call it here because on NT, the thread is exited
    // directly by PDL_Thread_Adapter::invoke ().
    //   PDL_TSS_Cleanup::instance ()->exit (status);

    // Call TSS destructors.
    PDL_OS::cleanup_tss (0 /* not main thread */);

    // Exit the thread.
    // Allow CWinThread-destructor to be invoked from AfxEndThread.
    // _endthreadex will be called from AfxEndThread so don't exit the
    // thread now if we are running an MFC thread.
    PDL_ENDTHREADEX (status);

#   elif defined (VXWORKS)
    PDL_hthread_t tid;
    PDL_OS::thr_self (tid);
    *((int *) status) = ::taskDelete (tid.id);
#   elif defined (PDL_PSOS)
    PDL_hthread_t tid;
    PDL_OS::thr_self (tid);

#     if defined (PDL_PSOS) && defined (PDL_PSOS_HAS_TSS)
    // Call TSS destructors.
    PDL_OS::cleanup_tss (0 /* not main thread */);
#     endif /* PDL_PSOS && PDL_PSOS_HAS_TSS */

    *((u_long *) status) = ::t_delete (tid);
#   endif /* PDL_HAS_PTHREADS */
# else
  PDL_UNUSED_ARG (status);
# endif /* PDL_HAS_THREADS */
}


# if defined (PDL_HAS_TSS_EMULATION) && defined (PDL_HAS_THREAD_SPECIFIC_STORAGE)
int
PDL_OS::thr_setspecific (PDL_OS_thread_key_t key, void *data)
{
  // PDL_TRACE ("PDL_OS::thr_setspecific");
#   if defined (PDL_HAS_THREADS)
#     if defined (PDL_HAS_PTHREADS)
#       if defined (PDL_HAS_FSU_PTHREADS)
      // Call pthread_init() here to initialize threads package.  FSU
      // threads need an initialization before the first thread constructor.
      // This seems to be the one; however, a segmentation fault may
      // indicate that another pthread_init() is necessary, perhaps in
      // Synch.cpp or Synch_T.cpp.  FSU threads will not reinit if called
      // more than once, so another call to pthread_init will not adversely
      // affect existing threads.
      pthread_init ();
#       endif /*  PDL_HAS_FSU_PTHREADS */
    PDL_OSCALL_RETURN (PDL_ADAPT_RETVAL (::pthread_setspecific (key, data), pdl_result_), int, -1);
#     elif defined (PDL_HAS_STHREADS)
    PDL_OSCALL_RETURN (PDL_ADAPT_RETVAL (::thr_setspecific (key, data), pdl_result_), int, -1);
#     elif defined (PDL_HAS_WTHREADS)
    ::TlsSetValue (key, data);
    return 0;
#     endif /* PDL_HAS_STHREADS */
#   else
  PDL_UNUSED_ARG (key);
  PDL_UNUSED_ARG (data);
  PDL_NOTSUP_RETURN (-1);
#   endif /* PDL_HAS_THREADS */
}
# endif /* PDL_HAS_TSS_EMULATION && PDL_HAS_THREAD_SPECIFIC_STORAGE */

int
PDL_OS::thr_setspecific (PDL_thread_key_t key, void *data)
{
  // PDL_TRACE ("PDL_OS::thr_setspecific");
# if defined (PDL_HAS_THREADS)
#   if defined (PDL_HAS_TSS_EMULATION)
    PDL_KEY_INDEX (key_index, key);

    if (key_index >= PDL_TSS_Emulation::total_keys ())
      {
        errno = EINVAL;
        data = 0;
        return -1;
      }
    else
      {
        PDL_TSS_Emulation::ts_object (key) = data;
        PDL_TSS_Cleanup::instance ()->key_used (key);

        return 0;
      }
#   elif defined (PDL_HAS_PTHREADS)
#     if defined (PDL_HAS_FSU_PTHREADS)
      // Call pthread_init() here to initialize threads package.  FSU
      // threads need an initialization before the first thread constructor.
      // This seems to be the one; however, a segmentation fault may
      // indicate that another pthread_init() is necessary, perhaps in
      // Synch.cpp or Synch_T.cpp.  FSU threads will not reinit if called
      // more than once, so another call to pthread_init will not adversely
      // affect existing threads.
      pthread_init ();
#     endif /*  PDL_HAS_FSU_PTHREADS */

#     if defined (PDL_HAS_PTHREADS_DRAFT4) || defined (PDL_HAS_PTHREADS_DRAFT6)
    PDL_OSCALL_RETURN (::pthread_setspecific (key, data), int, -1);
#     else
    PDL_OSCALL_RETURN (PDL_ADAPT_RETVAL (::pthread_setspecific (key, data),
                                         pdl_result_),
                       int, -1);
#     endif /* PDL_HAS_PTHREADS_DRAFT4, 6 */

#   elif defined (PDL_HAS_STHREADS)
    PDL_OSCALL_RETURN (PDL_ADAPT_RETVAL (::thr_setspecific (key, data), pdl_result_), int, -1);
#   elif defined (PDL_PSOS) && defined (PDL_PSOS_HAS_TSS)
    PDL_hthread_t tid;
    PDL_OS::thr_self (tid);
    if (::tsd_setval (key, tid, data) != 0)
     return -1;
    PDL_TSS_Cleanup::instance ()->key_used (key);
    return 0;
#   elif defined (PDL_HAS_WTHREADS)
    if(::TlsSetValue (key, data) == 0)
    {
        int sErrno = GetLastError();
        errno = sErrno;
        return -1;
    }

    PDL_TSS_Cleanup::instance ()->key_used (key);
    return 0;
#   else
    PDL_UNUSED_ARG (key);
    PDL_UNUSED_ARG (data);
    PDL_NOTSUP_RETURN (-1);
#   endif /* PDL_HAS_STHREADS */
# else
  PDL_UNUSED_ARG (key);
  PDL_UNUSED_ARG (data);
  PDL_NOTSUP_RETURN (-1);
# endif /* PDL_HAS_THREADS */
}

int
PDL_OS::thr_keyfree (PDL_thread_key_t key)
{
PDL_TRACE ("PDL_OS::thr_keyfree");
# if defined (PDL_HAS_THREADS)
#   if defined (PDL_HAS_TSS_EMULATION)
    return PDL_TSS_Cleanup::instance ()->remove (key);
#   elif defined (PDL_HAS_PTHREADS_DRAFT4) || defined (PDL_HAS_PTHREADS_DRAFT6)
    PDL_UNUSED_ARG (key);
    PDL_NOTSUP_RETURN (-1);
#   elif defined (PDL_HAS_PTHREADS)
    return ::pthread_key_delete (key);
#   elif defined (PDL_HAS_THR_KEYDELETE)
    return ::thr_keydelete (key);
#   elif defined (PDL_HAS_STHREADS)
    PDL_UNUSED_ARG (key);
    PDL_NOTSUP_RETURN (-1);
#   elif defined (PDL_HAS_WTHREADS)
    // Extract out the thread-specific table instance and free up
    // the key and destructor.
    PDL_TSS_Cleanup::instance ()->remove (key);
    PDL_WIN32CALL_RETURN (PDL_ADAPT_RETVAL (::TlsFree (key), pdl_result_), int, -1);
#   elif defined (PDL_PSOS) && defined (PDL_PSOS_HAS_TSS)
    // Extract out the thread-specific table instance and free up
    // the key and destructor.
    PDL_TSS_Cleanup::instance ()->remove (key);
    return (::tsd_delete (key) == 0) ? 0 : -1;
#   else
    PDL_UNUSED_ARG (key);
    PDL_NOTSUP_RETURN (-1);
#   endif /* PDL_HAS_TSS_EMULATION */
# else
  PDL_UNUSED_ARG (key);
  PDL_NOTSUP_RETURN (-1);
# endif /* PDL_HAS_THREADS */
}

# if defined (PDL_HAS_TSS_EMULATION) && defined (PDL_HAS_THREAD_SPECIFIC_STORAGE)
int
PDL_OS::thr_keycreate (PDL_OS_thread_key_t *key,
#   if defined (PDL_HAS_THR_C_DEST)
                       PDL_THR_C_DEST dest,
#   else
                       PDL_THR_DEST dest,
#   endif /* PDL_HAS_THR_C_DEST */
                       void *inst)
{
    // PDL_TRACE ("PDL_OS::thr_keycreate");
#   if defined (PDL_HAS_THREADS)
#     if defined (PDL_HAS_PTHREADS)
    PDL_UNUSED_ARG (inst);


#       if defined (PDL_HAS_STDARG_THR_DEST)
    PDL_OSCALL_RETURN (::pthread_keycreate (key, (void (*)(...)) dest), int, -1);
#       elif defined (PDL_HAS_PTHREADS_DRAFT4)
    PDL_OSCALL_RETURN (::pthread_keycreate (key, dest), int, -1);
#       elif defined (PDL_HAS_PTHREADS_DRAFT6)
    PDL_OSCALL_RETURN (::pthread_key_create (key, dest), int, -1);
#       else
    PDL_OSCALL_RETURN (PDL_ADAPT_RETVAL (::pthread_key_create (key, dest),
                                         pdl_result_),
                       int, -1);
#       endif /* PDL_HAS_STDARG_THR_DEST */
#     elif defined (PDL_HAS_STHREADS)
    PDL_UNUSED_ARG (inst);
    PDL_OSCALL_RETURN (PDL_ADAPT_RETVAL (::thr_keycreate (key, dest),
                                         pdl_result_),
                       int, -1);
#     elif defined (PDL_HAS_WTHREADS)
    *key = ::TlsAlloc ();

    if (*key != PDL_SYSCALL_FAILED)
      {
        // Extract out the thread-specific table instance and stash away
        // the key and destructor so that we can free it up later on...
        return PDL_TSS_Cleanup::instance ()->insert (*key, dest, inst);
      }
    else
      PDL_FAIL_RETURN (-1);
      /* NOTREACHED */
#     endif /* PDL_HAS_STHREADS */
#   else
  PDL_UNUSED_ARG (key);
  PDL_UNUSED_ARG (dest);
  PDL_UNUSED_ARG (inst);
  PDL_NOTSUP_RETURN (-1);
#   endif /* PDL_HAS_THREADS */
}
# endif /* PDL_HAS_TSS_EMULATION && PDL_HAS_THREAD_SPECIFIC_STORAGE */

int
PDL_OS::thr_keycreate (PDL_thread_key_t *key,
# if defined (PDL_HAS_THR_C_DEST)
                       PDL_THR_C_DEST dest,
# else
                       PDL_THR_DEST dest,
# endif /* PDL_HAS_THR_C_DEST */
                       void *inst)
{
  // PDL_TRACE ("PDL_OS::thr_keycreate");
# if defined (PDL_HAS_THREADS)
#   if defined (PDL_HAS_TSS_EMULATION)
    if (PDL_TSS_Emulation::next_key (*key) == 0)
      {
        PDL_TSS_Emulation::tss_destructor (*key, dest);

        // Extract out the thread-specific table instance and stash away
        // the key and destructor so that we can free it up later on...
        return PDL_TSS_Cleanup::instance ()->insert (*key, dest, inst);
      }
    else
      {
        errno = EAGAIN;
        return -1;
      }
#   elif defined (PDL_HAS_PTHREADS)
    PDL_UNUSED_ARG (inst);

#     if defined (PDL_HAS_STDARG_THR_DEST)
    PDL_OSCALL_RETURN (::pthread_keycreate (key, (void (*)(...)) dest), int, -1);
#     elif defined (PDL_HAS_PTHREADS_DRAFT4)
    PDL_OSCALL_RETURN (::pthread_keycreate (key, dest), int, -1);
#     elif defined (PDL_HAS_PTHREADS_DRAFT6)
    PDL_OSCALL_RETURN (::pthread_key_create (key, dest), int, -1);
#     else
    PDL_OSCALL_RETURN (PDL_ADAPT_RETVAL (::pthread_key_create (key, dest),
                                         pdl_result_),
                       int, -1);
#     endif /* PDL_HAS_STDARG_THR_DEST */

#   elif defined (PDL_HAS_STHREADS)
    PDL_UNUSED_ARG (inst);
    PDL_OSCALL_RETURN (PDL_ADAPT_RETVAL (::thr_keycreate (key, dest),
                                         pdl_result_),
                       int, -1);
#     elif defined (PDL_PSOS) && defined (PDL_PSOS_HAS_TSS)

    static u_long unique_name = 0;
    void *tsdanchor;

    ++unique_name;
    if (::tsd_create (PDL_reinterpret_cast (char *, unique_name),
                      0, TSD_NOALLOC, &tsdanchor, key) != 0)
      {
        return -1;
      }

    return PDL_TSS_Cleanup::instance ()->insert (*key, dest, inst);
#   elif defined (PDL_HAS_WTHREADS)
    *key = ::TlsAlloc ();

    if (*key != PDL_SYSCALL_FAILED)
      {
        // Extract out the thread-specific table instance and stash away
        // the key and destructor so that we can free it up later on...
        return PDL_TSS_Cleanup::instance ()->insert (*key, dest, inst);
      }
    else
      PDL_FAIL_RETURN (-1);
      /* NOTREACHED */
#   else
    PDL_UNUSED_ARG (key);
    PDL_UNUSED_ARG (dest);
    PDL_UNUSED_ARG (inst);
    PDL_NOTSUP_RETURN (-1);
#   endif /* PDL_HAS_TSS_EMULATION */
# else
  PDL_UNUSED_ARG (key);
  PDL_UNUSED_ARG (dest);
  PDL_UNUSED_ARG (inst);
  PDL_NOTSUP_RETURN (-1);
# endif /* PDL_HAS_THREADS */
}

int
PDL_OS::thr_key_used (PDL_thread_key_t key)
{
# if defined (PDL_WIN32) || defined (PDL_HAS_TSS_EMULATION) || (defined (PDL_PSOS) && defined (PDL_PSOS_HAS_TSS))
  PDL_TSS_Cleanup::instance ()->key_used (key);
  return 0;
# else
  PDL_UNUSED_ARG (key);
  PDL_NOTSUP_RETURN (-1);
# endif /* PDL_WIN32 || PDL_HAS_TSS_EMULATION || PDL_PSOS_HAS_TSS */
}

int
PDL_OS::thr_key_detach (void *inst)
{
# if defined (PDL_WIN32) || defined (PDL_HAS_TSS_EMULATION) || (defined (PDL_PSOS) && defined (PDL_PSOS_HAS_TSS))
  if (PDL_TSS_Cleanup::lockable ())
    return PDL_TSS_Cleanup::instance()->detach (inst);
  else
    // We're in static constructor/destructor phase.  Don't
    // try to use the PDL_TSS_Cleanup instance because its lock
    // might not have been constructed yet, or might have been
    // destroyed already.  Just leak the key . . .
    return -1;
# else
  PDL_UNUSED_ARG (inst);
  PDL_NOTSUP_RETURN (-1);
# endif /* PDL_WIN32 || PDL_HAS_TSS_EMULATION */
}

void
PDL_OS::unique_name (const void *object,
                     LPTSTR name,
                     size_t length)
{
#if defined(PDL_HAS_WINCE)
    assert(0);

#else

  // The process ID will provide uniqueness between processes on the
  // same machine. The "this" pointer of the <object> will provide
  // uniqueness between other "live" objects in the same process. The
  // uniqueness of this name is therefore only valid for the life of
  // <object>.
  TCHAR temp_name[PDL_UNIQUE_NAME_LEN];
  PDL_OS::sprintf (temp_name,
                   "%lx%d",
                   object,
                   PDL_static_cast (int, PDL_OS::getpid ()));
  PDL_OS::strncpy (name,
                   temp_name,
                   length);
#endif
}

int
PDL_OS::argv_to_string (ASYS_TCHAR **argv,
                        ASYS_TCHAR *&buf,
                        int substitute_env_args)
{
  if (argv == 0 || argv[0] == 0)
    return 0;

  int buf_len = 0;

  // Determine the length of the buffer.

  for (int i = 0; argv[i] != 0; i++)
    {
      ASYS_TCHAR *temp = 0;

      // Account for environment variables.
      if (substitute_env_args
          && (argv[i][0] == '$'
              && (temp = PDL_OS::getenv (&argv[i][1])) != 0))
        buf_len += PDL_OS::strlen (temp);
      else
        buf_len += PDL_OS::strlen (argv[i]);

      // Add one for the extra space between each string.
      buf_len++;
    }

  // Step through all argv params and copy each one into buf; separate
  // each param with white space.

  PDL_NEW_RETURN (buf,
                  ASYS_TCHAR,
                  buf_len + 1,
                  0);

  // Initial null charater to make it a null string.
  buf[0] = '\0';
  ASYS_TCHAR *end = buf;
  int j;

  for (j = 0; argv[j] != 0; j++)
    {
      ASYS_TCHAR *temp = 0;

      // Account for environment variables.
      if (substitute_env_args
      && (argv[j][0] == '$'
              && (temp = PDL_OS::getenv (&argv[j][1])) != 0))
        end = PDL_OS::strecpy (end, temp);
      else
        end = PDL_OS::strecpy (end, argv[j]);

      // Replace the null char that strecpy put there with white
      // space.
      end[-1] = ' ';
    }

  // Null terminate the string.
  *end = '\0';
  // The number of arguments.
  return j;
}

// Create a contiguous command-line argument buffer with each arg
// separated by spaces.

pid_t
PDL_OS::fork_exec (ASYS_TCHAR *argv[])
{
# if defined (PDL_WIN32)
  ASYS_TCHAR *buf;

  if (PDL_OS::argv_to_string (argv, buf) != -1)
    {
      PROCESS_INFORMATION process_info;
#   if !defined (PDL_HAS_WINCE)
      STARTUPINFO startup_info;
      ::ZeroMemory (&startup_info, sizeof STARTUPINFO);
      startup_info.cb         = sizeof STARTUPINFO;
      startup_info.dwFlags    = STARTF_USESTDHANDLES;
      startup_info.hStdInput  = ::GetStdHandle( STD_INPUT_HANDLE  );
      startup_info.hStdOutput = ::GetStdHandle( STD_OUTPUT_HANDLE );
      startup_info.hStdError  = ::GetStdHandle( STD_ERROR_HANDLE  );

      if (::CreateProcess (0,
                           (LPTSTR) ASYS_ONLY_WIDE_STRING (buf),
                           0, // No process attributes.
                           0,  // No thread attributes.
                           TRUE, // Allow handle inheritance.
                           DETACHED_PROCESS, // no need to use console.
                           0, // No environment.
                           0, // No current directory.
                           &startup_info,
                           &process_info))
#   else
      if (::CreateProcess (0,
                           (LPTSTR) ASYS_ONLY_WIDE_STRING (buf),
                           0, // No process attributes.
                           0,  // No thread attributes.
                           FALSE, // Can's inherit handles on CE
                           0, // Don't create a new console window.
                           0, // No environment.
                           0, // No current directory.
                           0, // Can't use startup info on CE
                           &process_info))
#   endif /* ! PDL_HAS_WINCE */
        {
          // Free resources allocated in kernel.
          PDL_OS::close (process_info.hThread);
          PDL_OS::close (process_info.hProcess);
          // Return new process id.
          PDL_OS::free(buf);
          return process_info.dwProcessId;
        }
    }

  // CreateProcess failed.
  return -1;
# elif defined (CHORUS)
  return PDL_OS::execv (argv[0], argv);
# else
      pid_t result = PDL_OS::fork ();

      switch (result)
        {
        case -1:
          // Error.
          return -1;
        case 0:
          // Child process.
          if (PDL_OS::execv (argv[0], argv) == -1)
            {
              PDL_ERROR ((LM_ERROR,
                          "%p Exec failed\n"));

              // If the execv fails, this child needs to exit.
              PDL_OS::exit (errno);
            }
        default:
          // Server process.  The fork succeeded.
          return result;
        }
# endif /* PDL_WIN32 */
}

/* BUG-37596 */
pid_t
PDL_OS::fork_exec_close (ASYS_TCHAR *argv[])
{
# if defined (PDL_WIN32)
  return  fork_exec(argv);
# elif defined (CHORUS)
  return PDL_OS::execv (argv[0], argv);
# else
      int i = PDL::max_handles () - 1;
      pid_t result = PDL_OS::fork ();

      switch (result)
        {
        case -1:
          // Error.
          return -1;
        case 0:
          // Child process.
          // Close down the files.
          for (; i >= 3; i--) /* don't close stdin(0),stdout(1),stderr(2) */
              PDL_OS::close (i); 

          if (PDL_OS::execv (argv[0], argv) == -1)
            {
              PDL_ERROR ((LM_ERROR,
                          "%p Exec failed\n"));

              // If the execv fails, this child needs to exit.
              PDL_OS::exit (errno);
            }
        default:
          // Server process.  The fork succeeded.
          return result;
        }
# endif /* PDL_WIN32 */
}

//BUG-21164 
int
PDL_OS::isParentAlive(pid_t aPPID)
{
#if defined(PDL_WIN32) || defined(PDL_WIN64)
    HANDLE hProcess = 0x00;
    hProcess = OpenProcess(PROCESS_TERMINATE , FALSE, aPPID);
    if (hProcess == 0x00)
    {
        return FALSE;
    }
    PDL_OS::close(hProcess);
    return TRUE;
#else
    pid_t sPPID;
    sPPID = getppid();
    if(aPPID != sPPID || sPPID == 1)
    {
        return FALSE;
    }
    return TRUE;   
#endif
}

ssize_t
PDL_OS::read_n (PDL_HANDLE handle,
                void *buf,
                size_t len)
{
  size_t bytes_transferred;
  ssize_t n;

  for (bytes_transferred = 0;
       bytes_transferred < len;
       bytes_transferred += n)
    {
      n = PDL_OS::read (handle,
                        (char *) buf + bytes_transferred,
                        len - bytes_transferred);
      if (n == -1 || n == 0)
        return n;
    }

  return bytes_transferred;
}

// Write <len> bytes from <buf> to <handle> (uses the <write>
// system call on UNIX and the <WriteFile> call on Win32).

ssize_t
PDL_OS::write_n (PDL_HANDLE handle,
                 const void *buf,
                 size_t len)
{
  size_t bytes_transferred;
  ssize_t n;

  for (bytes_transferred = 0;
       bytes_transferred < len;
       bytes_transferred += n)
    {
      n = PDL_OS::write (handle,
                         (char *) buf + bytes_transferred,
                         len - bytes_transferred);
      if (n == -1 || n == 0)
        return n;
    }

  return bytes_transferred;
}

# if defined (PDL_LACKS_WRITEV)

// "Fake" writev for operating systems without it.  Note that this is
// thread-safe.

extern "C" int
writev (PDL_HANDLE handle, PDL_WRITEV_TYPE iov[], int n)
{
  PDL_TRACE ("::writev");

  size_t length = 0;
  int i;

  // Determine the total length of all the buffers in <iov>.
  for (i = 0; i < n; i++)
    if (PDL_static_cast (int, iov[i].iov_len) < 0)
      return -1;
    else
      length += iov[i].iov_len;

  char *buf;

#   if defined (PDL_HAS_ALLOCA)
  buf = (char *) alloca (length);
#   else
  PDL_NEW_RETURN (buf,
                  char,
                  length,
                  -1);
#   endif /* !defined (PDL_HAS_ALLOCA) */

  char *ptr = buf;

  for (i = 0; i < n; i++)
    {
      PDL_OS::memcpy (ptr, iov[i].iov_base, iov[i].iov_len);
      ptr += iov[i].iov_len;
    }

  ssize_t result = PDL_OS::write (handle, buf, length);
#   if !defined (PDL_HAS_ALLOCA)
  PDL_OS::free(buf);
#   endif /* !defined (PDL_HAS_ALLOCA) */
  return result;
}
# endif /* PDL_LACKS_WRITEV */

# if defined (PDL_LACKS_READV)

// "Fake" readv for operating systems without it.  Note that this is
// thread-safe.

extern "C" int
readv (PDL_HANDLE handle,
       PDL_READV_TYPE *iov,
       int n)
{
PDL_TRACE ("readv");

  ssize_t length = 0;
  int i;

  for (i = 0; i < n; i++)
    if (PDL_static_cast (int, iov[i].iov_len) < 0)
      return -1;
    else
      length += iov[i].iov_len;

  char *buf;
#   if defined (PDL_HAS_ALLOCA)
  buf = (char *) alloca (length);
#   else
  PDL_NEW_RETURN (buf,
                  char,
                  length,
                  -1);
#   endif /* !defined (PDL_HAS_ALLOCA) */

  length = PDL_OS::read (handle, buf, length);

  if (length != -1)
    {
      char *ptr = buf;
      int copyn = length;

      for (i = 0;
           i < n && copyn > 0;
           i++)
        {
          PDL_OS::memcpy (iov[i].iov_base, ptr,
                          // iov_len is int on some platforms, size_t on others
                          copyn > (int) iov[i].iov_len
                            ? (size_t) iov[i].iov_len
                            : (size_t) copyn);
          ptr += iov[i].iov_len;
          copyn -= iov[i].iov_len;
        }
    }

#   if !defined (PDL_HAS_ALLOCA)
  PDL_OS::free(buf);
#   endif /* !defined (PDL_HAS_ALLOCA) */
  return length;
}
# endif /* PDL_LACKS_READV */

# if defined (PDL_NEEDS_FTRUNCATE)
extern "C" int
ftruncate (PDL_HANDLE handle, PDL_OFF_T len)
{
  struct flock fl;
  fl.l_whence = 0;
  fl.l_len = 0;
  fl.l_start = len;
  fl.l_type = F_WRLCK;

  return PDL_OS::fcntl (handle, F_FREESP, PDL_reinterpret_cast (long, &fl));
}
# endif /* PDL_NEEDS_FTRUNCATE */

# if defined (PDL_LACKS_MKTEMP) && !defined (PDL_HAS_MOSTLY_UNICODE_APIS)
char *
PDL_OS::mktemp (char *s)
{
  PDL_TRACE ("PDL_OS::mktemp");
  if (s == 0)
    // check for null template string failed!
    return 0;
  else
    {
      char *xxxxxx = PDL_OS::strstr (s, "XXXXXX");

      if (xxxxxx == 0)
        // the template string doesn't contain "XXXXXX"!
        return s;
      else
        {
          char unique_letter = 'a';
          struct stat sb;

          // Find an unused filename for this process.  It is assumed
          // that the user will open the file immediately after
          // getting this filename back (so, yes, there is a rpdl
          // condition if multiple threads in a process use the same
          // template).  This appears to match the behavior of the
          // SunOS 5.5 mktemp().
          PDL_OS::sprintf (xxxxxx, "%05d%c", PDL_OS::getpid (), unique_letter);
          while (PDL_OS::stat (s, &sb) >= 0)
            {
              if (++unique_letter <= 'z')
                PDL_OS::sprintf (xxxxxx, "%05d%c", PDL_OS::getpid (),
                                 unique_letter);
              else
                {
                  // maximum of 26 unique files per template, per process
                  PDL_OS::sprintf (xxxxxx, "%s", "");
                  return s;
                }
            }
        }
      return s;
    }
}
# endif /* PDL_LACKS_MKTEMP && !PDL_HAS_MOSTLY_UNICODE_APIS */

int
PDL_OS::socket_init (int version_high, int version_low)
{
# if defined (PDL_WIN32)
  if (PDL_OS::socket_initialized_ == 0)
    {
      WORD version_requested = MAKEWORD (version_high, version_low);
      WSADATA wsa_data;
      int error = WSAStartup (version_requested, &wsa_data);

      if (error != 0)
#   if defined (PDL_HAS_WINCE)
        {
          wchar_t fmt[] = PDL_TEXT("%s failed, WSAGetLastError returned %d");
          wchar_t buf[80];  // @@ Eliminate magic number.

          ::wsprintf (buf,
                      fmt,
                      PDL_TEXT("WSAStartup"),
                      error);

          ::MessageBox (NULL,
                        buf,
                        PDL_TEXT("WSAStartup failed!"),
                        MB_OK);
        }
#   else
      PDL_OS::fprintf (stderr,
                       "PDL_OS::socket_init; WSAStartup failed, "
                         "WSAGetLastError returned %d\n",
                       error);
#   endif /* PDL_HAS_WINCE */

      PDL_OS::socket_initialized_ = 1;
    }
# else
  PDL_UNUSED_ARG (version_high);
  PDL_UNUSED_ARG (version_low);
# endif /* PDL_WIN32 */
  return 0;
}

int
PDL_OS::socket_fini (void)
{
# if defined (PDL_WIN32)
    if (PDL_OS::socket_initialized_ != 0)
    {
        if (WSACleanup () != 0)
        {
            int error = ::WSAGetLastError ();
#   if defined (PDL_HAS_WINCE)
            wchar_t fmt[] = PDL_TEXT("%s failed, WSAGetLastError returned %d");
            wchar_t buf[80];  // @@ Eliminate magic number.

            ::wsprintf (buf,
                        fmt,
                        PDL_TEXT("WSACleanup"),
                        error);

            ::MessageBox (NULL, buf , PDL_TEXT("WSACleanup failed!"), MB_OK);
#   else
            PDL_OS::fprintf (stderr,
                             "PDL_OS::socket_fini; WSACleanup failed, "
                             "WSAGetLastError returned %d\n",
                             error);

#   endif /* PDL_HAS_WINCE */
        }
        PDL_OS::socket_initialized_ = 0;
    }
# endif /* PDL_WIN32 */
    return 0;
}

# if defined (PDL_LACKS_SYS_NERR)
int sys_nerr = ERRMAX + 1;
# endif /* PDL_LACKS_SYS_NERR */

# if defined (VXWORKS)
#   include /**/ <usrLib.h>   /* for ::sp() */

// This global function can be used from the VxWorks shell to pass
// arguments to a C main () function.
//
// usage: -> spa main, "arg1", "arg2"
//
// All arguments must be quoted, even numbers.
int
spa (FUNCPTR entry, ...)
{
  static const unsigned int MAX_ARGS = 10;
  static char *argv[MAX_ARGS];
  va_list pvar;
  unsigned int argc;

  // Hardcode a program name because the real one isn't available
  // through the VxWorks shell.
  argv[0] = "pdl_main";

  // Peel off arguments to spa () and put into argv.  va_arg () isn't
  // necessarily supposed to return 0 when done, though since the
  // VxWorks shell uses a fixed number (10) of arguments, it might 0
  // the unused ones.  This function could be used to increase that
  // limit, but then it couldn't depend on the trailing 0.  So, the
  // number of arguments would have to be passed.
  va_start (pvar, entry);

  for (argc = 1; argc <= MAX_ARGS; ++argc)
    {
      argv[argc] = va_arg (pvar, char *);

      if (argv[argc] == 0)
        break;
    }

  if (argc > MAX_ARGS  &&  argv[argc-1] != 0)
    {
      // try to read another arg, and warn user if the limit was exceeded
      if (va_arg (pvar, char *) != 0)
        PDL_OS::fprintf (stderr, "spa(): number of arguments limited to %d\n",
                         MAX_ARGS);
    }
  else
    {
      // fill unused argv slots with 0 to get rid of leftovers
      // from previous invocations
      for (unsigned int i = argc; i <= MAX_ARGS; ++i)
        argv[i] = 0;
    }

  // The hard-coded options are what ::sp () uses, except for the
  // larger stack size (instead of ::sp ()'s 20000).
  const int ret = ::taskSpawn (argv[0],    // task name
                               100,        // task priority
                               VX_FP_TASK, // task options
                               PDL_NEEDS_HUGE_THREAD_STACKSIZE, // stack size
                               entry,      // entry point
                               argc,       // first argument to main ()
                               (int) argv, // second argument to main ()
                               0, 0, 0, 0, 0, 0, 0, 0);
  va_end (pvar);

  // ::taskSpawn () returns the taskID on success: return 0 instead if
  // successful
  return ret > 0 ? 0 : ret;
}
# endif /* VXWORKS */

# if !defined (PDL_HAS_SIGINFO_T)
siginfo_t::siginfo_t (PDL_HANDLE handle)
  : si_handle_ (handle),
    si_handles_ (&handle)
{
}

siginfo_t::siginfo_t (PDL_HANDLE *handles)
  : si_handle_ (handles[0]),
    si_handles_ (handles)
{
}
# endif /* PDL_HAS_SIGINFO_T */

pid_t
PDL_OS::fork (const char * /*_ program_name _*/)
{
  PDL_TRACE ("PDL_OS::fork");
# if defined (PDL_LACKS_FORK)
//  PDL_UNUSED_ARG (program_name);
  PDL_NOTSUP_RETURN (pid_t (-1));
# else
  pid_t pid =
# if defined (PDL_HAS_STHREADS)
    ::fork1 ();
#else
    ::fork ();
#endif /* PDL_HAS_STHREADS */

#if !defined (PDL_HAS_MINIMAL_PDL_OS)
  if (pid == 0)
    PDL_LOG_MSG->sync (program_name);
#endif /* ! PDL_HAS_MINIMAL_PDL_OS */

  return pid;
# endif /* PDL_WIN32 */
}

int
PDL_OS::inet_aton (const char *host_name, struct in_addr *addr)
{
#if defined(ITRON)
  /* empty */
  PDL_NOTSUP_RETURN (-1);
#else
  PDL_UINT32 ip_addr = PDL_OS::inet_addr (host_name);

  if (ip_addr == (PDL_UINT32) htonl ((PDL_UINT32) ~0)
      // Broadcast addresses are weird...
      && PDL_OS::strcmp (host_name, "255.255.255.255") != 0)
    return 0;
  else if (addr == 0)
    return 0;
  else
    {
#if !defined(_UNICOS)
      PDL_OS::memcpy ((void *) addr, (void *) &ip_addr, sizeof ip_addr);
#else /* ! _UNICOS */
      // on UNICOS, perform assignment to bitfield, since doing the above
      // actually puts the address outside of the 32-bit bitfield
      addr->s_addr = ip_addr;
#endif /* ! _UNICOS */
      return 1;
    }
#endif
}

struct tm *
PDL_OS::localtime_r (const time_t *t, struct tm *res)
{
  PDL_TRACE ("PDL_OS::localtime_r");
#if defined (PDL_HAS_REENTRANT_FUNCTIONS)
# if defined (DIGITAL_UNIX)
  PDL_OSCALL_RETURN (::_Plocaltime_r (t, res), struct tm *, 0);
# elif defined (HPUX_10)
  return (::localtime_r (t, res) == 0 ? res : (struct tm *)0);
# else
  PDL_OSCALL_RETURN (::localtime_r (t, res), struct tm *, 0);
# endif /* DIGITAL_UNIX */
#elif defined(PDL_WIN32)
  errno_t err = ::localtime_s (res, t);
  return (err == 0) ? res : NULL;
#elif !defined (PDL_HAS_WINCE) && !defined(PDL_PSOS) || defined (PDL_PSOS_HAS_TIME)
  PDL_OS_GUARD

  PDL_UNUSED_ARG (res);
  struct tm * res_ptr;
  PDL_OSCALL (::localtime (t), struct tm *, 0, res_ptr);
  if (res_ptr == 0)
    return 0;
  else
    {
      *res = *res_ptr;
      return res;
    }
#elif defined(PDL_HAS_WINCE)
  SYSTEMTIME tlocal;
  ::GetLocalTime(&tlocal);

  res->tm_sec  = tlocal.wSecond;
  res->tm_min  = tlocal.wMinute;
  res->tm_hour = tlocal.wHour;
  res->tm_mday = tlocal.wDay;
  res->tm_mon  = tlocal.wMonth - 1;
  res->tm_year = tlocal.wYear - 1900;

  return res;
#else
  // @@ Same as PDL_OS::localtime (), you need to implement it
  //    yourself.
  PDL_UNUSED_ARG (t);
  PDL_UNUSED_ARG (res);
  PDL_NOTSUP_RETURN (0);
#endif /* PDL_HAS_REENTRANT_FUNCTIONS */
}

#define PDL_MAX_WIN_IO_SIZE (32 * 1024 * 1024)

ssize_t
PDL_OS::pread (PDL_HANDLE handle,
               void *buf,
               size_t nbytes,
               PDL_OFF_T offset)
{
# if defined (PDL_HAS_P_READ_WRITE)
#   if defined (PDL_WIN32)

  PDL_OS_GUARD

  size_t nMustReadSize;
  DWORD  bytes_read;
  DWORD  nTotalReadBytes = 0;
  size_t nReadSize;
  LARGE_INTEGER li;
  char*  curBuffPtr;
  DWORD  original_position;
  DWORD  altered_position;
  BOOL   result;

  // Remember the original file pointer position
  PDL_WIN32CALL( ::SetFilePointer (handle,
                                   0,
                                   NULL,
                                   FILE_CURRENT),
                 DWORD,
                 0xFFFFFFFF,
                 original_position );

  if (0xFFFFFFFF == original_position)
  {
      return -1;
  }

  curBuffPtr = (char*)buf;
  nMustReadSize = nbytes;
  li.QuadPart = offset;

  /* BUG-17248: ReadFile ( Win32 API ) cannot read more than 64MB
   * in one chunk */
  while( nMustReadSize > 0 )
  {
      // Go to the correct position
      PDL_WIN32CALL( ::SetFilePointer (handle,
                                       li.LowPart,
                                       &li.HighPart,
                                       FILE_BEGIN),
                     DWORD,
                     0xFFFFFFFF,
                     altered_position );

      if (0xFFFFFFFF == altered_position && errno != NO_ERROR)
      {
          return -1;
      }

      if( nMustReadSize >= PDL_MAX_WIN_IO_SIZE )
      {
          nReadSize = PDL_MAX_WIN_IO_SIZE;
      }
      else
      {
          nReadSize = nMustReadSize;
      }

      nMustReadSize -= nReadSize;

#     if defined (PDL_HAS_WINNT4) && (PDL_HAS_WINNT4 != 0)

      OVERLAPPED overlapped;
      overlapped.Internal = 0;
      overlapped.InternalHigh = 0;
      overlapped.Offset = li.LowPart;
      overlapped.OffsetHigh = li.HighPart;
      overlapped.hEvent = 0;

      PDL_WIN32CALL( ::ReadFile (handle,
                                 (void*)curBuffPtr,
                                 PDL_static_cast (DWORD, nReadSize),
                                 &bytes_read,
                                 &overlapped),
                     BOOL,
                     FALSE,
                     result );

      if (result == FALSE)
      {
          if ( errno != ERROR_IO_PENDING)
          {
              if ( errno == ERROR_HANDLE_EOF)
              {
                  return 0;
              }

              return -1;
          }
          else
          {
              PDL_WIN32CALL( ::GetOverlappedResult (handle,
                                                    &overlapped,
                                                    &bytes_read,
                                                    TRUE),
                             BOOL,
                             FALSE,
                             result );

              if (result == FALSE)
              {
                  return -1;
              }
          }
      }

#     else /* PDL_HAS_WINNT4 && (PDL_HAS_WINNT4 != 0) */

      PDL_WIN32CALL( ::ReadFile (handle,
                                 (void*)curBuffPtr,
                                 nReadSize,
                                 &bytes_read,
                                 NULL),
                     BOOL,
                     FALSE,
                     result );

      if (result == FALSE)
      {
          return -1;
      }

#     endif /* PDL_HAS_WINNT4 && (PDL_HAS_WINNT4 != 0) */

      nTotalReadBytes += bytes_read;
      li.QuadPart += bytes_read;
      curBuffPtr += bytes_read;

      if( bytes_read != nReadSize )
      {
          /* Read를 요청한 Byte수만큼 Read가 되지않았다. 더이상 File의
             끝까지 읽기를 수행했음.*/
          break;
      }
  }
  
  // Reset the original file pointer position
  PDL_WIN32CALL( ::SetFilePointer (handle,
                                   original_position,
                                   NULL,
                                   FILE_BEGIN),
                 DWORD,
                 0xFFFFFFFF,
                 original_position );

  if( 0xFFFFFFFF == original_position )
  {
      return -1;
  }

  return (ssize_t) nTotalReadBytes;

#   else /* PDL_WIN32 */

  return ::pread (handle, buf, nbytes, offset);

#   endif /* PDL_WIN32 */

# else /* PDL_HAS_P_READ_WRITE */

  if( nbytes == 0 ) 
  { 
    return 0; 
  } 

  PDL_OS_GUARD

  // Remember the original file pointer position
  PDL_OFF_T original_position = PDL_OS::lseek (handle,
                                           0,
                                           SEEK_SET);

  if (original_position == -1)
    return -1;

  // Go to the correct position
  PDL_OFF_T altered_position = PDL_OS::lseek (handle,
                                          offset,
                                          SEEK_SET);

  if (altered_position == -1)
    return -1;

  ssize_t bytes_read = PDL_OS::read (handle,
                                     buf,
                                     nbytes);

  if (bytes_read == -1)
    return -1;

  if (PDL_OS::lseek (handle,
                     original_position,
                     SEEK_SET) == -1)
    return -1;

  return bytes_read;

# endif /* PDL_HAD_P_READ_WRITE */
}

ssize_t
PDL_OS::pwrite (PDL_HANDLE handle,
                const void *buf,
                size_t nbytes,
                PDL_OFF_T offset)
{

# if defined (PDL_HAS_P_READ_WRITE)
#   if defined (PDL_WIN32)

  size_t nMustWriteSize;
  DWORD  bytes_written;
  DWORD  nTotalWriteBytes = 0;
  LARGE_INTEGER li;
  size_t nWriteSize;
  char*  curBuffPtr;
  DWORD  original_position;
  DWORD  altered_position;
  BOOL   result;

  PDL_OS_GUARD

  /* BUG-17269: [WIN] pread, pwrite에서 error발생시 errno가
   * 0이됩니다.*/
  // Remember the original file pointer position
  PDL_WIN32CALL( ::SetFilePointer (handle,
                                   0,
                                   NULL,
                                   FILE_CURRENT),
                 DWORD,
                 0xFFFFFFFF,
                 original_position );

  if ( 0xFFFFFFFF == original_position )
  {
      PDL_OS::set_errno_to_last_error();
      return -1;
  }

  nMustWriteSize = nbytes;
  li.QuadPart = offset;
  curBuffPtr = (char*)buf;
  
  /* BUG-17248: WriteFile ( Win32 API ) cannot write more than 64MB
   * in one chunk */
  while( nMustWriteSize > 0 )
  {
      if( nMustWriteSize >= PDL_MAX_WIN_IO_SIZE )
      {
          nWriteSize = PDL_MAX_WIN_IO_SIZE;
      }
      else
      {
          nWriteSize = nMustWriteSize;
      }

      nMustWriteSize -= nWriteSize;

      // Go to the correct position
      PDL_WIN32CALL( ::SetFilePointer (handle,
                                       li.LowPart,
                                       &li.HighPart,
                                       FILE_BEGIN),
                     DWORD,
                     0xFFFFFFFF,
                     altered_position );

      if ( ( 0xFFFFFFFF == altered_position ) &&
           ( errno != NO_ERROR ))
      {
          return -1;
      }

#     if defined (PDL_HAS_WINNT4) && (PDL_HAS_WINNT4 != 0)

      OVERLAPPED overlapped;
      overlapped.Internal = 0;
      overlapped.InternalHigh = 0;
      overlapped.Offset = li.LowPart;
      overlapped.OffsetHigh = li.HighPart;
      overlapped.hEvent = 0;

      PDL_WIN32CALL( ::WriteFile (handle,
                                  (void*)curBuffPtr,
                                  PDL_static_cast (DWORD, nbytes),
                                  &bytes_written,
                                  &overlapped),
                     BOOL,
                     FALSE,
                     result );

      if (result == FALSE)
      {
          if (::GetLastError () != ERROR_IO_PENDING)
          {
              return -1;
          }

          else
          {
              PDL_WIN32CALL( ::GetOverlappedResult (handle,
                                                    &overlapped,
                                                    &bytes_written,
                                                    TRUE),
                             BOOL,
                             FALSE,
                             result );

              if (result == FALSE)
              {
                  return -1;
              }
          }
      }

#     else /* PDL_HAS_WINNT4 && (PDL_HAS_WINNT4 != 0) */

      PDL_WIN32CALL( ::WriteFile (handle,
                                  (void*)curBuffPtr,
                                  nbytes,
                                  &bytes_written,
                                  NULL),
                     BOOL,
                     FALSE,
                     result );

      if (result == FALSE)
      {
          return -1;
      }
#     endif /* PDL_HAS_WINNT4 && (PDL_HAS_WINNT4 != 0) */

      curBuffPtr += bytes_written;
      nTotalWriteBytes += bytes_written;
      li.QuadPart += bytes_written;
  }/* While */

  // Reset the original file pointer position

  PDL_WIN32CALL( ::SetFilePointer (handle,
                                   original_position,
                                   NULL,
                                   FILE_BEGIN),
                 DWORD,
                 0xFFFFFFFF,
                 original_position );

  if ( 0xFFFFFFFF == original_position )
  {
      return -1;
  }

  return (ssize_t) nTotalWriteBytes;

#   else /* PDL_WIN32 */

  return ::pwrite (handle, buf, nbytes, offset);
#   endif /* PDL_WIN32 */
# else /* PDL_HAS_P_READ_WRITE */

  PDL_OS_GUARD

  // Remember the original file pointer position
  PDL_OFF_T original_position = PDL_OS::lseek (handle,
                                           0,
                                           SEEK_SET);
  if (original_position == -1)
    return -1;

  // Go to the correct position
  PDL_OFF_T altered_position = PDL_OS::lseek (handle,
                                          offset,
                                          SEEK_SET);
  if (altered_position == -1)
    return -1;

  ssize_t bytes_written = PDL_OS::write (handle,
                                         buf,
                                         nbytes);
  if (bytes_written == -1)
    return -1;

  if (PDL_OS::lseek (handle,
                     original_position,
                     SEEK_SET) == -1)
    return -1;

  return bytes_written;
# endif /* PDL_HAD_P_READ_WRITE */
}

PDL_HANDLE
PDL_OS::open (const char           *filename,
              int                   mode,
              int                   perms,
              LPSECURITY_ATTRIBUTES sa)
{
  PDL_TRACE ("PDL_OS::open");

#if defined (PDL_WIN32)
  PDL_UNUSED_ARG (perms);

  DWORD access = GENERIC_READ;
  if (PDL_BIT_ENABLED (mode, O_WRONLY))
    access = GENERIC_WRITE;
  else if (PDL_BIT_ENABLED (mode, O_RDWR))
    access = GENERIC_READ | GENERIC_WRITE;

  DWORD creation = OPEN_EXISTING;

  if ((mode & (_O_CREAT | _O_EXCL)) == (_O_CREAT | _O_EXCL))
    creation = CREATE_NEW;
  else if ((mode & (_O_CREAT | _O_TRUNC)) == (_O_CREAT | _O_TRUNC))
    creation = CREATE_ALWAYS;
  else if (PDL_BIT_ENABLED (mode, _O_CREAT))
    creation = OPEN_ALWAYS;
  else if (PDL_BIT_ENABLED (mode, _O_TRUNC))
    creation = TRUNCATE_EXISTING;

  DWORD flags = 0;

  if (PDL_BIT_ENABLED (mode, _O_TEMPORARY))
    flags |= FILE_FLAG_DELETE_ON_CLOSE | FILE_ATTRIBUTE_TEMPORARY;

#if !defined (PDL_HAS_WINCE)
  if (PDL_BIT_ENABLED (mode, O_DIRECTIO)) //hjohn add
    flags |= FILE_FLAG_NO_BUFFERING;
#endif

  if (PDL_BIT_ENABLED (mode, FILE_FLAG_WRITE_THROUGH))
    flags |= FILE_FLAG_WRITE_THROUGH;
  if (PDL_BIT_ENABLED (mode, FILE_FLAG_OVERLAPPED))
    flags |= FILE_FLAG_OVERLAPPED;
  if (PDL_BIT_ENABLED (mode, FILE_FLAG_NO_BUFFERING))
    flags |= FILE_FLAG_NO_BUFFERING;
  if (PDL_BIT_ENABLED (mode, FILE_FLAG_RANDOM_ACCESS))
    flags |= FILE_FLAG_RANDOM_ACCESS;
  if (PDL_BIT_ENABLED (mode, FILE_FLAG_SEQUENTIAL_SCAN))
    flags |= FILE_FLAG_SEQUENTIAL_SCAN;
  if (PDL_BIT_ENABLED (mode, FILE_FLAG_DELETE_ON_CLOSE))
    flags |= FILE_FLAG_DELETE_ON_CLOSE;
  if (PDL_BIT_ENABLED (mode, FILE_FLAG_BACKUP_SEMANTICS))
    flags |= FILE_FLAG_BACKUP_SEMANTICS;
  if (PDL_BIT_ENABLED (mode, FILE_FLAG_POSIX_SEMANTICS))
    flags |= FILE_FLAG_POSIX_SEMANTICS;

  PDL_MT (PDL_thread_mutex_t *pdl_os_monitor_lock = 0;)

  if (PDL_BIT_ENABLED (mode, _O_APPEND))
    {
      PDL_MT
        (
          pdl_os_monitor_lock = (PDL_thread_mutex_t *)
            PDL_OS_Object_Manager::preallocated_object[
              PDL_OS_Object_Manager::PDL_OS_MONITOR_LOCK];
          PDL_OS::thread_mutex_lock (pdl_os_monitor_lock);
        )
    }

  DWORD shared_mode = FILE_SHARE_READ | FILE_SHARE_WRITE;
  if (PDL_OS::get_win32_versioninfo().dwPlatformId ==
      VER_PLATFORM_WIN32_NT)
    shared_mode |= FILE_SHARE_DELETE;

#if defined (PDL_HAS_WINCE)
    wchar_t *wFilename;
    wFilename = PDL_Ascii_To_Wide::convert(filename);

    PDL_HANDLE h = ::CreateFile (wFilename, access,
                                shared_mode,
                                PDL_OS::default_win32_security_attributes (sa),
                                creation,
                                flags,
                                0);
    delete []wFilename;

#else

  PDL_HANDLE h = ::CreateFileA (filename, access,
                                shared_mode,
                                PDL_OS::default_win32_security_attributes (sa),
                                creation,
                                flags,
                                0);
#endif /* PDL_HAS_WINCE */

  if (PDL_BIT_ENABLED (mode, _O_APPEND))
    {
      if (h != PDL_INVALID_HANDLE)
        {
          ::SetFilePointer (h, 0, 0, FILE_END);
        }

      PDL_MT (PDL_OS::thread_mutex_unlock (pdl_os_monitor_lock);)
    }

  if (h == PDL_INVALID_HANDLE)
    PDL_FAIL_RETURN (h);
  else
    return h;
#elif defined (PDL_PSOS)
  PDL_UNUSED_ARG (perms);
  PDL_UNUSED_ARG (sa);
# if defined (PDL_PSOS_LACKS_PHILE)
  PDL_UNUSED_ARG (filename);
  return 0;
# else
  unsigned long result, handle;
  result = ::open_f (&handle, PDL_const_cast(char *, filename), 0);
  if (result != 0)
    {
      // We need to clean this up...not 100% correct!
      // To correct we should handle all the cases of TRUNC and CREAT
      if ((result == 0x200B) && (PDL_BIT_ENABLED (mode, O_CREAT)))
        {
          result = ::create_f(PDL_const_cast(char *, filename),1,0);
          if (result != 0)
            {
              errno = result;
              return PDL_static_cast (PDL_HANDLE, -1);
            }
          else  //File created...try to open it again
            {
              result = ::open_f (&handle, PDL_const_cast(char *, filename), 0);
              if (result != 0)
                {
                  errno = result;
                  return PDL_static_cast (PDL_HANDLE, -1);
                }

            }
        }
      else
        {
          errno = result;
          return PDL_static_cast (PDL_HANDLE, -1);
        }
    }
  return PDL_static_cast (PDL_HANDLE, handle);
# endif /* defined (PDL_PSOS_LACKS_PHILE) */
#else
  PDL_UNUSED_ARG (sa);
  PDL_OSCALL_RETURN (::open (filename, mode, perms), PDL_HANDLE, -1);
#endif /* PDL_WIN32 */
}

#if defined (PDL_HAS_UNICODE) && defined (PDL_WIN32)

PDL_HANDLE
PDL_OS::open (const wchar_t *filename,
              int mode,
              int perms,
              LPSECURITY_ATTRIBUTES sa)
{
  PDL_UNUSED_ARG (perms);
  PDL_TRACE ("PDL_OS::open");

  // Warning: This function ignores _O_APPEND
  DWORD access = GENERIC_READ;
  if (PDL_BIT_ENABLED (mode, O_WRONLY))
    access = GENERIC_WRITE;
  else if (PDL_BIT_ENABLED (mode, O_RDWR))
    access = GENERIC_READ | GENERIC_WRITE;

  DWORD creation = OPEN_EXISTING;

  if ((mode & (_O_CREAT | _O_EXCL)) == (_O_CREAT | _O_EXCL))
    creation = CREATE_NEW;
  else if ((mode & (_O_CREAT | _O_TRUNC)) == (_O_CREAT | _O_TRUNC))
    creation = CREATE_ALWAYS;
  else if (PDL_BIT_ENABLED (mode, _O_CREAT))
    creation = OPEN_ALWAYS;
  else if (PDL_BIT_ENABLED (mode, _O_TRUNC))
    creation = TRUNCATE_EXISTING;

  DWORD flags = 0;

  if (PDL_BIT_ENABLED (mode, _O_TEMPORARY))
    flags |= FILE_FLAG_DELETE_ON_CLOSE | FILE_ATTRIBUTE_TEMPORARY;

  if (PDL_BIT_ENABLED (mode, FILE_FLAG_WRITE_THROUGH))
    flags |= FILE_FLAG_WRITE_THROUGH;
  if (PDL_BIT_ENABLED (mode, FILE_FLAG_OVERLAPPED))
    flags |= FILE_FLAG_OVERLAPPED;
  if (PDL_BIT_ENABLED (mode, FILE_FLAG_NO_BUFFERING))
    flags |= FILE_FLAG_NO_BUFFERING;
  if (PDL_BIT_ENABLED (mode, FILE_FLAG_RANDOM_ACCESS))
    flags |= FILE_FLAG_RANDOM_ACCESS;
  if (PDL_BIT_ENABLED (mode, FILE_FLAG_SEQUENTIAL_SCAN))
    flags |= FILE_FLAG_SEQUENTIAL_SCAN;
  if (PDL_BIT_ENABLED (mode, FILE_FLAG_DELETE_ON_CLOSE))
    flags |= FILE_FLAG_DELETE_ON_CLOSE;
  if (PDL_BIT_ENABLED (mode, FILE_FLAG_BACKUP_SEMANTICS))
    flags |= FILE_FLAG_BACKUP_SEMANTICS;
  if (PDL_BIT_ENABLED (mode, FILE_FLAG_POSIX_SEMANTICS))
    flags |= FILE_FLAG_POSIX_SEMANTICS;

  PDL_MT (PDL_thread_mutex_t *pdl_os_monitor_lock = 0;)

  if (PDL_BIT_ENABLED (mode, _O_APPEND))
    {
      PDL_MT
        (
          pdl_os_monitor_lock = (PDL_thread_mutex_t *)
            PDL_OS_Object_Manager::preallocated_object[
              PDL_OS_Object_Manager::PDL_OS_MONITOR_LOCK];
          PDL_OS::thread_mutex_lock (pdl_os_monitor_lock);
        )
    }

  DWORD shared_mode = FILE_SHARE_READ | FILE_SHARE_WRITE;
  if (PDL_OS::get_win32_versioninfo().dwPlatformId ==
      VER_PLATFORM_WIN32_NT)
    shared_mode |= FILE_SHARE_DELETE;

  PDL_HANDLE h = ::CreateFileW (filename, access,
                                shared_mode,
                                PDL_OS::default_win32_security_attributes (sa),
                                creation,
                                flags,
                                0);

  if (PDL_BIT_ENABLED (mode, _O_APPEND))
    {
      if (h != PDL_INVALID_HANDLE)
        {
          ::SetFilePointer (h, 0, 0, FILE_END);
        }

      PDL_MT (PDL_OS::thread_mutex_unlock (pdl_os_monitor_lock);)
    }

  if (h == PDL_INVALID_HANDLE)
    PDL_FAIL_RETURN (h);
  else
    return h;
}

#endif /* PDL_HAS_UNICODE && PDL_WIN32 */

# if defined (PDL_LACKS_DIFFTIME)
double
PDL_OS::difftime (time_t t1, time_t t0)
{
  /* return t1 - t0 in seconds */
  struct tm tms[2], *ptms[2], temp;
  double seconds;
  double days;
  int swap = 0;

  /* extract the tm structure from time_t */
  ptms[1] = gmtime_r (&t1, &tms[1]);
  if (ptms[1] == 0) return 0.0;

  ptms[0] = gmtime_r (&t0, &tms[0]);
  if (ptms[0] == 0) return 0.0;

    /* make sure t1 is > t0 */
  if (tms[1].tm_year < tms[0].tm_year)
    swap = 1;
  else if (tms[1].tm_year == tms[0].tm_year)
    {
      if (tms[1].tm_yday < tms[0].tm_yday)
        swap = 1;
      else if (tms[1].tm_yday == tms[0].tm_yday)
        {
          if (tms[1].tm_hour < tms[0].tm_hour)
            swap = 1;
          else if (tms[1].tm_hour == tms[0].tm_hour)
            {
              if (tms[1].tm_min < tms[0].tm_min)
                swap = 1;
              else if (tms[1].tm_min == tms[0].tm_min)
                {
                  if (tms[1].tm_sec < tms[0].tm_sec)
                    swap = 1;
                }
            }
        }
    }

  if (swap)
    temp = tms[0], tms[0] = tms[1], tms[1] = temp;

  seconds = 0.0;
  if (tms[1].tm_year > tms[0].tm_year)
    {
      // Accumulate the time until t[0] catches up to t[1]'s year.
      seconds = 60 - tms[0].tm_sec;
      tms[0].tm_sec = 0;
      tms[0].tm_min += 1;
      seconds += 60 * (60 - tms[0].tm_min);
      tms[0].tm_min = 0;
      tms[0].tm_hour += 1;
      seconds += 60*60 * (24 - tms[0].tm_hour);
      tms[0].tm_hour = 0;
      tms[0].tm_yday += 1;

#   define ISLEAPYEAR(y) ((y)&3u?0:(y)%25u?1:(y)/25u&12?0:1)

      if (ISLEAPYEAR(tms[0].tm_year))
        seconds += 60*60*24 * (366 - tms[0].tm_yday);
      else
        seconds += 60*60*24 * (365 - tms[0].tm_yday);

      tms[0].tm_yday = 0;
      tms[0].tm_year += 1;

      while (tms[1].tm_year > tms[0].tm_year)
        {
          if (ISLEAPYEAR(tms[0].tm_year))
            seconds += 60*60*24 * 366;
          else
            seconds += 60*60*24 * 365;

          tms[0].tm_year += 1;
        }

#   undef ISLEAPYEAR

    }
  else
    {
      // Normalize
      if (tms[1].tm_sec < tms[0].tm_sec)
        {
          if (tms[1].tm_min == 0)
            {
              if (tms[1].tm_hour == 0)
                {
                  tms[1].tm_yday -= 1;
                  tms[1].tm_hour += 24;
                }
              tms[1].tm_hour -= 1;
              tms[1].tm_min += 60;
            }
          tms[1].tm_min -= 1;
          tms[1].tm_sec += 60;
        }
      tms[1].tm_sec -= tms[0].tm_sec;

      if (tms[1].tm_min < tms[0].tm_min)
        {
          if (tms[1].tm_hour == 0)
            {
              tms[1].tm_yday -= 1;
              tms[1].tm_hour += 24;
            }
          tms[1].tm_hour -= 1;
          tms[1].tm_min += 60;
        }
      tms[1].tm_min -= tms[0].tm_min;

      if (tms[1].tm_hour < tms[0].tm_hour)
        {
          tms[1].tm_yday -= 1;
          tms[1].tm_hour += 24;
        }
      tms[1].tm_hour -= tms[0].tm_hour;

      tms[1].tm_yday -= tms[0].tm_yday;
    }

  // accumulate the seconds
  seconds += tms[1].tm_sec;
  seconds += 60 * tms[1].tm_min;
  seconds += 60*60 * tms[1].tm_hour;
  seconds += 60*60*24 * tms[1].tm_yday;

  return seconds;
}
# endif /* PDL_LACKS_DIFFTIME */

# if defined (PDL_HAS_MOSTLY_UNICODE_APIS)

#if defined (PDL_HAS_WINCE)
char* PDL_OS::ctime (const time_t *t)
#else
wchar_t * PDL_OS::ctime (const time_t *t)
#endif
{
#if defined (PDL_HAS_WINCE)
  char buf[26];              // 26 is a "magic number" ;
  return PDL_OS::ctime_r (t, buf, 26);
#else
  PDL_OSCALL_RETURN (::_wctime (t), wchar_t *, 0);
#endif /* PDL_HAS_WINCE */
}

#if !defined (PDL_HAS_WINCE)
wchar_t *
PDL_OS::ctime_r (const time_t *clock,
                 wchar_t      *buf,
                 int           buflen)
{
#if !defined (PDL_HAS_WINCE)
  wchar_t *result;
  PDL_OSCALL (::_wctime (clock), wchar_t *, 0, result);
  if (result != 0)
    ::wcsncpy (buf, result, buflen);
  return buf;
#else
  // buflen must be at least 26 wchar_t long.
  if (buflen < 26)              // Again, 26 is a magic number.
    return 0;
  // This is really stupid, converting FILETIME to timeval back and
  // forth.  It assumes FILETIME and DWORDLONG are the same structure
  // internally.
  ULARGE_INTEGER _100ns;
  _100ns.QuadPart = (DWORDLONG) *clock * 10000 * 1000
                     + PDL_Time_Value::FILETIME_to_timval_skew;
  FILETIME file_time;
  file_time.dwLowDateTime = _100ns.LowPart;
  file_time.dwHighDateTime = _100ns.HighPart;

#   if 1
  FILETIME localtime;
  SYSTEMTIME systime;
  FileTimeToLocalFileTime (&file_time, &localtime);
  FileTimeToSystemTime (&localtime, &systime);
#   else
  SYSTEMTIME systime;
  FileTimeToSystemTime ((FILETIME *) &file_time, &systime);
#   endif /* 0 */
  PDL_OS::sprintf (buf, PDL_OS_CTIME_R_FMTSTR,
                   PDL_OS::day_of_week_name[systime.wDayOfWeek],
                   PDL_OS::month_name[systime.wMonth - 1],
                   systime.wDay,
                   systime.wHour,
                   systime.wMinute,
                   systime.wSecond,
                   systime.wYear);
  return buf;
#endif /* PDL_HAS_WINCE */
}
#endif /* PDL_HAS_WINCE */
# endif /* PDL_HAS_MOSTLY_UNICODE_APIS */

time_t
PDL_OS::mktime (struct tm *t)
{
  PDL_TRACE ("PDL_OS::mktime");
#   if defined (PDL_PSOS) && ! defined (PDL_PSOS_HAS_TIME)
  PDL_UNUSED_ARG (t);
  PDL_NOTSUP_RETURN (-1);
#   elif defined (PDL_HAS_WINCE)
  SYSTEMTIME    t_sys;
  FILETIME      t_file;
  t_sys.wSecond    = t->tm_sec;
  t_sys.wMinute    = t->tm_min;
  t_sys.wHour      = t->tm_hour;
  t_sys.wDay       = t->tm_mday;
  t_sys.wMonth     = t->tm_mon + 1;     // SYSTEMTIME is 1-indexed, tm is 0-indexed
  t_sys.wYear      = t->tm_year + 1900; // SYSTEMTIME is real; tm is since 1900
  t_sys.wDayOfWeek = t->tm_wday;        // Ignored in below function call.
  if (SystemTimeToFileTime (&t_sys, &t_file) == 0)
    return -1;
  PDL_Time_Value tv;
  tv.initialize (t_file);
  return tv.sec ();
#   else
#     if defined (PDL_HAS_THREADS)  &&  !defined (PDL_HAS_MT_SAFE_MKTIME)
  PDL_OS_GUARD
#     endif /* PDL_HAS_THREADS  &&  ! PDL_HAS_MT_SAFE_MKTIME */

  PDL_OSCALL_RETURN (::mktime (t), time_t, (time_t) -1);
#   endif /* PDL_PSOS && ! PDL_PSOS_HAS_TIME */
}

# if !defined (PDL_HAS_THREADS) || defined (PDL_LACKS_RWLOCK_T)
int
PDL_OS::rwlock_init (PDL_rwlock_t *rw,
                     int           type,
                     LPCTSTR       name,
                     void         *arg)
{
#   if defined (PDL_HAS_THREADS) && defined (PDL_LACKS_RWLOCK_T)
  // NT, POSIX, and VxWorks don't support this natively.
  PDL_UNUSED_ARG (name);
  int result = -1;

  // Since we cannot use the user specified name for all three
  // objects, we will create three completely new names.
  TCHAR name1[PDL_UNIQUE_NAME_LEN+1];
  TCHAR name2[PDL_UNIQUE_NAME_LEN+1];
  TCHAR name3[PDL_UNIQUE_NAME_LEN+1];
  TCHAR name4[PDL_UNIQUE_NAME_LEN+1];

  PDL_OS::unique_name ((const void *) &rw->lock_,
                       name1,
                       PDL_UNIQUE_NAME_LEN);
  PDL_OS::unique_name ((const void *) &rw->waiting_readers_,
                       name2,
                       PDL_UNIQUE_NAME_LEN);
  PDL_OS::unique_name ((const void *) &rw->waiting_writers_,
                       name3,
                       PDL_UNIQUE_NAME_LEN);
  PDL_OS::unique_name ((const void *) &rw->waiting_important_writer_,
                       name4,
                       PDL_UNIQUE_NAME_LEN);

  /*[BUG-46055]
   * string just has address and pid. 
   * therefore, the length of string can't exceed PDL_UNIQUE_NAME_LEN(100). 
   * I add following code just in case...
   */
  name1[PDL_UNIQUE_NAME_LEN]= '\0'; 
  name2[PDL_UNIQUE_NAME_LEN]= '\0'; 
  name3[PDL_UNIQUE_NAME_LEN]= '\0'; 
  name4[PDL_UNIQUE_NAME_LEN]= '\0'; 

  PDL_condattr_t attributes;
  if (PDL_OS::condattr_init (attributes, type) == 0)
    {
      if (PDL_OS::mutex_init (&rw->lock_, type, name1, arg) == 0
          && PDL_OS::cond_init (&rw->waiting_readers_,
                                attributes, name2, arg) == 0
          && PDL_OS::cond_init (&rw->waiting_writers_,
                                attributes, name3, arg) == 0
          && PDL_OS::cond_init (&rw->waiting_important_writer_,
                                attributes, name4, arg) == 0)
        {
          // Success!
          rw->ref_count_ = 0;
          rw->num_waiting_writers_ = 0;
          rw->num_waiting_readers_ = 0;
          rw->important_writer_ = 0;
          result = 0;
        }
      PDL_OS::condattr_destroy (attributes);
    }

  if (result == -1)
    {
      // Save/restore errno.
      PDL_Errno_Guard error ((PDL_ERRNO_TYPE&)errno);
      PDL_OS::mutex_destroy (&rw->lock_);
      PDL_OS::cond_destroy (&rw->waiting_readers_);
      PDL_OS::cond_destroy (&rw->waiting_writers_);
      PDL_OS::cond_destroy (&rw->waiting_important_writer_);
    }
  return result;
#   else
  PDL_UNUSED_ARG (rw);
  PDL_UNUSED_ARG (type);
  PDL_UNUSED_ARG (name);
  PDL_UNUSED_ARG (arg);
  PDL_NOTSUP_RETURN (-1);
#   endif /* PDL_HAS_THREADS */
}
# endif /* ! PDL_HAS_THREADS || PDL_LACKS_RWLOCK_T */

#if defined (PDL_LACKS_COND_T) && ! defined (PDL_PSOS_DIAB_MIPS)
// NOTE: The PDL_OS::cond_* functions for some non-Unix platforms are
// defined here either because they're too big to be inlined, or
// to avoid use before definition if they were inline.

int
PDL_OS::cond_destroy (PDL_cond_t *cv)
{
  PDL_TRACE ("PDL_OS::cond_destroy");
# if defined (PDL_HAS_THREADS)
#   if defined (PDL_HAS_WTHREADS)
  PDL_OS::event_destroy (&cv->waiters_done_);
#   elif defined (VXWORKS) || defined (PDL_PSOS)
  PDL_OS::sema_destroy (&cv->waiters_done_);
#   endif /* VXWORKS */
  PDL_OS::thread_mutex_destroy (&cv->waiters_lock_);
  return PDL_OS::sema_destroy (&cv->sema_);
# else
  PDL_UNUSED_ARG (cv);
  PDL_NOTSUP_RETURN (-1);
# endif /* PDL_HAS_THREADS */
}

// @@ The following functions could be inlined if i could figure where
// to put it among the #ifdefs!
int
PDL_OS::condattr_init (PDL_condattr_t &attributes,
                       int type)
{
  attributes.type = type;
  return 0;
}

int
PDL_OS::condattr_destroy (PDL_condattr_t &)
{
  return 0;
}

int
PDL_OS::cond_init (PDL_cond_t      *cv,
                   PDL_condattr_t  &attributes,
                   LPCTSTR          name,
                   void            *arg)
{
  return PDL_OS::cond_init (cv, attributes.type, name, arg);
}

int
PDL_OS::cond_init (PDL_cond_t      *cv,
                   short            type,
                   LPCTSTR          name,
                   void            *arg)
{
PDL_TRACE ("PDL_OS::cond_init");
# if defined (PDL_HAS_THREADS)
  cv->waiters_ = 0;
  cv->was_broadcast_ = 0;

  int result = 0;
  if (PDL_OS::sema_init (&cv->sema_, 0, type, name, arg) == -1)
    result = -1;
  else if (PDL_OS::thread_mutex_init (&cv->waiters_lock_) == -1)
    result = -1;
#   if defined (VXWORKS) || defined (PDL_PSOS) || defined(ITRON)
  else if (PDL_OS::sema_init (&cv->waiters_done_, 0, type) == -1)
#   else
  else if (PDL_OS::event_init (&cv->waiters_done_) == -1)
#   endif /* VXWORKS */
    result = -1;
  return result;
# else
  PDL_UNUSED_ARG (cv);
  PDL_UNUSED_ARG (type);
  PDL_UNUSED_ARG (name);
  PDL_UNUSED_ARG (arg);
  PDL_NOTSUP_RETURN (-1);
# endif /* PDL_HAS_THREADS */
}

int
PDL_OS::cond_signal (PDL_cond_t *cv)
{
PDL_TRACE ("PDL_OS::cond_signal");
# if defined (PDL_HAS_THREADS)
  // If there aren't any waiters, then this is a no-op.  Note that
  // this function *must* be called with the <external_mutex> held
  // since other wise there is a rpdl condition that can lead to the
  // lost wakeup bug...  This is needed to ensure that the <waiters_>
  // value is not in an inconsistent internal state while being
  // updated by another thread.
  PDL_OS::thread_mutex_lock (&cv->waiters_lock_);
  int have_waiters = cv->waiters_ > 0;
  PDL_OS::thread_mutex_unlock (&cv->waiters_lock_);

  if (have_waiters != 0)
    return PDL_OS::sema_post (&cv->sema_);
  else
    return 0; // No-op
# else
  PDL_UNUSED_ARG (cv);
  PDL_NOTSUP_RETURN (-1);
# endif /* PDL_HAS_THREADS */
}

int
PDL_OS::cond_broadcast (PDL_cond_t *cv)
{
PDL_TRACE ("PDL_OS::cond_broadcast");
# if defined (PDL_HAS_THREADS)
  // The <external_mutex> must be locked before this call is made.

  // This is needed to ensure that <waiters_> and <was_broadcast_> are
  // consistent relative to each other.
  PDL_OS::thread_mutex_lock (&cv->waiters_lock_);
  int have_waiters = 0;

  if (cv->waiters_ > 0)
    {
      // We are broadcasting, even if there is just one waiter...
      // Record the fact that we are broadcasting.  This helps the
      // cond_wait() method know how to optimize itself.  Be sure to
      // set this with the <waiters_lock_> held.
      cv->was_broadcast_ = 1;
      have_waiters = cv->waiters_;
    }
  PDL_OS::thread_mutex_unlock (&cv->waiters_lock_);
  int result = 0;

  /* BUG-18710: [WIN] cond_signal함수가 오동작 합니다.
   *
   * sema_post의 두번째 인자가 0이 되는 경우가 존재합니다.
   * 이 경우는 cv->waiters_값을 Mutex를 잡지 않은 상태에서 읽기때문에
   * 발생합니다.
   */
  if (have_waiters > 0)
    {
      // Wake up all the waiters.
      if (PDL_OS::sema_post (&cv->sema_, have_waiters) == -1)
        result = -1;
      // Wait for all the awakened threads to acquire their part of
      // the counting semaphore.
#   if defined (VXWORKS) || defined (PDL_PSOS) || defined(ITRON)
      else if (PDL_OS::sema_wait (&cv->waiters_done_) == -1)
#   else
      else if (PDL_OS::event_wait (&cv->waiters_done_) == -1)
#   endif /* VXWORKS */
        result = -1;
      // This is okay, even without the <waiters_lock_> held because
      // no other waiter threads can wake up to access it.
      cv->was_broadcast_ = 0;
    }
  return result;
# else
  PDL_UNUSED_ARG (cv);
  PDL_NOTSUP_RETURN (-1);
# endif /* PDL_HAS_THREADS */
}

int
PDL_OS::cond_wait (PDL_cond_t *cv,
                   PDL_mutex_t *external_mutex)
{
  PDL_TRACE ("PDL_OS::cond_wait");
# if defined (PDL_HAS_THREADS)
  // Prevent rpdl conditions on the <waiters_> count.
  PDL_OS::thread_mutex_lock (&cv->waiters_lock_);
  cv->waiters_++;
  PDL_OS::thread_mutex_unlock (&cv->waiters_lock_);

  int result = 0;

#   if defined (PDL_HAS_SIGNAL_OBJECT_AND_WAIT)
  if (external_mutex->type_ == USYNC_PROCESS)
    // This call will automatically release the mutex and wait on the semaphore.
    PDL_WIN32CALL (PDL_ADAPT_RETVAL (::SignalObjectAndWait (external_mutex->proc_mutex_,
                                                            cv->sema_, INFINITE, FALSE),
                                     result),
                   int, -1, result);
  else
#   endif /* PDL_HAS_SIGNAL_OBJECT_AND_WAIT */
    {
      // We keep the lock held just long enough to increment the count of
      // waiters by one.  Note that we can't keep it held across the call
      // to PDL_OS::sema_wait() since that will deadlock other calls to
      // PDL_OS::cond_signal().
      if (PDL_OS::mutex_unlock (external_mutex) != 0)
        return -1;

      // Wait to be awakened by a PDL_OS::cond_signal() or
      // PDL_OS::cond_broadcast().
      result = PDL_OS::sema_wait (&cv->sema_);
    }

  // Reacquire lock to avoid rpdl conditions on the <waiters_> count.
  PDL_OS::thread_mutex_lock (&cv->waiters_lock_);

  // We're ready to return, so there's one less waiter.
  cv->waiters_--;

  int last_waiter = cv->was_broadcast_ && cv->waiters_ == 0;

  // Release the lock so that other collaborating threads can make
  // progress.
  PDL_OS::thread_mutex_unlock (&cv->waiters_lock_);

  if (result == -1)
    // Bad things happened, so let's just return below.
    /* NOOP */;
#   if defined (PDL_HAS_SIGNAL_OBJECT_AND_WAIT)
  else if (external_mutex->type_ == USYNC_PROCESS)
    {
      if (last_waiter)

        // This call atomically signals the <waiters_done_> event and
        // waits until it can acquire the mutex.  This is important to
        // prevent unfairness.
        PDL_WIN32CALL (PDL_ADAPT_RETVAL (::SignalObjectAndWait (cv->waiters_done_,
                                                                external_mutex->proc_mutex_,
                                                                INFINITE, FALSE),
                                         result),
                       int, -1, result);
      else
        // We must always regain the <external_mutex>, even when
        // errors occur because that's the guarantee that we give to
        // our callers.
        PDL_OS::mutex_lock (external_mutex);

      return result;
      /* NOTREACHED */
    }
#   endif /* PDL_HAS_SIGNAL_OBJECT_AND_WAIT */
  // If we're the last waiter thread during this particular broadcast
  // then let all the other threads proceed.
  else if (last_waiter)
#   if defined (VXWORKS) || defined (PDL_PSOS) || defined(ITRON)
    PDL_OS::sema_post (&cv->waiters_done_);
#   else
    PDL_OS::event_signal (&cv->waiters_done_);
#   endif /* VXWORKS */

  // We must always regain the <external_mutex>, even when errors
  // occur because that's the guarantee that we give to our callers.
  PDL_OS::mutex_lock (external_mutex);

  return result;
# else
  PDL_UNUSED_ARG (cv);
  PDL_UNUSED_ARG (external_mutex);
  PDL_NOTSUP_RETURN (-1);
# endif /* PDL_HAS_THREADS */
}

int
PDL_OS::cond_timedwait (PDL_cond_t *cv,
                        PDL_mutex_t *external_mutex,
                        PDL_Time_Value *timeout)
{
  PDL_TRACE ("PDL_OS::cond_timedwait");
# if defined (PDL_HAS_THREADS)
  // Handle the easy case first.
  if (timeout == 0)
    return PDL_OS::cond_wait (cv, external_mutex);
#   if defined (PDL_HAS_WTHREADS) || defined (VXWORKS) || defined (PDL_PSOS)

  // Prevent rpdl conditions on the <waiters_> count.
  PDL_OS::thread_mutex_lock (&cv->waiters_lock_);
  cv->waiters_++;
  PDL_OS::thread_mutex_unlock (&cv->waiters_lock_);

  int result = 0;
  PDL_Errno_Guard error ((PDL_ERRNO_TYPE&)errno, 0);
  int msec_timeout;

  if (timeout->sec () == 0 && timeout->usec () == 0)
    msec_timeout = 0; // Do a "poll."
  else
    {
      // Note that we must convert between absolute time (which is
      // passed as a parameter) and relative time (which is what
      // WaitForSingleObjects() expects).
      PDL_Time_Value relative_time;
      relative_time = *timeout - PDL_OS::gettimeofday ();

      // Watchout for situations where a context switch has caused the
      // current time to be > the timeout.
      PDL_Time_Value PDL_Time_Value_zero;
      PDL_Time_Value_zero.initialize();
      if (relative_time < PDL_Time_Value_zero)
        msec_timeout = 0;
      else
        msec_timeout = relative_time.msec ();
    }

#     if defined (PDL_HAS_SIGNAL_OBJECT_AND_WAIT)
  if (external_mutex->type_ == USYNC_PROCESS)
    // This call will automatically release the mutex and wait on the
    // semaphore.
    result = ::SignalObjectAndWait (external_mutex->proc_mutex_,
                                    cv->sema_,
                                    msec_timeout,
                                    FALSE);
  else
#     endif /* PDL_HAS_SIGNAL_OBJECT_AND_WAIT */
    {
      // We keep the lock held just long enough to increment the count
      // of waiters by one.  Note that we can't keep it held across
      // the call to WaitForSingleObject since that will deadlock
      // other calls to PDL_OS::cond_signal().
      if (PDL_OS::mutex_unlock (external_mutex) != 0)
        return -1;

      // Wait to be awakened by a PDL_OS::signal() or
      // PDL_OS::broadcast().
#     if defined (PDL_WIN32)
#       if !defined (PDL_USES_WINCE_SEMA_SIMULATION)
      result = ::WaitForSingleObject (cv->sema_, msec_timeout);
#       else /* PDL_USES_WINCE_SEMA_SIMULATION */
      // Can't use Win32 API on our simulated semaphores.
      PDL_Time_Value sPDL_Time_Value;
      sPDL_Time_Value.initialize(0, msec_timeout * 1000);
      result = PDL_OS::sema_wait (&cv->sema_, sPDL_Time_Value);
#       endif /* PDL_USES_WINCE_SEMA_SIMULATION */
#     elif defined (PDL_PSOS)
      // Inline the call to PDL_OS::sema_wait () because it takes an
      // PDL_Time_Value argument.  Avoid the cost of that conversion . . .
      u_long ticks = (KC_TICKS2SEC * msec_timeout) / PDL_ONE_SECOND_IN_MSECS;
      //Tick set to 0 tells pSOS to wait forever is SM_WAIT is set.
      if(ticks == 0)
        result = ::sm_p (cv->sema_.sema_, SM_NOWAIT, ticks); //no timeout
      else
        result = ::sm_p (cv->sema_.sema_, SM_WAIT, ticks);
#     elif defined (VXWORKS)
      // Inline the call to PDL_OS::sema_wait () because it takes an
      // PDL_Time_Value argument.  Avoid the cost of that conversion . . .
      int ticks_per_sec = ::sysClkRateGet ();
      int ticks = msec_timeout * ticks_per_sec / PDL_ONE_SECOND_IN_MSECS;
      result = ::semTake (cv->sema_.sema_, ticks);
#     endif /* PDL_WIN32 || VXWORKS */
    }

  // Reacquire lock to avoid rpdl conditions.
  PDL_OS::thread_mutex_lock (&cv->waiters_lock_);
  cv->waiters_--;

  int last_waiter = cv->was_broadcast_ && cv->waiters_ == 0;

  PDL_OS::thread_mutex_unlock (&cv->waiters_lock_);

#     if defined (PDL_WIN32)
  if (result != WAIT_OBJECT_0)
    {
      switch (result)
        {
        case WAIT_TIMEOUT:
          error = ETIME;
          break;
        default:
          error = ::GetLastError ();
          break;
        }
      result = -1;
    }
#     elif defined (PDL_PSOS)
  if (result != 0)
    {
      switch (result)
        {
        case ERR_TIMEOUT:  // Timeout occured with SM_WAIT
        case ERR_NOMSG:    // Didn't acquire semaphore w/ SM_NOWAIT (ticks=0)
          error = ETIME;
          break;
        default:
          error = errno;
          break;
        }
      result = -1;
    }
#     elif defined (VXWORKS)
  if (result == ERROR)
    {
      switch (errno)
        {
        case S_objLib_OBJ_TIMEOUT:
        case S_objLib_OBJ_UNAVAILABLE:
          error = ETIME;
          break;
        default:
          error = errno;
          break;
        }
      result = -1;
    }
#     endif /* PDL_WIN32 || VXWORKS */
#     if defined (PDL_HAS_SIGNAL_OBJECT_AND_WAIT)
  if (external_mutex->type_ == USYNC_PROCESS)
    {
      if (last_waiter)
        // This call atomically signals the <waiters_done_> event and
        // waits until it can acquire the mutex.  This is important to
        // prevent unfairness.
        PDL_WIN32CALL (PDL_ADAPT_RETVAL (::SignalObjectAndWait (cv->waiters_done_,
                                                                external_mutex->proc_mutex_,
                                                                INFINITE, FALSE),
                                         result),
                       int, -1, result);
      else
        // We must always regain the <external_Mutex>, even when
        // errors occur because that's the guarantee that we give to
        // our callers.
        PDL_OS::mutex_lock (external_mutex);

      return result;
      /* NOTREACHED */
    }
#     endif /* PDL_HAS_SIGNAL_OBJECT_AND_WAIT */
  // Note that this *must* be an "if" statement rather than an "else
  // if" statement since the caller may have timed out and hence the
  // result would have been -1 above.
  if (last_waiter)
    // Release the signaler/broadcaster if we're the last waiter.
#     if defined (PDL_WIN32)
    PDL_OS::event_signal (&cv->waiters_done_);
#     else
    PDL_OS::sema_post (&cv->waiters_done_);
#     endif /* PDL_WIN32 */

  // We must always regain the <external_mutex>, even when errors
  // occur because that's the guarantee that we give to our callers.
  PDL_OS::mutex_lock (external_mutex);

  return result;
#   endif /* PDL_HAS_WTHREADS || PDL_HAS_VXWORKS || PDL_PSOS */
# else
  PDL_UNUSED_ARG (cv);
  PDL_UNUSED_ARG (external_mutex);
  PDL_UNUSED_ARG (timeout);
  PDL_NOTSUP_RETURN (-1);
# endif /* PDL_HAS_THREADS */
}

# if defined (PDL_HAS_WTHREADS)
int
PDL_OS::cond_timedwait (PDL_cond_t *cv,
                        PDL_thread_mutex_t *external_mutex,
                        PDL_Time_Value *timeout)
{
  PDL_TRACE ("PDL_OS::cond_timedwait");
#   if defined (PDL_HAS_THREADS)
  // Handle the easy case first.
  if (timeout == 0)
    return PDL_OS::cond_wait (cv, external_mutex);

  // Prevent rpdl conditions on the <waiters_> count.
  PDL_OS::thread_mutex_lock (&cv->waiters_lock_);
  cv->waiters_++;
  PDL_OS::thread_mutex_unlock (&cv->waiters_lock_);

  int result = 0;
  int error = 0;
  int msec_timeout;

  if (timeout->sec () == 0 && timeout->usec () == 0)
    msec_timeout = 0; // Do a "poll."
  else
    {
      // Note that we must convert between absolute time (which is
      // passed as a parameter) and relative time (which is what
      // WaitForSingleObjects() expects).
      PDL_Time_Value relative_time;
      relative_time = *timeout - PDL_OS::gettimeofday ();

      // Watchout for situations where a context switch has caused the
      // current time to be > the timeout.
      PDL_Time_Value PDL_Time_Value_zero;
      PDL_Time_Value_zero.initialize();
      if (relative_time < PDL_Time_Value_zero)
        msec_timeout = 0;
      else
        msec_timeout = relative_time.msec ();
    }

  // We keep the lock held just long enough to increment the count of
  // waiters by one.  Note that we can't keep it held across the call
  // to WaitForSingleObject since that will deadlock other calls to
  // PDL_OS::cond_signal().
  if (PDL_OS::thread_mutex_unlock (external_mutex) != 0)
    return -1;

  // Wait to be awakened by a PDL_OS::signal() or PDL_OS::broadcast().
#     if !defined (PDL_USES_WINCE_SEMA_SIMULATION)
  result = ::WaitForSingleObject (cv->sema_, msec_timeout);
#     else
  // Can't use Win32 API on simulated semaphores.
  result = PDL_OS::sema_wait (&cv->sema_,
                              *timeout);

  if (result != WAIT_OBJECT_0 && errno == ETIME)
    result = WAIT_TIMEOUT;

#     endif /* PDL_USES_WINCE_SEMA_SIMULATION */

  // Reacquire lock to avoid rpdl conditions.
  PDL_OS::thread_mutex_lock (&cv->waiters_lock_);

  cv->waiters_--;

  int last_waiter = cv->was_broadcast_ && cv->waiters_ == 0;

  PDL_OS::thread_mutex_unlock (&cv->waiters_lock_);

  if (result != WAIT_OBJECT_0)
    {
      switch (result)
        {
        case WAIT_TIMEOUT:
          error = ETIME;
          break;
        default:
          error = ::GetLastError ();
          break;
        }
      result = -1;
    }

  if (last_waiter)
    // Release the signaler/broadcaster if we're the last waiter.
    PDL_OS::event_signal (&cv->waiters_done_);

  // We must always regain the <external_mutex>, even when errors
  // occur because that's the guarantee that we give to our callers.
  PDL_OS::thread_mutex_lock (external_mutex);
  errno = error;
  return result;
#   else
  PDL_NOTSUP_RETURN (-1);
#   endif /* PDL_HAS_THREADS */
}

int
PDL_OS::cond_wait (PDL_cond_t *cv,
                   PDL_thread_mutex_t *external_mutex)
{
  PDL_TRACE ("PDL_OS::cond_wait");
#   if defined (PDL_HAS_THREADS)
  PDL_OS::thread_mutex_lock (&cv->waiters_lock_);
  cv->waiters_++;
  PDL_OS::thread_mutex_unlock (&cv->waiters_lock_);

  int result = 0;
  int error = 0;

  // We keep the lock held just long enough to increment the count of
  // waiters by one.  Note that we can't keep it held across the call
  // to PDL_OS::sema_wait() since that will deadlock other calls to
  // PDL_OS::cond_signal().
  if (PDL_OS::thread_mutex_unlock (external_mutex) != 0)
    return -1;

  // Wait to be awakened by a PDL_OS::cond_signal() or
  // PDL_OS::cond_broadcast().
#     if !defined (PDL_USES_WINCE_SEMA_SIMULATION)
  result = ::WaitForSingleObject (cv->sema_, INFINITE);
#     else
  // Can't use Win32 API on simulated semaphores.
  result = PDL_OS::sema_wait (&cv->sema_);

  if (result != WAIT_OBJECT_0 && errno == ETIME)
    result = WAIT_TIMEOUT;

#     endif /* PDL_USES_WINCE_SEMA_SIMULATION */

  // Reacquire lock to avoid rpdl conditions.
  PDL_OS::thread_mutex_lock (&cv->waiters_lock_);

  cv->waiters_--;

  int last_waiter = cv->was_broadcast_ && cv->waiters_ == 0;

  PDL_OS::thread_mutex_unlock (&cv->waiters_lock_);

  if (result != WAIT_OBJECT_0)
    {
      switch (result)
        {
        case WAIT_TIMEOUT:
          error = ETIME;
          break;
        default:
          error = ::GetLastError ();
          break;
        }
    }
  else if (last_waiter)
    // Release the signaler/broadcaster if we're the last waiter.
    PDL_OS::event_signal (&cv->waiters_done_);

  // We must always regain the <external_mutex>, even when errors
  // occur because that's the guarantee that we give to our callers.
  PDL_OS::thread_mutex_lock (external_mutex);

  // Reset errno in case mutex_lock() also fails...
  errno = error;
  return result;
#   else
  PDL_NOTSUP_RETURN (-1);
#   endif /* PDL_HAS_THREADS */
}
# endif /* PDL_HAS_WTHREADS */
#endif /* PDL_LACKS_COND_T */

void
PDL_OS::exit (int status)
{
  PDL_TRACE ("PDL_OS::exit");

#if defined (PDL_HAS_NONSTATIC_OBJECT_MANAGER) && !defined (PDL_HAS_WINCE) && !defined (PDL_DOESNT_INSTANTIATE_NONSTATIC_OBJECT_MANAGER)
  // Shut down the PDL_Object_Manager, if it had registered its exit_hook.
  // With PDL_HAS_NONSTATIC_OBJECT_MANAGER, the PDL_Object_Manager is
  // instantiated on the main's stack.  ::exit () doesn't destroy it.
  if (exit_hook_)
    (*exit_hook_) ();
#endif /* PDL_HAS_NONSTATIC_OBJECT_MANAGER && !PDL_HAS_WINCE && !PDL_DOESNT_INSTANTIATE_NONSTATIC_OBJECT_MANAGER */

#if !defined (PDL_HAS_WINCE)
# if defined (PDL_WIN32)
  ::ExitProcess ((UINT) status);
# elif defined (PDL_PSOSIM)
  ::u_exit (status);
# else
  ::exit (status);
# endif /* PDL_WIN32 */
#else
  // @@ This is not exactly the same as ExitProcess.  But this is the
  // closest one I can get.
  ::TerminateProcess (::GetCurrentProcess (), status);
#endif /* PDL_HAS_WINCE */
}

# if defined (PDL_PSOS)

// bit masks and shifts for prying info out of the pSOS time encoding
const u_long PDL_PSOS_Time_t::year_mask     = 0x0000FFFFul;
const u_long PDL_PSOS_Time_t::month_mask    = 0x000000FFul;
const u_long PDL_PSOS_Time_t::day_mask      = 0x000000FFul;
const u_long PDL_PSOS_Time_t::hour_mask     = 0x0000FFFFul;
const u_long PDL_PSOS_Time_t::minute_mask   = 0x000000FFul;
const u_long PDL_PSOS_Time_t::second_mask   = 0x000000FFul;
const int PDL_PSOS_Time_t::year_shift   = 16;
const int PDL_PSOS_Time_t::month_shift  = 8;
const int PDL_PSOS_Time_t::hour_shift   = 16;
const int PDL_PSOS_Time_t::minute_shift = 8;
const int PDL_PSOS_Time_t::year_origin = 1900;
const int PDL_PSOS_Time_t::month_origin = 1;

// maximum number of clock ticks supported
const u_long PDL_PSOS_Time_t::max_ticks = ~0UL;

PDL_PSOS_Time_t::PDL_PSOS_Time_t (void)
  : date_ (0),
    time_ (0),
    ticks_ (0)
{
}

// default ctor: date, time, and ticks all zeroed

PDL_PSOS_Time_t::PDL_PSOS_Time_t (const timespec_t& t)
{
  struct tm* tm_struct = PDL_OS::gmtime (&(t.tv_sec));

  // Encode date values from tm struct into pSOS date bit array.
  date_ = (PDL_PSOS_Time_t::year_mask &
           PDL_static_cast (u_long,
                            tm_struct->tm_year + PDL_PSOS_Time_t::year_origin)) <<
    PDL_PSOS_Time_t::year_shift;
  date_ |= (PDL_PSOS_Time_t::month_mask &
            PDL_static_cast (u_long,
                             tm_struct->tm_mon + PDL_PSOS_Time_t::month_origin)) <<
    PDL_PSOS_Time_t::month_shift;
  date_ |= PDL_PSOS_Time_t::day_mask &
    PDL_static_cast (u_long, tm_struct->tm_mday);
  // Encode time values from tm struct into pSOS time bit array.
  time_ = (PDL_PSOS_Time_t::hour_mask  &
            PDL_static_cast (u_long, tm_struct->tm_hour)) <<
    PDL_PSOS_Time_t::hour_shift;
  time_ |= (PDL_PSOS_Time_t::minute_mask &
            PDL_static_cast (u_long, tm_struct->tm_min)) <<
    PDL_PSOS_Time_t::minute_shift;
  time_ |= PDL_PSOS_Time_t::second_mask &
    PDL_static_cast (u_int, tm_struct->tm_sec);

  // encode nanoseconds as system clock ticks
  ticks_ = PDL_static_cast (u_long,
                            ((PDL_static_cast (double, t.tv_nsec) *
                              PDL_static_cast (double, KC_TICKS2SEC)) /
                             PDL_static_cast (double, 1000000000)));

}

// ctor from a timespec_t

PDL_PSOS_Time_t::operator timespec_t (void)
{
  struct tm tm_struct;

  // Decode date and time bit arrays and fill in fields of tm_struct.

  tm_struct.tm_year =
    PDL_static_cast (int, (PDL_PSOS_Time_t::year_mask &
                           (date_ >> PDL_PSOS_Time_t::year_shift))) -
    PDL_PSOS_Time_t::year_origin;
  tm_struct.tm_mon =
    PDL_static_cast (int, (PDL_PSOS_Time_t::month_mask &
                           (date_ >> PDL_PSOS_Time_t::month_shift))) -
    PDL_PSOS_Time_t::month_origin;
  tm_struct.tm_mday =
    PDL_static_cast (int, (PDL_PSOS_Time_t::day_mask & date_));
  tm_struct.tm_hour =
    PDL_static_cast (int, (PDL_PSOS_Time_t::hour_mask &
                           (time_ >> PDL_PSOS_Time_t::hour_shift)));
  tm_struct.tm_min =
    PDL_static_cast (int, (PDL_PSOS_Time_t::minute_mask &
                           (time_ >> PDL_PSOS_Time_t::minute_shift)));
  tm_struct.tm_sec =
    PDL_static_cast (int, (PDL_PSOS_Time_t::second_mask & time_));

  // Indicate values we don't know as negative numbers.
  tm_struct.tm_wday  = -1;
  tm_struct.tm_yday  = -1;
  tm_struct.tm_isdst = -1;

  timespec_t t;

  // Convert calendar time to time struct.
  t.tv_sec = PDL_OS::mktime (&tm_struct);

  // Encode nanoseconds as system clock ticks.
  t.tv_nsec = PDL_static_cast (long,
                               ((PDL_static_cast (double, ticks_) *
                                 PDL_static_cast (double, 1000000000)) /
                                PDL_static_cast (double, KC_TICKS2SEC)));
  return t;
}

// type cast operator (to a timespec_t)

u_long
PDL_PSOS_Time_t::get_system_time (PDL_PSOS_Time_t& t)
{
  u_long ret_val = 0;

#   if defined (PDL_PSOSIM) // system time is broken in simulator.
  timeval tv;
  int result = 0;
  PDL_OSCALL (::gettimeofday (&tv, 0), int, -1, result);
  if (result == -1)
    return 1;

  PDL_Time_Value atv;

  atv.initialize(tv);
  timespec ts = atv;
  PDL_PSOS_Time_t pt (ts);
  t.date_ = pt.date_;
  t.time_ = pt.time_;
  t.ticks_ = pt.ticks_;
#   else
  ret_val = tm_get (&(t.date_), &(t.time_), &(t.ticks_));
#   endif  /* PDL_PSOSIM */
  return ret_val;
}

// Static member function to get current system time.

u_long
PDL_PSOS_Time_t::set_system_time (const PDL_PSOS_Time_t& t)
{
  return tm_set (t.date_, t.time_, t.ticks_);
}

// Static member function to set current system time.

#   if defined (PDL_PSOSIM)

u_long
PDL_PSOS_Time_t::init_simulator_time (void)
{
  // This is a hack using a direct UNIX system call, because the
  // appropriate PDL_OS method ultimately uses the pSOS tm_get
  // function, which would fail because the simulator's system time is
  // uninitialized (chicken and egg).
  timeval t;
  int result = 0;
  PDL_OSCALL (::gettimeofday (&t, 0),
              int,
              -1,
              result);

  if (result == -1)
    return 1;
  else
    {
      PDL_Time_Value tv;

      tv.initialize(t);
      timespec ts = tv;
      PDL_PSOS_Time_t pt (ts);
      u_long ret_val =
        PDL_PSOS_Time_t::set_system_time (pt);
      return ret_val;

    }
}

// Static member function to initialize system time, using UNIX calls.

#   endif /* PDL_PSOSIM */
# endif /* PDL_PSOS && ! PDL_PSOS_DIAB_MIPS */

# if defined (__DGUX) && defined (PDL_HAS_THREADS) && defined (_POSIX4A_DRAFT10_SOURCE)
extern "C" int __d6_sigwait (sigset_t *set);

extern "C" int __d10_sigwait (const sigset_t *set, int *sig)
{
  sigset_t unconst_set = *set;
  int caught_sig = __d6_sigwait (&unconst_set);

  if (caught == -1)
    return -1;

  *sig = caught_sig;
  return 0;
}
# endif /* __DGUX && PTHREADS && _POSIX4A_DRAFT10_SOURCE */

# if defined (CHORUS)
extern "C"
void
pdl_sysconf_dump (void)
{
  PDL_Time_Value time = PDL_OS::gettimeofday ();

  if (time == -1)
    PDL_DEBUG ((LM_DEBUG,
                ASYS_TEXT ("Cannot get time\n")));
  else
    time.dump ();

  PDL_DEBUG ((LM_DEBUG,
              "ARG_MAX \t= \t%d\t"
              "DELAYTIMER_MAX \t= \t%d\n"
              "_MQ_OPEN_MAX \t= \t%d\t"
              "_MQ_PRIO_MAX \t= \t%d\n"
              "_MQ_DFL_MSGSIZE\t= \t%d\t"
              "_MQ_DFL_MAXMSGNB\t= \t%d\n"
              "_MQ_PATHMAX \t= \t%d\n"
              "NGROUPS_MAX \t= \t%d\t"
              "OPEN_MAX \t= \t%d\n"
              "PAGESIZE \t= \t%d\n"
              "PTHREAD_DESTRUCTOR_ITERATIONS \t= \t%d\n"
              "PTHREAD_KEYS_MAX \t= \t%d\n"
              "PTHREAD_STACK_MIN \t= \t%d\n"
              "PTHREAD_THREADS_MAX \t= \t%d\n"
              "SEM_VALUE_MAX \t= \t%d\n"
              "SEM_PATHMAX \t= \t%d\n"
              "TIMER_MAX \t= \t%d\n"
              "TZNAME_MAX \t= \t%d\n"
              "_POSIX_MESSAGE_PASSING \t= \t%d\n"
              "_POSIX_SEMAPHORES \t= \t%d\n"
              "_POSIX_SHARED_MEMORY_OBJECTS \t= \t%d\n"
              "_POSIX_THREADS \t= \t%d\n"
              "_POSIX_THREAD_ATTR_STACKADDR \t= \t%d\n"
              "_POSIX_THREAD_ATTR_STACKSIZE \t= \t%d\n"
              "_POSIX_THREAD_PRIORITY_SCHEDULING= \t%d\n"
              "_POSIX_THREAD_PRIO_INHERIT \t= \t%d\n"
              "_POSIX_THREAD_PRIO_PROTECT \t= \t%d\n"
              "_POSIX_THREAD_PROCESS_SHARED \t= \t%d\n"
              "_POSIX_THREAD_SAFE_FUNCTIONS \t= \t%d\n"
              "_POSIX_TIMERS \t= \t%d\n"
              "_POSIX_VERSION \t= \t%d\n",
              PDL_OS::sysconf (_SC_ARG_MAX),
              PDL_OS::sysconf (_SC_DELAYTIMER_MAX),
              PDL_OS::sysconf (_SC_MQ_OPEN_MAX),
              PDL_OS::sysconf (_SC_MQ_PRIO_MAX),
              PDL_OS::sysconf (_SC_MQ_DFL_MSGSIZE),
              PDL_OS::sysconf (_SC_MQ_DFL_MAXMSGNB),
              PDL_OS::sysconf (_SC_MQ_PATHMAX),
              PDL_OS::sysconf (_SC_NGROUPS_MAX),
              PDL_OS::sysconf (_SC_OPEN_MAX),
              PDL_OS::sysconf (_SC_PAGESIZE),
              PDL_OS::sysconf (_SC_PTHREAD_DESTRUCTOR_ITERATIONS),
              PDL_OS::sysconf (_SC_PTHREAD_KEYS_MAX),
              PDL_OS::sysconf (_SC_PTHREAD_STACK_MIN),
              PDL_OS::sysconf (_SC_PTHREAD_THREADS_MAX),
              PDL_OS::sysconf (_SC_SEM_VALUE_MAX),
              PDL_OS::sysconf (_SC_SHM_PATHMAX),
              PDL_OS::sysconf (_SC_TIMER_MAX),
              PDL_OS::sysconf (_SC_TZNAME_MAX),
              PDL_OS::sysconf (_SC_MESSAGE_PASSING),
              PDL_OS::sysconf (_SC_SEMAPHORES),
              PDL_OS::sysconf (_SC_SHARED_MEMORY_OBJECTS),
              PDL_OS::sysconf (_SC_THREADS),
              PDL_OS::sysconf (_SC_THREAD_ATTR_STACKADDR),
              PDL_OS::sysconf (_SC_THREAD_ATTR_STACKSIZE),
              PDL_OS::sysconf (_SC_THREAD_PRIORITY_SCHEDULING),
              PDL_OS::sysconf (_SC_THREAD_PRIO_INHERIT),
              PDL_OS::sysconf (_SC_THREAD_PRIO_PROTECT),
              PDL_OS::sysconf (_SC_THREAD_PROCESS_SHARED),
              PDL_OS::sysconf (_SC_THREAD_SAFE_FUNCTIONS),
              PDL_OS::sysconf (_SC_TIMERS),
              PDL_OS::sysconf (_SC_VERSION)));
}
# endif /* CHORUS */


# define PDL_OS_PREALLOCATE_OBJECT(TYPE, ID)\
    {\
      TYPE *obj_p = 0;\
      PDL_NEW_RETURN (obj_p, TYPE, sizeof(TYPE), -1);\
      preallocated_object[ID] = (void *) obj_p;\
    }
# define PDL_OS_DELETE_PREALLOCATED_OBJECT(TYPE, ID)\
    PDL_OS::free((TYPE *) preallocated_object[ID]);\
    preallocated_object[ID] = 0;

PDL_Object_Manager_Base::PDL_Object_Manager_Base (void)
  : object_manager_state_ (OBJ_MAN_UNINITIALIZED)
  , dynamically_allocated_ (0)
  , next_ (0)
{
}

int
PDL_Object_Manager_Base::starting_up_i ()
{
  return object_manager_state_ < OBJ_MAN_INITIALIZED;
}

int
PDL_Object_Manager_Base::shutting_down_i ()
{
  return object_manager_state_ > OBJ_MAN_INITIALIZED;
}

extern "C"
void
PDL_OS_Object_Manager_Internal_Exit_Hook (void)
{
  if (PDL_OS_Object_Manager::instance_)
    PDL_OS_Object_Manager::instance ()->fini ();
}

PDL_OS_Object_Manager *PDL_OS_Object_Manager::instance_ = 0;

void *PDL_OS_Object_Manager::preallocated_object[
  PDL_OS_Object_Manager::PDL_OS_PREALLOCATED_OBJECTS] = { 0 };

PDL_OS_Object_Manager::PDL_OS_Object_Manager (void)
  // default_mask_ isn't initialized, because it's defined by <init>.
{
  // If instance_ was not 0, then another PDL_OS_Object_Manager has
  // already been instantiated (it is likely to be one initialized by
  // way of library/DLL loading).  Let this one go through
  // construction in case there really is a good reason for it (like,
  // PDL is a static/archive library, and this one is the non-static
  // instance (with PDL_HAS_NONSTATIC_OBJECT_MANAGER, or the user has
  // a good reason for creating a separate one) but the original one
  // will be the one retrieved from calls to
  // PDL_Object_Manager::instance().

  // Be sure that no further instances are created via instance ().
  if (instance_ == 0)
    instance_ = this;

  init ();
}

sigset_t *
PDL_OS_Object_Manager::default_mask (void)
{
  return PDL_OS_Object_Manager::instance ()->default_mask_;
}

#if defined (PDL_HAS_WIN32_STRUCTURAL_EXCEPTIONS)
PDL_SEH_EXCEPT_HANDLER
PDL_OS_Object_Manager::seh_except_selector (void)
{
  return PDL_OS_Object_Manager::instance ()->seh_except_selector_;
}

PDL_SEH_EXCEPT_HANDLER
PDL_OS_Object_Manager::seh_except_selector (PDL_SEH_EXCEPT_HANDLER n)
{
  PDL_OS_Object_Manager *instance =
    PDL_OS_Object_Manager::instance ();

  PDL_SEH_EXCEPT_HANDLER retv = instance->seh_except_selector_;
  instance->seh_except_selector_ = n;
  return retv;
}

PDL_SEH_EXCEPT_HANDLER
PDL_OS_Object_Manager::seh_except_handler (void)
{
  return PDL_OS_Object_Manager::instance ()->seh_except_handler_;
}

PDL_SEH_EXCEPT_HANDLER
PDL_OS_Object_Manager::seh_except_handler (PDL_SEH_EXCEPT_HANDLER n)
{
  PDL_OS_Object_Manager *instance =
    PDL_OS_Object_Manager::instance ();

  PDL_SEH_EXCEPT_HANDLER retv = instance->seh_except_handler_;
  instance->seh_except_handler_ = n;
  return retv;
}
#endif /* PDL_HAS_WIN32_STRUCTURAL_EXCEPTIONS */

PDL_OS_Object_Manager *
PDL_OS_Object_Manager::instance (void)
{
  // This function should be called during construction of static
  // instances, or before any other threads have been created in the
  // process.  So, it's not thread safe.

  if (instance_ == 0)
  {
      static PDL_OS_Object_Manager gManager;
      PDL_OS_Object_Manager *instance_pointer;

      instance_pointer = &gManager;

      PDL_ASSERT (instance_pointer == instance_);

      instance_pointer->dynamically_allocated_ = 1;

    }

  return instance_;
}

int
PDL_OS_Object_Manager::init (void)
{
  if (starting_up_i ())
    {
      // First, indicate that this PDL_OS_Object_Manager instance is being
      // initialized.
      object_manager_state_ = OBJ_MAN_INITIALIZING;

      if (this == instance_)
        {
# if defined (PDL_MT_SAFE) && (PDL_MT_SAFE != 0)
#   if defined (PDL_HAS_WINCE_BROKEN_ERRNO)
          PDL_CE_Errno::init ();
#   endif /* PDL_HAS_WINCE_BROKEN_ERRNO */

          PDL_OS_PREALLOCATE_OBJECT (PDL_thread_mutex_t, PDL_OS_MONITOR_LOCK)
          if (PDL_OS::thread_mutex_init (PDL_reinterpret_cast (
            PDL_thread_mutex_t *,
            PDL_OS_Object_Manager::preallocated_object[
              PDL_OS_MONITOR_LOCK])) != 0)
            PDL_OS_Object_Manager::print_error_message (
              __LINE__, PDL_TEXT("PDL_OS_MONITOR_LOCK"));
          PDL_OS_PREALLOCATE_OBJECT (PDL_recursive_thread_mutex_t,
                                     PDL_TSS_CLEANUP_LOCK)
          if (PDL_OS::recursive_mutex_init (PDL_reinterpret_cast (
            PDL_recursive_thread_mutex_t *,
            PDL_OS_Object_Manager::preallocated_object[
              PDL_TSS_CLEANUP_LOCK])) != 0)
            PDL_OS_Object_Manager::print_error_message (
              __LINE__, PDL_TEXT("PDL_TSS_CLEANUP_LOCK"));
          PDL_OS_PREALLOCATE_OBJECT (PDL_thread_mutex_t,
                                     PDL_LOG_MSG_INSTANCE_LOCK)
          if (PDL_OS::thread_mutex_init (PDL_reinterpret_cast (
            PDL_thread_mutex_t *,
            PDL_OS_Object_Manager::preallocated_object[
              PDL_LOG_MSG_INSTANCE_LOCK])) != 0)
            PDL_OS_Object_Manager::print_error_message (
              __LINE__, PDL_TEXT("PDL_LOG_MSG_INSTANCE_LOCK"));
#   if defined (PDL_HAS_TSS_EMULATION)
          PDL_OS_PREALLOCATE_OBJECT (PDL_recursive_thread_mutex_t,
                                     PDL_TSS_KEY_LOCK)
          if (PDL_OS::recursive_mutex_init (PDL_reinterpret_cast (
            PDL_recursive_thread_mutex_t *,
            PDL_OS_Object_Manager::preallocated_object[
              PDL_TSS_KEY_LOCK])) != 0)
            PDL_OS_Object_Manager::print_error_message (
              __LINE__, "PDL_TSS_KEY_LOCK");
#     if defined (PDL_HAS_THREAD_SPECIFIC_STORAGE)
          PDL_OS_PREALLOCATE_OBJECT (PDL_recursive_thread_mutex_t,
                                     PDL_TSS_BASE_LOCK)
          if (PDL_OS::recursive_mutex_init (PDL_reinterpret_cast (
            PDL_recursive_thread_mutex_t *,
            PDL_OS_Object_Manager::preallocated_object[
              PDL_TSS_BASE_LOCK])) != 0)
            PDL_OS_Object_Manager::print_error_message (
              __LINE__, "PDL_TSS_BASE_LOCK");
#     endif /* PDL_HAS_THREAD_SPECIFIC_STORAGE */
#   endif /* PDL_HAS_TSS_EMULATION */
# endif /* PDL_MT_SAFE */

          // Open Winsock (no-op on other platforms).
          PDL_OS::socket_init (PDL_WSOCK_VERSION);

          // Register the exit hook, for use by PDL_OS::exit ().
          PDL_OS::set_exit_hook (&PDL_OS_Object_Manager_Internal_Exit_Hook);
        }

      PDL_NEW_RETURN (default_mask_, sigset_t, sizeof(sigset_t), -1);

      //fix for UMR
      PDL_OS::memset( (void *)default_mask_, 0, sizeof(sigset_t) );

      PDL_OS::sigfillset (default_mask_);

      // Finally, indicate that the PDL_OS_Object_Manager instance has
      // been initialized.
      object_manager_state_ = OBJ_MAN_INITIALIZED;

# if defined (PDL_WIN32)
      PDL_OS::win32_versioninfo_.dwOSVersionInfoSize =
        sizeof (OSVERSIONINFO);
      ::GetVersionEx (&PDL_OS::win32_versioninfo_);
# endif /* PDL_WIN32 */
      return 0;
    } else {
      // Had already initialized.
      return 1;
    }
}

// Clean up an PDL_OS_Object_Manager.  There can be instances of this object
// other than The Instance.  This can happen if a user creates one for some
// reason.  All objects clean up their per-object information and managed
// objects, but only The Instance cleans up the static preallocated objects.
int
PDL_OS_Object_Manager::fini (void)
{
  if (instance_ == 0  ||  shutting_down_i ())
    // Too late.  Or, maybe too early.  Either fini () has already
    // been called, or init () was never called.
    return object_manager_state_ == OBJ_MAN_SHUT_DOWN  ?  1  :  -1;

  // No mutex here.  Only the main thread should destroy the singleton
  // PDL_OS_Object_Manager instance.

  // Indicate that the PDL_OS_Object_Manager instance is being shut
  // down.  This object manager should be the last one to be shut
  // down.
  object_manager_state_ = OBJ_MAN_SHUTTING_DOWN;

  // Only clean up preallocated objects when the singleton Instance is being
  // destroyed.
  if (this == instance_)
    {
      // Close down Winsock (no-op on other platforms).
      PDL_OS::socket_fini ();

#if ! defined (PDL_HAS_STATIC_PREALLOCATION)
      // Cleanup the dynamically preallocated objects.
# if defined (PDL_MT_SAFE) && (PDL_MT_SAFE != 0)
#   if !defined (__Lynx__)
      // LynxOS 3.0.0 has problems with this after fork.
      if (PDL_OS::thread_mutex_destroy (PDL_reinterpret_cast (
        PDL_thread_mutex_t *,
        PDL_OS_Object_Manager::preallocated_object[PDL_OS_MONITOR_LOCK])) != 0)
#      if !defined (__QNX__)
        PDL_OS_Object_Manager::print_error_message (__LINE__, PDL_TEXT("PDL_OS_MONITOR_LOCK"));
#      else
        ;
#      endif
#   endif /* ! __Lynx__ */
      PDL_OS_DELETE_PREALLOCATED_OBJECT (PDL_thread_mutex_t,
                                         PDL_OS_MONITOR_LOCK)
#   if !defined (__Lynx__)
      // LynxOS 3.0.0 has problems with this after fork.
      if (PDL_OS::recursive_mutex_destroy (PDL_reinterpret_cast (
        PDL_recursive_thread_mutex_t *,
        PDL_OS_Object_Manager::preallocated_object[
          PDL_TSS_CLEANUP_LOCK])) != 0)
#      if !defined (__QNX__)
        PDL_OS_Object_Manager::print_error_message (__LINE__, PDL_TEXT("PDL_TSS_CLEANUP_LOCK"));
#      else
        ;
#      endif
#   endif /* ! __Lynx__ */
      PDL_OS_DELETE_PREALLOCATED_OBJECT (PDL_recursive_thread_mutex_t,
                                         PDL_TSS_CLEANUP_LOCK)
#   if !defined (__Lynx__)
      // LynxOS 3.0.0 has problems with this after fork.
      if (PDL_OS::thread_mutex_destroy (PDL_reinterpret_cast (
        PDL_thread_mutex_t *,
        PDL_OS_Object_Manager::preallocated_object
            [PDL_LOG_MSG_INSTANCE_LOCK])) != 0)
#      if !defined (__QNX__)
        PDL_OS_Object_Manager::print_error_message (__LINE__, PDL_TEXT("PDL_LOG_MSG_INSTANCE_LOCK "));
#      else
        ;
#      endif
#   endif /* ! __Lynx__ */
      PDL_OS_DELETE_PREALLOCATED_OBJECT (PDL_thread_mutex_t,
                                         PDL_LOG_MSG_INSTANCE_LOCK)
#   if defined (PDL_HAS_TSS_EMULATION)
#     if !defined (__Lynx__)
        // LynxOS 3.0.0 has problems with this after fork.
        if (PDL_OS::recursive_mutex_destroy (PDL_reinterpret_cast (
          PDL_recursive_thread_mutex_t *,
          PDL_OS_Object_Manager::preallocated_object[
            PDL_TSS_KEY_LOCK])) != 0)
          PDL_OS_Object_Manager::print_error_message (
            __LINE__, "PDL_TSS_KEY_LOCK");
#     endif /* ! __Lynx__ */
      PDL_OS_DELETE_PREALLOCATED_OBJECT (PDL_recursive_thread_mutex_t,
                                         PDL_TSS_KEY_LOCK)
#     if defined (PDL_HAS_THREAD_SPECIFIC_STORAGE)
#       if !defined (__Lynx__)
          // LynxOS 3.0.0 has problems with this after fork.
          if (PDL_OS::recursive_mutex_destroy (PDL_reinterpret_cast (
            PDL_recursive_thread_mutex_t *,
            PDL_OS_Object_Manager::preallocated_object[
              PDL_TSS_BASE_LOCK])) != 0)
            PDL_OS_Object_Manager::print_error_message (
              __LINE__, "PDL_TSS_BASE_LOCK");
#       endif /* ! __Lynx__ */
      PDL_OS_DELETE_PREALLOCATED_OBJECT (PDL_recursive_thread_mutex_t,
                                         PDL_TSS_BASE_LOCK)
#     endif /* PDL_HAS_THREAD_SPECIFIC_STORAGE */
#   endif /* PDL_HAS_TSS_EMULATION */
#   if defined (PDL_HAS_WINCE_BROKEN_ERRNO)
          PDL_CE_Errno::fini ();
#   endif /* PDL_HAS_WINCE_BROKEN_ERRNO */

# endif /* PDL_MT_SAFE */
#endif /* ! PDL_HAS_STATIC_PREALLOCATION */
    }

  PDL_OS::free(default_mask_);
  default_mask_ = 0;

  // Indicate that this PDL_OS_Object_Manager instance has been shut down.
  object_manager_state_ = OBJ_MAN_SHUT_DOWN;

  if (dynamically_allocated_)
    {
//      delete this;
    }

  if (this == instance_)
    instance_ = 0;

  return 0;
}

int pdl_exit_hook_marker = 0;

void
PDL_OS_Object_Manager::print_error_message (u_int line_number, LPCTSTR message)
{
  // To avoid duplication of these const strings in OS.o.
#if !defined (PDL_HAS_WINCE)
  fprintf (stderr, "OS.cpp, line %u: %s ",
           line_number,
           message);
  perror ("failed");
#else
  // @@ Need to use the following information.
  PDL_UNUSED_ARG (line_number);
  PDL_UNUSED_ARG (message);

  LPTSTR lpMsgBuf = 0;
  ::FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER |
                   FORMAT_MESSAGE_FROM_SYSTEM,
                   NULL,
                   ::GetLastError (),
                   MAKELANGID (LANG_NEUTRAL,
                               SUBLANG_DEFAULT),
                   // Default language
                   (LPTSTR) &lpMsgBuf,
                   0,
                   NULL);
  ::MessageBox (NULL,
                lpMsgBuf,
                PDL_TEXT("PDL_OS error"),
                MB_OK);
#endif
}

int
PDL_OS_Object_Manager::starting_up (void)
{
  return PDL_OS_Object_Manager::instance_
    ? instance_->starting_up_i ()
    : 1;
}

int
PDL_OS_Object_Manager::shutting_down (void)
{
  return PDL_OS_Object_Manager::instance_
    ? instance_->shutting_down_i ()
    : 1;
}

#if !defined (PDL_HAS_NONSTATIC_OBJECT_MANAGER)
class PDL_Export PDL_OS_Object_Manager_Manager
  // = TITLE
  //    Ensure that the <PDL_OS_Object_Manager> gets initialized at
  //    program startup, and destroyed at program termination.
  //
  // = DESCRIPTION
  //    Without PDL_HAS_NONSTATIC_OBJECT_MANAGER, a static instance of this
  //    class is created.  Therefore, it gets created before main ()
  //    is called.  And it gets destroyed after main () returns.
{
public:
  PDL_OS_Object_Manager_Manager (void);
  ~PDL_OS_Object_Manager_Manager (void);

private:
  PDL_thread_t saved_main_thread_id_;
  // Save the main thread ID, so that destruction can be suppressed.
};

PDL_OS_Object_Manager_Manager::PDL_OS_Object_Manager_Manager (void)
  : saved_main_thread_id_ (PDL_OS::thr_self ())
{
  // Ensure that the Object_Manager gets initialized before any
  // application threads have been spawned.  Because this will be called
  // during construction of static objects, that should always be the
  // case.
  (void) PDL_OS_Object_Manager::instance ();
}

PDL_OS_Object_Manager_Manager::~PDL_OS_Object_Manager_Manager (void)
{
  if (PDL_OS::thr_equal (PDL_OS::thr_self (),
                         saved_main_thread_id_))
    {
//      delete PDL_OS_Object_Manager::instance_;
      PDL_OS_Object_Manager::instance_ = 0;
    }
  // else if this destructor is not called by the main thread, then do
  // not delete the PDL_OS_Object_Manager.  That causes problems, on
  // WIN32 at least.
}

static PDL_OS_Object_Manager_Manager PDL_OS_Object_Manager_Manager_instance;
#endif /* ! PDL_HAS_NONSTATIC_OBJECT_MANAGER */



//          **** Warning ****
// You should not use the following function under CE at all.  This
// function is used to make Svc_Conf_l.cpp compile under WinCE.  It
// might not do what it is expected to do under regular environments.
//          **** Warning ****

#   if defined (UNDER_CE) && (UNDER_CE < 211)
void
exit (int status)
{
#     if defined (PDL_HAS_NONSTATIC_OBJECT_MANAGER) && !defined (PDL_HAS_WINCE) && !defined (PDL_DOESNT_INSTANTIATE_NONSTATIC_OBJECT_MANAGER)
  // Shut down the PDL_Object_Manager, if it had registered its exit_hook.
  // With PDL_HAS_NONSTATIC_OBJECT_MANAGER, the PDL_Object_Manager is
  // instantiated on the main's stack.  ::exit () doesn't destroy it.
  if (exit_hook_)
    (*exit_hook_) ();
#     endif /* PDL_HAS_NONSTATIC_OBJECT_MANAGER && !PDL_HAS_WINCE && !PDL_DOESNT_INSTANTIATE_NONSTATIC_OBJECT_MANAGER */

  PDL_OS::exit (status);
}
#   endif /* UNDER_CE && UNDER_CE < 211 */

#if defined (PDL_HAS_STRPTIME)
# if defined (PDL_LACKS_NATIVE_STRPTIME)
int
PDL_OS::strptime_getnum (char *buf,
                         int *num,
                         int *bi,
                         int *fi,
                         int min,
                         int max)
{
  int i = 0, tmp = 0;

  while (isdigit (buf[i]))
    {
      tmp = (tmp * 10) + (buf[i] - '0');
      if (max && (tmp > max))
        return 0;
      i++;
    }

  if (tmp < min)
    return 0;
  else if (i)
    {
      *num = tmp;
      (*fi)++;
      *bi += i;
      return 1;
    }
  else
    return 0;
}
# endif /* PDL_LACKS_NATIVE_STRPTIME */

char *
PDL_OS::strptime (char       *buf,
                  const char *format,
                  struct tm  *tm)
{
#if !defined (PDL_HAS_WINCE)
#if defined (PDL_LACKS_NATIVE_STRPTIME)
  int bi = 0;
  int fi = 0;
  int percent = 0;

  if (!buf || !format)
    return 0;

  while (format[fi] != '\0')
    {
      if (percent)
        {
          percent = 0;
          switch (format[fi])
            {
            case '%':                        // an escaped %
              if (buf[bi] == '%')
                {
                  fi++; bi++;
                }
              else
                return buf + bi;
              break;

              /* not supported yet: weekday via locale long/short names
                 case 'a':                        / * weekday via locale * /
                 / * FALL THROUGH * /
                 case 'A':                        / * long/short names * /
                 break;
                 */

              /* not supported yet:
                 case 'b':                        / * month via locale * /
                 / * FALL THROUGH * /
                 case 'B':                        / * long/short names * /
                 / * FALL THROUGH * /
                 case 'h':
                 break;
                 */

              /* not supported yet:
                 case 'c':                        / * %x %X * /
                 break;
                 */

              /* not supported yet:
                 case 'C':                        / * date & time -  * /
                 / * locale long format * /
                 break;
                 */

            case 'd':                        /* day of month (1-31) */
              /* FALL THROUGH */
            case 'e':
              if (!PDL_OS::strptime_getnum (buf + bi, &tm->tm_mday, &bi, &fi, 1, 31))
                return buf + bi;

              break;

            case 'D':                        /* %m/%d/%y */
              if (!PDL_OS::strptime_getnum (buf + bi, &tm->tm_mon, &bi, &fi, 1, 12))
                return buf + bi;

              fi--;
              tm->tm_mon--;

              if (buf[bi] != '/')
                return buf + bi;

              bi++;

              if (!PDL_OS::strptime_getnum (buf + bi, &tm->tm_mday, &bi, &fi, 1, 31))
                return buf + bi;

              fi--;
              if (buf[bi] != '/')
                return buf + bi;
              bi++;
              if (!PDL_OS::strptime_getnum (buf + bi, &tm->tm_year, &bi, &fi, 0, 99))
                return buf + bi;
              if (tm->tm_year < 69)
                tm->tm_year += 100;
              break;

            case 'H':                        /* hour (0-23) */
              /* FALL THROUGH */
            case 'k':
              if (!PDL_OS::strptime_getnum (buf + bi, &tm->tm_hour, &bi, &fi, 0, 23))
                return buf + bi;
              break;

              /* not supported yet:
                 case 'I':                        / * hour (0-12) * /
                 / * FALL THROUGH * /
                 case 'l':
                 break;
                 */

            case 'j':                        /* day of year (0-366) */
              if (!PDL_OS::strptime_getnum (buf + bi, &tm->tm_yday, &bi, &fi, 1, 366))
                return buf + bi;

              tm->tm_yday--;
              break;

            case 'm':                        /* an escaped % */
              if (!PDL_OS::strptime_getnum (buf + bi, &tm->tm_mon, &bi, &fi, 1, 12))
                return buf + bi;

              tm->tm_mon--;
              break;

            case 'M':                        /* minute (0-59) */
              if (!PDL_OS::strptime_getnum (buf + bi, &tm->tm_min, &bi, &fi, 0, 59))
                return buf + bi;

              break;

              /* not supported yet:
                 case 'p':                        / * am or pm for locale * /
                 break;
                 */

              /* not supported yet:
                 case 'r':                        / * %I:%M:%S %p * /
                 break;
                 */

            case 'R':                        /* %H:%M */
              if (!PDL_OS::strptime_getnum (buf + bi, &tm->tm_hour, &bi, &fi, 0, 23))
                return buf + bi;

              fi--;
              if (buf[bi] != ':')
                return buf + bi;
              bi++;
              if (!PDL_OS::strptime_getnum (buf + bi, &tm->tm_min, &bi, &fi, 0, 59))
                return buf + bi;

              break;

            case 'S':                        /* seconds (0-61) */
              if (!PDL_OS::strptime_getnum (buf + bi, &tm->tm_sec, &bi, &fi, 0, 61))
                return buf + bi;
              break;

            case 'T':                        /* %H:%M:%S */
              if (!PDL_OS::strptime_getnum (buf + bi, &tm->tm_hour, &bi, &fi, 0, 23))
                return buf + bi;

              fi--;
              if (buf[bi] != ':')
                return buf + bi;
              bi++;
              if (!PDL_OS::strptime_getnum (buf + bi, &tm->tm_min, &bi, &fi, 0, 59))
                return buf + bi;

              fi--;
              if (buf[bi] != ':')
                return buf + bi;
              bi++;
              if (!PDL_OS::strptime_getnum (buf + bi, &tm->tm_sec, &bi, &fi, 0, 61))
                return buf + bi;

              break;

            case 'w':                        /* day of week (0=Sun-6) */
              if (!PDL_OS::strptime_getnum (buf + bi, &tm->tm_wday, &bi, &fi, 0, 6))
                return buf + bi;

              break;

              /* not supported yet: date, based on locale
                 case 'x':                        / * date, based on locale * /
                 break;
                 */

              /* not supported yet:
                 case 'X':                        / * time, based on locale * /
                 break;
                 */

            case 'y':                        /* the year - 1900 (0-99) */
              if (!PDL_OS::strptime_getnum (buf + bi, &tm->tm_year, &bi, &fi, 0, 99))
                return buf + bi;

              if (tm->tm_year < 69)
                tm->tm_year += 100;
              break;

            case 'Y':                        /* the full year (1999) */
              if (!PDL_OS::strptime_getnum (buf + bi, &tm->tm_year, &bi, &fi, 0, 0))
                return buf + bi;

              tm->tm_year -= 1900;
              break;

            default:                        /* unrecognised */
              return buf + bi;
            } /* switch (format[fi]) */

        }
      else
        { /* if (percent) */
          if (format[fi] == '%')
            {
              percent = 1;
              fi++;
            }
          else
            {
              if (format[fi] == buf[bi])
                {
                  fi++;
                  bi++;
                }
              else
                return buf + bi;
            }
        } /* if (percent) */
    } /* while (format[fi] */

  return buf + bi;
#else  /* ! PDL_LACKS_NATIVE_STRPTIME */
  return ::strptime (buf,
                     format,
                     tm);
#endif /* ! PDL_LACKS_NATIVE_STRPTIME */
#else /* ! PDL_HAS_WINCE */
  PDL_UNUSED_ARG (buf);
  PDL_UNUSED_ARG (format);
  PDL_UNUSED_ARG (tm);

  PDL_NOTSUP_RETURN (0);
#endif /* ! PDL_HAS_WINCE */
}
#endif /* PDL_HAS_STRPTIME */

/*BUGBUG_NT*/
#if defined(PDL_WIN32) || defined(PDL_HAS_WINCE)
/* Ace라이브러리를 이용한 class가 global로 정의되어 있으면
   PDL초기화보다 먼저 PDL API가 사용되게 된다.
   그러므로 마찬가지로 PDL초기화가 먼저 되게 하기 위해
   OS.cpp에 초기화 클래스의 인스턴스를 하나 만드는 방식을 사용한다.
   우선 WIN32에 대해서만 이렇게 만들어놓았지만,
   다른 모든 플랫폼에 대해서도 이렇게 해주어야 할 것이다.
*/
class AceInitializer {
protected:
	// 이렇게 해주어야 PDL 초기화 객체들이 파괴되지 않는다..
	PDL_MAIN_OBJECT_MANAGER
public :
	AceInitializer();
};
AceInitializer::AceInitializer() {
	// 여기서 PDL초기화 객체들을 만들면 안된다.
	// constructor를 빠져나가면서 파괴되기 때문이다.
}

AceInitializer initializer;

// global objects
struct rlimit win32_rlimit = {-1,-1};

#endif /* PDL_WIN32 */

#if defined(VXWORKS)
struct rlimit vxworks_rlimit = {-1,-1};
#endif

#if defined(SYMBIAN)
struct rlimit symbian_rlimit = {-1,-1};
#endif

/*BUGBUG_NT ADD*/

#if defined (PDL_WIN32)

/*
 * In Windows, rlimit has no meaning.
 */
int getrlimit(int resource, struct rlimit *rlp)
{
    rlp->rlim_cur = PDL_INT64_MAX;
    rlp->rlim_max = PDL_INT64_MAX;

    return 0;
}

int setrlimit(int resource, const struct rlimit *rlp)
{
    return 0;
}

#endif

