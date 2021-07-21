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
 * $Id: acpPset.h 9884 2010-02-03 06:50:04Z gurugio $
 ******************************************************************************/

#if !defined(_O_ACP_PSET_H_)
#define _O_ACP_PSET_H_

/**
 * @file
 * @ingroup CorePset
 * Pset is for representing of CPU set used by CPU-Bind APIs.
 * Pset = Processor Set
 */

#include <acp.h>
#include <acpError.h>


ACP_EXTERN_C_BEGIN

/**
 * Maxmimum CPU count of PSET libaray
 */
#define ACP_PSET_MAX_CPU   1024 /* can keep 1024 count of CPUs */

#define ACP_PSET_NBITS_UNIT  (sizeof(acp_uint64_t) * 8)

/**
 * Clear All bits of PSets
 *
 * @param psets to be clear
 */
#define ACP_PSET_ZERO(psets)                                                \
    do {                                                                    \
        size_t si__MACRO_LOCAL_VAR;                                         \
        size_t sMax__MACRO_LOCAL_VAR =                                      \
            sizeof(acp_pset_t) / sizeof (acp_uint64_t);                     \
        for (si__MACRO_LOCAL_VAR = 0;                                       \
             si__MACRO_LOCAL_VAR < sMax__MACRO_LOCAL_VAR;                   \
             ++si__MACRO_LOCAL_VAR)                                         \
        {                                                                   \
            (psets)->mBits[si__MACRO_LOCAL_VAR] = 0;                        \
        }                                                                   \
    } while (0)

/**
 * Set 1 bits of Psets
 *
 * @param psets Target CPU Sets
 * @param number Bit Numbers to be set on
 */
#define ACP_PSET_SET(psets, number)                                         \
    do {                                                                    \
        acp_uint64_t sNumber__MACRO_LOCAL_VAR;                              \
        acp_uint64_t sMask__MACRO_LOCAL_VAR;                                \
        sNumber__MACRO_LOCAL_VAR =                                          \
            (acp_uint64_t)((number) / ACP_PSET_NBITS_UNIT);                 \
        sMask__MACRO_LOCAL_VAR   =                                          \
            ((acp_uint64_t)1) << ((number) % ACP_PSET_NBITS_UNIT);          \
        (psets)->mBits[sNumber__MACRO_LOCAL_VAR] |= sMask__MACRO_LOCAL_VAR; \
    }                                                                       \
    while(0)

/**
 * Clear 1 bits of Psets
 *
 * @param psets Target CPU Sets
 * @param number Bit Numbers to be clear
 */
#define ACP_PSET_CLR(psets, number)                                         \
    do {                                                                    \
        acp_uint64_t sNumber__MACRO_LOCAL_VAR;                              \
        acp_uint64_t sMask__MACRO_LOCAL_VAR;                                \
        sNumber__MACRO_LOCAL_VAR = (number) / ACP_PSET_NBITS_UNIT;          \
        sMask__MACRO_LOCAL_VAR   =                                          \
            ((acp_uint64_t)1) << ((number) % ACP_PSET_NBITS_UNIT);          \
        (psets)->mBits[sNumber__MACRO_LOCAL_VAR] &= ~(sMask__MACRO_LOCAL_VAR); \
    } while(0)

/**
 * Check whether a specific bits is on or not
 *
 * @param psets Target CPU Sets
 * @param number Bit Numbers to be checked
 */
#define ACP_PSET_ISSET(psets, number) acpPsetIsset((psets), (number))

/**
 * Get the total count of CPU sets
 *
 * @param psets Target CPU Sets
 */
#define ACP_PSET_COUNT(psets) acpPsetGetCount(psets)


typedef struct acp_pset_t
{
    acp_uint64_t mBits[ACP_PSET_MAX_CPU / ACP_PSET_NBITS_UNIT];
} acp_pset_t;


ACP_INLINE acp_uint32_t acpPsetIsset(acp_pset_t *aPsets, acp_uint32_t aNumber)
{
    acp_uint64_t sNumber;
    acp_uint64_t sMask;

    sNumber = (acp_uint64_t)((aNumber) / ACP_PSET_NBITS_UNIT);
    sMask = ((acp_uint64_t)1) << (aNumber % ACP_PSET_NBITS_UNIT);

    return ( ( aPsets->mBits[sNumber] & sMask) > 0 ? 1 : 0);
}

ACP_INLINE acp_uint64_t acpPsetGetCount(acp_pset_t *aPsets)
{
    acp_uint64_t i; /* cpu  */
    acp_uint64_t j; /* bits */
    acp_uint64_t sCount = 0;

    for (i = 0;
         i < (acp_uint64_t)(sizeof(acp_pset_t) / sizeof(acp_uint64_t));
         i++)
    {
        for (j = 0; j < ACP_PSET_NBITS_UNIT; j++)
        {
            sCount += ( ((aPsets->mBits[i]) & ( (acp_uint64_t)1 << j))
                        != 0 ? 1 : 0);
        }
    }

    return sCount;
}

/**
 *  Pset APIs
 */

ACP_EXPORT acp_rc_t acpPsetGetConf(acp_pset_t *aSystemPset,
                                   acp_bool_t *aIsSupportBind,
                                   acp_bool_t *aIsSupportMultipleBind);

ACP_EXPORT acp_rc_t acpPsetBindProcess(const acp_pset_t *aPset);

ACP_EXPORT acp_rc_t acpPsetBindThread(const acp_pset_t *aPset);

ACP_EXPORT acp_rc_t acpPsetUnbindProcess(void);

ACP_EXPORT acp_rc_t acpPsetUnbindThread(void);


ACP_EXTERN_C_END


#endif
