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
 
/***********************************************************************
 * $Id$
 **********************************************************************/

#ifndef _O_MTCN_H_
# define  _O_MTCN_H_  1

#include <acpTypes.h>

typedef acp_uint64_t mtnChar;

typedef union mtnIndex{
    struct US7ASCII {
        acp_uint64_t index;
    } US7ASCII;
    struct KO16KSC5601 {
        acp_uint64_t index;
    } KO16KSC5601;
} mtnIndex;

extern acp_sint32_t mtnSkip( const acp_uint8_t * s1,
                             acp_uint16_t        s1Length,
                             mtnIndex          * index,
                             acp_uint16_t        skip );

extern acp_sint32_t mtnCharAt( mtnChar           * c,
                               const acp_uint8_t * s1,
                               acp_uint16_t        s1Length,
                               mtnIndex          * index );

extern acp_sint32_t mtnAddChar( acp_uint8_t  * s1,
                                acp_uint16_t   s1Max,
                                acp_uint16_t * s1Length,
                                mtnChar        c );

extern acp_sint32_t mtnToAscii( mtnChar * c );

extern acp_sint32_t mtnToLocal( mtnChar * c );

acp_sint32_t mtnAsciiAt( mtnChar           * c,
                         const acp_uint8_t * s1,
                         acp_uint16_t        s1Length,
                         mtnIndex          * index );

acp_sint32_t mtnAddAscii( acp_uint8_t  * s1,
                          acp_uint16_t   s1Max,
                          acp_uint16_t * s1Length,
                          mtnChar        c );

#endif /* _O_MTCN_H_ */
 
