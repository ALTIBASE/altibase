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

#ifndef _O_ACICONVCP932_H_
#define  _O_ACICONVCP932_H_  1

#include <aciConv.h>

ACP_EXTERN_C_BEGIN

/* PROJ-2590 [기능성] CP932 database character set 지원 */
ACP_EXPORT
acp_sint32_t aciConvConvertMbToWc4Cp932( void           * aSrc,
                                         acp_sint32_t     aSrcRemain,
                                         acp_sint32_t   * aSrcAdvance,
                                         void           * aDest,
                                         acp_sint32_t     aDestRemain );

/* PROJ-2590 [기능성] CP932 database character set 지원 */
ACP_EXPORT
acp_sint32_t aciConvConvertWcToMb4Cp932( void           * aSrc,
                                         acp_sint32_t     aSrcRemain,
                                         acp_sint32_t   * aSrcAdvance,
                                         void           * aDest,
                                         acp_sint32_t     aDestRemain );

ACP_EXTERN_C_END

#endif /* _O_ACICONVCP932_H_ */
 
