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
 * $Id: acpAtomic-SPARC.c 4529 2009-02-13 01:45:30Z jykim $
 ******************************************************************************/


typedef signed long acp_sint32_t;
typedef signed long long acp_sint64_t;

acp_sint32_t acpAtomicCas32(volatile void         *aAddr,
                                       volatile acp_sint32_t  aWith,
                                       volatile acp_sint32_t  aCmp)
{
    __asm__ __volatile__ ("membar #StoreLoad | #LoadLoad\n\t"
                          "cas [%2],%3,%0\n\t"
                          "membar #StoreLoad | #StoreStore"
                          : "=&r"(aWith)
                          : "0"(aWith), "r"(aAddr), "r"(aCmp)
                          : "memory");
    return aWith;
}

typedef union acpAtomicLong
{
    acp_sint32_t mInt32[2];
    acp_sint64_t mInt64;
} acpAtomicLong;

acp_sint64_t acpAtomicCas64(volatile void         *aAddr,
                                       volatile acp_sint64_t  aWith,
                                       volatile acp_sint64_t  aCmp)
{
    acpAtomicLong sPrev;
    acpAtomicLong sWith;
    acpAtomicLong sCmp;

    sWith.mInt64 = aWith;
    sCmp.mInt64  = aCmp;

    __asm__ __volatile__ ("membar #StoreLoad | #LoadLoad\n\t"
                          "sllx   %2,32,%2\n\t"   /* load high word of aWith  */
                          "srl    %3,0,%3\n\t"    /* load low word of aWith   */
                          "or     %2,%3,%2\n\t"   /* combine words of aWith   */
                          "sllx   %5,32,%5\n\t"   /* load high word of aCmp   */
                          "srl    %6,0,%6\n\t"    /* load low word of aCmp    */
                          "or     %5,%6,%5\n\t"   /* combine words of aCmp    */
                          "casx   [%4],%5,%2\n\t" /* compare and swap         */
                          "srl    %2,0,%1\n\t"    /* capture low word of old  */
                          "srlx   %2,32,%0\n\t"   /* capture high word of old */
                          "membar #StoreLoad | #StoreStore"
                          : "=r"(sPrev.mInt32[0]),
                            "=r"(sPrev.mInt32[1])
                          : "r"(sWith.mInt32[0]),
                            "r"(sWith.mInt32[1]),
                            "r"(aAddr),
                            "r"(sCmp.mInt32[0]),
                            "r"(sCmp.mInt32[1])
                          : "memory");
    return sPrev.mInt64;
}

