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
 * $Id: aciConvKsc5601.h 11319 2010-06-23 02:39:42Z djin $
 **********************************************************************/

#ifndef _O_ACICONVKSC5601_H_
#define  _O_ACICONVKSC5601_H_  1

#include <aciConv.h>

ACP_EXTERN_C_BEGIN

/*
 * KSC5601.1987-0, now KS X 1001:2002
 */

/* XOR 을 하기 위한 임의의 값. 특별한 의미를 나타내지 않는다. */
#define KSC5601_XOR_VALUE (1037)

ACP_EXPORT
acp_sint32_t aciConvConvertMbToWc4Ksc5601( void    * aSrc, 
                            acp_sint32_t      aSrcRemain,
                            acp_sint32_t    * aSrcAdvance, 
                            void    * aDest, 
                            acp_sint32_t      aDestRemain );

ACP_EXPORT
acp_sint32_t aciConvConvertWcToMb4Ksc5601( void    * aSrc, 
                            acp_sint32_t      aSrcRemain,
                            acp_sint32_t    * aSrcAdvance, 
                            void    * aDest, 
                            acp_sint32_t      aDestRemain );

ACP_EXTERN_C_END

#endif /* _O_ACICONVKSC5601_H_ */
 
