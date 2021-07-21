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
 * $Id: acpAtomic.c 11386 2010-06-28 09:15:31Z djin $
 ******************************************************************************/

#include <acpAtomic.h>


#if defined(ALTI_CFG_CPU_PARISC)
# include <acpAtomic-PARISC_USE_LOCKS.c>
#elif defined(ALTI_CFG_CPU_SPARC)
# include <acpAtomic-SPARC.c>
#else
# if defined(ALTI_CFG_CPU_POWERPC)
#  include <acpAtomic-PPC.c>

# elif defined(ALTI_CFG_CPU_IA64)
#  include <acpAtomic-IA64.c>

# elif defined(ALTI_CFG_CPU_ALPHA)
#  include <acpAtomic-ALPHA.c>

# elif defined(ALTI_CFG_CPU_X86)
#  if defined(ALTI_CFG_OS_LINUX)
#   include <acpAtomic-X86_LINUX.c>
#  elif defined(ALTI_CFG_OS_FREEBSD)
#   include <acpAtomic-X86_LINUX.c>
#  elif defined(ALTI_CFG_OS_DARWIN)
#   include <acpAtomic-X86_LINUX.c>
#  elif defined(ALTI_CFG_OS_SOLARIS)
#   include <acpAtomic-X86_SOLARIS.c>
#  endif  /* ALTI_CFG_OS_LINUX */
# elif defined(ALTI_CFG_CPU_X86_OLD)
#  include <acpAtomic-X86_LINUX.c>
# endif  /* ALTI_CFG_CPU_POWERPC */
/*
implementations of acpAtomicAddrLock() and acpAtomicAddrUnlock()
for a platform do not support hardware atomic operations.
# elif defined(ALTI_CFG_CPU_NO_HARDWARE_ATOMIC)
#  include <acpAtomic-OTHER_USE_LOCKS.c>   
*/
#endif  /* ALTI_CFG_CPU_PARISC */
