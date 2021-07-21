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
 
// PDL.cpp,v 4.266 2000/02/07 23:26:24 kirthika Exp

#define PDL_BUILD_DLL
#include "PDL2.h"
#include "Handle_Set2.h"
// #include "Auto_Ptr.h"
// #include "INET_Addr.h"
// #include "Object_Manager.h"
// #include "SString.h"
#include "PDLVersion.h"
//#include "Message_Block.h"

#if defined (PDL_LACKS_INLINE_FUNCTIONS)
#include "PDL2.i"
#endif /* PDL_LACKS_INLINE_FUNCTIONS */



PDL_RCSID(pdl, PDL, "PDL.cpp,v 4.266 2000/02/07 23:26:24 kirthika Exp")

// Static data members.
u_int PDL::init_fini_count_ = 0;

// Keeps track of whether we're in some global debug mode.
char PDL::debug_ = 0;

// Hex characters.
const char PDL::hex_chars_[] = "0123456789abcdef";

// Size of a VM page.
size_t PDL::pagesize_ = 0;

// Size of allocation granularity.
size_t PDL::allocation_granularity_ = 0;

int
PDL::out_of_handles (int error)
{
  // EMFILE is common to all platforms.
  if (error == EMFILE ||
#if defined (PDL_WIN32)
      // On Win32, we need to check for ENOBUFS also.
      error == ENOBUFS ||
#elif defined (HPUX)
      // On HPUX, we need to check for EADDRNOTAVAIL also.
      error == EADDRNOTAVAIL ||
#elif defined (linux)
      // On linux, we need to check for ENOENT also.
      error == ENOENT ||
      // For RedHat5.2, need to check for EINVAL too.
      error == EINVAL ||
      // Without threads check for EOPNOTSUPP
      error == EOPNOTSUPP ||
#elif defined (sun)
      // On sun, we need to check for ENOSR also.
      error == ENOSR ||
      // Without threads check for ENOTSUP
      error == ENOTSUP ||
#elif defined (__FreeBSD__)
      // On FreeBSD we need to check for EOPNOTSUPP (LinuxThreads) or
      // ENOSYS (libc_r threads) also.
       error == EOPNOTSUPP ||
       error == ENOSYS ||
#endif /* PDL_WIN32 */
      error == ENFILE)
    return 1;
  else
    return 0;
}

u_int
PDL::major_version (void)
{
  return PDL_MAJOR_VERSION;
}

u_int
PDL::minor_version (void)
{
  return PDL_MINOR_VERSION;
}

u_int
PDL::beta_version (void)
{
  return PDL_BETA_VERSION;
}

const char*
PDL::compiler_name (void)
{
#ifdef PDL_CC_NAME
  return PDL_CC_NAME;
#else
  return "";
#endif
}

u_int
PDL::compiler_major_version (void)
{
#ifdef PDL_CC_MAJOR_VERSION
  return PDL_CC_MAJOR_VERSION;
#else
  return 0;
#endif
}

u_int
PDL::compiler_minor_version (void)
{
#ifdef PDL_CC_MINOR_VERSION
  return PDL_CC_MINOR_VERSION;
#else
  return 0;
#endif
}

u_int
PDL::compiler_beta_version (void)
{
#ifdef PDL_CC_BETA_VERSION
  return PDL_CC_BETA_VERSION;
#else
  return 0;
#endif
}

int
PDL::terminate_process (pid_t pid)
{
#if defined (PDL_HAS_PHARLAP)
  PDL_UNUSED_ARG (pid);
  PDL_NOTSUP_RETURN (-1);
#elif defined (PDL_WIN32)
  // Create a handle for the given process id.
  PDL_HANDLE process_handle =
    ::OpenProcess (PROCESS_TERMINATE,
                   FALSE, // New handle is not inheritable.
                   pid);

  if (process_handle == PDL_INVALID_HANDLE
      || process_handle == 0)
    return -1;
  else
    {
      // Kill the process associated with process_handle.
      BOOL terminate_result =
        ::TerminateProcess (process_handle, 0);
      // Free up the kernel resources.
      PDL_OS::close (process_handle);
      return terminate_result;
    }
#elif defined (CHORUS)
  KnCap cap_;

  // Use the pid to find out the actor's capability, then kill it.
  if (::acap (AM_MYSITE, pid, &cap_) == 0)
    return ::akill (&cap_);
  else
    return -1;
#else
  return PDL_OS::kill (pid, 9);
#endif /* PDL_WIN32 */
}

int
PDL::process_active (pid_t pid)
{
#if !defined(PDL_WIN32)
  int retval = PDL_OS::kill (pid, 0);

  if (retval == 0)
    return 1;
  else if (errno == ESRCH)
    return 0;
  else
    return -1;
#else
  // Create a handle for the given process id.
  PDL_HANDLE process_handle =
    ::OpenProcess (PROCESS_QUERY_INFORMATION, FALSE, pid);
  if (process_handle == PDL_INVALID_HANDLE
      || process_handle == 0)
  {
    return 0;
  }
  else
  {
      DWORD status;
      if (::GetExitCodeProcess (process_handle,
                                &status) == 0
          || status != STILL_ACTIVE)
     {
       ::CloseHandle(process_handle);
        return 0;
     }
      else
     {
       ::CloseHandle(process_handle);
        return 1;
     }
  }
#endif /* PDL_WIN32 */
}

// Split a string up into 'token'-delimited pieces, ala Perl's
// "split".

char *
PDL::strsplit_r (char *str,
                 const char *token,
                 char *&next_start)
{
  char *result = 0;

  if (str != 0)
    next_start = str;

  if (next_start != 0)
    {
      char *tok_loc = PDL_OS::strstr (next_start, token);

      if (tok_loc != 0)
        {
          // Return the beginning of the string.
          result = next_start;

          // Insure it's terminated.
          *tok_loc = '\0';
          next_start = tok_loc + PDL_OS::strlen (token);
        }
      else
        {
          result = next_start;
          next_start = (char *) 0;
        }
    }

  return result;
}

const char *
PDL::execname (const char *old_name)
{
#if defined (PDL_WIN32)
  if (PDL_OS::strstr (old_name, ".exe") == 0)
    {
      char *new_name;

      size_t size =
        PDL_OS::strlen (old_name)
        + PDL_OS::strlen (".exe")
        + 1;

      PDL_NEW_RETURN (new_name,
                      char,
                      size,
                      0);
      char *end = new_name;

      end = PDL_OS::strecpy (new_name, old_name);

      // Concatenate the .exe suffix onto the end of the executable.
      PDL_OS::strcpy (end, ".exe");

      return new_name;
    }
#endif /* PDL_WIN32 */
  return old_name;
}

#if defined (PDL_HAS_UNICODE)
size_t
PDL::strrepl (wchar_t *s, wchar_t search, wchar_t replace)
{
  PDL_TRACE ("PDL::strrepl");

  size_t replaced = 0;

  for (size_t i = 0; s[i] != '\0'; i++)
    if (s[i] == search)
      {
        s[i] = replace;
        replaced++;
      }

  return replaced;
}

wchar_t *
PDL::strsplit_r (wchar_t *str,
                 const wchar_t *token,
                 wchar_t *&next_start)
{
  wchar_t *result = 0;

  if (str != 0)
    next_start = str;

  if (next_start != 0)
    {
      wchar_t *tok_loc = PDL_OS::strstr (next_start, token);

      if (tok_loc != 0)
        {
          // Return the beginning of the string.
          result = next_start;

          // Insure it's terminated.
          *tok_loc = '\0';
          next_start = tok_loc + PDL_OS::strlen (token);
        }
      else
        {
          result = next_start;
          next_start = (wchar_t *) 0;
        }
    }

  return result;
}

const wchar_t *
PDL::execname (const wchar_t *old_name)
{
#if defined (PDL_WIN32)
  if (PDL_OS::strstr (old_name, L".exe") == 0)
    {
      wchar_t *new_name;

      size_t size =
        PDL_OS::strlen (old_name)
        + PDL_OS::strlen (L".exe")
        + 1;

      PDL_NEW_RETURN (new_name,
                      wchar_t,
                      size,
                      0);
      wchar_t *end = new_name;

      end = PDL_OS::strecpy (new_name, old_name);

      // Concatenate the .exe suffix onto the end of the executable.
      PDL_OS::strcpy (end, L".exe");

      return new_name;
    }
#endif /* PDL_WIN32 */
  return old_name;
}
#endif /* PDL_HAS_UNICODE */

u_long
PDL::hash_pjw (const char *str, size_t len)
{
  u_long hash = 0;

  for (size_t i = 0; i < len; i++)
    {
      const char temp = str[i];
      hash = (hash << 4) + (temp * 13);

      u_long g = hash & 0xf0000000;

      if (g)
        {
          hash ^= (g >> 24);
          hash ^= g;
        }
    }

  return hash;
}

u_long
PDL::hash_pjw (const char *str)
{
  return PDL::hash_pjw (str, PDL_OS::strlen (str));
}

#if !defined (PDL_HAS_WCHAR_TYPEDEFS_CHAR)
u_long
PDL::hash_pjw (const wchar_t *str, size_t len)
{
  u_long hash = 0;

  for (size_t i = 0; i < len; i++)
    {
      const wchar_t temp = str[i];
      hash = (hash << 4) + (temp * 13);

      u_long g = hash & 0xf0000000;

      if (g)
        {
          hash ^= (g >> 24);
          hash ^= g;
        }
    }

  return hash;
}

u_long
PDL::hash_pjw (const wchar_t *str)
{
  return PDL::hash_pjw (str, PDL_OS::strlen (str));
}
#endif /* PDL_HAS_WCHAR_TYPEDEFS_CHAR */

#if !defined (PDL_HAS_WCHAR_TYPEDEFS_USHORT)
u_long
PDL::hash_pjw (const PDL_USHORT16 *str, size_t len)
{
  u_long hash = 0;

  for (size_t i = 0; i < len; i++)
    {
      const PDL_USHORT16 temp = str[i];
      hash = (hash << 4) + (temp * 13);

      u_long g = hash & 0xf0000000;

      if (g)
        {
          hash ^= (g >> 24);
          hash ^= g;
        }
    }

  return hash;
}

u_long
PDL::hash_pjw (const PDL_USHORT16 *str)
{
  return PDL::hash_pjw (str, PDL_OS::strlen (str));
}
#endif /* ! PDL_HAS_WCHAR_TYPEDEFS_USHORT */

// The CRC routine was taken from the FreeBSD implementation of cksum,
// that falls under the following license:
/*-
 * Copyright (c) 1991, 1993
 *      The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * James W. Williams of NASA Goddard Space Flight Center.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

u_long PDL::crc_table_[] =
{
  0x0,
  0x04c11db7, 0x09823b6e, 0x0d4326d9, 0x130476dc, 0x17c56b6b,
  0x1a864db2, 0x1e475005, 0x2608edb8, 0x22c9f00f, 0x2f8ad6d6,
  0x2b4bcb61, 0x350c9b64, 0x31cd86d3, 0x3c8ea00a, 0x384fbdbd,
  0x4c11db70, 0x48d0c6c7, 0x4593e01e, 0x4152fda9, 0x5f15adac,
  0x5bd4b01b, 0x569796c2, 0x52568b75, 0x6a1936c8, 0x6ed82b7f,
  0x639b0da6, 0x675a1011, 0x791d4014, 0x7ddc5da3, 0x709f7b7a,
  0x745e66cd, 0x9823b6e0, 0x9ce2ab57, 0x91a18d8e, 0x95609039,
  0x8b27c03c, 0x8fe6dd8b, 0x82a5fb52, 0x8664e6e5, 0xbe2b5b58,
  0xbaea46ef, 0xb7a96036, 0xb3687d81, 0xad2f2d84, 0xa9ee3033,
  0xa4ad16ea, 0xa06c0b5d, 0xd4326d90, 0xd0f37027, 0xddb056fe,
  0xd9714b49, 0xc7361b4c, 0xc3f706fb, 0xceb42022, 0xca753d95,
  0xf23a8028, 0xf6fb9d9f, 0xfbb8bb46, 0xff79a6f1, 0xe13ef6f4,
  0xe5ffeb43, 0xe8bccd9a, 0xec7dd02d, 0x34867077, 0x30476dc0,
  0x3d044b19, 0x39c556ae, 0x278206ab, 0x23431b1c, 0x2e003dc5,
  0x2ac12072, 0x128e9dcf, 0x164f8078, 0x1b0ca6a1, 0x1fcdbb16,
  0x018aeb13, 0x054bf6a4, 0x0808d07d, 0x0cc9cdca, 0x7897ab07,
  0x7c56b6b0, 0x71159069, 0x75d48dde, 0x6b93dddb, 0x6f52c06c,
  0x6211e6b5, 0x66d0fb02, 0x5e9f46bf, 0x5a5e5b08, 0x571d7dd1,
  0x53dc6066, 0x4d9b3063, 0x495a2dd4, 0x44190b0d, 0x40d816ba,
  0xaca5c697, 0xa864db20, 0xa527fdf9, 0xa1e6e04e, 0xbfa1b04b,
  0xbb60adfc, 0xb6238b25, 0xb2e29692, 0x8aad2b2f, 0x8e6c3698,
  0x832f1041, 0x87ee0df6, 0x99a95df3, 0x9d684044, 0x902b669d,
  0x94ea7b2a, 0xe0b41de7, 0xe4750050, 0xe9362689, 0xedf73b3e,
  0xf3b06b3b, 0xf771768c, 0xfa325055, 0xfef34de2, 0xc6bcf05f,
  0xc27dede8, 0xcf3ecb31, 0xcbffd686, 0xd5b88683, 0xd1799b34,
  0xdc3abded, 0xd8fba05a, 0x690ce0ee, 0x6dcdfd59, 0x608edb80,
  0x644fc637, 0x7a089632, 0x7ec98b85, 0x738aad5c, 0x774bb0eb,
  0x4f040d56, 0x4bc510e1, 0x46863638, 0x42472b8f, 0x5c007b8a,
  0x58c1663d, 0x558240e4, 0x51435d53, 0x251d3b9e, 0x21dc2629,
  0x2c9f00f0, 0x285e1d47, 0x36194d42, 0x32d850f5, 0x3f9b762c,
  0x3b5a6b9b, 0x0315d626, 0x07d4cb91, 0x0a97ed48, 0x0e56f0ff,
  0x1011a0fa, 0x14d0bd4d, 0x19939b94, 0x1d528623, 0xf12f560e,
  0xf5ee4bb9, 0xf8ad6d60, 0xfc6c70d7, 0xe22b20d2, 0xe6ea3d65,
  0xeba91bbc, 0xef68060b, 0xd727bbb6, 0xd3e6a601, 0xdea580d8,
  0xda649d6f, 0xc423cd6a, 0xc0e2d0dd, 0xcda1f604, 0xc960ebb3,
  0xbd3e8d7e, 0xb9ff90c9, 0xb4bcb610, 0xb07daba7, 0xae3afba2,
  0xaafbe615, 0xa7b8c0cc, 0xa379dd7b, 0x9b3660c6, 0x9ff77d71,
  0x92b45ba8, 0x9675461f, 0x8832161a, 0x8cf30bad, 0x81b02d74,
  0x857130c3, 0x5d8a9099, 0x594b8d2e, 0x5408abf7, 0x50c9b640,
  0x4e8ee645, 0x4a4ffbf2, 0x470cdd2b, 0x43cdc09c, 0x7b827d21,
  0x7f436096, 0x7200464f, 0x76c15bf8, 0x68860bfd, 0x6c47164a,
  0x61043093, 0x65c52d24, 0x119b4be9, 0x155a565e, 0x18197087,
  0x1cd86d30, 0x029f3d35, 0x065e2082, 0x0b1d065b, 0x0fdc1bec,
  0x3793a651, 0x3352bbe6, 0x3e119d3f, 0x3ad08088, 0x2497d08d,
  0x2056cd3a, 0x2d15ebe3, 0x29d4f654, 0xc5a92679, 0xc1683bce,
  0xcc2b1d17, 0xc8ea00a0, 0xd6ad50a5, 0xd26c4d12, 0xdf2f6bcb,
  0xdbee767c, 0xe3a1cbc1, 0xe760d676, 0xea23f0af, 0xeee2ed18,
  0xf0a5bd1d, 0xf464a0aa, 0xf9278673, 0xfde69bc4, 0x89b8fd09,
  0x8d79e0be, 0x803ac667, 0x84fbdbd0, 0x9abc8bd5, 0x9e7d9662,
  0x933eb0bb, 0x97ffad0c, 0xafb010b1, 0xab710d06, 0xa6322bdf,
  0xa2f33668, 0xbcb4666d, 0xb8757bda, 0xb5365d03, 0xb1f740b4
};

// Compute a POSIX 1003.2 checksum.  The routine takes an string and
// computes the CRC for it (it stops on the first '\0' character).

// UNICOS UINT32's are 64-bit on the Cray PVP architecture
#if !defined(_UNICOS)
#  define COMPUTE(var, ch) (var) = ((var) << 8) ^ PDL::crc_table_[((var) >> 24) ^ (ch)]
#else /* ! _UNICOS */
#  define COMPUTE(var, ch) (var) = ( 0x00000000ffffffff & ((var) << 8)) ^ PDL::crc_table_[((var) >> 24) ^ (ch)]
#endif /* ! _UNICOS */

u_long
PDL::crc32 (const char *string)
{
  register PDL_UINT32 crc = 0;

  u_long len = 0;

  for (const char *p = string;
       *p != 0;
       ++p)
    {
      COMPUTE (crc, *p);
      ++len;
    }

  // Include the length of the string.

  for (; len != 0; len >>= 8)
    COMPUTE (crc, len & 0xff);

  return ~crc;
}

u_long
PDL::crc32 (const char *buffer, PDL_UINT32 len)
{
  register PDL_UINT32 crc = 0;

  for (const char *p = buffer;
       p != buffer + len;
       ++p)
    {
      COMPUTE (crc, *p);
    }

  // Include the length of the string.

  for (; len != 0; len >>= 8)
    COMPUTE (crc, len & 0xff);

  return ~crc;
}

#undef COMPUTE

size_t
PDL::strrepl (char *s, char search, char replace)
{
  PDL_TRACE ("PDL::strrepl");

  size_t replaced = 0;

  for (size_t i = 0; s[i] != '\0'; i++)
    if (s[i] == search)
      {
        s[i] = replace;
        replaced++;
      }

  return replaced;
}

#if !defined (PDL_HAS_WINCE)
ASYS_TCHAR *
PDL::strenvdup (const ASYS_TCHAR *str)
{
  PDL_TRACE ("PDL::strenvdup");

  return PDL_OS::strenvdup (str);
}
#endif /* PDL_HAS_WINCE */

/*

Examples:

Source               NT                    UNIX
==================================================================
netsvc               netsvc.dll            libnetsvc.so
                     (PATH will be         (LD_LIBRARY_PATH
                      evaluated)            evaluated)

libnetsvc.dll        libnetsvc.dll         libnetsvc.dll + warning
netsvc.so            netsvc.so + warning   libnetsvc.so

..\../libs/netsvc    ..\..\libs\netsvc.dll ../../libs/netsvc.so
                     (absolute path used)  (absolute path used)

*/

#if ! defined (PDL_PSOS_DIAB_MIPS)
int
PDL::ldfind (const ASYS_TCHAR filename[],
             ASYS_TCHAR       pathname[],
             size_t           maxpathnamelen)
{
  PDL_TRACE ("PDL::ldfind");

  ASYS_TCHAR  tempcopy[MAXPATHLEN + 1];
  ASYS_TCHAR  searchpathname[MAXPATHLEN + 1];
  ASYS_TCHAR  searchfilename[MAXPATHLEN + 2];
  ASYS_TCHAR *s;
  ASYS_TCHAR *separator_ptr;
  const ASYS_TCHAR *dll_suffix = ASYS_TEXT (PDL_DLL_SUFFIX);

#if defined (PDL_WIN32) && !defined (PDL_HAS_WINCE) && \
    !defined (PDL_HAS_PHARLAP)
  ASYS_TCHAR expanded_filename[MAXPATHLEN];
#if !defined (PDL_HAS_MOSTLY_UNICODE_APIS)
  if (::ExpandEnvironmentStringsA (filename,
                                   expanded_filename,
                                   sizeof expanded_filename))
#else
  if (::ExpandEnvironmentStringsW (filename,
                                   expanded_filename,
                                   sizeof expanded_filename))
#endif /* PDL_HAS_MOSTLY_UNICODE_APIS */
    filename = expanded_filename;
#endif /* PDL_WIN32 && !PDL_HAS_WINCE && !PDL_HAS_PHARLAP */

  // Create a copy of filename to work with.
  if (PDL_OS::strlen (filename) + 1
      > (sizeof tempcopy / sizeof (ASYS_TCHAR)))
    {
      errno = ENOMEM;
      return -1;
    }
  else
    PDL_OS::strcpy (tempcopy, filename);

  // Insert canonical directory separators.

#if (PDL_DIRECTORY_SEPARATOR_CHAR != '/')
  // Make all the directory separators ``canonical'' to simplify
  // subsequent code.
  PDL::strrepl (tempcopy, PDL_DIRECTORY_SEPARATOR_CHAR, '/');
#endif /* PDL_DIRECTORY_SEPARATOR_CHAR */

  // Separate filename from pathname.
  separator_ptr = PDL_OS::strrchr (tempcopy, '/');

  // This is a relative path.
  if (separator_ptr == 0)
    {
      searchpathname[0] = '\0';
      PDL_OS::strcpy (searchfilename, tempcopy);
    }
  else // This is an absolute path.
    {
      PDL_OS::strcpy (searchfilename, separator_ptr + 1);
      separator_ptr[1] = '\0';
      PDL_OS::strcpy (searchpathname, tempcopy);
    }

  int got_suffix = 0;

  // Check to see if this has an appropriate DLL suffix for the OS
  // platform.

  s = PDL_OS::strrchr (searchfilename, '.');

  if (s != 0)
    {
      // If we have a dot, we have a suffix
      got_suffix = 1;

      // Check whether this matches the appropriate platform-specific
      // suffix.
      if (PDL_OS::strcmp (s, dll_suffix) != 0)
        {
          PDL_ERROR ((LM_WARNING,
                      ASYS_TEXT ("Warning: improper suffix for a ")
                      ASYS_TEXT ("shared library on this platform: %s\n"),
                      s));
        }
    }

  // Make sure we've got enough space in searchfilename.
  if (PDL_OS::strlen (searchfilename)
      + PDL_OS::strlen (PDL_DLL_PREFIX)
      + got_suffix ? 0 : PDL_OS::strlen (dll_suffix) >= (sizeof searchfilename /
                                                         sizeof (ASYS_TCHAR)))
    {
      errno = ENOMEM;
      return -1;
    }

#if defined (PDL_WIN32) && defined (_DEBUG) && !defined (PDL_DISABLE_DEBUG_DLL_CHECK)
  size_t len_searchfilename = PDL_OS::strlen (searchfilename);
  if (! got_suffix)
    {
      searchfilename [len_searchfilename] = 'd';
      searchfilename [len_searchfilename+1] = 0;
    }

  for (int tag = 1; tag >= 0; tag --)
    {
      if (tag == 0)
        searchfilename [len_searchfilename] = 0;

#endif /* PDL_WIN32 && _DEBUG && !PDL_DISABLE_DEBUG_DLL_CHECK */
  // Use absolute pathname if there is one.
  if (PDL_OS::strlen (searchpathname) > 0)
    {
      if (PDL_OS::strlen (searchfilename)
          + PDL_OS::strlen (searchpathname) >= maxpathnamelen)
        {
          errno = ENOMEM;
          return -1;
        }
      else
        {
#if (PDL_DIRECTORY_SEPARATOR_CHAR != '/')
          // Revert to native path name separators.
          PDL::strrepl (searchpathname,
                        '/',
                        PDL_DIRECTORY_SEPARATOR_CHAR);
#endif /* PDL_DIRECTORY_SEPARATOR_CHAR */
          // First, try matching the filename *without* adding a
          // prefix.
#if defined (PDL_HAS_BROKEN_CONDITIONAL_STRING_CASTS)
          PDL_OS::sprintf (pathname,
                           ASYS_TEXT ("%s%s%s"),
                           searchpathname,
                           searchfilename,
                           got_suffix ? PDL_static_cast (char *,
                                                         ASYS_TEXT (""))
                                      : PDL_static_cast (char *,
                                                         dll_suffix));
#else /* ! defined (PDL_HAS_BROKEN_CONDITIONAL_STRING_CASTS) */
          PDL_OS::sprintf (pathname,
                           ASYS_TEXT ("%s%s%s"),
                           searchpathname,
                           searchfilename,
                           got_suffix ? ASYS_TEXT ("") : dll_suffix);
#endif /* ! defined (PDL_HAS_BROKEN_CONDITIONAL_STRING_CASTS) */
          if (PDL_OS::access (pathname, F_OK) == 0)
            return 0;

          // Second, try matching the filename *with* adding a prefix.
#if defined (PDL_HAS_BROKEN_CONDITIONAL_STRING_CASTS)
          PDL_OS::sprintf (pathname,
                           ASYS_TEXT ("%s%s%s%s"),
                           searchpathname,
                           PDL_DLL_PREFIX,
                           searchfilename,
                           got_suffix ? PDL_static_cast (char *,
                                                         ASYS_TEXT (""))
                                      : PDL_static_cast (char *,
                                                         dll_suffix));
#else /* ! defined (PDL_HAS_BROKEN_CONDITIONAL_STRING_CASTS) */
          PDL_OS::sprintf (pathname,
                           ASYS_TEXT ("%s%s%s%s"),
                           searchpathname,
                           PDL_DLL_PREFIX,
                           searchfilename,
                           got_suffix ? ASYS_TEXT ("") : dll_suffix);
#endif /* ! defined (PDL_HAS_BROKEN_CONDITIONAL_STRING_CASTS) */
          if (PDL_OS::access (pathname, F_OK) == 0)
            return 0;
        }
    }

  // Use relative filenames via LD_LIBRARY_PATH or PATH (depending on
  // OS platform).
  else
    {
      char *ld_path =
#if defined PDL_DEFAULT_LD_SEARCH_PATH
        PDL_DEFAULT_LD_SEARCH_PATH;
#else
        PDL_OS::getenv (PDL_LD_SEARCH_PATH);
#endif /* PDL_DEFAULT_LD_SEARCH_PATH */

#if defined (PDL_WIN32)
      char *ld_path_temp = 0;
      if (ld_path != 0)
        {
          ld_path_temp = (char *) PDL_OS::malloc (PDL_OS::strlen (ld_path) + 2);
          if (ld_path_temp != 0)
            {
              PDL_OS::strcpy (ld_path_temp, PDL_LD_SEARCH_PATH_SEPARATOR_STR);
              PDL_OS::strcat (ld_path_temp, ld_path);
              ld_path = ld_path_temp;
            }
          else
            {
              PDL_OS::free ((void *) ld_path_temp);
              ld_path = ld_path_temp = 0;
            }
        }
#endif /* PDL_WIN32 */

      if (ld_path != 0
          && (ld_path = PDL_OS::strdup (ld_path)) != 0)
        {
          // strtok has the strange behavior of not separating the
          // string ":/foo:/bar" into THREE tokens.  One would expect
          // that the first iteration the token would be an empty
          // string, the second iteration would be "/foo", and the
          // third iteration would be "/bar".  However, this is not
          // the case; one only gets two iterations: "/foo" followed
          // by "/bar".

          // This is especially a problem in parsing Unix paths
          // because it is permissible to specify 'the current
          // directory' as an empty entry.  So, we introduce the
          // following special code to cope with this:

          // Look at each dynamic lib directory in the search path.

          char *nextholder = 0;
          const char *path_entry =
            PDL::strsplit_r (ld_path,
                             PDL_LD_SEARCH_PATH_SEPARATOR_STR,
                             nextholder);
          int result = 0;

          for (;;)
            {
              // Check if at end of search path.
              if (path_entry == 0)
                {
                  errno = ENOENT;
                  result = -1;
                  break;
                }
              else if (PDL_OS::strlen (path_entry)
                       + 1
                       + PDL_OS::strlen (searchfilename)
                       >= maxpathnamelen)
                {
                  errno = ENOMEM;
                  result = -1;
                  break;
                }
              // This works around the issue where a path might have
              // an empty component indicating 'current directory'.
              // We need to do it here rather than anywhere else so
              // that the loop condition will still work.
              else if (path_entry[0] == '\0')
                path_entry = ".";

              // First, try matching the filename *without* adding a
              // prefix.
#if defined (PDL_HAS_BROKEN_CONDITIONAL_STRING_CASTS)
              PDL_OS::sprintf (pathname,
                               ASYS_TEXT ("%s%c%s%s"),
                               path_entry,
                               PDL_DIRECTORY_SEPARATOR_CHAR,
                               searchfilename,
                               got_suffix ? PDL_static_cast (char *,
                                                             ASYS_TEXT (""))
                                          : PDL_static_cast (char *,
                                                             dll_suffix));
#else /* ! defined (PDL_HAS_BROKEN_CONDITIONAL_STRING_CASTS) */
              PDL_OS::sprintf (pathname,
                               ASYS_TEXT ("%s%c%s%s"),
                               path_entry,
                               PDL_DIRECTORY_SEPARATOR_CHAR,
                               searchfilename,
                               got_suffix ? ASYS_TEXT ("") : dll_suffix);
#endif /* ! defined (PDL_HAS_BROKEN_CONDITIONAL_STRING_CASTS) */
              if (PDL_OS::access (pathname, F_OK) == 0)
                break;

              // Second, try matching the filename *with* adding a
              // prefix.
#if defined (PDL_HAS_BROKEN_CONDITIONAL_STRING_CASTS)
              PDL_OS::sprintf (pathname,
                               ASYS_TEXT ("%s%c%s%s%s"),
                               path_entry,
                               PDL_DIRECTORY_SEPARATOR_CHAR,
                               PDL_DLL_PREFIX,
                               searchfilename,
                               got_suffix ? PDL_static_cast (char *,
                                                            ASYS_TEXT (""))
                                          : PDL_static_cast (char *,
                                                             dll_suffix));
#else /* ! defined (PDL_HAS_BROKEN_CONDITIONAL_STRING_CASTS) */
              PDL_OS::sprintf (pathname,
                               ASYS_TEXT ("%s%c%s%s%s"),
                               path_entry,
                               PDL_DIRECTORY_SEPARATOR_CHAR,
                               PDL_DLL_PREFIX,
                               searchfilename,
                               got_suffix ? ASYS_TEXT ("") : dll_suffix);
#endif /* ! defined (PDL_HAS_BROKEN_CONDITIONAL_STRING_CASTS) */
              if (PDL_OS::access (pathname, F_OK) == 0)
                break;

              // Fetch the next item in the path
              path_entry = PDL::strsplit_r (0,
                                            PDL_LD_SEARCH_PATH_SEPARATOR_STR,
                                            nextholder);
            }

#if defined (PDL_WIN32)
          if (ld_path_temp != 0)
            PDL_OS::free (ld_path_temp);
#endif /* PDL_WIN32 */
          PDL_OS::free ((void *) ld_path);
#if defined (PDL_WIN32) && defined (_DEBUG) && !defined (PDL_DISABLE_DEBUG_DLL_CHECK)
          if (result == 0 || tag == 0)
#endif /* PDL_WIN32 && _DEBUG && !PDL_DISABLE_DEBUG_DLL_CHECK */
            return result;
        }
    }
#if defined (PDL_WIN32) && defined (_DEBUG) && !defined (PDL_DISABLE_DEBUG_DLL_CHECK)
    }
#endif /* PDL_WIN32 && _DEBUG && !PDL_DISABLE_DEBUG_DLL_CHECK */

  errno = ENOENT;
  return -1;
}

FILE *
PDL::ldopen (const ASYS_TCHAR *filename,
             const ASYS_TCHAR *type)
{
  PDL_TRACE ("PDL::ldopen");

  ASYS_TCHAR buf[MAXPATHLEN + 1];
  if (PDL::ldfind (filename,
                   buf,
                   sizeof (buf) /sizeof (ASYS_TCHAR)) == -1)
    return 0;
  else
    return PDL_OS::fopen (buf, type);
}

PDL_HANDLE
PDL::open_temp_file (const char *name, int mode, int perm)
{
#if defined (PDL_WIN32)
  return PDL_OS::open (name,
                       mode | _O_TEMPORARY);
#else
  // Open it.
  PDL_HANDLE handle = PDL_OS::open (name, mode, perm);

  if (handle == PDL_INVALID_HANDLE)
    return PDL_INVALID_HANDLE;

  // Unlink it so that the file will be removed automatically when the
  // process goes away.
  if (PDL_OS::unlink (name) == -1)
    return -1;
  else
    // Return the handle.
    return handle;
#endif /* PDL_WIN32 */
}
#endif /* ! PDL_PSOS_DIAB_MIPS */

const char *
PDL::basename (const char *pathname, char delim)
{
  const char *temp = PDL_OS::strrchr (pathname, delim);

  if (temp == 0)
    return pathname;
  else
    return temp + 1;
}

const char *
PDL::dirname (const char *pathname, char delim)
{
  static char return_dirname[MAXPATHLEN + 1];

  const char *temp = PDL_OS::strrchr (pathname, delim);

  if (temp == 0)
    {
      return_dirname[0] = '.';
      return_dirname[1] = '\0';

      return return_dirname;
    }
  else
    {
      PDL_OS::strncpy (return_dirname,
                       pathname,
                       MAXPATHLEN);
      return_dirname[temp - pathname] = '\0';
      return return_dirname;
    }
}

#if defined (PDL_HAS_UNICODE)
const wchar_t *
PDL::basename (const wchar_t *pathname, wchar_t delim)
{
  PDL_TRACE ("PDL::basename");
  const wchar_t *temp = PDL_OS::strrchr (pathname, delim);

  if (temp == 0)
    return pathname;
  else
    return temp + 1;
}
#endif /* PDL_HAS_UNICODE */

ssize_t
PDL::recv (PDL_SOCKET handle,
           void *buf,
           size_t len,
           int flags,
           const PDL_Time_Value *timeout)
{
  if (timeout == 0)
    return PDL_OS::recv (handle, (char *) buf, len, flags);
  else
    {
#if defined (PDL_HAS_RECV_TIMEDWAIT)
      PDL_Time_Value copy = *timeout;
      copy += PDL_OS::gettimeofday ();
      timespec_t ts = copy;
      return ::recv_timedwait (handle, buf, len, flags, &ts);
#else
      int val = 0;
      if (PDL::enter_recv_timedwait (handle, timeout, val) ==-1)
        return -1;
      else
        {
          ssize_t bytes_transferred = PDL_OS::recv (handle, (char *) buf, len, flags);
          PDL::restore_non_blocking_mode (handle, val);
          return bytes_transferred;
        }
#endif /* PDL_HAS_RECV_TIMEDWAIT */
    }
}

ssize_t
PDL::recv (PDL_SOCKET handle,
           void *buf,
           size_t n,
           const PDL_Time_Value *timeout)
{
  if (timeout == 0)
    return PDL::recv_i (handle, buf, n);
  else
    {
#if defined (PDL_HAS_READ_TIMEDWAIT)
      PDL_Time_Value copy = *timeout;
      copy += PDL_OS::gettimeofday ();
      timespec_t ts = copy;
      return ::read_timedwait (handle, buf, n, &ts);
#else
      int val = 0;
      if (PDL::enter_recv_timedwait (handle, timeout, val) == -1)
        return -1;
      else
        {
          ssize_t bytes_transferred = PDL::recv_i (handle, buf, n);
          PDL::restore_non_blocking_mode (handle, val);
          return bytes_transferred;
        }
#endif /* PDL_HAS_READ_TIMEDWAIT */
    }
}

ssize_t
PDL::recvmsg (PDL_SOCKET handle,
              struct msghdr *msg,
              int flags,
              const PDL_Time_Value *timeout)
{
  if (timeout == 0)
    return PDL_OS::recvmsg (handle, msg, flags);
  else
    {
#if defined (PDL_HAS_RECVMSG_TIMEDWAIT)
      PDL_Time_Value copy = *timeout;
      copy += PDL_OS::gettimeofday ();
      timespec_t ts = copy;
      return ::recvmsg_timedwait (handle, msg, flags, &ts);
#else
      int val = 0;
      if (PDL::enter_recv_timedwait (handle, timeout, val) == -1)
        return -1;
      else
        {
          int bytes_transferred = PDL_OS::recvmsg (handle, msg, flags);
          PDL::restore_non_blocking_mode (handle, val);
          return bytes_transferred;
        }
#endif /* PDL_HAS_RECVMSG_TIMEDWAIT */
    }
}

ssize_t
PDL::recvfrom (PDL_SOCKET handle,
               char *buf,
               int len,
               int flags,
               struct sockaddr *addr,
               int *addrlen,
               const PDL_Time_Value *timeout)
{
  if (timeout == 0)
    return PDL_OS::recvfrom (handle, buf, len, flags, addr, addrlen);
  else
    {
#if defined (PDL_HAS_RECVFROM_TIMEDWAIT)
      PDL_Time_Value copy = *timeout;
      copy += PDL_OS::gettimeofday ();
      timespec_t ts = copy;
      return ::recvfrom_timedwait (handle, buf, len, flags, addr, addrlen, &ts);
#else
      int val = 0;
      if (PDL::enter_recv_timedwait (handle, timeout, val) == -1)
        return -1;
      else
        {
          int bytes_transferred = PDL_OS::recvfrom (handle, buf, len, flags, addr, addrlen);
          PDL::restore_non_blocking_mode (handle, val);
          return bytes_transferred;
        }
#endif /* PDL_HAS_RECVFROM_TIMEDWAIT */
    }
}

ssize_t
PDL::recv_n (PDL_SOCKET handle,
             void *buf,
             size_t len,
             int flags,
             const PDL_Time_Value *timeout)
{
  if (timeout == 0)
    return PDL::recv_n_i (handle,
                          buf,
                          len,
                          flags);
  else
    return PDL::recv_n_i (handle,
                          buf,
                          len,
                          flags,
                          timeout);
}

ssize_t
PDL::recv_n_i (PDL_SOCKET handle,
               void *buf,
               size_t len,
               int flags)
{
  size_t bytes_transferred;
  ssize_t n;

  for (bytes_transferred = 0;
       bytes_transferred < len;
       bytes_transferred += n)
    {
      n = PDL_OS::recv (handle,
                        (char *) buf + bytes_transferred,
                        len - bytes_transferred,
                        flags);
      if (n == -1)
        {
          // If blocked, try again.
          if (errno == EWOULDBLOCK)
            n = 0;

          //
          // No timeouts in this version.
          //

          // Other errors.
          return -1;
        }
      else if (n == 0)
        return 0;
    }

  return bytes_transferred;
}

ssize_t
PDL::recv_n_i (PDL_SOCKET handle,
               void *buf,
               size_t len,
               int flags,
               const PDL_Time_Value *timeout)
{
  int val = 0;
  PDL::record_and_set_non_blocking_mode (handle, val);

  size_t bytes_transferred;
  ssize_t n;
  int error = 0;

  for (bytes_transferred = 0;
       bytes_transferred < len;
       bytes_transferred += n)
    {
      int result = PDL::handle_read_ready (handle,
                                           timeout);

      if (result == -1)
        {
          // Timed out; return bytes transferred.
          if (errno == ETIME)
            break;

          // Other errors.
          error = 1;
          break;
        }

      n = PDL_OS::recv (handle,
                        (char *) buf + bytes_transferred,
                        len - bytes_transferred,
                        flags);

      // Errors (note that errno cannot be EWOULDBLOCK since select()
      // just told us that data is available to read).
      if (n == -1 || n == 0)
        {
          error = 1;
          break;
        }
    }

  PDL::restore_non_blocking_mode (handle, val);

  if (error)
    return -1;
  else
    return bytes_transferred;
}

ssize_t
PDL::recv_n (PDL_SOCKET handle,
             void *buf,
             size_t len,
             const PDL_Time_Value *timeout)
{
  if (timeout == 0)
    return PDL::recv_n_i (handle,
                          buf,
                          len);
  else
    return PDL::recv_n_i (handle,
                          buf,
                          len,
                          timeout);
}

ssize_t
PDL::recv_n_i (PDL_SOCKET handle,
               void *buf,
               size_t len)
{
  size_t bytes_transferred;
  ssize_t n;

  for (bytes_transferred = 0;
       bytes_transferred < len;
       bytes_transferred += n)
    {
      n = PDL::recv_i (handle,
                       (char *) buf + bytes_transferred,
                       len - bytes_transferred);
      if (n == -1)
        {
          // If blocked, try again.
          if (errno == EWOULDBLOCK)
            n = 0;

          //
          // No timeouts in this version.
          //

          // Other errors.
          return -1;
        }
      else if (n == 0)
        return 0;
    }

  return bytes_transferred;
}

ssize_t
PDL::recv_n_i (PDL_SOCKET handle,
               void *buf,
               size_t len,
               const PDL_Time_Value *timeout)
{
  int val = 0;
  PDL::record_and_set_non_blocking_mode (handle, val);

  size_t bytes_transferred;
  ssize_t n;
  int error = 0;

  for (bytes_transferred = 0;
       bytes_transferred < len;
       bytes_transferred += n)
    {
      int result = PDL::handle_read_ready (handle,
                                           timeout);

      if (result == -1)
        {
          // Timed out; return bytes transferred.
          if (errno == ETIME)
            break;

          // Other errors.
          error = 1;
          break;
        }

      n = PDL::recv_i (handle,
                       (char *) buf + bytes_transferred,
                       len - bytes_transferred);

      // Errors (note that errno cannot be EWOULDBLOCK since select()
      // just told us that data is available to read).
      if (n == -1 || n == 0)
        {
          error = 1;
          break;
        }
    }

  PDL::restore_non_blocking_mode (handle, val);

  if (error)
    return -1;
  else
    return bytes_transferred;
}

// This is basically an interface to PDL_OS::readv, that doesn't use
// the struct iovec explicitly.  The ... can be passed as an arbitrary
// number of (char *ptr, int len) tuples.  However, the count N is the
// *total* number of trailing arguments, *not* a couple of the number
// of tuple pairs!


ssize_t
PDL::recvv (PDL_SOCKET handle,
            iovec *iov,
            int iovcnt,
            const PDL_Time_Value *timeout)
{
  if (timeout == 0)
    return PDL_OS::recvv (handle, iov, iovcnt);
  else
    {
#if defined (PDL_HAS_READV_TIMEDWAIT)
      PDL_Time_Value copy = *timeout;
      copy += PDL_OS::gettimeofday ();
      timespec_t ts = copy;
      return ::readv_timedwait (handle, iov, iovcnt, &ts);
#else
      int val = 0;
      if (PDL::enter_recv_timedwait (handle, timeout, val) == -1)
        return -1;
      else
        {
          ssize_t bytes_transferred = PDL_OS::recvv (handle, iov, iovcnt);
          PDL::restore_non_blocking_mode (handle, val);
          return bytes_transferred;
        }
#endif /* PDL_HAS_READV_TIMEDWAIT */
    }
}

ssize_t
PDL::recvv_n (PDL_SOCKET handle,
              iovec *iov,
              int iovcnt,
              const PDL_Time_Value *timeout)
{
  if (timeout == 0)
    return PDL::recvv_n_i (handle,
                           iov,
                           iovcnt);
  else
    return PDL::recvv_n_i (handle,
                           iov,
                           iovcnt,
                           timeout);
}

ssize_t
PDL::recvv_n_i (PDL_SOCKET handle,
                iovec *iov,
                int iovcnt)
{
  ssize_t bytes_transferred = 0;

  for (int s = 0;
       s < iovcnt;
       )
    {
      ssize_t n = PDL_OS::recvv (handle,
                                 iov + s,
                                 iovcnt - s);
      if (n == -1)
        {
          // If blocked, try again.
          if (errno == EWOULDBLOCK)
            n = 0;

          //
          // No timeouts in this version.
          //

          // Other errors.
          return -1;
        }
      else if (n == 0)
        return 0;

      for (bytes_transferred += n;
           s < iovcnt
             && n >= PDL_static_cast (ssize_t,
                                      iov[s].iov_len);
           s++)
        n -= iov[s].iov_len;

      if (n != 0)
        {
          char *base = PDL_reinterpret_cast (char *,
                                             iov[s].iov_base);
          iov[s].iov_base = base + n;
          iov[s].iov_len = iov[s].iov_len - n;
        }
    }

  return bytes_transferred;
}

ssize_t
PDL::recvv_n_i (PDL_SOCKET handle,
                iovec *iov,
                int iovcnt,
                const PDL_Time_Value *timeout)
{
  int val = 0;
  PDL::record_and_set_non_blocking_mode (handle, val);

  ssize_t bytes_transferred = 0;
  int error = 0;

  for (int s = 0;
       s < iovcnt;
       )
    {
      int result = PDL::handle_read_ready (handle,
                                           timeout);

      if (result == -1)
        {
          // Timed out; return bytes transferred.
          if (errno == ETIME)
            break;

          // Other errors.
          error = 1;
          break;
        }

      ssize_t n = PDL_OS::recvv (handle,
                                 iov + s,
                                 iovcnt - s);

      // Errors (note that errno cannot be EWOULDBLOCK since select()
      // just told us that data is available to read).
      if (n == -1 || n == 0)
        {
          error = 1;
          break;
        }

      for (bytes_transferred += n;
           s < iovcnt
             && n >= PDL_static_cast (ssize_t,
                                      iov[s].iov_len);
           s++)
        n -= iov[s].iov_len;

      if (n != 0)
        {
          char *base = PDL_reinterpret_cast (char *,
                                             iov[s].iov_base);
          iov[s].iov_base = base + n;
          iov[s].iov_len = iov[s].iov_len - n;
        }
    }

  PDL::restore_non_blocking_mode (handle, val);

  if (error)
    return -1;
  else
    return bytes_transferred;
}


ssize_t
PDL::send (PDL_SOCKET handle,
           const void *buf,
           size_t n,
           int flags,
           const PDL_Time_Value *timeout)
{
  if (timeout == 0)
    return PDL_OS::send (handle, (const char *) buf, n, flags);
  else
    {
#if defined (PDL_HAS_SEND_TIMEDWAIT)
      PDL_Time_Value copy = *timeout;
      copy += PDL_OS::gettimeofday();
      timespec_t ts = copy;
      return ::send_timedwait (handle, buf, n, flags, &ts);
#else
      int val = 0;
      if (PDL::enter_send_timedwait (handle, timeout, val) == -1)
        return -1;
      else
        {
          ssize_t bytes_transferred = PDL_OS::send (handle, (const char *) buf, n, flags);
          PDL::restore_non_blocking_mode (handle, val);
          return bytes_transferred;
        }
#endif /* PDL_HAS_SEND_TIMEDWAIT */
    }
}

ssize_t
PDL::send (PDL_SOCKET handle,
           const void *buf,
           size_t n,
           const PDL_Time_Value *timeout)
{
  if (timeout == 0)
    return PDL::send_i (handle, buf, n);
  else
    {
#if defined (PDL_HAS_WRITE_TIMEDWAIT)
      PDL_Time_Value copy = *timeout;
      copy += PDL_OS::gettimeofday ();
      timespec_t ts = copy;
      return ::write_timedwait (handle, buf, n, &ts);
#else
      int val = 0;
      if (PDL::enter_send_timedwait (handle, timeout, val) == -1)
        return -1;
      else
        {
          ssize_t bytes_transferred = PDL::send_i (handle, buf, n);
          PDL::restore_non_blocking_mode (handle, val);
          return bytes_transferred;
        }
#endif /* PDL_HAS_WRITE_TIMEDWAIT */
    }
}

ssize_t
PDL::sendmsg (PDL_SOCKET handle,
              const struct msghdr *msg,
              int flags,
              const PDL_Time_Value *timeout)
{
  if (timeout == 0)
    return PDL_OS::sendmsg (handle, msg, flags);
  else
    {
#if defined (PDL_HAS_SENDMSG_TIMEDWAIT)
      PDL_Time_Value copy = *timeout;
      copy += PDL_OS::gettimeofday ();
      timespec_t ts = copy;
      return ::sendmsg_timedwait (handle, msg, flags, &ts);
#else
      int val = 0;
      if (PDL::enter_send_timedwait (handle, timeout, val) == -1)
        return -1;
      else
        {
          int bytes_transferred = PDL_OS::sendmsg (handle, msg, flags);
          PDL::restore_non_blocking_mode (handle, val);
          return bytes_transferred;
        }
#endif /* PDL_HAS_SENDMSG_TIMEDWAIT */
    }
}

ssize_t
PDL::sendto (PDL_SOCKET handle,
             const char *buf,
             int len,
             int flags,
             const struct sockaddr *addr,
             int addrlen,
             const PDL_Time_Value *timeout)
{
  if (timeout == 0)
    return PDL_OS::sendto (handle, buf, len, flags, addr, addrlen);
  else
    {
#if defined (PDL_HAS_SENDTO_TIMEDWAIT)
      PDL_Time_Value copy = *timeout;
      copy += PDL_OS::gettimeofday ();
      timespec_t ts = copy;
      return ::sendto_timedwait (handle, buf, len, flags, addr, addrlen, ts);
#else
      int val = 0;
      if (PDL::enter_send_timedwait (handle, timeout, val) == -1)
        return -1;
      else
        {
          int bytes_transferred = PDL_OS::sendto (handle, buf, len, flags, addr, addrlen);
          PDL::restore_non_blocking_mode (handle, val);
          return bytes_transferred;
        }
#endif /* PDL_HAS_SENDTO_TIMEDWAIT */
    }
}

ssize_t
PDL::send_n (PDL_SOCKET handle,
             const void *buf,
             size_t len,
             int flags,
             const PDL_Time_Value *timeout)
{
  if (timeout == 0)
    return PDL::send_n_i (handle,
                          buf,
                          len,
                          flags);
  else
    return PDL::send_n_i (handle,
                          buf,
                          len,
                          flags,
                          timeout);
}

ssize_t
PDL::send_n_i (PDL_SOCKET handle,
               const void *buf,
               size_t len,
               int flags)
{
  size_t bytes_transferred;
  ssize_t n;

  for (bytes_transferred = 0;
       bytes_transferred < len;
       bytes_transferred += n)
    {
      n = PDL_OS::send (handle,
                        (char *) buf + bytes_transferred,
                        len - bytes_transferred,
                        flags);
      if (n == -1)
        {
          // If blocked, try again.
          if (errno == EWOULDBLOCK)
            n = 0;

          //
          // No timeouts in this version.
          //

          // Other errors.
          return -1;
        }
      else if (n == 0)
        return 0;
    }

  return bytes_transferred;
}

ssize_t
PDL::send_n_i (PDL_SOCKET handle,
               const void *buf,
               size_t len,
               int flags,
               const PDL_Time_Value *timeout)
{
  int val = 0;
  PDL::record_and_set_non_blocking_mode (handle, val);

  size_t bytes_transferred;
  ssize_t n;
  int error = 0;

  for (bytes_transferred = 0;
       bytes_transferred < len;
       bytes_transferred += n)
    {
      int result = PDL::handle_write_ready (handle,
                                            timeout);

      if (result == -1)
        {
          // Timed out; return bytes transferred.
          if (errno == ETIME)
            break;

          // Other errors.
          error = 1;
          break;
        }

      n = PDL_OS::send (handle,
                        (char *) buf + bytes_transferred,
                        len - bytes_transferred,
                        flags);

      // Errors (note that errno cannot be EWOULDBLOCK since select()
      // just told us that data can be written).
      if (n == -1 || n == 0)
        {
          error = 1;
          break;
        }
    }

  PDL::restore_non_blocking_mode (handle, val);

  if (error)
    return -1;
  else
    return bytes_transferred;
}

ssize_t
PDL::send_n (PDL_SOCKET handle,
             const void *buf,
             size_t len,
             const PDL_Time_Value *timeout)
{
  if (timeout == 0)
    return PDL::send_n_i (handle,
                          buf,
                          len);
  else
    return PDL::send_n_i (handle,
                          buf,
                          len,
                          timeout);
}

ssize_t
PDL::send_n_i (PDL_SOCKET handle,
               const void *buf,
               size_t len)
{
  size_t bytes_transferred;
  ssize_t n;

  for (bytes_transferred = 0;
       bytes_transferred < len;
       bytes_transferred += n)
    {
      n = PDL::send_i (handle,
                       (char *) buf + bytes_transferred,
                       len - bytes_transferred);
      if (n == -1)
        {
          // If blocked, try again.
          if (errno == EWOULDBLOCK)
            n = 0;

          //
          // No timeouts in this version.
          //

          // Other errors.
          return -1;
        }
      else if (n == 0)
        return 0;
    }

  return bytes_transferred;
}

ssize_t
PDL::send_n_i (PDL_SOCKET handle,
               const void *buf,
               size_t len,
               const PDL_Time_Value *timeout)
{
  int val = 0;
  PDL::record_and_set_non_blocking_mode (handle, val);

  size_t bytes_transferred;
  ssize_t n;
  int error = 0;

  for (bytes_transferred = 0;
       bytes_transferred < len;
       bytes_transferred += n)
    {
      int result = PDL::handle_write_ready (handle,
                                            timeout);

      if (result == -1)
        {
          // Timed out; return bytes transferred.
          if (errno == ETIME)
            break;

          // Other errors.
          error = 1;
          break;
        }

      n = PDL::send_i (handle,
                       (char *) buf + bytes_transferred,
                       len - bytes_transferred);

      // Errors (note that errno cannot be EWOULDBLOCK since select()
      // just told us that data can be written).
      if (n == -1 || n == 0)
        {
          error = 1;
          break;
        }
    }

  PDL::restore_non_blocking_mode (handle, val);

  if (error)
    return -1;
  else
    return bytes_transferred;
}

ssize_t
PDL::sendv (PDL_SOCKET handle,
            const iovec *iov,
            int iovcnt,
            const PDL_Time_Value *timeout)
{
  if (timeout == 0)
    return PDL_OS::sendv (handle, iov, iovcnt);
  else
    {
#if defined (PDL_HAS_WRITEV_TIMEDWAIT)
      PDL_Time_Value copy = *timeout;
      copy += PDL_OS::gettimeofday ();
      timespec_t ts = copy;
      return ::sendv_timedwait (handle, iov, iovcnt, &ts);
#else
      int val = 0;
      if (PDL::enter_send_timedwait (handle, timeout, val) == -1)
        return -1;
      else
        {
          ssize_t bytes_transferred = PDL_OS::sendv (handle, iov, iovcnt);
          PDL::restore_non_blocking_mode (handle, val);
          return bytes_transferred;
        }
#endif /* PDL_HAS_WRITEV_TIMEDWAIT */
    }
}

ssize_t
PDL::sendv_n (PDL_SOCKET handle,
              const iovec *iov,
              int iovcnt,
              const PDL_Time_Value *timeout)
{
  if (timeout == 0)
    return PDL::sendv_n_i (handle,
                           iov,
                           iovcnt);
  else
    return PDL::sendv_n_i (handle,
                           iov,
                           iovcnt,
                           timeout);
}

ssize_t
PDL::sendv_n_i (PDL_SOCKET handle,
                const iovec *i,
                int iovcnt)
{
  iovec *iov = PDL_const_cast (iovec *, i);

  ssize_t bytes_transferred = 0;

  for (int s = 0;
       s < iovcnt;
       )
    {
      ssize_t n = PDL_OS::sendv (handle,
                                 iov + s,
                                 iovcnt - s);
      if (n == -1)
        {
          // If blocked, try again.
          if (errno == EWOULDBLOCK)
            n = 0;

          //
          // No timeouts in this version.
          //

          // Other errors.
          return -1;
        }
      else if (n == 0)
        return 0;

      for (bytes_transferred += n;
           s < iovcnt
             && n >= PDL_static_cast (ssize_t,
                                      iov[s].iov_len);
           s++)
        n -= iov[s].iov_len;

      if (n != 0)
        {
          char *base = PDL_reinterpret_cast (char *,
                                             iov[s].iov_base);
          iov[s].iov_base = base + n;
          iov[s].iov_len = iov[s].iov_len - n;
        }
    }

  return bytes_transferred;
}

ssize_t
PDL::sendv_n_i (PDL_SOCKET handle,
                const iovec *i,
                int iovcnt,
                const PDL_Time_Value *timeout)
{
  iovec *iov = PDL_const_cast (iovec *, i);

  int val = 0;
  PDL::record_and_set_non_blocking_mode (handle, val);

  ssize_t bytes_transferred = 0;
  int error = 0;

  for (int s = 0;
       s < iovcnt;
       )
    {
      int result = PDL::handle_write_ready (handle,
                                            timeout);

      if (result == -1)
        {
          // Timed out; return bytes transferred.
          if (errno == ETIME)
            break;

          // Other errors.
          error = 1;
          break;
        }

      ssize_t n = PDL_OS::sendv (handle,
                                 iov + s,
                                 iovcnt - s);

      // Errors (note that errno cannot be EWOULDBLOCK since select()
      // just told us that data can be written).
      if (n == -1 || n == 0)
        {
          error = 1;
          break;
        }

      for (bytes_transferred += n;
           s < iovcnt
             && n >= PDL_static_cast (ssize_t,
                                      iov[s].iov_len);
           s++)
        n -= iov[s].iov_len;

      if (n != 0)
        {
          char *base = PDL_reinterpret_cast (char *,
                                             iov[s].iov_base);
          iov[s].iov_base = base + n;
          iov[s].iov_len = iov[s].iov_len - n;
        }
    }

  PDL::restore_non_blocking_mode (handle, val);

  if (error)
    return -1;
  else
    return bytes_transferred;
}


ssize_t
PDL::readv_n (PDL_HANDLE handle,
              iovec *iov,
              int iovcnt)
{
  ssize_t bytes_transferred = 0;

  for (int s = 0;
       s < iovcnt;
       )
    {
      ssize_t n = PDL_OS::readv (handle,
                                 iov + s,
                                 iovcnt - s);
      if (n == -1 || n == 0)
        return n;

      for (bytes_transferred += n;
           s < iovcnt
             && n >= PDL_static_cast (ssize_t,
                                      iov[s].iov_len);
           s++)
        n -= iov[s].iov_len;

      if (n != 0)
        {
          char *base = PDL_reinterpret_cast (char *,
                                             iov[s].iov_base);
          iov[s].iov_base = base + n;
          iov[s].iov_len = iov[s].iov_len - n;
        }
    }

  return bytes_transferred;
}

ssize_t
PDL::writev_n (PDL_HANDLE handle,
               const iovec *i,
               int iovcnt)
{
  iovec *iov = PDL_const_cast (iovec *, i);

  ssize_t bytes_transferred = 0;

  for (int s = 0;
       s < iovcnt;
       )
    {
      ssize_t n = PDL_OS::writev (handle,
                                  iov + s,
                                  iovcnt - s);
      if (n == -1 || n == 0)
        return n;

      for (bytes_transferred += n;
           s < iovcnt
             && n >= PDL_static_cast (ssize_t,
                                      iov[s].iov_len);
           s++)
        n -= iov[s].iov_len;

      if (n != 0)
        {
          char *base = PDL_reinterpret_cast (char *,
                                             iov[s].iov_base);
          iov[s].iov_base = base + n;
          iov[s].iov_len = iov[s].iov_len - n;
        }
    }

  return bytes_transferred;
}

int
PDL::handle_ready (PDL_SOCKET handle,
                   const PDL_Time_Value *timeout,
                   int read_ready,
                   int write_ready,
                   int exception_ready)
{
#if defined (PDL_HAS_POLL) && defined (PDL_HAS_LIMITED_SELECT)
  PDL_UNUSED_ARG (write_ready);
  PDL_UNUSED_ARG (exception_ready);

  struct pollfd fds;

  fds.fd = handle;
  fds.events = read_ready ? POLLIN : POLLOUT;
  fds.revents = 0;

  int result = PDL_OS::poll (&fds, 1, timeout);

#else

  PDL_Handle_Set handle_set;
  handle_set.set_bit (handle);

  // Wait for data or for the timeout to elapse.
  int select_width;
#  if defined (PDL_WIN64)
  // This arg is ignored on Windows and causes pointer truncation
  // warnings on 64-bit compiles.
  select_width = 0;
#  else
  select_width = int (handle) + 1;
#  endif /* PDL_WIN64 */
  int result = PDL_OS::select (select_width,
                               read_ready ? handle_set.fdset () : 0, // read_fds.
                               write_ready ? handle_set.fdset () : 0, // write_fds.
                               exception_ready ? handle_set.fdset () : 0, // exception_fds.
                               timeout);

#endif /* PDL_HAS_POLL && PDL_HAS_LIMITED_SELECT */

  switch (result)
    {
    case 0:  // Timer expired.
      errno = ETIME;
      /* FALLTHRU */
    case -1: // we got here directly - select() returned -1.
      return -1;
    case 1: // Handle has data.
      /* FALLTHRU */
    default: // default is case result > 0; return a
      // PDL_ASSERT (result == 1);
      return result;
    }
}

int
PDL::enter_recv_timedwait (PDL_SOCKET handle,
                           const PDL_Time_Value *timeout,
                           int &val)
{
  int result = PDL::handle_read_ready (handle,
                                       timeout);

  if (result == -1)
    return -1;

  PDL::record_and_set_non_blocking_mode (handle,
                                         val);

  return result;
}

int
PDL::enter_send_timedwait (PDL_SOCKET handle,
                           const PDL_Time_Value *timeout,
                           int &val)
{
  int result = PDL::handle_write_ready (handle,
                                        timeout);

  if (result == -1)
    return -1;

  PDL::record_and_set_non_blocking_mode (handle,
                                         val);

  return result;
}

void
PDL::record_and_set_non_blocking_mode (PDL_SOCKET handle,
                                       int &val)
{
  // We need to record whether we are already *in* nonblocking mode,
  // so that we can correctly reset the state when we're done.
  val = PDL::get_flags (handle);

  if (PDL_BIT_DISABLED (val, PDL_NONBLOCK))
    // Set the handle into non-blocking mode if it's not already in
    // it.
    PDL::set_flags (handle, PDL_NONBLOCK);
}

void
PDL::restore_non_blocking_mode (PDL_SOCKET handle,
                                int val)
{
  if (PDL_BIT_DISABLED (val,
                        PDL_NONBLOCK))
    {
      // Save/restore errno.
      PDL_Errno_Guard error ((PDL_ERRNO_TYPE&)errno);
      // Only disable PDL_NONBLOCK if we weren't in non-blocking mode
      // originally.
      PDL::clr_flags (handle, PDL_NONBLOCK);
    }
}


// Format buffer into printable format.  This is useful for debugging.
// Portions taken from mdump by J.P. Knight (J.P.Knight@lut.ac.uk)
// Modifications by Todd Montgomery.

int
PDL::format_hexdump (const char *buffer,
                     int size,
                     ASYS_TCHAR *obuf,
                     int obuf_sz)
{
  PDL_TRACE ("PDL::format_hexdump");

  u_char c;
  ASYS_TCHAR textver[16 + 1];

  int maxlen = (obuf_sz / 68) * 16;

  if (size > maxlen)
    size = maxlen;

  int i;

  for (i = 0; i < (size >> 4); i++)
    {
      int j;

      for (j = 0 ; j < 16; j++)
        {
          c = (u_char) buffer[(i << 4) + j];
          PDL_OS::sprintf (obuf,
                           ASYS_TEXT ("%02x "),
                           c);
          obuf += 3;
          if (j == 7)
            {
              PDL_OS::sprintf (obuf,
                               ASYS_TEXT (" "));
              obuf++;
            }
          textver[j] = isprint (c) ? c : '.';
        }

      textver[j] = 0;

      PDL_OS::sprintf (obuf,
                       ASYS_TEXT ("  %s\n"),
                       textver);

      while (*obuf != '\0')
        obuf++;
    }

  if (size % 16)
    {
      for (i = 0 ; i < size % 16; i++)
        {
          c = (u_char) buffer[size - size % 16 + i];
          PDL_OS::sprintf (obuf,
                           ASYS_TEXT ("%02x "),
                           c);
          obuf += 3;
          if (i == 7)
            {
              PDL_OS::sprintf (obuf,
                               ASYS_TEXT (" "));
              obuf++;
            }
          textver[i] = isprint (c) ? c : '.';
        }

      for (i = size % 16; i < 16; i++)
        {
          PDL_OS::sprintf (obuf,
                           ASYS_TEXT ("   "));
          obuf += 3;
          textver[i] = ' ';
        }

      textver[i] = 0;
      PDL_OS::sprintf (obuf,
                       ASYS_TEXT ("  %s\n"),
                       textver);
    }
  return size;
}

// Returns the current timestamp in the form
// "hour:minute:second:microsecond."  The month, day, and year are
// also stored in the beginning of the date_and_time array.  Returns 0
// if unsuccessful, else returns pointer to beginning of the "time"
// portion of <day_and_time>.
#if defined(PDL_HAS_WINCE)
char *
PDL::timestamp (char date_and_time[],
                int date_and_timelen)
#else
ASYS_TCHAR *
PDL::timestamp (ASYS_TCHAR date_and_time[],
                int date_and_timelen)
#endif
{
  if (date_and_timelen < 35)
  {
    errno = EINVAL;
    return 0;
  }

#if defined (WIN32)
  // @@ Jesper, I think Win32 supports all the UNIX versions below.
  // Therefore, we can probably remove this WIN32 ifdef altogether.
  SYSTEMTIME local;
  ::GetLocalTime (&local);

  PDL_OS::sprintf (date_and_time,
#if !defined(PDL_HAS_WINCE)
                   ASYS_TEXT ("%02d/%02d/%04d %02d.%02d.%02d.%06d"),
#else
                   "%02d/%02d/%04d %02d.%02d.%02d.%06d",
#endif
                   (int) local.wMonth, // new, also the %02d in sprintf
                   (int) local.wDay,   // new, also the %02d in sprintf
                   (int) local.wYear,  // new, also the %02d in sprintf
                   (int) local.wHour,
                   (int) local.wMinute,
                   (int) local.wSecond,
                   (int) local.wMilliseconds * 1000);
#else  /* UNIX */
  char timebuf[26] = {'\0',}; // This magic number is based on the ctime(3c) man page.
  PDL_Time_Value cur_time = PDL_OS::gettimeofday ();
  time_t secs = cur_time.sec ();

  if (PDL_OS::ctime_r (&secs,
                       timebuf,
                       sizeof timebuf) == NULL)
  {
      return 0;
  }
  PDL_OS::strncpy (date_and_time,
                   timebuf,
                   date_and_timelen);
  PDL_OS::sprintf (&date_and_time[19],
                   ".%06ld",
                   cur_time.usec ());
#endif /* WIN32 */
  date_and_time[26] = '\0';
  return &date_and_time[11];
}

// This function rounds the request to a multiple of the page size.

size_t
PDL::round_to_pagesize (PDL_OFF_T len)
{
  PDL_TRACE ("PDL::round_to_pagesize");

  if (PDL::pagesize_ == 0)
    PDL::pagesize_ = PDL_OS::getpagesize ();

  return (len + (PDL::pagesize_ - 1)) & ~(PDL::pagesize_ - 1);
}

size_t
PDL::round_to_allocation_granularity (PDL_OFF_T len)
{
  PDL_TRACE ("PDL::round_to_allocation_granularity");

  if (PDL::allocation_granularity_ == 0)
    PDL::allocation_granularity_ = PDL_OS::allocation_granularity ();

  return (len + (PDL::allocation_granularity_ - 1)) & ~(PDL::allocation_granularity_ - 1);
}

PDL_SOCKET
PDL::handle_timed_complete (PDL_SOCKET h,
                            PDL_Time_Value *timeout,
                            int is_tli)
{
  PDL_TRACE ("PDL::handle_timed_complete");

#if !defined (PDL_WIN32) && defined (PDL_HAS_POLL) && defined (PDL_HAS_LIMITED_SELECT)

  struct pollfd fds;

  fds.fd = h;
  fds.events = POLLIN | POLLOUT;
  fds.revents = 0;

#else
  PDL_Handle_Set rd_handles;
  PDL_Handle_Set wr_handles;

  rd_handles.set_bit (h);
  wr_handles.set_bit (h);
#endif /* !PDL_WIN32 && PDL_HAS_POLL && PDL_HAS_LIMITED_SELECT */

#if defined (PDL_WIN32)
  PDL_Handle_Set ex_handles;
  ex_handles.set_bit (h);
#endif /* PDL_WIN32 */

  int need_to_check = 0;
  int known_failure = 0;

#if defined (PDL_WIN32)
  int n = PDL_OS::select (0,
                          0,
                          wr_handles,
                          ex_handles,
                          timeout);
#else
# if defined (PDL_HAS_POLL) && defined (PDL_HAS_LIMITED_SELECT)

  int n = PDL_OS::poll (&fds, 1, timeout);

# else
  int n = PDL_OS::select (int (h) + 1,
                          rd_handles,
                          wr_handles,
                          0,
                          timeout);
# endif /* PDL_HAS_POLL && PDL_HAS_LIMITED_SELECT */
#endif /* PDL_WIN32 */

  // If we failed to connect within the time period allocated by the
  // caller, then we fail (e.g., the remote host might have been too
  // busy to accept our call).
  if (n <= 0)
    {
      if (n == 0 && timeout != 0)
        errno = ETIMEDOUT;
      return PDL_INVALID_SOCKET;
    }

  // Check if the handle is ready for reading and the handle is *not*
  // ready for writing, which may indicate a problem.  But we need to
  // make sure...
#if defined (PDL_WIN32)
  PDL_UNUSED_ARG (is_tli);

  // On Win32, ex_handle set indicates a failure. We'll do the check
  // to try and get an errno value, but the connect failed regardless of
  // what getsockopt says about the error.
  if (ex_handles.is_set (h))
    {
      need_to_check = 1;
      known_failure = 1;
    }
#elif defined (VXWORKS)
  PDL_UNUSED_ARG (is_tli);

  // Force the check on VxWorks.  The read handle for "h" is not set,
  // so "need_to_check" is false at this point.  The write handle is
  // set, for what it's worth.
  need_to_check = 1;
#else
  if (is_tli)

# if defined (PDL_HAS_POLL) && defined (PDL_HAS_LIMITED_SELECT)
    need_to_check = (fds.revents & POLLIN) && !(fds.revents & POLLOUT);
# else
    need_to_check = rd_handles.is_set (h) && !wr_handles.is_set (h);
# endif /* PDL_HAS_POLL && PDL_HAS_LIMITED_SELECT */

  else
#if defined(AIX)
    // AIX is broken... both success and failed connect will set the
    // write handle only, so always check.
    need_to_check = 1;
#else
# if defined (PDL_HAS_POLL) && defined (PDL_HAS_LIMITED_SELECT)
    need_to_check = (fds.revents & POLLIN);
# else
    need_to_check = rd_handles.is_set (h);
# endif /* PDL_HAS_POLL && PDL_HAS_LIMITED_SELECT */
#endif /* AIX */
#endif /* PDL_WIN32 */

  if (need_to_check)
    {
#if defined (SOL_SOCKET) && defined (SO_ERROR)
      int sock_err = 0;
      int sock_err_len = sizeof (sock_err);
      int sockopt_ret = PDL_OS::getsockopt (h, SOL_SOCKET, SO_ERROR,
                                            (char *)&sock_err, &sock_err_len);
      if (sockopt_ret < 0)
        {
          h = PDL_INVALID_SOCKET;
        }

      if (sock_err != 0 || known_failure)
        {
          h = PDL_INVALID_SOCKET;
          errno = sock_err;
        }
#else
      char dummy;

      // The following recv() won't block provided that the
      // PDL_NONBLOCK flag has not been turned off .
      n = PDL::recv (h, &dummy, 1, MSG_PEEK);

      // If no data was read/peeked at, check to see if it's because
      // of a non-connected socket (and therefore an error) or there's
      // just no data yet.
      if (n <= 0)
        {
          if (n == 0)
            {
              errno = ECONNREFUSED;
              h = PDL_INVALID_SOCKET;
            }
          else if (errno != EWOULDBLOCK && errno != EAGAIN)
            h = PDL_INVALID_SOCKET;
        }
#endif
    }

  // 1. The HANDLE is ready for writing and doesn't need to be checked or
  // 2. recv() returned an indication of the state of the socket - if there is
  // either data present, or a recv is legit but there's no data yet,
  // the connection was successfully established.
  return h;
}

PDL_HANDLE
PDL::handle_timed_open (PDL_Time_Value *timeout,
                        LPCTSTR name,
                        int flags,
                        int perms)
{
  PDL_TRACE ("PDL::handle_timed_open");

  if (timeout != 0)
    {
      // Open the named pipe or file using non-blocking mode...
      PDL_HANDLE handle = PDL_OS::open (name,
                                        flags | PDL_NONBLOCK,
                                        perms);
      if (handle == PDL_INVALID_HANDLE
          && (errno == EWOULDBLOCK
              && (timeout->sec () > 0 || timeout->usec () > 0)))
        // This expression checks if we were polling.
        errno = ETIMEDOUT;

      return handle;
    }
  else
    return PDL_OS::open (name, flags, perms);
}

// Wait up to <timeout> amount of time to accept a connection.

int
PDL::handle_timed_accept (PDL_SOCKET listener,
                          PDL_Time_Value *timeout,
                          int restart)
{
  PDL_TRACE ("PDL::handle_timed_accept");
  // Make sure we don't bomb out on erroneous values.
  if (listener == PDL_INVALID_SOCKET)
    return -1;

#if defined (PDL_HAS_POLL) && defined (PDL_HAS_LIMITED_SELECT)

  struct pollfd fds;

  fds.fd = listener;
  fds.events = POLLIN;
  fds.revents = 0;

#else
  // Use the select() implementation rather than poll().
  PDL_Handle_Set rd_handle;
  rd_handle.set_bit (listener);
#endif /* PDL_HAS_POLL && PDL_HAS_LIMITED_SELECT */

  // We need a loop here if <restart> is enabled.

  for (;;)
    {
#if defined (PDL_HAS_POLL) && defined (PDL_HAS_LIMITED_SELECT)

      int n = PDL_OS::poll (&fds, 1, timeout);

#else
      int select_width;
#  if defined (PDL_WIN64)
      // This arg is ignored on Windows and causes pointer truncation
      // warnings on 64-bit compiles.
      select_width = 0;
#  else
      select_width = int (listener) + 1;
#  endif /* PDL_WIN64 */
      int n = PDL_OS::select (select_width,
                              rd_handle, 0, 0,
                              timeout);
#endif /* PDL_HAS_POLL && PDL_HAS_LIMITED_SELECT */

      switch (n)
        {
        case -1:
          if (errno == EINTR && restart)
            continue;
          else
            return -1;
          /* NOTREACHED */
        case 0:
          if (timeout != 0
              && timeout->sec () == 0
              && timeout->usec () == 0)
            errno = EWOULDBLOCK;
          else
            errno = ETIMEDOUT;
          return -1;
          /* NOTREACHED */
        case 1:
          return 0;
          /* NOTREACHED */
        default:
          errno = EINVAL;
          return -1;
          /* NOTREACHED */
        }
    }
  PDL_NOTREACHED (return 0);
}

// Bind socket to an unused port.

int
PDL::bind_port (PDL_SOCKET handle,
                PDL_UINT32 ip_addr)
{
  PDL_TRACE ("PDL::bind_port");

  sockaddr_in sock_addr;

  PDL_OS::memset ((void *) &sock_addr, 0, sizeof sock_addr);
  sock_addr.sin_family = AF_INET;
#if defined (PDL_HAS_SIN_LEN)
  sock_addr.sin_len = sizeof sock_addr;
#endif /* PDL_HAS_SIN_LEN */
  sock_addr.sin_addr.s_addr = ip_addr;

#if !defined (PDL_LACKS_WILDCARD_BIND)
  // The OS kernel should select a free port for us.
  sock_addr.sin_port = 0;
  return PDL_OS::bind (handle,
                       PDL_reinterpret_cast(sockaddr *, &sock_addr),
                       sizeof sock_addr);
#else
  static u_short upper_limit = PDL_MAX_DEFAULT_PORT;
  int round_trip = upper_limit;
  int lower_limit = IPPORT_RESERVED;

  // We have to select the port explicitly.

  for (;;)
    {
      sock_addr.sin_port = htons (upper_limit);

      if (PDL_OS::bind (handle,
                        PDL_reinterpret_cast(sockaddr *, &sock_addr),
                        sizeof sock_addr) >= 0)
        {
#if defined (PDL_WIN32)
          upper_limit--;
#endif /* PDL_WIN32 */
          return 0;
        }
      else if (errno != EADDRINUSE)
        return -1;
      else
        {
          upper_limit--;

          // Wrap back around when we reach the bottom.
          if (upper_limit <= lower_limit)
            upper_limit = PDL_MAX_DEFAULT_PORT;

          // See if we have already gone around once!
          if (upper_limit == round_trip)
            {
              errno = EAGAIN;
              return -1;
            }
        }
    }
#endif /* PDL_HAS_WILDCARD_BIND */
}

// Make the current process a UNIX daemon.  This is based on Stevens
// code from APUE.

int
PDL::daemonize (const char pathname[],
                int close_all_handles,
                const char program_name[])
{
  PDL_TRACE ("PDL::daemonize");
#if !defined (PDL_LACKS_FORK)
  pid_t pid = PDL_OS::fork ();

  if (pid == -1)
    return -1;
  else if (pid != 0)
    PDL_OS::exit (0); // Parent exits.

  // 1st child continues.
  PDL_OS::setsid (); // Become session leader.

  PDL_OS::signal (SIGHUP, SIG_IGN);

  pid = PDL_OS::fork (program_name);

  if (pid != 0)
    PDL_OS::exit (0); // First child terminates.

  // Second child continues.

  if (pathname != 0)
    // change working directory.
    PDL_OS::chdir (pathname);

  PDL_OS::umask (0); // clear our file mode creation mask.

  // Close down the files.
  if (close_all_handles)
    for (int i = PDL::max_handles () - 1; i >= 0; i--)
      PDL_OS::close (i);
# if defined(DEBUG)
  if (close_all_handles != 0)
  {
      PDL_OS::bDaemonPritable = 1;
  }
# endif
  return 0;
#else
/*BUGBUG_NT*/
#if defined(PDL_WIN32)
  // No need to daemonize in windows platform
  /* Close standard handles */

#if !defined(PDL_HAS_WINCE)
  ::CloseHandle( ::GetStdHandle( STD_INPUT_HANDLE  ) );
  ::CloseHandle( ::GetStdHandle( STD_OUTPUT_HANDLE ) );
  ::CloseHandle( ::GetStdHandle( STD_ERROR_HANDLE  ) );
#endif

  return 0;
#else
  PDL_UNUSED_ARG (pathname);
  PDL_UNUSED_ARG (close_all_handles);
  PDL_UNUSED_ARG (program_name);

  PDL_NOTSUP_RETURN (-1);
#endif
/*BUGBUG_NT*/
#endif /* PDL_LACKS_FORK */
}

pid_t
PDL::fork (const char *program_name,
           int avoid_zombies)
{
  if (avoid_zombies == 0)
    return PDL_OS::fork (program_name);
  else
    {
      // This algorithm is adapted from an example in the Stevens book
      // "Advanced Programming in the Unix Environment" and an item in
      // Andrew Gierth's Unix Programming FAQ.  It creates an orphan
      // process that's inherited by the init process; init cleans up
      // when the orphan process terminates.
      //
      // Another way to avoid zombies is to ignore or catch the
      // SIGCHLD signal; we don't use that approach here.

      pid_t pid = PDL_OS::fork ();
      if (pid == 0)
        {
          // The child process forks again to create a grandchild.
          switch (PDL_OS::fork (program_name))
            {
            case 0: // grandchild returns 0.
              return 0;
            case -1: // assumes all errnos are < 256
              PDL_OS::_exit (errno);
            default:  // child terminates, orphaning grandchild
              PDL_OS::_exit (0);
            }
        }

      // Parent process waits for child to terminate.

      if (pid < 0) // fork error
      {
          return -1;
      }
      else
      {
          /* success to fork */
      }

#if defined (PDL_HAS_UNION_WAIT)
      union wait status;
      if (PDL_OS::waitpid (pid, &(status.w_status), 0) < 0)
#else
      PDL_exitcode status;
      if (PDL_OS::waitpid (pid, &status, 0) < 0)
#endif /* PDL_HAS_UNION_WAIT */
      {
          return -1;
      }
      else
      {
          /* success to get pid */
      }

      
      // child terminated by calling exit()?
      if (WIFEXITED ((status)))
        {
          // child terminated normally?
          if (WEXITSTATUS ((status)) == 0)
            return 1;
          else
            errno = WEXITSTATUS ((status));
        }
      else
        // child didn't call exit(); perhaps it received a signal?
        errno = EINTR;

      return -1;
    }
}

int
PDL::max_handles (void)
{
  PDL_TRACE ("PDL::max_handles");
#if defined (RLIMIT_NOFILE) && !defined (PDL_LACKS_RLIMIT)
  rlimit rl;

  //fix for UMR
  rl.rlim_cur = 0;
  rl.rlim_max = 0;

  PDL_OS::getrlimit (RLIMIT_NOFILE, &rl);
# if defined (RLIM_INFINITY)
  if (rl.rlim_cur != RLIM_INFINITY)
    return rl.rlim_cur;
# else
    return rl.rlim_cur;
# endif /* RLIM_INFINITY */
# if defined (_SC_OPEN_MAX)
  return PDL_OS::sysconf (_SC_OPEN_MAX);
# elif defined (FD_SETSIZE)
  return FD_SETSIZE;
# else
  PDL_NOTSUP_RETURN (-1);
# endif /* _SC_OPEN_MAX */
/*BUGBUG_NT*/
#else
# if defined (PDL_WIN32)
  return FD_SETSIZE;
# elif defined (VXWORKS)
  return FD_SETSIZE;
# elif defined (SYMBIAN)
  return FD_SETSIZE;
# else
  PDL_NOTSUP_RETURN (-1);
# endif /* PDL_WIN32 */
/*BUGBUG_NT*/
#endif /* defined (RLIMIT_NOFILE) && !defined (PDL_LACKS_RLIMIT) */
}

// Set the number of currently open handles in the process.
//
// If NEW_LIMIT == -1 set the limit to the maximum allowable.
// Otherwise, set it to be the value of NEW_LIMIT.

int
PDL::set_handle_limit (int new_limit)
{
    PDL_TRACE ("PDL::set_handle_limit");
    int cur_limit = PDL::max_handles ();
    int max_limit = cur_limit;
    int rc;
    
    if (cur_limit == -1)
        return -1;

#if !defined (PDL_LACKS_RLIMIT) && defined (RLIMIT_NOFILE)
    struct rlimit rl;

    PDL_OS::memset ((void *) &rl, 0, sizeof rl);

    if( PDL_OS::getrlimit (RLIMIT_NOFILE, &rl) == -1 )
    {
        return -1;
    }
    else
    {
        /* BUG-18226: PDL::set_handle_limit does abnormal operation on AIX
         *
         * when call PDL_OS::getrlimit, r1.rlim_max is -1.
         */
        if( rl.rlim_max > FD_SETSIZE)
        {
            /* if r1.rlim_max == -1, set r1.rlim_max to be FD_SETSIZE
             * that is the max of FD.*/
            rl.rlim_max = FD_SETSIZE;
        }
    }

    max_limit = rl.rlim_max;
#endif /* PDL_LACKS_RLIMIT */

    // new_limit이 -1이면 Max값으로 설정하라는 것임.
    if (new_limit == -1)
    {
        new_limit = max_limit;
    }

    if (new_limit < 0)
    {
        errno = EINVAL;
        return -1;
    }
    else if (new_limit > cur_limit)
    {
#if !defined (PDL_LACKS_RLIMIT) && defined (RLIMIT_NOFILE)
        rl.rlim_cur = new_limit;
        rc = PDL_OS::setrlimit (RLIMIT_NOFILE, &rl);

        return rc;
#else
        // Must return EINVAL errno.
        PDL_NOTSUP_RETURN (-1);
#endif /* PDL_LACKS_RLIMIT */
    }
    else
    {
#if !defined (PDL_LACKS_RLIMIT) && defined (RLIMIT_NOFILE)
        rl.rlim_cur = new_limit;
        return PDL_OS::setrlimit (RLIMIT_NOFILE, &rl);
#else
        // We give a chance to platforms without RLIMIT to work.
        // Instead of PDL_NOTSUP_RETURN (0), just return 0 because
        // new_limit is <= cur_limit, so it's a no-op.
        return 0;
#endif /* PDL_LACKS_RLIMIT */
    }


    // Irix complains without this return statement.  DEC cxx
    // (correctly) says that it's not reachable.  PDL_NOTREACHED won't
    // work here, because it handles both platforms the same.
    // IRIX does not complain anymore [7.2]
    PDL_NOTREACHED (return 0);
}

// Flags are file status flags to turn on.

int
PDL::set_flags (PDL_SOCKET handle, int flags)
{
  PDL_TRACE ("PDL::set_flags");
#if defined (PDL_WIN32) || defined (VXWORKS) || defined (PDL_LACKS_FCNTL)
  switch (flags)
    {
    case PDL_NONBLOCK:
      // nonblocking argument (1)
      // blocking:            (0)
      {
        u_long nonblock = 1;
        return PDL_OS::ioctl (handle, FIONBIO, &nonblock);
      }
    default:
      PDL_NOTSUP_RETURN (-1);
    }
#else
  int val = PDL_OS::fcntl (handle, F_GETFL, 0);

  if (val == -1)
    return -1;

  // Turn on flags.
  PDL_SET_BITS (val, flags);

  if (PDL_OS::fcntl (handle, F_SETFL, val) == -1)
    return -1;
  else
    return 0;
#endif /* PDL_WIN32 || PDL_LACKS_FCNTL */
}


// Flags are the file status flags to turn off.

int
PDL::clr_flags (PDL_SOCKET handle, int flags)
{
  PDL_TRACE ("PDL::clr_flags");

#if defined (PDL_WIN32) || defined (VXWORKS) || defined (PDL_LACKS_FCNTL)
  switch (flags)
    {
    case PDL_NONBLOCK:
      // nonblocking argument (1)
      // blocking:            (0)
      {
        u_long nonblock = 0;
        return PDL_OS::ioctl (handle, FIONBIO, &nonblock);
      }
    default:
      PDL_NOTSUP_RETURN (-1);
    }
#else
  int val = PDL_OS::fcntl (handle, F_GETFL, 0);

  if (val == -1)
    return -1;

  // Turn flags off.
  PDL_CLR_BITS (val, flags);

  if (PDL_OS::fcntl (handle, F_SETFL, val) == -1)
    return -1;
  else
    return 0;
#endif /* PDL_WIN32 || PDL_LACKS_FCNTL */
}

int
PDL::map_errno (int error)
{
  switch (error)
    {
#if defined (PDL_WIN32)
    case WSAEWOULDBLOCK:
      return EAGAIN; // Same as UNIX errno EWOULDBLOCK.
#endif /* PDL_WIN32 */
    }

  return error;
}

// Euclid's greatest common divisor algorithm.
u_long
PDL::gcd (u_long x, u_long y)
{
  if (y == 0)
  {
    return x;
  }
  else
  {
    return PDL::gcd (y, x % y);
  }
}


// Calculates the minimum enclosing frame size for the given values.
u_long
PDL::minimum_frame_size (u_long period1, u_long period2)
{
  // if one of the periods is zero, treat it as though it as
  // uninitialized and return the other period as the frame size
  if (0 == period1)
  {
    return period2;
  }
  if (0 == period2)
  {
    return period1;
  }

  // if neither is zero, find the greatest common divisor of the two periods
  u_long greatest_common_divisor = PDL::gcd (period1, period2);

  // explicitly consider cases to reduce risk of possible overflow errors
  if (greatest_common_divisor == 1)
  {
    // periods are relative primes: just multiply them together
    return period1 * period2;
  }
  else if (greatest_common_divisor == period1)
  {
    // the first period divides the second: return the second
    return period2;
  }
  else if (greatest_common_divisor == period2)
  {
    // the second period divides the first: return the first
    return period1;
  }
  else
  {
    // the current frame size and the entry's effective period
    // have a non-trivial greatest common divisor: return the
    // product of factors divided by those in their gcd.
    return (period1 * period2) / greatest_common_divisor;
  }
}


u_long
PDL::is_prime (const u_long n,
               const u_long min_factor,
               const u_long max_factor)
{
  if (n > 3)
    for (u_long factor = min_factor;
         factor <= max_factor;
         ++factor)
      if (n / factor * factor == n)
        return factor;

  return 0;
}

#if defined(PDL_HAS_WINCE)
const char *
PDL::sock_error (int error)
#else
const ASYS_TCHAR *
PDL::sock_error (int error)
#endif /* PDL_HAS_WINCE */
{
#if defined (PDL_WIN32)

#if defined(PDL_HAS_WINCE)
  static char unknown_msg[64];
#else
  static ASYS_TCHAR unknown_msg[64];
#endif /* PDL_HAS_WINCE */

  switch (error)
    {
    case WSAVERNOTSUPPORTED:
      return "version of WinSock not supported";

    case WSASYSNOTREADY:
      return "WinSock not present or not responding";

    case WSAEINVAL:
      return "app version not supported by DLL";

    case WSAHOST_NOT_FOUND:
      return "Authoritive: Host not found";

    case WSATRY_AGAIN:
      return "Non-authoritive: host not found or server failure";

    case WSANO_RECOVERY:
      return "Non-recoverable: refused or not implemented";

    case WSANO_DATA:
      return "Valid name, no data record for type";

      /*
        case WSANO_ADDRESS:
        return "Valid name, no MX record";
          */
    case WSANOTINITIALISED:
      return "WSA Startup not initialized";

    case WSAENETDOWN:
      return "Network subsystem failed";

    case WSAEINPROGRESS:
      return "Blocking operation in progress";

    case WSAEINTR:
      return "Blocking call cancelled";

    case WSAEAFNOSUPPORT:
      return "address family not supported";

    case WSAEMFILE:
      return "no file handles available";

    case WSAENOBUFS:
      return "no buffer space available";

    case WSAEPROTONOSUPPORT:
      return "specified protocol not supported";

    case WSAEPROTOTYPE:
      return "protocol wrong type for this socket";

    case WSAESOCKTNOSUPPORT:
      return "socket type not supported for address family";

    case WSAENOTSOCK:
      return "handle is not a socket";

    case WSAEWOULDBLOCK:
      return "socket marked as non-blocking and SO_LINGER set not 0";

    case WSAEADDRINUSE:
      return "address already in use";

    case WSAECONNABORTED:
      return "connection aborted";

    case WSAECONNRESET:
      return "connection reset";

    case WSAENOTCONN:
      return "not connected";

    case WSAETIMEDOUT:
      return "connection timed out";

    case WSAECONNREFUSED:
      return "connection refused";

    case WSAEHOSTDOWN:
      return "host down";

    case WSAEHOSTUNREACH:
      return "host unreachable";

    case WSAEADDRNOTAVAIL:
      return "address not available";

    default:
      PDL_OS::sprintf (unknown_msg, "unknown error: %d", error);
      return unknown_msg;
      /* NOTREACHED */
    }
#else
  PDL_UNUSED_ARG (error);
  PDL_NOTSUP_RETURN (0);
#endif /* PDL_WIN32 */
}


// Routine to return a handle from which ioctl() requests can be made.

PDL_SOCKET
PDL::get_handle (void)
{
// Solaris 2.x
  PDL_SOCKET handle = PDL_INVALID_SOCKET;
#if defined (sparc)
  handle = PDL_OS::open ("/dev/udp", O_RDONLY);
#elif defined (__unix) || defined (__Lynx__) || defined (_AIX)
  // Note: DEC CXX doesn't define "unix" BSD compatible OS: HP UX,
  // AIX, SunOS 4.x

  handle = PDL_OS::socket (PF_INET, SOCK_DGRAM, 0);
#endif /* sparc */
  return handle;
}

#if defined (PDL_WIN32)
// Return value in buffer for a key/name pair from the Windows
// Registry up to buf_len size.

static int
get_reg_value (const TCHAR *key,
               const TCHAR *name,
               TCHAR *buffer,
               DWORD &buf_len)
{
  HKEY hk;
  DWORD buf_type;
  LONG rc = ::RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                            key,
                            0,
                            KEY_READ,
                            &hk);
  // 1. open key that defines the interfaces used for TCP/IP?
  if (rc != ERROR_SUCCESS)
    // print_error_string(TEXT("RegOpenKeyEx"), rc);
    return -1;

  rc = ::RegQueryValueEx (hk,
                          name,
                          0,
                          &buf_type,
                          (u_char *) buffer,
                          &buf_len);
  if (rc != ERROR_SUCCESS)
    {
      // print_error_string(TEXT("RegEnumKeyEx"), rc);
      RegCloseKey (hk);
      return -2;
    }

  ::RegCloseKey (hk);
  return 0;
}
#endif /* PDL_WIN32 */

// return an array of all configured IP interfaces on this host, count
// rc = 0 on success (count == number of interfaces else -1 caller is
// responsible for calling delete [] on parray

char *
PDL::strndup (const char *str, size_t n)
{
  const char *t = str;
  size_t len;

  // Figure out how long this string is (remember, it might not be
  // NUL-terminated).

  for (len = 0;
       len < n && *t++ != '\0';
       len++)
    continue;

  char *s;
  PDL_ALLOCATOR_RETURN (s,
                        (char *) PDL_OS::malloc (len + 1),
                        0);
  s[len] = '\0';
  return PDL_OS::strncpy (s, str, len);
}

char *
PDL::strnnew (const char *str, size_t n)
{
  const char *t = str;
  size_t len;

  // Figure out how long this string is (remember, it might not be
  // NUL-terminated).

  for (len = 0;
       len < n && *t++ != '\0';
       len++)
    continue;

  char *s;
  PDL_NEW_RETURN (s,
                  char,
                  len + 1,
                  0);
  s[len] = '\0';
  return PDL_OS::strncpy (s, str, len);
}

const char *
PDL::strend (const char *s)
{
  while (*s++ != '\0')
    continue;

  return s;
}



