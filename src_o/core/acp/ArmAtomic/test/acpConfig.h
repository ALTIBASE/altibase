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
 
/*******************************************************************************
 * $Id: acpConfig.h 11316 2010-06-22 23:56:42Z djin $
 ******************************************************************************/

#if !defined(_O_ACP_CONFIG_H_)
#define _O_ACP_CONFIG_H_


#include <acpConfigPlatform.h>

#if defined(ALTI_CFG_OS_WINDOWS)

#if !defined _CRT_RAND_S
#define _CRT_RAND_S
#endif

#if !defined(FD_SETSIZE)
#define FD_SETSIZE 1024
#endif

#if !defined(_WIN32_WINNT)
#if _MSC_VER >= 1500
#define _WIN32_WINNT 0x0502
#else
#define _WIN32_WINNT 0x0400
#endif
#endif

#include <malloc.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <wspiapi.h>
#include <windows.h>
#include <winnt.h>
#include <aclapi.h>
#include <direct.h>
#include <strsafe.h>
#include <iphlpapi.h>
#include <io.h>
#include <dbghelp.h>
#include <shlobj.h>
#else
#include <pthread.h>
#endif

#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <float.h>
#include <math.h>

#if !defined(ALTI_CFG_OS_WINDOWS)

#if defined(ALTI_CFG_OS_LINUX)
#include <malloc.h>
#include <sys/mount.h>
#include <ifaddrs.h>
#endif

#include <unistd.h>
#include <dirent.h>
#include <dlfcn.h>
#include <time.h>
#include <pwd.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/param.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sched.h>
#include <semaphore.h>

#if !defined(ALTI_CFG_OS_DARWIN)
#include <ucontext.h>
#endif

#include <setjmp.h>
#include <netdb.h>
#include <sys/un.h>
#endif

#if defined(ALTI_CFG_OS_AIX)
#include <sys/ndd_var.h>
#include <sys/kinfo.h>
#include <sys/debug.h>
#include <sys/ldr.h>
#include <sys/devinfo.h>
#if (ALTI_CFG_OS_MAJOR >= 5) && (ALTI_CFG_OS_MINOR >= 3) && (ACP_CFG_AIX_USEPOLL == 0)
#include <sys/pollset.h>
#endif
#elif defined(ALTI_CFG_OS_HPUX)
#include <a.out.h>
#include <elf.h>
#include <dl.h>
#include <sys/stropts.h>
#include <sys/dlpi.h>
#include <sys/diskio.h>
#if (ALTI_CFG_OS_MAJOR > 11) ||                                      \
    ((ALTI_CFG_OS_MAJOR == 11) && (ALTI_CFG_OS_MINOR >= 11))
#include <sys/devpoll.h>
#if (ALTI_CFG_OS_MAJOR >= 11) && (ALTI_CFG_OS_MINOR >= 31)
# if defined(ALTI_CFG_CPU_IA64)
# include <sys/mercury.h>
# else
# include <mercury.h>
# endif
#endif
#endif
#if defined(ALTI_CFG_CPU_IA64)
#include <uwx.h>
#include <uwx_self.h>
#endif
#elif defined(ALTI_CFG_OS_LINUX)
#  if defined(ACP_CFG_COMPILE_64BIT)
#  include <execinfo.h>
#  endif
#include <elf.h>
#if (ALTI_CFG_OS_MAJOR > 2) ||                                       \
    ((ALTI_CFG_OS_MAJOR == 2) && (ALTI_CFG_OS_MINOR >= 6))
#include <sys/epoll.h>
#endif
#elif defined(ALTI_CFG_OS_SOLARIS)
#include <ieeefp.h>
#include <net/if_arp.h>
#include <sys/atomic.h>
#include <sys/sockio.h>
#include <sys/frame.h>
#include <sys/stack.h>
#include <sys/elf.h>
#include <sys/processor.h>
#include <sys/types.h>
#include <sys/dkio.h>
#if (ALTI_CFG_OS_MAJOR > 2) ||                                       \
    ((ALTI_CFG_OS_MAJOR == 2) && (ALTI_CFG_OS_MINOR >= 10))
#include <port.h>
#elif ((ALTI_CFG_OS_MAJOR == 2) && (ALTI_CFG_OS_MINOR >= 8))
#include <sys/devpoll.h>
#endif
#elif defined(ALTI_CFG_OS_TRU64)
#include <math.h>
#include <fp_class.h>
#include <excpt.h>
#include <rld_interface.h>
#include <sys/sysinfo.h>
#include <machine/hal_sysinfo.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/processor.h>
#include <radset.h>
#include <numa.h>
#elif defined(ALTI_CFG_OS_FREEBSD)
#include <elf.h>
#include <ifaddrs.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <sys/disk.h>
#elif defined(ALTI_CFG_OS_DARWIN)
#include <stdio.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <net/if.h> 
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/utsname.h>
#include <sys/sysctl.h>
#include <sys/mount.h>
#include <sys/time.h>
#include <mach/mach.h>
#include <mach/mach_error.h>
#include <ifaddrs.h>
#include <net/if_dl.h>
#include <sys/socket.h>
#include <sys/disk.h>
#endif

#if defined(_MSC_pppVER) && (_MSC_VER >= 1400)
#pragma warning(push)
#pragma warning(disable: 4995)
#include <intrin.h>
#pragma warning(pop)
#endif

#if defined(ALTI_CFG_OS_WINDOWS)
#  define ACP_MAKE_EXE_FILENAME(aPath) (aPath "." ACP_PROC_EXE_EXT)
#else
#  define ACP_MAKE_EXE_FILENAME(aPath) (aPath)
#endif

#if defined(ALTI_CFG_OS_TRU64)
extern int setenv(const char *, const char *, int);
extern int fp_class(double);
extern int finite(double);
#elif defined(ALTI_CFG_OS_AIX)
extern int getkerninfo(int, struct kinfo_ndd*, int *, int32long64_t);
extern int mread_real_time(timebasestruct_t *, size_t);
#endif

#endif
