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
 * $Id: aciConvGbkext2.h 563 2014-06-30 00:49:01Z myungsub.shin $
 **********************************************************************/

#ifndef _O_ACICONVGBKEXT2_H_
#define  _O_ACICONVGBKEXT2_H_  1

#include <aciConv.h>

/* PROJ-2414 [기능성] GBK, CP936 character set 추가 */
#define ACI_GBKEXT2_XOR_VALUE ( 36149 )

ACP_EXTERN_C_BEGIN

/* PROJ-2414 [기능성] GBK, CP936 character set 추가 */
ACP_EXPORT
acp_sint32_t aciConvConvertMbToWc4Gbkext2( void         * aSrc,
                                           acp_sint32_t   aSrcRemain,
                                           acp_sint32_t * aSrcAdvance,
                                           void         * aDest,
                                           acp_sint32_t   aDestRemain );

ACP_EXTERN_C_END

#endif /* _O_ACICONVGBKEXT2_H_ */
