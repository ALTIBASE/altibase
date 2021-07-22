/***********************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: idvTimeGetClock.c 66601 2014-09-02 02:09:31Z djin $
 **********************************************************************/

#include <assert.h>
#include <idConfig.h>
#include <idTypes.h>

#if defined(HP_HPUX)

#include <machine/reg.h>
#include <machine/inline.h>

/* ------------------------------------------------
 *  HP
 * ----------------------------------------------*/
ULong idvGetClockTickFromSystem()
{
    register ULong sTick;

/* BUG-18474 HP장비에서 idvGetClockTickFromSystem이 문제가 있습니다.
 *
 * HP장비에서는 Server는 GCC로 build하지 않기때문에 GCC는 client라고 가정하고
 * 0을 바로 return하도록 수정하였습니다.
 * */
#if defined(_USE_GCC_)
    sTick = 0;
#else
    #if defined(COMPILE_64BIT)
        _MFCTL(CR_IT, sTick); /* or CR16 -- same thing, both define to 16 */
    #else
        sTick = 0;
    #endif
#endif

    return sTick;
}

#elif defined(SPARC_SOLARIS) || defined(X86_SOLARIS)
/* ------------------------------------------------
 *  SOLARIS
 * ----------------------------------------------*/
ULong idvGetClockTickFromSystem()
{
    return 0;
}

#elif defined(OS_LINUX_KERNEL)
/* ------------------------------------------------
 *  LINUX
 * ----------------------------------------------*/

ULong idvGetClockTickFromSystem()
{
    ULong sTick;

# if defined(INTEL_LINUX)
    __asm__ __volatile__("rdtsc" : "=A" (sTick) : );

# elif defined(POWERPC64_LINUX)
    UInt  sUpper, sLower, sTmp;

    sTick = 0;
    __asm__ volatile(
                "0:                  \n"
                "\tmftbu   %0        \n"
                "\tmftb    %1        \n"
                "\tmftbu   %2        \n"
                "\tcmpw    %2,%0     \n"
                "\tbne     0b        \n"
                : "=r"(sUpper), "=r"(sLower), "=r"(sTmp)
                );
    sTick = (ULong)sUpper;
    sTick = sTick<<32;
    sTick = sTick|sLower;

# elif defined(AMD64_LINUX) || defined(XEON_LINUX) || defined(X86_64_LINUX)
    {
        UInt a, d;
        asm volatile("rdtsc" : "=a" (a), "=d" (d));
        sTick = ((ULong)a) | (((ULong)d) << 32);
    }
# endif

    return sTick;
}
#else

/* Not supported Platform */
ULong idvGetClockTickFromSystem()
{
    return 0;
}
#endif
