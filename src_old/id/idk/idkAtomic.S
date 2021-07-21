/********************************************************************
 * Copyright 1999-2006, ALTIBASE Corporation or its subsidiaries.
 * All rights reserved.
 ********************************************************************
 * $Id: idkAtomic.S 58974 2013-03-27 07:27:44Z kclee $
 ********************************************************************/

#include <idConfig.h>

/* ia64 */
#if defined(_IA64_) || defined(IA64_HP_HPUX) || defined(IA64_LINUX) || defined(IA64_SUSE_LINUX)
#include <idkAtomic-ia64.S>

/* x86/i386/ia32  */
#elif defined(INTEL_LINUX) || defined(X86_SOLARIS) || defined(NTO_QNX) || defined(PC_CYGWIN)
#include <idkAtomic-i386.S>

/* x86_64 */
#elif defined(AMD64_LINUX) || defined(XEON_LINUX) || defined(X86_64_LINUX) || defined(X86_64_DARWIN)
#include <idkAtomic-x86_64.S>

/* SPARC v8+/v9  */
#elif defined(SPARC_SOLARIS)
#include <idkAtomic-sparcv8p.S>

/* HP-PARISC */
#elif defined(HP_HPUX)
#include <idkAtomic-hppa2.S>

/* DEC ALPHA */
#elif defined(DEC_TRU64)
#include <idkAtomic-alpha.S>

/* IBM POWERPC */
#elif defined(IBM_AIX)
#include <idkAtomic-power.s>

#else
#error Atomic Operation does not supported for given processor
#endif
