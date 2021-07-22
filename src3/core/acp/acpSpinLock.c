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
 * $Id:
 ******************************************************************************/

#include <acpSpinLock.h>

#if !defined(__GNUC__) && defined(ALTI_CFG_CPU_PARISC)
/* ALINT:S2-DISABLE */
/* Inline assembly porting for non-gcc compiler */

/* All functions are included at object file, not source file. */

/* 1. generate object files with gcc. */
/* For example, followings are building acpSpinLock.c at 32bit HPUX-PARISC. */
/* ex) */
/* $ gcc -c -o objects/acpSpinLock-PARISC_32MODE.o acpSpinLock-PARISC_32MODE.c */
/* $ gcc -c -fPIC -o objects/acpSpinLock-PARISC_32MODE_pic.o acpSpinLock-PARISC_32MODE.c */
 
/* 2. fix core.mk to link building object files. For example, */
/* ex) */
/*   ifeq ($(ALTI_CFG_CPU),PARISC) */
/*    ifeq ($(compile_bit),32) */
/*      CORE_OBJS   += $(CORE_DIR)/src/acp/objects/acpSpinLock-PARISC_32MODE.o */
/*      CORE_SHOBJS += $(CORE_DIR)/src/acp/objects/acpSpinLock-PARISC_32MODE_pic.o */
/*    else */
/*      CORE_OBJS   += $(CORE_DIR)/src/acp/objects/acpSpinLock-PARISC_64MODE.o */
/*      CORE_SHOBJS += $(CORE_DIR)/src/acp/objects/acpSpinLock-PARISC_64MODE_pic.o */
/*    endif */
/*   endif */
/* ALINT:S2-ENABLE */

#else

ACP_EXPORT acp_bool_t acpSpinLockTryLock(acp_spin_lock_t *aLock)
{
    acp_sint32_t sVal;

#if defined(ALTI_CFG_CPU_PARISC)
    volatile acp_sint32_t *sLock;
    sLock = ACP_ALIGN_ALL_PTR(&aLock->mLock[0], 16);

    __asm__ __volatile__ (
#if defined(ACP_CFG_COMPILE_32BIT)
        "ldcw"
#else
        "ldcw,co"
#endif
        " 0(%1), %0"
        : "=r"(sVal)
        : "r"(sLock));
#else
    sVal = acpAtomicCas32(&aLock->mLock, 0, 1);
#endif

    return (sVal != 0) ? ACP_TRUE : ACP_FALSE;
}

# endif
